import cv2 as cv
import sys
import bitarray
import bitarray.util

hDownscale = 8
assert 240 % hDownscale == 0
vDownscale = 8
assert 160 % vDownscale == 0
fps = 3
assert 30 % fps == 0

# max size of segment in bits
intSize = 5

if len(sys.argv < 2):
    raise ValueError("Pass the path to the input video file as the first cmdline arg")
path = sys.argv[1]
cap = cv.VideoCapture(path)

out = bitarray.bitarray()
frameNum = 0 
maxNumPixels = 0

def addToArray(out : bitarray.bitarray, num:int):
    while num >= (2 ** intSize):
        out.extend(bitarray.util.int2ba(2 ** intSize - 1, intSize))
        num -= (2 ** intSize - 1)
        out.extend(bitarray.util.int2ba(0, intSize))
    out.extend(bitarray.util.int2ba(num, intSize))

while cap.isOpened():
    ret, ogFrame = cap.read()
    if not ret:
        break
    if frameNum % (30 // fps) != 0:
        frameNum += 1
        continue
    
    _, frame = cv.threshold(
        cv.cvtColor(ogFrame, cv.COLOR_BGR2GRAY), 127, 255, cv.THRESH_BINARY
    )
    startV, startH = frame.shape
    endH = 240 // hDownscale
    endV = 160 // vDownscale
    hRatio = endH / startH
    vRatio = endV / startV
    smallFrame = cv.resize(frame, None, None, hRatio, vRatio, cv.INTER_AREA)
    currentColor = smallFrame[0][0] != 0
    addToArray(out, 1 if currentColor != 0 else 0)
    
    numPixelsOfThisColor = 0
    for row in range(0, endV):
        for col in range(0, endH):
            color = (smallFrame[row][col] != 0)
            if color == currentColor:
                numPixelsOfThisColor += 1
            else:
                addToArray(out, numPixelsOfThisColor)
                numPixelsOfThisColor = 1
                currentColor = color
    addToArray(out, numPixelsOfThisColor)
    if frameNum % 30 == 0:
        print("frame ", frameNum, "done")
    frameNum += 1

with open("out.bin", "wb") as outFile:
    outFile.write(out)

cap.release()
cv.destroyAllWindows()
