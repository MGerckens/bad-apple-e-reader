import cv2 as cv
import numpy as np
import sys
import bitarray
import bitarray.util

if len(sys.argv) < 2:
    raise ValueError("Pass the path to the input bin file as the first cmdline arg")
path = sys.argv[1]

hDownscale = 8
vDownscale = 8
fps = 3
intSize = 5

hRes = 240 // hDownscale
vRes = 160 // vDownscale

writer = cv.VideoWriter(
    "output.avi", cv.VideoWriter_fourcc(*"DX50"), 30.0, (hRes, vRes), False
)

fileContents = bitarray.bitarray()
with open(path, "rb") as binFile:
    fileContents.frombytes(binFile.read())

numCalls = 0

bitOffset = 0
def getFromFile(numBits=5):
    global bitOffset
    val = bitarray.util.ba2int(fileContents[bitOffset : bitOffset + numBits])
    bitOffset += numBits
    return val

preview = False
currentColor = 0 if (getFromFile() == 0) else 255
currentPixel = 0
thisFrame = np.empty((hRes * vRes, 1), np.uint8)
newFrame = False

while bitOffset < len(fileContents):
    numPixels = getFromFile()
    for i in range(0, numPixels):
        thisFrame[currentPixel] = currentColor
        currentPixel += 1
        if currentPixel >= hRes * vRes:
            currentColor = 0 if (getFromFile() == 0) else 255
            currentPixel = 0
            frame = np.reshape(thisFrame, (vRes, hRes))
            if preview:
                cv.imshow("frame", frame)
                if cv.waitKey(1) & 0xFF == ord("q"):
                    break
            for i in range(0, 30 // fps):
                writer.write(frame)
            thisFrame = np.empty((hRes * vRes, 1), np.uint8)
            newFrame = True
        else:
            newFrame = False
    if not newFrame:
        currentColor = 255 if currentColor == 0 else 0
