import shutil
import re
import argparse
from os.path import exists
from os import listdir
import sys

try:
    import psutil
except:
    print("psutil not installed, run \"pip install psutil\"")
    sys.exit(1)

print("UF2-Upload V1.0\nCopyright 2022 203 Electronics.\n");

parser = argparse.ArgumentParser(description='Host utilts for UF2 Bootloader Devices - 203 Electronics, 2022')
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

def get_uf2_drives(name = None):
    if name:
        name = name.replace("\'", "").replace("\"", "")
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

if(args.l):
    print(f"{len(uf2_devices)} device(s) detected:")
    for index, uf2_device in enumerate(uf2_devices):
        print(f"Device #{index + 1}: {uf2_device.model} ({uf2_device.board_id})")

if(args.f):
    file = args.f.replace("\'", "").replace("\"", "")
    if exists(file) is False:
        raise Exception("File does not exist!")
    filename = file.split('\\')[-1]
    if filename.endswith(".uf2") is False:
        raise Exception("File is not an UF2 file!")
    for index, uf2_device in enumerate(uf2_devices):
        print(f"Uploading {filename} to device {uf2_device.model} ({uf2_device.board_id})...")
        try: #UF2 disconnects after uploaded and copy will throw error
            shutil.copy2(file, uf2_device.mountpoint)
        except:
            pass
        print("Done Uploading\n")

        if(args.r != True):
            break
