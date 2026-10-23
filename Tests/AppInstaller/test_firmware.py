"""Run the production C++ installer with host-only filesystem/RTOS adapters."""
import importlib.util
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("firmware_installer_cli", ROOT / "Tools/AppInstaller/install.py")
cli = importlib.util.module_from_spec(spec)
spec.loader.exec_module(cli)


class FirmwareTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("c++"), "C++ compiler required")
    def test_production_state_machine_and_commit_recovery(self):
        with tempfile.TemporaryDirectory(prefix="matrixos-installer-") as directory:
            tmp = Path(directory)
            info = b'{"name":"Test","author":"Test","version":1,"color":[0,255,0],"osMinimalVer":[4,0,0],"appMainFile":"main.py"}'
            package = cli.build_package("TestApp", {"AppInfo.json": info,
                "main.py": b"def loop():\n    pass\n", "empty.bin": b"", "resources/test.txt": b"data"})
            (tmp / "input.mpkg").write_bytes(package)
            binary = tmp / "firmware-test"
            subprocess.run(["c++", "-std=c++17", "-Wall", "-Wextra", "-Werror",
                "-fsanitize=address,undefined", "-g", "-I", str(Path(__file__).parent),
                "-I", str(ROOT / "Library/ArduinoJson/src"),
                str(Path(__file__).with_suffix(".cpp")), "-o", str(binary)], check=True,
                capture_output=True, text=True)
            result = subprocess.run([str(binary), str(tmp / "card"), str(tmp / "input.mpkg")],
                                    check=True, capture_output=True, text=True)
            self.assertIn("data preservation passed", result.stdout)


if __name__ == "__main__":
    unittest.main()
