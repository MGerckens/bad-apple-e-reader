// #including data like this is bad practice, but if this demo requires >1
// source file then terrible things must have happened
#include "video.h"

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;

#define REG_DISPLAYCONTROL *((volatile uint32 *)(0x04000000))
#define VIDEOMODE_3 0x0003
#define BGMODE_2 0x0400

#define SCREENBUFFER ((volatile uint16 *)0x06000000)
#define SCREEN_X 240
#define SCREEN_Y 160

#define INPUT_X 24
#define INPUT_Y 16

#define X_DOWNSCALE (SCREEN_X / INPUT_X)
#define Y_DOWNSCALE (SCREEN_Y / INPUT_Y)

#define WHITE 0x7FFF
#define BLACK 0

#define REG_VCOUNT (*(volatile uint16 *)0x04000006)
void vsync() {
  while (REG_VCOUNT >= SCREEN_Y)
    ;
  while (REG_VCOUNT < SCREEN_Y)
    ;
  ;
}

int main() {
  REG_DISPLAYCONTROL = VIDEOMODE_3 | BGMODE_2;
  uint16 fileOffset = 0;
  uint16 currentColor = videoData[fileOffset++];
  int temp = 0;
  uint16 currentInPixel = 0;
  uint16 numPixelsWritten = 0;
  uint8 numChunks = 0;
  uint16 numChunksWritten = 0;
  uint8 newFrame = 1;
  vsync();
  while (1) {
    numChunks = videoData[fileOffset++];
    for (uint8 i = 0; i < numChunks; ++i) {
      for (uint8 x_off = 0; x_off < X_DOWNSCALE; ++x_off) {
        for (uint8 y_off = 0; y_off < Y_DOWNSCALE; ++y_off) {
          temp = (((numChunksWritten + i) % INPUT_X) * X_DOWNSCALE + x_off) +
                 ((((numChunksWritten + i) / INPUT_X) * Y_DOWNSCALE + y_off) *
                  SCREEN_X);

          SCREENBUFFER[temp] = currentColor;
          ++numPixelsWritten;
          if (numPixelsWritten >= SCREEN_X * SCREEN_Y) {
            numPixelsWritten = 0;
            newFrame = 1;
            currentColor = (videoData[fileOffset++] == 0) ? BLACK : WHITE;
            vsync();
          } else {
            newFrame = 0;
          }
        }
      }
    }
    if (newFrame) {
      numChunksWritten = 0;
    } else {
      currentColor = (currentColor == WHITE) ? BLACK : WHITE;
      numChunksWritten += numChunks;
    }
  }

  return 0;
}