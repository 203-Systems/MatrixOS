Mystrix HID application installer CLI

Status: CLI and firmware INSTALL_APP receiver are implemented in this checkout.
Mystrix1 firmware builds and host fault tests pass. Installation, replacement,
committed-status verification and launcher return were tested on a Mystrix Pro
over macOS HID. Hardware fault and power-loss tests remain pending. Use matching firmware.
Unsupported firmware receives only a capability query; no upload is attempted.

Python 3.9+; install HID dependency in a virtual environment:
  python3 -m venv .venv
  .venv/bin/python -m pip install -r Tools/AppInstaller/requirements.txt

Run from the repository root:
  .venv/bin/python Tools/AppInstaller/install.py install /path/to/HelloPython

Pass the application's folder path. The CLI collects its files, builds the package
in memory, and sends it immediately. No folder picker, manual packaging step, or
package output file. Paths containing spaces must be quoted normally by the shell.
Keep USB in normal mode. BEGIN enters the existing Developer APP using ExecuteAPP;
its End() cancels an incomplete upload before another app starts. The existing
command task performs storage operations serially; there is no installer worker.
After confirmed success, the CLI uses the existing app-exit command to return to
the launcher. If the user already changed apps, it leaves that app alone.
AppInfo.json must be directly inside the folder. --name sets the installed folder
name; without it, the source folder basename is used.

Update an existing app with --replace. This replaces application code/resources
while preserving application data under /MatrixOS/AppData.

Other commands:
  python3 Tools/AppInstaller/install.py install /path/to/HelloPython --dry-run
  python3 Tools/AppInstaller/install.py caps
  python3 Tools/AppInstaller/install.py --serial DEVICE_SERIAL status 0x12345678
  python3 Tools/AppInstaller/install.py --serial DEVICE_SERIAL abort 0x12345678

--dry-run validates and prints the manifest without importing hidapi, opening USB,
or creating a package file. --serial appears before the command and is required
when multiple supported devices are connected. Device discovery covers the current
Mystrix1 and Mystrix2 PID families and vendor HID usage. macOS opens HID shared.

All regular files in the source folder are included, except __pycache__, .git,
.venv and .DS_Store. Symlinks and non-ASCII file paths are rejected in v1. Keep development-only files outside the
app folder if they should not ship. Local limits: 8 MiB package, 128 files, 32 KiB
manifest. Device-advertised limits can be smaller. MPKG v1 requires a .py entrypoint
and RGB-array AppInfo color, matching the documented standalone Python format.

Retries resend the identical request. On an unconfirmed operation the CLI prints
the session ID; use status before retrying or aborting. The CLI does not automatically
abort a failed transfer or claim success after a commit acknowledgement. It requires
a committed status matching the uploaded package's size and CRC32. BEGIN and FINISH
allow 60 seconds per reply for recovery and synchronous validation/commit. Cross-process
upload resume is not implemented; abort an incomplete upload and install again.

Protocol: PROTOCOL.txt. Firmware: OS/Commands/AppInstaller.cpp. There are no remote open/read/write/delete-file commands.

Host tests (simulated HID peer plus production C++ installer with host adapters;
the C++ test uses AddressSanitizer/UBSan and requires a C++17 compiler):
  python3 -m unittest discover -s Tests/AppInstaller -v

Journal recovery runs on the next BEGIN, inside the Developer APP context; it does
not add a boot hook. After interruption during directory switching, run install
again to recover the transaction before replacing the app.
