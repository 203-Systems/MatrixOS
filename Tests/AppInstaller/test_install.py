"""Host tests with a simulated HID peer; no physical device is accessed."""
import contextlib
import importlib.util
import io
import json
import struct
import tempfile
import types
import unittest
from pathlib import Path
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("app_installer", ROOT / "Tools/AppInstaller/install.py")
a = importlib.util.module_from_spec(spec)
spec.loader.exec_module(a)


def app_files():
    return {"AppInfo.json": json.dumps({"name": "Test", "author": "Tests", "color": [1, 2, 3],
            "version": 1, "osMinimalVer": [4, 0, 0], "appMainFile": "main.py"}).encode(),
            "main.py": b"def loop():\n    pass\n", "assets/data.bin": bytes(range(256))}


class Peer:
    def __init__(self, drop_data=False, unsupported=False, wrong_crc=False):
        self.queue, self.requests, self.cache = [], [], {}
        self.received = bytearray()
        self.drop_data, self.unsupported, self.wrong_crc = drop_data, unsupported, wrong_crc
        self.state, self.expected_crc, self.expected_size = 0, 0, 0
        self.data_writes = 0

    def write(self, packet):
        self.requests.append(packet)
        assert len(packet) == 64 and packet[0] == 0xCB
        if packet[1] in (0x11, 0x12, 0x1F):
            data = {0x11: b"Developer\0", 0x12: b"203 Systems\0", 0x1F: b""}[packet[1]]
            self.queue.append((bytes([0xCB, packet[1] | 0x80]) + data).ljust(64, b"\0"))
            return 64
        cmd, sub, seq, session, size = struct.unpack_from(">BBHIB", packet, 1)
        assert cmd == 0x70 and size <= 54
        body = packet[10:10 + size]
        if self.unsupported:
            return 64
        if packet in self.cache:
            self.queue.append(self.cache[packet])
            return 64
        status, reply = 0, b""
        if sub == 0:
            reply = struct.pack(">4sBBHII", b"MIAP", 1, 50, 128, 8 * 1024 * 1024, 32768)
        elif sub == 1:
            assert session == 0 and len(body) == 25
            flags, self.expected_size, self.expected_crc = struct.unpack_from(">BII", body, 16)
            assert flags in (0, 1)
            session, self.state = 0x12345678, 1
        else:
            assert session == 0x12345678
            if sub == 2:
                offset = struct.unpack_from(">I", body)[0]
                assert offset == len(self.received)
                self.received.extend(body[4:])
                self.data_writes += 1
                reply = struct.pack(">I", len(self.received))
            elif sub == 3:
                assert len(self.received) == self.expected_size
                assert a.crc(self.received) == self.expected_crc
                a.validate_package(self.received)
                self.state = 4
            elif sub == 4:
                reply = struct.pack(">BBII", self.state, 0, len(self.received),
                                    self.expected_crc ^ int(self.wrong_crc))
            elif sub == 5:
                self.state = 5
            else:
                raise AssertionError("Unexpected file-access sub ID")
        response = (b"\xcb" + struct.pack(">BBHIBB", 0xF0, sub, seq, session, status, len(reply)) + reply).ljust(64, b"\0")
        self.cache[packet] = response
        if sub == 2 and self.drop_data:
            self.drop_data = False
        else:
            self.queue.append(response)
        return 64

    def read(self, size, timeout_ms):
        return self.queue.pop(0) if self.queue else b""


class Tests(unittest.TestCase):
    def test_package_roundtrip_and_determinism(self):
        data = a.build_package("GravityMarble", app_files())
        self.assertEqual(data, a.build_package("GravityMarble", app_files()))
        meta = a.validate_package(data)
        self.assertEqual(meta["app"], "GravityMarble")
        self.assertEqual(sum(i["size"] for i in meta["files"]), sum(map(len, app_files().values())))

    def test_corruption_truncation_and_trailing_bytes(self):
        data = a.build_package("Test", app_files())
        for broken in (data[:-1], data + b"x", data[:-1] + bytes([data[-1] ^ 1]), b"zip"):
            with self.subTest(length=len(broken)), self.assertRaises(a.InstallError):
                a.validate_package(broken)

    def test_rejects_unsafe_or_conflicting_paths(self):
        for name in ("../escape", "/absolute", "a\\b", "a/../b", "a.", "CON.txt", "MAIN.py", "assets"):
            files = app_files()
            files[name] = b"unsafe"
            with self.subTest(name=name), self.assertRaises(a.InstallError):
                a.build_package("Test", files)

    def test_requires_valid_manifest_and_entrypoint(self):
        for field, value in (("appMainFile", "../escape.py"), ("appMainFile", "missing.py"),
                             ("color", [256, 0, 0]), ("version", True)):
            files = app_files()
            info = json.loads(files["AppInfo.json"])
            info[field] = value
            files["AppInfo.json"] = json.dumps(info).encode()
            with self.subTest(field=field, value=value), self.assertRaises(a.InstallError):
                a.build_package("Test", files)

    def test_automatic_memory_packaging_no_output_file(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            for name, data in app_files().items():
                path = root / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(data)
            cache = root / "__pycache__"
            cache.mkdir()
            (cache / "ignored.pyc").write_bytes(b"ignored")
            data = a.package_source(root, "Test")
            self.assertEqual(len(a.validate_package(data)["files"]), 3)
            self.assertFalse(list(root.rglob("*.mpkg")))
            with patch.object(a, "connect", side_effect=AssertionError("must not open HID")):
                with contextlib.redirect_stdout(io.StringIO()):
                    a.main(["install", str(root), "--name", "Test", "--dry-run"])

    def test_rejects_symlinks(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "link").symlink_to(Path(__file__))
            with self.assertRaises(a.InstallError):
                a.package_source(root, "Test")

    def test_install_stream_and_lost_ack_retry(self):
        package = a.build_package("Test", app_files())
        peer = Peer(drop_data=True)
        client = a.Client(peer, timeout=0.002, retries=1)
        with contextlib.redirect_stderr(io.StringIO()):
            result = client.install(package)
        self.assertEqual(result["state"], "committed")
        self.assertTrue(result["launcher_returned"])
        self.assertEqual(bytes(peer.received), package)
        self.assertEqual(peer.data_writes, (len(package) + 49) // 50)
        self.assertGreater(len(peer.requests), len(set(peer.requests)))
        self.assertTrue(all(p[2] in range(6) for p in peer.requests))

    def test_committed_identity_must_match(self):
        peer = Peer(wrong_crc=True)
        with contextlib.redirect_stderr(io.StringIO()), self.assertRaisesRegex(a.InstallError, "identity"):
            a.Client(peer, timeout=0.002).install(a.build_package("Test", app_files()))

    def test_old_firmware_receives_only_capability_query(self):
        peer = Peer(unsupported=True)
        with self.assertRaisesRegex(a.InstallError, "Firmware may not support"):
            a.Client(peer, timeout=0.001, retries=1).install(a.build_package("Test", app_files()))
        self.assertEqual(len(peer.requests), 2)
        self.assertTrue(all(p[2] == a.CAPS for p in peer.requests))
        self.assertEqual(peer.requests[0], peer.requests[1])

    def test_abort_queries_transaction_state(self):
        peer = Peer()
        client = a.Client(peer)
        client.call(a.BEGIN, b"x" * 16 + struct.pack(">BII", 0, 10, 20))
        client.call(a.ABORT)
        self.assertEqual(client.status()["state"], "aborted")

    def test_stale_response_is_ignored(self):
        peer = Peer()
        peer.queue.append((b"\xcb" + struct.pack(">BBHIBB", 0xF0, 0, 0, 0, 0, 0)).ljust(64, b"\0"))
        client = a.Client(peer)
        client.sequence = 100
        self.assertEqual(client.capabilities()["version"], 1)

    def test_bad_response_length_and_device_errors(self):
        class Broken(Peer):
            def write(self, packet):
                result = super().write(packet)
                response = bytearray(self.queue[-1])
                response[10] = 54
                self.queue[-1] = bytes(response)
                return result
        with self.assertRaisesRegex(a.InstallError, "Malformed"):
            a.Client(Broken()).capabilities()
        class Busy(Peer):
            def write(self, packet):
                result = super().write(packet)
                response = bytearray(self.queue[-1])
                response[9] = 2
                self.queue[-1] = bytes(response)
                return result
        with self.assertRaisesRegex(a.InstallError, "busy"):
            a.Client(Busy()).capabilities()

    def test_discovers_both_mystrix_pid_families(self):
        for product in (0x1040, 0x107F, 0x1080, 0x10BF):
            device = types.SimpleNamespace(open_path=lambda path: None, close=lambda: None)
            module = types.SimpleNamespace(device=lambda: device, enumerate=lambda vid: [
                {"path": b"test-device", "usage_page": 0xFF00, "usage": 1,
                 "product_id": product, "serial_number": "serial"}])
            with self.subTest(product=product), patch.dict(a.sys.modules, {"hid": module}), patch.object(a.sys, "platform", "linux"):
                self.assertIs(a.connect("serial"), device)

    def test_multiple_devices_require_serial(self):
        module = types.SimpleNamespace(enumerate=lambda vid: [
            {"path": serial.encode(), "usage_page": 0xFF00, "usage": 1,
             "product_id": 0x1040, "serial_number": serial} for serial in ("one", "two")])
        with patch.dict(a.sys.modules, {"hid": module}), patch.object(a.sys, "platform", "linux"):
            with self.assertRaisesRegex(a.InstallError, "found 2"):
                a.connect(None)


if __name__ == "__main__":
    unittest.main()
