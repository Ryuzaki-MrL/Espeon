#ifndef ESPEON_H
#define ESPEON_H

#include <Arduino.h>

extern volatile bool sram_modified;
extern uint8_t btn_directions;
extern uint8_t btn_faces;

typedef uint16_t fbuffer_t;
extern uint16_t palette[];

void espeon_update(void);
void espeon_init(void);
void espeon_faint(const char* msg);
fbuffer_t* espeon_get_framebuffer(void);
void espeon_clear_framebuffer(fbuffer_t col);
void espeon_end_frame(void);
void espeon_clear_screen(uint16_t col);
void espeon_set_palette(const uint32_t* col);
void espeon_render_border();
void espeon_save_sram(unsigned char* ram, uint32_t size);
void espeon_load_sram(unsigned char* ram, uint32_t size);
const unsigned char* espeon_load_rom(const char* path);

#endif
