#include "def.h"
#include "erapi.h"
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

// copied structs and memory mappings from libtonc, to avoid linking it, which
// saves about 1.5kB
#define REG_VCOUNT (*(vu16 *)0x04000006)
#define REG_DISPCNT (*(vu16 *)0x04000000)
#define REG_BG0CNT (*(vu16 *)0x04000008)
#define pal_bg_mem ((vu16 *)0x05000000)
#define REG_BG0HOFS (*(vu16 *)(0x04000010))
#define REG_BG0VOFS (*(vu16 *)(0x04000012))
#define SOUNDCNT_X (*(vu16 *)0x04000084)
#define BLDY (*(vu16 *)0x04000054)

typedef u16 SCREENMAT[32][32];
typedef u16 SCREENBLOCK[1024];
typedef struct {
  u32 data[8];
} TILE;
typedef TILE CHARBLOCK[512];

#define se_mat ((SCREENMAT *)0x06000000)
#define se_mem ((SCREENBLOCK *)0x06000000)
#define tile_mem ((CHARBLOCK *)0x06000000)

void vid_vsync() {
  while (REG_VCOUNT >= 160)
    ; // wait till VDraw
  while (REG_VCOUNT < 160)
    ; // wait till VBlank
}

extern const unsigned char videoData[22918];

const unsigned char tileset[64] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11,
    0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
    0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
    0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11};

#define SCREEN_X 240
#define SCREEN_Y 160

#define INPUT_X 30
#define INPUT_Y 20

#define FPS 3

#define X_DOWNSCALE (SCREEN_X / INPUT_X)
#define Y_DOWNSCALE (SCREEN_Y / INPUT_Y)
_Static_assert(
    X_DOWNSCALE % 2 == 0,
    "X downscale must be a multiple of 2 due to mode 4 byte-writing nonsense");

unsigned fileIdx = 0;
unsigned numCalls = 0;
u8 getFromFile() {
  static u8 cache[5];
  u8 result;
  switch (numCalls % 8) {
  case 0:
    memcpy(&cache, &(videoData[fileIdx]), 5);
    fileIdx += 5;
    result = cache[0] >> 3;
    break;
  case 1:
    result = ((cache[0] & 0x07) << 2) | ((cache[1] & 0xC0) >> 6);
    break;
  case 2:
    result = (cache[1] & 0x3E) >> 1;
    break;
  case 3:
    result = ((cache[1] & 0x01) << 4) | ((cache[2] & 0xF0) >> 4);
    break;
  case 4:
    result = ((cache[2] & 0x0F) << 1) | ((cache[3] & 0x80) >> 7);
    break;
  case 5:
    result = (cache[3] & 0x7C) >> 2;
    break;
  case 6:
    result = ((cache[3] & 0x03) << 3) | ((cache[4] & 0xE0) >> 5);
    break;
  case 7:
    result = (cache[4] & 0x1F);
    break;
  }
  ++numCalls;
  return result;
}

void waitForNextFrame() {
  for (int i = 0; i < (60 / FPS); ++i) {
    vid_vsync();
  }
}

unsigned currentSbb = 30;
void flip() {
  // write one frame ahead then change actiuve screen block to prevent tearing
  if (currentSbb == 30) {
    currentSbb = 28;
    REG_BG0CNT = (30 << 8);

  } else {
    currentSbb = 30;
    REG_BG0CNT = (28 << 8);
  }
}

int main() {
  BLDY = 0;
  SOUNDCNT_X &= ~0x80;

  pal_bg_mem[0] = 0x0000;
  pal_bg_mem[1] = 0x7FFF;

  memcpy(&tile_mem[0][0], &tileset[0], sizeof(tileset));
  REG_BG0HOFS = 0;
  REG_BG0VOFS = 0;

  // REG_BG0CNT = BG_CBB(0) | BG_SBB(30) | BG_4BPP | BG_REG_32x32;
  // REG_DISPCNT = DCNT_MODE0 | DCNT_BG0;
  REG_DISPCNT = 0x0100;   // mode 0 background 0
  REG_BG0CNT = (28 << 8); // CBB 0, SBB 28, 4bpp, 32x32
  ERAPI_LayerShow(0);

  while (true) {
    memset(&se_mem[30][0], 0, 32 * sizeof(SCREENBLOCK));
    memset(&se_mem[28][0], 0, 32 * sizeof(SCREENBLOCK));
    u8 currentColor = getFromFile();
    unsigned numChunks = 0;
    unsigned numChunksWritten = 0;
    bool newFrame = true;
    waitForNextFrame();
    while (fileIdx < sizeof(videoData)) {
      numChunks = getFromFile();
      for (unsigned i = 0; i < numChunks; ++i) {
        se_mat[currentSbb][ERAPI_Div(numChunksWritten, INPUT_X)]
              [ERAPI_Mod(numChunksWritten, INPUT_X)] = currentColor;
        ++numChunksWritten;
        if (numChunksWritten >= INPUT_X * INPUT_Y) {
          numChunksWritten = 0;
          newFrame = true;
          currentColor = (getFromFile() == 0) ? 0 : 1;
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
    numCalls = 0;
    fileIdx = 0;
  }

  return 0;
}
