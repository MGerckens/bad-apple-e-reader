# words cannot express my hatred of makefiles
import subprocess
import pathlib

pathlib.Path("C:/Projects/BadAppleGBA/BadAppleGBA_mb.bin").unlink(missing_ok=True)
pathlib.Path("C:/Projects/BadAppleGBA/BadAppleGBA_mb.vpk").unlink(missing_ok=True)
pathlib.Path("C:/Projects/BadAppleGBA/BadAppleGBA_mb.sav").unlink(missing_ok=True)
for rawFile in pathlib.Path(".").glob("*.raw"):
    rawFile.unlink()

subprocess.run("C:/devkitPro/devkitARM/arm-none-eabi/bin/objcopy.exe -O binary C:/Projects/BadAppleGBA/BadAppleGBA_mb.elf C:/Projects/BadAppleGBA/BadAppleGBA_mb.bin")
with open("C:/Projects/BadAppleGBA/BadAppleGBA_mb.bin", "rb") as inFile:
    fileContents = bytearray(inFile.read())
fileContents[0xC4] = 1
fileContents[0xC5] = 1
with open("C:/Projects/BadAppleGBA/BadAppleGBA_mb.bin", "wb") as outFile:
    outFile.write(fileContents)
subprocess.run("C:/Projects/BadAppleEncode/GBADevEnv/nevpk.exe -i C:/Projects/BadAppleGBA/BadAppleGBA_mb.bin -o C:/Projects/BadAppleGBA/BadAppleGBA_mb.vpk -c -level 2")
subprocess.run("C:/Projects/BadAppleEncode/GBADevEnv/neflmake.exe -i C:/Projects/BadAppleGBA/BadAppleGBA_mb.vpk -o C:/Projects/BadAppleGBA/BadAppleGBA_mb.sav -type 2 -name \"Bad Apple\" -region 1 -v")
subprocess.run("C:/Projects/BadAppleEncode/GBADevEnv/nedcmake.exe -i C:/Projects/BadAppleGBA/BadAppleGBA_mb.vpk -o C:/Projects/BadAppleGBA/BadAppleGBA_mb -type 2 -region 1 -name \"Bad Apple\" -fill 1 -save 1")

