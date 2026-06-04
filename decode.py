import cv2 as cv
import numpy as np
import sys

if len(sys.argv < 2):
    raise ValueError("Pass the path to the input bin file as the first cmdline arg")
path = sys.argv[1]

hDownscale = 10
vDownscale = 10
fps = 3

hRes = 240 // hDownscale
vRes = 160 // vDownscale

writer = cv.VideoWriter("output.avi", cv.VideoWriter_fourcc(*"DX50"), 30.0, (hRes, vRes), False)

frameNum = 0
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
        fileContentsCache = fileContents[fileOffset:fileOffset+7]
        fileOffset += 7
    if numCalls % 8 == 0:
        o = fileContentsCache[0] >> 5
    elif numCalls % 8 == 1:
        o = ((fileContentsCache[0] & 0x07) << 2) | ((fileContentsCache[1] & 0xC0) >> 6)
    elif numCalls % 8 == 2:
        o = (fileContentsCache[1] & 0x3E) >> 1
    elif numCalls % 8 == 3:
        o = ((fileContentsCache[1] & 0x01) << 4) | ((fileContentsCache[2] & 0xF0) >> 4)
    elif numCalls % 8 == 4:
        o = (fileContentsCache[2] & 0x0F) | ((fileContentsCache[3] & 0x80) >> 7)
    elif numCalls % 8 == 5:
        o = (fileContentsCache[3] & 0x7C) >> 2
    elif numCalls % 8 == 6:
        o = ((fileContentsCache[3] & 0x03) << 3) | ((fileContentsCache[4] & 0xE0) >> 5)
    elif numCalls % 8 == 7:
        o = (fileContentsCache[4] & 0x1F)
    numCalls += 1
    return o

preview = False

while fileOffset < len(fileContents):
    currentColor = 0 if (getFromFile() == 0) else 255
    currentPixel = 0
    thisFrame = np.empty((hRes * vRes, 1), np.uint8)
    while currentPixel < (hRes * vRes):
        numPixels = getFromFile()
        thisFrame[currentPixel:currentPixel + numPixels].fill(currentColor)
        currentPixel += numPixels
        currentColor = 255 if currentColor == 0 else 0
    frame = np.reshape(thisFrame, (vRes, hRes))
    if preview:
        cv.imshow('frame',frame)
        if cv.waitKey(1) & 0xFF == ord('q'):
            break
    frameNum += 1
    for i in range(0, 30 // fps):
        writer.write(frame)
    