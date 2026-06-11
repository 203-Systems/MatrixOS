import shutil
import re
import argparse
from os.path import basename, exists
from os import listdir
import sys
import time

try:
    import psutil
except:
    print("psutil not installed, run \"pip install psutil\"")
    sys.exit(1)

print("UF2-Upload V1.0\nCopyright 2024 203 Systems.\n");

MATRIXOS_VID = 0x0203
MYSTRIX_PID_MASK = 0xFFC0
MYSTRIX_PID_BASES = {0x1040, 0x1080}
SYSTEM_REPORT_ID = 0xCB
MATRIXOS_COMMAND_BOOTLOADER = 0x40
HID_REPORT_SIZES = (63, 32)
HID_WRITE_DELAY = 0.05
BOOTLOADER_WAIT_TIMEOUT = 10.0
BOOTLOADER_WAIT_INTERVAL = 0.25
MATRIXOS_SYSTEM_HID_COLLECTION = "&col06"
MATRIXOS_SYSTEM_HID_INTERFACE = 4
MATRIXOS_SYSTEM_HID_USAGE_PAGE = 0xFF00
MATRIXOS_SYSTEM_HID_USAGE = 1

parser = argparse.ArgumentParser(description='Host utilts for UF2 Bootloader Devices - 203 Systems, 2022')
parser.add_argument('-f',  metavar='<file path>', type=ascii,
                    help='UF2 file to upload to target')
parser.add_argument('-d', metavar='<device name>', type=ascii,
                    help='Target UF2 Device name (Model in INFO_UF2.TXT)')
parser.add_argument('-r', action='store_true',
                    help='Repeat upload for matched UF2 devices')
parser.add_argument('-l', action='store_true',
                    help='List all matched UF2 devices')

args = parser.parse_args()

if len(sys.argv) < 2:
    parser.print_help()
    sys.exit(1)


class uf2:
    mountpoint = None
    info = None
    model = None
    board_id = None
    date = None
    def __repr__(self):
        return f"UF2(mountpoing='{self.mountpoint}'. info='{self.info}', model='{self.model}', board_id='{self.board_id}', date='{self.date}'"

def get_arg_value(value):
    if value:
        return value.replace("\'", "").replace("\"", "")
    return None

def is_mystrix_matrixos_hid(device):
    vendor_id = device.get("vendor_id")
    product_id = device.get("product_id")
    product_string = device.get("product_string") or ""

    if vendor_id != MATRIXOS_VID:
        return False

    if product_id is not None and (product_id & MYSTRIX_PID_MASK) in MYSTRIX_PID_BASES:
        return True

    return "Mystrix" in product_string

def hid_path_text(device):
    path = device.get("path")
    if isinstance(path, bytes):
        return path.decode("utf-8", "replace")
    return str(path)

def is_matrixos_system_hid(device):
    path = hid_path_text(device).lower()
    if MATRIXOS_SYSTEM_HID_COLLECTION in path:
        return True

    return device.get("usage_page") == MATRIXOS_SYSTEM_HID_USAGE_PAGE and \
        device.get("interface_number") == MATRIXOS_SYSTEM_HID_INTERFACE and \
        device.get("usage") == MATRIXOS_SYSTEM_HID_USAGE

def get_matrixos_hid_devices():
    try:
        import hid
    except:
        print("hidapi not installed, run \"pip install hidapi\" to auto-enter bootloader from MatrixOS")
        return []

    devices = [device for device in hid.enumerate() if is_mystrix_matrixos_hid(device)]
    system_devices = [device for device in devices if is_matrixos_system_hid(device)]
    system_devices.sort(key = hid_path_text, reverse = True)
    devices.sort(key = hid_path_text, reverse = True)
    return system_devices or devices

def make_bootloader_report(report_size):
    return bytes([SYSTEM_REPORT_ID, MATRIXOS_COMMAND_BOOTLOADER] + [0] * (report_size - 1))

def send_bootloader_command(device):
    try:
        import hid

        hid_device = hid.device()
        hid_device.open_path(device["path"])
        sent = False
        last_error = None
        for report_size in HID_REPORT_SIZES:
            try:
                sent = hid_device.write(make_bootloader_report(report_size)) > 0
                if sent:
                    break
            except Exception as error:
                last_error = error
        if not sent and last_error is not None:
            raise last_error
        if sent:
            time.sleep(HID_WRITE_DELAY)
        hid_device.close()
        return sent
    except Exception as error:
        product = device.get("product_string") or "Unknown HID"
        print(f"Failed to send bootloader command to {product}: {error}")
        return False

def enter_bootloader_from_matrixos(name = None, repeat = False):
    devices = get_matrixos_hid_devices()
    if not devices:
        return []

    print("MatrixOS device detected. Entering bootloader...")
    for device in (devices if repeat else devices[:1]):
        if not send_bootloader_command(device):
            continue

        uf2_devices = wait_for_uf2_drives(name)
        if uf2_devices:
            return uf2_devices

    return []

def wait_for_uf2_drives(name = None, timeout = BOOTLOADER_WAIT_TIMEOUT):
    deadline = time.time() + timeout
    while time.time() < deadline:
        uf2_devices = get_uf2_drives(name)
        if uf2_devices:
            return uf2_devices
        time.sleep(BOOTLOADER_WAIT_INTERVAL)
    return []

def enter_bootloader_and_wait(name = None, repeat = False):
    uf2_devices = enter_bootloader_from_matrixos(name, repeat)
    if uf2_devices:
        return uf2_devices

    print("Waiting for UF2 drive...")
    return wait_for_uf2_drives(name)

def get_uf2_drives(name = None):
    name = get_arg_value(name)
    mount_drives = []

    if sys.platform == "darwin":
        volumes = listdir("/Volumes")

        def mount_map(volume):
            return f"/Volumes/{volume}/"

        mount_drives = list(map(mount_map, volumes))
    else:
        disk_partitions = list(filter(lambda x: 'removable' in x.opts, psutil.disk_partitions()))

        def mount_map(key):
            return key.mountpoint

        mount_drives = list(map(mount_map, disk_partitions))

    uf2_devices = []
    for drive_mount in mount_drives:
        if exists(drive_mount + "INFO_UF2.TXT"):
            with open(drive_mount + 'INFO_UF2.TXT', "r") as f:
                info = f.read()
                uf2_device = uf2()
                uf2_device.mountpoint = drive_mount
                uf2_device.info = info.partition('\n')[0]
                uf2_device.model = re.search('(?:Model: )([^\r\n]+)', info).group(1)
                uf2_device.board_id = re.search('(?:Board-ID: )([^\r\n]+)', info).group(1)
                uf2_device.date = re.search('(?:Date: )([^\r\n]+)', info).group(1)
                if(name == None or bool(re.search(name, uf2_device.model))):
                    uf2_devices.append(uf2_device)
        else:
            print("INFO_UF2.TXT does not exists")
    return uf2_devices

uf2_devices = get_uf2_drives(args.d)

if(args.f):
    file = get_arg_value(args.f)
    if exists(file) is False:
        raise Exception("File does not exist!")
    filename = basename(file)
    if filename.endswith(".uf2") is False:
        raise Exception("File is not an UF2 file!")

    if not uf2_devices:
        uf2_devices = enter_bootloader_and_wait(args.d, args.r)

if(args.l):
    print(f"{len(uf2_devices)} device(s) detected:")
    for index, uf2_device in enumerate(uf2_devices):
        print(f"Device #{index + 1}: {uf2_device.model} ({uf2_device.board_id})")

if(args.f):
    for index, uf2_device in enumerate(uf2_devices):
        print(f"Uploading {filename} to device {uf2_device.model} ({uf2_device.board_id})...")
        try: #UF2 disconnects after uploaded and copy will throw error
            shutil.copy2(file, uf2_device.mountpoint)
        except:
            pass
        print("Done Uploading\n")

        if(args.r != True):
            break
