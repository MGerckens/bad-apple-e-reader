#include "def.h"

#define bool u8
#define true 1
#define false 0

// copied structs and memory mappings from libtonc, to avoid linking it, which
// saves about 1.5kB
#define REG_VCOUNT (*(vu16 *)0x04000006)
#define REG_DISPCNT (*(vu16 *)0x04000000)
#define pal_bg_mem ((vu16 *)0x05000000)
#define SOUNDCNT_X (*(vu16 *)0x04000084)
#define BLDY (*(vu16 *)0x04000054)

inline static void vid_vsync() {
  while (REG_VCOUNT >= 160)
    ; // wait till VDraw
  while (REG_VCOUNT < 160)
    ; // wait till VBlank
}

extern const unsigned char videoData[22589];

#define SCREEN_X 240
#define SCREEN_Y 160

#define INPUT_X 30
#define INPUT_Y 20

#define FPS 3

#define X_DOWNSCALE (SCREEN_X / INPUT_X)
#define Y_DOWNSCALE (SCREEN_Y / INPUT_Y)

unsigned fileBitIdx = 0;
u8 getFromFile(unsigned numBits) {
  u8 idxMod8 = fileBitIdx % 8;

  u8 mask1 = ((1 << (numBits)) - 1) << (8 - numBits) >> idxMod8;
  u8 leftoverBits = (numBits < (8 - idxMod8)) ? 0 : numBits - (8 - idxMod8);
  u8 mask2 = ((1 << leftoverBits) - 1) << (8 - leftoverBits);

  u8 val = videoData[fileBitIdx / 8];
  val &= mask1;

  u8 result = 0;
  if (leftoverBits != 0) {
    u8 valPart2 = videoData[(fileBitIdx + numBits) / 8];
    valPart2 &= mask2;
    valPart2 >>= (8 - leftoverBits);
    result = (val << leftoverBits) | valPart2;
  } else {
    result = val >> ((8 - numBits) - idxMod8);
  }

  fileBitIdx += numBits;
  return result;
}

inline static void waitForNextFrame() {
  for (int i = 0; i < (60 / FPS); ++i) {
    vid_vsync();
  }
}

u16 *writePage = ((u16 *)0x06000000);
inline static void flip() {
  writePage = (u16 *)((u32)writePage ^ 0x0A000);
  REG_DISPCNT ^= 0x0010; // update control register
}

int main() {
  BLDY = 0;
  SOUNDCNT_X &= ~0x80;

  pal_bg_mem[0] = 0x0000;
  pal_bg_mem[1] = 0x7FFF;

  REG_DISPCNT = 0x0414; // mode 0 background 0

  while (true) {
    u8 currentColor = getFromFile(1);
    unsigned numChunks = 0;
    unsigned numChunksWritten = 0;
    bool newFrame = true;
    waitForNextFrame();
    while (fileBitIdx < (sizeof(videoData) * 8)) {
      numChunks = getFromFile(5);
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
          currentColor = (getFromFile(1) == 0) ? 0 : 1;
          waitForNextFrame();
          flip();
        } else {
          newFrame = false;
        }
      }
      if (!newFrame) {
        currentColor = (currentColor == 0) ? 1 : 0;
      }
    }
    fileBitIdx = 0;
  }

  return 0;
}
