#include "tonc_bios.h"
#include "tonc_irq.h"
#include "tonc_memdef.h"
#include "tonc_memmap.h"
#include "tonc_video.h"

#include <string.h>

extern const unsigned char videoData[];

#define SCREEN_X M4_WIDTH
#define SCREEN_Y M4_HEIGHT

#define INPUT_X 24
#define INPUT_Y 16

#define X_DOWNSCALE (SCREEN_X / INPUT_X)
#define Y_DOWNSCALE (SCREEN_Y / INPUT_Y)
_Static_assert(
    X_DOWNSCALE % 2 == 0,
    "X downscale must be a multiple of 2 due to mode 4 byte-writing nonsense");

#define BLACK 0
#define WHITE 1

u16 *writePage = ((u16 *)MEM_VRAM);

u8 getFromFile() {
    static unsigned fileIdx = 0;
    static unsigned numCalls = 0;
    static u8 cache[7];
    if (numCalls % 8 == 0) {
        memcpy(&cache, &(videoData[fileIdx]), 7);
        fileIdx += 7;
    }
    u8 result;
    switch (numCalls % 8) {
        case 0:
            result = cache[0] >> 1;
            break;
        case 1:
            result = ((cache[0] & 0x01) << 6) | ((cache[1] & 0xFC) >> 2);
            break;
        case 2:
            result = ((cache[1] & 0x03) << 5) | ((cache[2] & 0xF8) >> 3);
            break;
        case 3:
            result = ((cache[2] & 0x07) << 4) | ((cache[3] & 0xF0) >> 4);
            break;
        case 4:
            result = ((cache[3] & 0x0F) << 3) | ((cache[4] & 0xE0) >> 5);
            break;
        case 5:
            result = ((cache[4] & 0x1F) << 2) | ((cache[5] & 0xC0) >> 6);
            break;
        case 6:
            result = ((cache[5] & 0x3F) << 1) | ((cache[6] & 0x80) >> 7);
            break;
        case 7:
            result = (cache[6] & 0x7F);
            break;
    }
    ++numCalls;
    return result;
}

int main() {
  irq_init(NULL);
  irq_add(II_VBLANK, NULL);

  pal_bg_mem[BLACK] = 0;
  pal_bg_mem[WHITE] = 0x7FFF;

  REG_DISPCNT = DCNT_MODE4 | DCNT_BG2;
  u8 currentColor = getFromFile();
  unsigned numChunks = 0;
  unsigned numChunksWritten = 0;
  bool newFrame = true;
  VBlankIntrWait();
  while (1) {
    numChunks = getFromFile();
    for (unsigned i = 0; i < numChunks; ++i) {
      for (unsigned y_off = 0; y_off < Y_DOWNSCALE; ++y_off) {
        for (unsigned x_off = 1; x_off < X_DOWNSCALE; x_off += 2) {
          writePage[((((numChunksWritten / INPUT_X) * Y_DOWNSCALE + y_off) *
                      SCREEN_X) +
                     ((numChunksWritten % INPUT_X) * X_DOWNSCALE + x_off)) /
                    2] = (currentColor << 8) | currentColor;
        }
      }
      ++numChunksWritten;
      if (numChunksWritten >= INPUT_X * INPUT_Y) {
        numChunksWritten = 0;
        newFrame = true;
        currentColor = (getFromFile() == 0) ? BLACK : WHITE;
        VBlankIntrWait();
        writePage = vid_flip();
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