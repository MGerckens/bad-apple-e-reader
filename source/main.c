#include "def.h"
#include "erapi.h"

#define bool u8
#define true 1
#define false 0

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

unsigned currentSbb = 30;
inline static void flip() {
  // write one frame ahead then change actiuve screen block to prevent tearing
  if (currentSbb == 30) {
    currentSbb = 28;
    REG_BG0CNT = (30 << 8);

  } else {
    currentSbb = 30;
    REG_BG0CNT = (28 << 8);
  }
}

void memsetImpl(void* dest, u8 val, unsigned count){
  for(unsigned i = 0; i < count; ++i){
    *((u8*)dest + i) = val;
  }
}

int main() {
  BLDY = 0;
  SOUNDCNT_X &= ~0x80;

  pal_bg_mem[0] = 0x0000;
  pal_bg_mem[1] = 0x7FFF;

  memsetImpl(&tile_mem[0][0], 0, sizeof(TILE));
  memsetImpl(&tile_mem[0][1], 0x11, sizeof(TILE));

  REG_DISPCNT = 0x0100;   // mode 0 background 0
  REG_BG0CNT = (28 << 8); // CBB 0, SBB 28, 4bpp, 32x32

  while (true) {
    memsetImpl(&se_mem[30][0], 0, 32 * sizeof(SCREENBLOCK));
    memsetImpl(&se_mem[28][0], 0, 32 * sizeof(SCREENBLOCK));
    u8 currentColor = getFromFile(1);
    unsigned numChunks = 0;
    unsigned numChunksWritten = 0;
    bool newFrame = true;
    waitForNextFrame();
    while (fileBitIdx < (sizeof(videoData) * 8)) {
      numChunks = getFromFile(5);
      for (unsigned i = 0; i < numChunks; ++i) {
        se_mat[currentSbb][ERAPI_Div(numChunksWritten, INPUT_X)]
              [ERAPI_Mod(numChunksWritten, INPUT_X)] = currentColor;
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
