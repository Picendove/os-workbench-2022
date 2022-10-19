#include <game.h>

#define SIDE 16
static int w, h;

//**
static int location_x = 0;
static int location_y = 0;
//**

static void init() {
  AM_GPU_CONFIG_T info = {0};
  ioe_read(AM_GPU_CONFIG, &info);
  w = info.width;
  h = info.height;
}

static void draw_tile(int x, int y, int w, int h, uint32_t color) {
  uint32_t pixels[w * h]; // WARNING: large stack-allocated memory
  AM_GPU_FBDRAW_T event = {
    .x = x, .y = y, .w = w, .h = h, .sync = 1,
    .pixels = pixels,
  };
  for (int i = 0; i < w * h; i++) {
    pixels[i] = color;
  }
  ioe_write(AM_GPU_FBDRAW, &event);
}

void splash(const char operate) {
  init();
  if(operate == 'W' && location_y != h/16) {
    ++location_y;
  }else if(operate == 'A' && location_x != 0) {
    --location_x;
  }else if(operate == 'S' && location_y != 0) {
    --location_y;
  }else if(operate == 'D' && location_x != w/16) {
    ++location_x;
  }else{;
  }

  for (int x = 0; x * SIDE <= w; x ++) {
    for (int y = 0; y * SIDE <= h; y++) {
      if((x == location_x) && (y == location_y)) {
        draw_tile(x * SIDE, y * SIDE, SIDE, SIDE, 0x0000ff); // blue
      } else {
        draw_tile(x * SIDE, y * SIDE, SIDE, SIDE, 0xffffff); // white
      }
    }
  }
}
