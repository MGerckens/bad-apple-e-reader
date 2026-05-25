// #including data like this is bad practice, but if this demo requires >1
// source file then terrible things must have happened
#include "video.h"

#include "tonc.h"

#define SCREENBUFFER m3_mem

#define SCREEN_X M3_WIDTH
#define SCREEN_Y M3_HEIGHT

#define INPUT_X 24
#define INPUT_Y 16

#define X_DOWNSCALE (SCREEN_X / INPUT_X)
#define Y_DOWNSCALE (SCREEN_Y / INPUT_Y)

#define WHITE 0x7FFF
#define BLACK 0

int main() {
  irq_init(NULL);
  irq_add(II_VBLANK, NULL);

  REG_DISPCNT = DCNT_MODE3 | DCNT_BG2;
  unsigned fileOffset = 0;
  u16 currentColor = videoData[fileOffset++];
  unsigned numChunks = 0;
  unsigned numChunksWritten = 0;
  bool newFrame = true;
  VBlankIntrWait();
  while (1) {
    numChunks = videoData[fileOffset++];
    for (unsigned i = 0; i < numChunks; ++i) {
      for (unsigned x_off = 0; x_off < X_DOWNSCALE; ++x_off) {
        for (unsigned y_off = 0; y_off < Y_DOWNSCALE; ++y_off) {
          SCREENBUFFER[((numChunksWritten) / INPUT_X) * Y_DOWNSCALE + y_off]
                      [((numChunksWritten) % INPUT_X) * X_DOWNSCALE +
                       x_off] = currentColor;
        }
      }
      ++numChunksWritten;
      if (numChunksWritten >= INPUT_X * INPUT_Y) {
            numChunksWritten = 0;
            newFrame = true;
            currentColor = (videoData[fileOffset++] == 0) ? BLACK : WHITE;
            VBlankIntrWait();
          } else {
            newFrame = false;
          }
    }
    if (newFrame) {
    } else {
      currentColor = (currentColor == WHITE) ? BLACK : WHITE;
    }
  }

  return 0;
}