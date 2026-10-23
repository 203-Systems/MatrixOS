#!/usr/bin/env python3
"""Mystrix Python app installer CLI (INSTALL_APP HID protocol v1).

Requires matching firmware with INSTALL_APP support. See PROTOCOL.txt.
"""
import argparse
import ctypes
import json
import re
import secrets
import struct
import sys
import time
import zlib
from pathlib import Path

REPORT = 0xCB
INSTALL_APP = 0x70
CAPS, BEGIN, DATA, FINISH, STATUS, ABORT = range(6)
PACKAGE_HEADER = struct.Struct(">4sBBHIIII")
HEADER = struct.Struct(">BBHIB")
REPLY = struct.Struct(">BBHIBB")
MAX_FILES = 128
MAX_BYTES = 8 * 1024 * 1024
MAX_MANIFEST = 32768
RESERVED_NAMES = {"CON", "PRN", "AUX", "NUL"} | {
    prefix + str(i) for prefix in ("COM", "LPT") for i in range(1, 10)}
ERRORS = {1: "unsupported", 2: "busy / USB is not in normal mode", 3: "invalid payload",
          4: "invalid session", 5: "offset mismatch", 6: "storage I/O failure",
          7: "checksum mismatch", 8: "application already exists (use --replace)",
          9: "invalid application", 10: "capacity exceeded", 11: "cannot enter the existing Developer APP installation context"}
STATES = ("idle", "receiving", "validating", "committing", "committed", "aborted", "failed")


class InstallError(Exception):
    pass


def crc(data):
    return zlib.crc32(data) & 0xFFFFFFFF


def safe_path(name):
    # A portable FAT subset; excludes aliases from trailing spaces/dots and drive paths.
    if not name or len(name.encode("utf-8")) > 160 or name.count("/") > 15:
        raise InstallError("Invalid or overlong application path")
    for part in name.split("/"):
        if (part in ("", ".", "..") or part[-1:] in (" ", ".")
                or any(ord(c) < 32 or ord(c) >= 127 or c in '\\:*?"<>|' for c in part)):
            raise InstallError("Unsafe application path: " + name)
        if part.split(".")[0].upper() in RESERVED_NAMES:
            raise InstallError("Reserved FAT filename: " + name)
    return name


def validate_app(entries):
    try:
        info = json.loads(entries["AppInfo.json"].decode("utf-8"))
        if not isinstance(info, dict):
            raise ValueError("AppInfo must be an object")
        for field in ("name", "author", "appMainFile"):
            if not isinstance(info.get(field), str) or not info[field]:
                raise ValueError("Missing string field " + field)
        if type(info.get("version")) is not int or not 0 <= info["version"] <= 0xFFFFFFFF:
            raise ValueError("Invalid version")
        for field in ("color", "osMinimalVer"):
            values = info.get(field)
            if (not isinstance(values, list) or len(values) != 3
                    or any(type(v) is not int or not 0 <= v <= 255 for v in values)):
                raise ValueError("Invalid " + field)
        main = safe_path(info["appMainFile"])
        if main not in entries or not main.endswith(".py"):
            raise ValueError("appMainFile must refer to a packaged .py file")
    except (KeyError, ValueError, UnicodeError) as e:
        raise InstallError("Invalid AppInfo.json: " + str(e)) from e


def build_package(app, entries):
    """Build package bytes in memory; no package file is created."""
    files = sorted(entries.items())
    manifest = json.dumps({"app": app, "files": [
        {"path": path, "size": len(data), "crc32": crc(data)}
        for path, data in files]}, separators=(",", ":"), ensure_ascii=False).encode("utf-8")
    payload = b"".join(data for _, data in files)
    package = PACKAGE_HEADER.pack(b"MPKG", 1, 0, 0, len(manifest), len(payload),
                                  crc(manifest), crc(payload)) + manifest + payload
    validate_package(package)
    return package


def package_source(source, name=None):
    """Collect a local app at install time, without an interactive folder picker."""
    source = Path(source)
    if not source.is_dir() or source.is_symlink():
        raise InstallError("Source must be a local application directory, not a symlink")
    entries, total = {}, 0
    for path in sorted(source.rglob("*")):
        relative = path.relative_to(source)
        if any(p in ("__pycache__", ".git", ".DS_Store", ".venv") for p in relative.parts):
            continue
        if path.is_symlink():
            raise InstallError("Application symlinks are not supported: " + str(relative))
        if path.is_file():
            if len(entries) >= MAX_FILES or path.stat().st_size > MAX_BYTES - total:
                raise InstallError("Application exceeds local package limits")
            data = path.read_bytes()
            total += len(data)
            if total > MAX_BYTES:
                raise InstallError("Application exceeds local package limits")
            entries[relative.as_posix()] = data
    return build_package(name or source.name, entries)


def validate_package(package):
    if not PACKAGE_HEADER.size <= len(package) <= MAX_BYTES:
        raise InstallError("Invalid package size")
    magic, version, flags, reserved, msize, psize, mcrc, pcrc = PACKAGE_HEADER.unpack_from(package)
    if magic != b"MPKG" or version != 1 or flags or reserved:
        raise InstallError("Unsupported MPKG format")
    if not 0 < msize <= MAX_MANIFEST or PACKAGE_HEADER.size + msize + psize != len(package):
        raise InstallError("Invalid package lengths")
    manifest = package[PACKAGE_HEADER.size:PACKAGE_HEADER.size + msize]
    payload = package[PACKAGE_HEADER.size + msize:]
    if crc(manifest) != mcrc or crc(payload) != pcrc:
        raise InstallError("Package checksum mismatch")
    try:
        meta = json.loads(manifest.decode("utf-8"))
        if not isinstance(meta, dict) or set(meta) != {"app", "files"}:
            raise ValueError("Invalid manifest fields")
        if not isinstance(meta["app"], str) or not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9_-]{0,63}", meta["app"]):
            raise ValueError("Invalid application directory name")
        files = meta["files"]
        if not isinstance(files, list) or not 1 <= len(files) <= MAX_FILES:
            raise ValueError("Invalid file count")
        entries, names, offset = {}, set(), 0
        for item in files:
            if not isinstance(item, dict) or set(item) != {"path", "size", "crc32"}:
                raise ValueError("Invalid file entry")
            path, size, checksum = item["path"], item["size"], item["crc32"]
            if not isinstance(path, str):
                raise ValueError("Invalid path type")
            safe_path(path)
            normalized = path.casefold()
            if normalized in names:
                raise ValueError("Duplicate file path")
            names.add(normalized)
            if (type(size) is not int or not 0 <= size <= len(payload) - offset
                    or type(checksum) is not int or not 0 <= checksum <= 0xFFFFFFFF):
                raise ValueError("Invalid file size/checksum")
            data = payload[offset:offset + size]
            if crc(data) != checksum:
                raise ValueError("File checksum mismatch: " + path)
            entries[path] = data
            offset += size
        for path in names:
            parts = path.split("/")
            if any("/".join(parts[:i]) in names for i in range(1, len(parts))):
                raise ValueError("File/directory path conflict")
        if offset != len(payload):
            raise ValueError("Unclaimed package bytes")
        validate_app(entries)
    except (KeyError, ValueError, UnicodeError) as e:
        raise InstallError("Invalid package manifest: " + str(e)) from e
    return meta


class Client:
    def __init__(self, device, timeout=2.0, retries=2):
        self.device = device
        self.timeout = timeout
        self.retries = retries
        self.sequence = secrets.randbelow(65536)
        self.session = 0

    def call(self, operation, data=b"", timeout=None):
        if len(data) > 54:
            raise InstallError("Request payload exceeds HID capacity")
        self.sequence = (self.sequence + 1) & 0xFFFF
        request_session = self.session
        packet = bytes([REPORT]) + HEADER.pack(INSTALL_APP, operation, self.sequence, request_session, len(data)) + data
        packet = packet.ljust(64, b"\0")
        for _ in range(self.retries + 1):
            if self.device.write(packet) != 64:
                raise InstallError("Incomplete HID write; query transaction status before retrying installation")
            deadline = time.monotonic() + (self.timeout if timeout is None else timeout)
            while time.monotonic() < deadline:
                reply = bytes(self.device.read(64, timeout_ms=100))
                if len(reply) < 1 + REPLY.size or reply[0] != REPORT:
                    continue
                command, op, seq, session, status, length = REPLY.unpack_from(reply, 1)
                if (command, op, seq) != (INSTALL_APP | 0x80, operation, self.sequence):
                    continue
                if operation != BEGIN and session != request_session:
                    continue
                if length > 53 or len(reply) < 1 + REPLY.size + length:
                    raise InstallError("Malformed installer response")
                if status:
                    raise InstallError(ERRORS.get(status, "device error " + str(status)))
                if operation == BEGIN:
                    if not session:
                        raise InstallError("Device returned an invalid session")
                    self.session = session
                return reply[1 + REPLY.size:1 + REPLY.size + length]
        if operation == CAPS:
            raise InstallError("No installer capability reply. Firmware may not support HID installation; exit USB Storage Mode and check firmware support.")
        raise InstallError(f"HID timeout (session {self.session:#x}); outcome unknown, query status")

    def capabilities(self):
        reply = self.call(CAPS)
        if len(reply) != 16:
            raise InstallError("Invalid installer capabilities")
        magic, version, chunk, files, size, manifest = struct.unpack(">4sBBHII", reply)
        if magic != b"MIAP" or version != 1 or not 1 <= chunk <= 50:
            raise InstallError("Unsupported installer protocol")
        return {"version": version, "chunk": chunk, "max_files": files,
                "max_package": size, "max_manifest": manifest}

    def status(self):
        data = self.call(STATUS)
        if len(data) != 10:
            raise InstallError("Invalid status response")
        state, error, offset, checksum = struct.unpack(">BBII", data)
        if state >= len(STATES):
            raise InstallError("Unknown transaction state")
        return {"session": self.session, "state": STATES[state], "error": error,
                "offset": offset, "package_crc32": checksum}

    def _system(self, command):
        """Use existing system commands; never retry an app-exit action."""
        packet = bytes([REPORT, command]).ljust(64, b"\0")
        if self.device.write(packet) != 64:
            return None
        deadline = time.monotonic() + self.timeout
        while time.monotonic() < deadline:
            reply = bytes(self.device.read(64, timeout_ms=100))
            if len(reply) >= 2 and reply[:2] == bytes([REPORT, command | 0x80]):
                return reply[2:].split(b"\0", 1)[0]
        return None

    def leave_context(self):
        try:
            # Do not exit an unrelated app if the user has already changed context.
            if self._system(0x11) == b"Developer" and self._system(0x12) == b"203 Systems":
                return self._system(0x1F) is not None
        except OSError:
            pass
        print("Installation complete; reopen the launcher to refresh apps.", file=sys.stderr)
        return False

    def install(self, package, replace=False):
        meta = validate_package(package)
        caps = self.capabilities()
        if (len(meta["files"]) > caps["max_files"] or len(package) > caps["max_package"]
                or PACKAGE_HEADER.unpack_from(package)[4] > caps["max_manifest"]):
            raise InstallError("Package exceeds device limits")
        self.call(BEGIN, secrets.token_bytes(16) + struct.pack(">BII", int(replace), len(package), crc(package)), timeout=60)
        print(f"Installation session: {self.session:#x}", file=sys.stderr)
        last_percent = -1
        for offset in range(0, len(package), caps["chunk"]):
            chunk = package[offset:offset + caps["chunk"]]
            ack = self.call(DATA, struct.pack(">I", offset) + chunk)
            if ack != struct.pack(">I", offset + len(chunk)):
                raise InstallError("Package offset mismatch")
            percent = (offset + len(chunk)) * 100 // len(package)
            if percent // 10 != last_percent // 10:
                print(f"Uploading {meta['app']}: {percent}%", file=sys.stderr)
                last_percent = percent
        # The existing command task validates/commits synchronously. Still confirm
        # the final transaction identity with STATUS before reporting success.
        self.call(FINISH, timeout=60)
        deadline = time.monotonic() + 30
        while time.monotonic() < deadline:
            result = self.status()
            if result["state"] == "committed":
                if result["offset"] != len(package) or result["package_crc32"] != crc(package):
                    raise InstallError("Committed package identity mismatch")
                result["launcher_returned"] = self.leave_context()
                return result
            if result["state"] in ("failed", "aborted"):
                raise InstallError("Installation " + result["state"] + ": " +
                                   ERRORS.get(result["error"], str(result["error"])))
            time.sleep(0.2)
        raise InstallError("Commit not confirmed; query status before starting another installation")


def connect(serial):
    try:
        import hid
    except ImportError as e:
        raise InstallError("Install dependency: python3 -m pip install hidapi") from e
    if sys.platform == "darwin":
        native = ctypes.CDLL(hid.__file__)
        configure = getattr(native, "hid_darwin_set_open_exclusive", None)
        if configure:
            configure.argtypes = [ctypes.c_int]
            configure.restype = None
            configure(0)
    matches = {i["path"]: i for i in hid.enumerate(0x0203)
               if i.get("usage_page") == 0xFF00 and i.get("usage") == 1
               and (i["product_id"] & 0xFFC0) in (0x1040, 0x1080)
               and (serial is None or i.get("serial_number") == serial)}
    if len(matches) != 1:
        raise InstallError(f"Expected one Mystrix in normal mode, found {len(matches)}; use --serial or exit MSC")
    device = hid.device()
    try:
        device.open_path(next(iter(matches)))
    except Exception:
        device.close()
        raise
    return device


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--serial", help="Device USB serial number")
    commands = parser.add_subparsers(dest="command", required=True)
    commands.add_parser("caps", help="Query device installer support")
    install = commands.add_parser("install", help="Package an app in memory and send INSTALL_APP")
    install.add_argument("source", type=Path, help="Local app source path; packaged automatically")
    install.add_argument("--name", help="Installed app directory name (default: source basename)")
    install.add_argument("--replace", action="store_true", help="Explicitly allow replacing the existing application")
    install.add_argument("--dry-run", action="store_true", help="Validate package locally without opening HID")
    for name in ("status", "abort"):
        sub = commands.add_parser(name)
        sub.add_argument("session", type=lambda v: int(v, 0), help="Session ID printed by install (hex or decimal)")
    args = parser.parse_args(argv)
    package = None
    if args.command == "install":
        package = package_source(args.source, args.name)
        meta = validate_package(package)
        if args.dry_run:
            print(json.dumps(meta, indent=2, ensure_ascii=False))
            return
    if args.command in ("status", "abort") and not 1 <= args.session <= 0xFFFFFFFF:
        raise InstallError("Session must be a nonzero uint32")
    device = connect(args.serial)
    client = Client(device)
    try:
        if args.command == "install":
            result = client.install(package, replace=args.replace)
        else:
            caps = client.capabilities()
            if args.command == "caps":
                result = caps
            else:
                client.session = args.session
                if args.command == "abort":
                    client.call(ABORT)
                result = client.status()
        print(json.dumps(result, indent=2))
    except (InstallError, OSError):
        if client.session:
            print(f"Check with: install.py --serial <device-serial> status {client.session:#x}", file=sys.stderr)
        raise
    finally:
        device.close()


if __name__ == "__main__":
    try:
        main()
    except (InstallError, OSError, ValueError) as error:
        print("Error: " + str(error), file=sys.stderr)
        sys.exit(1)
