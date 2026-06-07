import cv2 as cv
import numpy as np
import sys

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

with open(path, "rb") as binFile:
    fileContents = binFile.read()

fileContentsCache = bytes()
numCalls = 0
fileOffset = 0


# this is specific to intSize == 5, a generic algorithm is definitely possible but I haven't bothered
def getFromFile():
    global numCalls
    global fileOffset
    global fileContentsCache
    if numCalls % 8 == 0:
        fileContentsCache = fileContents[fileOffset : fileOffset + intSize]
        fileOffset += intSize
        o = fileContentsCache[0] >> 3
    elif numCalls % 8 == 1:
        o = ((fileContentsCache[0] & 0x07) << 2) | ((fileContentsCache[1] & 0xC0) >> 6)
    elif numCalls % 8 == 2:
        o = (fileContentsCache[1] & 0x3E) >> 1
    elif numCalls % 8 == 3:
        o = ((fileContentsCache[1] & 0x01) << 4) | ((fileContentsCache[2] & 0xF0) >> 4)
    elif numCalls % 8 == 4:
        o = ((fileContentsCache[2] & 0x0F) << 1) | ((fileContentsCache[3] & 0x80) >> 7)
    elif numCalls % 8 == 5:
        o = (fileContentsCache[3] & 0x7C) >> 2
    elif numCalls % 8 == 6:
        o = ((fileContentsCache[3] & 0x03) << 3) | ((fileContentsCache[4] & 0xE0) >> 5)
    elif numCalls % 8 == 7:
        o = fileContentsCache[4] & 0x1F
    numCalls += 1
    return o


preview = False
currentColor = 0 if (getFromFile() == 0) else 255
currentPixel = 0
thisFrame = np.empty((hRes * vRes, 1), np.uint8)
newFrame = False

while fileOffset < len(fileContents):
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
