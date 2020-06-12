#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include "lcd.h"
#include "interrupt.h"
#include "espeon.h"
#include "mem.h"

#define MODE2_BOUNDS 	(204/4)
#define MODE3_BOUNDS 	(284/4)
#define MODE0_BOUNDS 	(456/4)
#define SCANLINE_CYCLES	(456/4)

struct sprite {
	int16_t y, x, tile, flags;
};

static uint8_t lcd_line;
static uint8_t lcd_stat;
static uint8_t lcd_ly_compare;
static uint32_t lcd_cycles;
volatile uint8_t skip_frames;

static QueueHandle_t lcdqueue;

/* LCD STAT */
static uint8_t ly_int;
static uint8_t mode2_oam_int;
static uint8_t mode1_vblank_int;
static uint8_t mode0_hblank_int;
static uint8_t ly_int_flag;
static uint8_t lcd_mode;
static uint8_t lcd_stat_tracker;

/* LCD Context */
struct LCDC {
	uint8_t lcd_enabled = 1;
	uint8_t lcd_line;
	uint8_t window_tilemap_select;
	uint8_t window_enabled;
	uint8_t tilemap_select;
	uint8_t bg_tiledata_select;
	uint8_t sprite_size;
	uint8_t sprites_enabled;
	uint8_t bg_enabled;
	uint8_t bg_palette;
	uint8_t spr_palette1;
	uint8_t spr_palette2;
	uint8_t scroll_x;
	uint8_t scroll_y;
	uint8_t window_x;
	uint8_t window_y;
};

static LCDC lcdc;

static uint8_t bgpalette[] = {3, 2, 1, 0};
static uint8_t sprpalette1[] = {0, 1, 2, 3};
static uint8_t sprpalette2[] = {0, 1, 2, 3};

enum {
	PRIO  = 0x80,
	VFLIP = 0x40,
	HFLIP = 0x20,
	PNUM  = 0x10
};

static inline void lcd_match_lyc()
{
	ly_int_flag = (lcd_line == lcd_ly_compare);
	if(ly_int && ly_int_flag) {
		interrupt(INTR_LCDSTAT);
	}
}

void lcd_reset(void)
{
	xQueueReset(lcdqueue);
	espeon_clear_framebuffer(palette[0x00]);
	lcd_mode = 1;
	lcd_line = 0;
	lcd_cycles = 0;
}

uint8_t lcd_get_stat(void)
{
	return lcd_stat | (ly_int_flag<<2) | lcd_mode;
}

static inline void lcd_set_palettes(const LCDC& lcdc)
{
	uint8_t n = lcdc.bg_palette;
	bgpalette[0] = (n>>0)&3;
	bgpalette[1] = (n>>2)&3;
	bgpalette[2] = (n>>4)&3;
	bgpalette[3] = (n>>6)&3;
	n = lcdc.spr_palette1;
	sprpalette1[1] = (n>>2)&3;
	sprpalette1[2] = (n>>4)&3;
	sprpalette1[3] = (n>>6)&3;
	n = lcdc.spr_palette2;
	sprpalette2[1] = (n>>2)&3;
	sprpalette2[2] = (n>>4)&3;
	sprpalette2[3] = (n>>6)&3;
}

void lcd_write_bg_palette(uint8_t n)
{
	lcdc.bg_palette = n;
}

void lcd_write_spr_palette1(uint8_t n)
{
	lcdc.spr_palette1 = n;
}

void lcd_write_spr_palette2(uint8_t n)
{
	lcdc.spr_palette2 = n;
}

void lcd_write_scroll_x(uint8_t n)
{
	lcdc.scroll_x = n;
}

void lcd_write_scroll_y(uint8_t n)
{
	lcdc.scroll_y = n;
}

uint8_t lcd_get_line(void)
{
	return lcd_line;
}

void lcd_write_stat(uint8_t c)
{
	ly_int                = !!(c & 0x40);
	mode2_oam_int         = !!(c & 0x20);
	mode1_vblank_int      = !!(c & 0x10);
	mode0_hblank_int      = !!(c & 0x08);
	
	lcd_stat = (c & 0xF8);
}

void lcd_write_control(uint8_t c)
{
	lcdc.bg_enabled            = !!(c & 0x01);
	lcdc.sprites_enabled       = !!(c & 0x02);
	lcdc.sprite_size           = !!(c & 0x04);
	lcdc.tilemap_select        = !!(c & 0x08);
	lcdc.bg_tiledata_select    = !!(c & 0x10);
	lcdc.window_enabled        = !!(c & 0x20);
	lcdc.window_tilemap_select = !!(c & 0x40);
	lcdc.lcd_enabled           = !!(c & 0x80);
	
	if(!lcdc.lcd_enabled) {
		lcd_reset();
		skip_frames = 2;
	} else {
		lcd_match_lyc();
	}
}

void lcd_set_ly_compare(uint8_t c)
{
	lcd_ly_compare = c;
	if(lcdc.lcd_enabled)
		lcd_match_lyc();
}

void lcd_set_window_y(uint8_t n)
{
	lcdc.window_y = n;
}

void lcd_set_window_x(uint8_t n)
{
	lcdc.window_x = n;
}

static inline void sort_sprites(struct sprite *s, int n)
{
	int swapped, i;

	do
	{
		swapped = 0;
		for(i = 0; i < n-1; i++)
		{
			if(s[i].x < s[i+1].x)
			{
				sprite c = s[i];
				s[i] = s[i+1];
				s[i+1] = c;
				swapped = 1;
			}
		}
	}
	while(swapped);
}

static inline int scan_sprites(struct sprite *s, int line, int size)
{
	int i, c = 0;
	for(i = 0; i<40; i++)
	{
		int y, offs = i * 4;
	
		y = mem[0xFE00 + offs++] - 16;
		if(line < y || line >= y + 8+(size*8))
			continue;
	
		s[c].y     = y;
		s[c].x     = mem[0xFE00 + offs++]-8;
		s[c].tile  = mem[0xFE00 + offs++];
		s[c].flags = mem[0xFE00 + offs++];
		c++;
	
		if(c == 10)
			break;
	}
	return c;
}

static void draw_bg_and_window(fbuffer_t *b, int line, struct LCDC& lcdc)
{
	int x, offset = line * 160;
	bool windowVisible = line >= lcdc.window_y && lcdc.window_enabled && line - lcdc.window_y < 144;

	for(x = 0; x < 160; x++, offset++)
	{
		uint32_t map_select, map_offset, tile_num, tile_addr, xm, ym;
		uint8_t b1, b2, mask, colour;

		/* Convert LCD x,y into full 256*256 style internal coords */
		if(windowVisible && x + 7 >= lcdc.window_x)
		{
			xm = x + 7 - lcdc.window_x;
			ym = line - lcdc.window_y;
			map_select = lcdc.window_tilemap_select;
		}
		else {
			if(!lcdc.bg_enabled)
			{
				b[offset] = palette[bgpalette[0]];
				continue;
			}
			xm = (x + lcdc.scroll_x) & 0xFF;
			ym = (line + lcdc.scroll_y) & 0xFF;
			map_select = lcdc.tilemap_select;
		}

		/* Which pixel is this tile on? Find its offset. */
		/* (y/8)*32 calculates the offset of the row the y coordinate is on.
		 * As 256/32 is 8, divide by 8 to map one to the other, this is the row number.
		 * Then multiply the row number by the width of a row, 32, to find the offset.
		 * Finally, add x/(256/32) to find the offset within that row. 
		 */
		map_offset = (ym/8)*32 + xm/8;

		tile_num = mem[0x9800 + map_select*0x400 + map_offset];
		if(lcdc.bg_tiledata_select)
			tile_addr = 0x8000 + tile_num*16;
		else
			tile_addr = 0x9000 + ((signed char)tile_num)*16;

		b1 = mem[tile_addr+(ym&7)*2];
		b2 = mem[tile_addr+(ym&7)*2+1];
		mask = 128>>(xm&7);
		colour = (!!(b2&mask)<<1) | !!(b1&mask);
		
		b[offset] = palette[bgpalette[colour]];
	}
}

static void draw_sprites(fbuffer_t *b, int line, int nsprites, struct sprite *s, struct LCDC& lcdc)
{
	for(uint8_t i = 0; i < nsprites; i++)
	{
		uint32_t b1, b2, tile_addr, sprite_line, x, offset;

		/* Sprite is offscreen */
		if(s[i].x < -7)
			continue;

		/* Which line of the sprite (0-7) are we rendering */
		sprite_line = s[i].flags & VFLIP ? (lcdc.sprite_size ? 15 : 7)-(line - s[i].y) : line - s[i].y;

		/* Address of the tile data for this sprite line */
		tile_addr = 0x8000 + (s[i].tile*16) + sprite_line*2;

		/* The two bytes of data holding the palette entries */
		b1 = mem[tile_addr];
		b2 = mem[tile_addr+1];

		/* For each pixel in the line, draw it */
		offset = s[i].x + line * 160;
		for(x = 0; x < 8; x++, offset++)
		{
			uint8_t mask, colour;
			uint8_t *pal;

			if((s[i].x + x) >= 160)
				continue;

			mask = s[i].flags & HFLIP ? 128>>(7-x) : 128>>x;
			colour = ((!!(b2&mask))<<1) | !!(b1&mask);
			if(colour == 0)
				continue;

			pal = (s[i].flags & PNUM) ? sprpalette2 : sprpalette1;
			
			/* Sprite is behind BG, only render over palette entry 0 */
			if(s[i].flags & PRIO)
			{
				if(b[offset] != palette[bgpalette[0]])
					continue;
			}
			
			//b[offset] = pal[colour];
			b[offset] = palette[pal[colour]];
		}
	}
}

static void render_line(void *arg)
{
	struct LCDC cline;
	fbuffer_t* b = espeon_get_framebuffer();
	
	while(true) {
		if(!xQueueReceive(lcdqueue, &cline, portMAX_DELAY))
			continue;
		
		if(lcd_mode==1)
			continue;
		
		int line = cline.lcd_line;
		struct sprite s[10];
		
		lcd_set_palettes(cline);
		
		/* Draw the background layer */
		draw_bg_and_window(b, line, cline);
		
		/* Draw sprites */
		if(cline.sprites_enabled) {
			uint8_t sprcnt = scan_sprites(s, line, cline.sprite_size);
			if(sprcnt) {
				sort_sprites(s, sprcnt);
				draw_sprites(b, line, sprcnt, s, cline);
			}
		}

		if(line == 143) {
			if (skip_frames) {
				--skip_frames;
			} else {
				espeon_end_frame();
			}
		}
	}
}

void lcd_cycle(uint32_t cycles)
{	
	if(!lcdc.lcd_enabled)
		return;
	
	lcd_cycles += cycles;
  
	if(lcd_line >= 144) {
		if (lcd_mode != 1) {
			/* Mode 1: Vertical blanking */
			if (mode1_vblank_int) {
				interrupt(INTR_LCDSTAT);
			}
			interrupt(INTR_VBLANK);
			lcd_stat_tracker = 0;
			lcd_mode = 1;
			
			//espeon_end_frame();
		}
		if (lcd_cycles >= SCANLINE_CYCLES) {
			lcd_cycles -= SCANLINE_CYCLES;
			//lcd_cycles = 0;
			++lcd_line;
			if (lcd_line > 153) {
				lcd_line = 0;
			}
			lcd_match_lyc();
		}
	}
	else if(lcd_cycles < MODE2_BOUNDS) {
		if (lcd_mode != 2) {
			if (mode2_oam_int) {
				interrupt(INTR_LCDSTAT);
			}
			lcd_stat_tracker = 1;
			lcd_mode = 2;
			
			/* Mode 2: Scanning OAM for (X, Y) coordinates of sprites that overlap this line */
			// lcdc.sprcount = scan_sprites(lcdc.spr, lcd_line, lcdc.sprite_size);
			// if (lcdc.sprcount)
				// sort_sprites(lcdc.spr, lcdc.sprcount);
		}
	}
	else if(lcd_cycles < MODE3_BOUNDS) {
		if (lcd_mode != 3) {
			lcd_stat_tracker = 1;
			lcd_mode = 3;
			
			/* send scanline early */
			lcdc.lcd_line = lcd_line;
			xQueueSend(lcdqueue, &lcdc, 0);
			
			/* Mode 3: Reading OAM and VRAM to generate the picture */
			// fbuffer_t* b = espeon_get_framebuffer();
			// lcd_set_palettes(lcdc);
			// draw_bg_and_window(b, lcd_line, lcdc);
			// draw_sprites(b, lcd_line, sprcount, spr, lcdc);
		}
	}
	else if(lcd_cycles < MODE0_BOUNDS) {
		if (lcd_mode != 0) {
			/* Mode 0: Horizontal blanking */
			if (mode0_hblank_int) {
				interrupt(INTR_LCDSTAT);
			}
			lcd_stat_tracker = 3;
			lcd_mode = 0;
		}
	}
	else {
		++lcd_line;
		lcd_match_lyc();
		lcd_mode = 0;
		lcd_cycles -= SCANLINE_CYCLES;
		//lcd_cycles = 0;
	}
}

bool lcd_init()
{	
	lcdqueue = xQueueCreate(143, sizeof(LCDC));
	if(!lcdqueue)
		return false;
	
	lcd_write_control(mem[0xFF40]);
	
	xTaskCreatePinnedToCore(&render_line, "renderScanline", 4096, NULL, 5, NULL, 0);
	
	return true;
}
