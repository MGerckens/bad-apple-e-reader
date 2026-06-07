# words cannot express my hatred of makefiles
import subprocess
import pathlib
import sys


# yoinked from https://stackoverflow.com/a/54240102
def get_win_path(cyg_path):
    return (
        subprocess.check_output(["cygpath", "-w", cyg_path], shell=True)
        .strip(b"\n")
        .decode()
    )


if sys.platform == "win32" or sys.platform == "cygwin":
    eReaderToolsDir = pathlib.Path(get_win_path(sys.argv[1]))
    neflmakeDir = pathlib.Path(get_win_path(sys.argv[2]))
    devkitArmDir = pathlib.Path(get_win_path(sys.argv[3]))
else:
    eReaderToolsDir = pathlib.Path(sys.argv[1])
    neflmakeDir = pathlib.Path(sys.argv[2])
    devkitArmDir = pathlib.Path(sys.argv[3])

pathlib.Path("BadAppleGBA_mb.bin").unlink(missing_ok=True)
pathlib.Path("BadAppleGBA_mb.vpk").unlink(missing_ok=True)
pathlib.Path("BadAppleGBA_mb.sav").unlink(missing_ok=True)
for rawFile in pathlib.Path(".").glob("*.raw"):
    rawFile.unlink()

subprocess.run(
    [
        str(devkitArmDir / "objcopy.exe"),
        "-O",
        "binary",
        "BadAppleGBA_mb.elf",
        "BadAppleGBA_mb.bin",
    ],
    shell=True,
    capture_output=True,
)

# mark as multiboot
with open("BadAppleGBA_mb.bin", "rb") as inFile:
    fileContents = bytearray(inFile.read())
fileContents[0xC4] = 1
fileContents[0xC5] = 1
with open("BadAppleGBA_mb.bin", "wb") as outFile:
    outFile.write(fileContents)

subprocess.run(
    [
        str(eReaderToolsDir / "nevpk.exe"),
        "-i",
        "BadAppleGBA_mb.bin",
        "-o",
        "BadAppleGBA_mb.vpk",
        "-c",
        "-level",
        "2",
    ],
    shell=True,
)
subprocess.run(
    [
        str(neflmakeDir / "neflmake.exe"),
        "-i",
        "BadAppleGBA_mb.vpk",
        "-o",
        "BadAppleGBA_mb.sav",
        "-type",
        "2",
        "-name",
        "Bad Apple!!",
        "-region",
        "1",
        "-v",
    ],
    shell=True,
)

thisDir = pathlib.Path(".").resolve()
subprocess.run(
    [
        "nedcmake.exe",
        "-i",
        str(thisDir / "BadAppleGBA_mb.vpk"),
        "-o",
        str(thisDir / "BadAppleGBA_mb"),
        "-type",
        "2",
        "-region",
        "1",
        "-name",
        "Bad Apple!!",
        "-fill",
        "1",
        "-save",
        "1",
    ],
    shell=True,
    cwd=str(eReaderToolsDir),
)
