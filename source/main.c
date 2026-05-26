#include "tonc_bios.h"
#include "tonc_irq.h"
#include "tonc_memdef.h"
#include "tonc_memmap.h"
#include "tonc_video.h"

#include <string.h>
#include <stddef.h>

extern const unsigned char videoData[22918];

#define SCREEN_X M4_WIDTH
#define SCREEN_Y M4_HEIGHT

#define INPUT_X 30
#define INPUT_Y 20

#define FPS 3

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
  static u8 cache[5];
  u8 result;
  switch (numCalls % 8) {
  case 0:
    memcpy(&cache, &(videoData[fileIdx]), 5);
    fileIdx += 5;
    if (fileIdx >= sizeof(videoData)) {
      fileIdx = 0;
    }
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

static volatile unsigned numFrames = 0;
void vblankIntHandler(){
  ++numFrames;
}

void waitForNextFrame(){
  while(numFrames < (60 / FPS)){
    ;
  }
  numFrames = 0;
}

int main() {
  irq_init(NULL);
  irq_add(II_VBLANK, vblankIntHandler);

  pal_bg_mem[BLACK] = 0;
  pal_bg_mem[WHITE] = 0x7FFF;

  REG_DISPCNT = DCNT_MODE4 | DCNT_BG2;
  u8 currentColor = getFromFile();
  unsigned numChunks = 0;
  unsigned numChunksWritten = 0;
  bool newFrame = true;
  waitForNextFrame();
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
        waitForNextFrame();
        writePage = vid_flip();
      } else {
        newFrame = false;
      }
    }
    if (!newFrame) {
      currentColor = (currentColor == WHITE) ? BLACK : WHITE;
    }
  }

  return 0;
}