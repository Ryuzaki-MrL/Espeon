#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "mem.h"
#include "rom.h"
#include "lcd.h"
#include "mbc.h"
#include "interrupt.h"
#include "timer.h"
#include "cpu.h"

bool usebootrom = false;
static unsigned char *mem;
static int DMA_pending;
static int joypad_select_buttons, joypad_select_directions;
uint8_t btn_directions, btn_faces;
static const s_rominfo *rominfo;
static const unsigned char *rom;

unsigned char* mem_get_bytes()
{
	return mem;
}

/* TODO: every system should be reading and writing
	directly to the memory, not the other way around */
unsigned char mem_get_byte(unsigned short i)
{
	unsigned long elapsed;
	unsigned char mask = 0;

	if(DMA_pending && i < 0xFF80)
	{
		elapsed = cpu_get_cycles() - DMA_pending;
		if(elapsed >= 160)
			DMA_pending = 0;
		else
		{
			return mem[0xFE00+elapsed];
		}
	}

	if(i >= 0x4000 && i < 0x8000)
		return rombank[i - 0x4000];
	
	// else if (i >= 0x8000 && i < 0xA000)
		// return ((mem[0xFF41]&0x3)==3) ? 0xFF : mem[i];
	
	else if (i >= 0xA000 && i < 0xC000)
		return mbc_read_ram(i);
	
	// else if (i >= 0xFE00 && i < 0xFE9F)
		// return ((mem[0xFF41]&0x2)==1) ? 0xFF : mem[i];

	// else if(i < 0xFF00)
		// return mem[i];

	else switch(i)
	{
		case 0xFF00:	/* Joypad */
			if(!joypad_select_buttons)
				mask = btn_faces;
			if(!joypad_select_directions)
				mask = btn_directions;
			return 0xC0 | (0xF^mask) | (joypad_select_buttons | joypad_select_directions);
		break;
		case 0xFF04:
			return timer_get_div();
		break;
		case 0xFF05:
			return timer_get_counter();
		break;
		case 0xFF06:
			return timer_get_modulo();
		break;
		case 0xFF07:
			return timer_get_tac();
		break;
		case 0xFF0F:
			return 0xE0 | IF;
		break;
		case 0xFF41:
			return lcd_get_stat();
		break;
		case 0xFF44:
			return lcd_get_line();
		break;
		case 0xFF4D:	/* GBC speed switch */
			return 0xFF;
		break;
		case 0xFFFF:
			return IE;
		break;
	}

	return mem[i];
}

void mem_write_byte(unsigned short d, unsigned char i)
{
	if (d < 0x8000) {
		mbc_write_rom(d, i);
		return;
	}
	
	/* VRAM */
	// if (d >= 0x8000 && d < 0xA000) {
		// if ((mem[0xFF41] & 0x3) == 3)
			// return;
	// }
	
	/* SRAM */
	else if (d >= 0xA000 && d < 0xC000) {
		mbc_write_ram(d, i);
		return;
	}

	else switch(d)
	{
		case 0xFF00:	/* Joypad */
			joypad_select_buttons = i&0x20;
			joypad_select_directions = i&0x10;
		break;
		case 0xFF04:
			timer_reset_div();
		break;
		case 0xFF05:
			timer_set_counter(i);
		break;
		case 0xFF06:
			timer_set_modulo(i);
		break;
		case 0xFF07:
			timer_set_tac(i);
		break;
		case 0xFF0F:
			IF = i;
		break;
		case 0xFF40:
			lcd_write_control(i);
		break;
		case 0xFF41:
			lcd_write_stat(i);
		break;
		case 0xFF42:
			lcd_write_scroll_y(i);
		break;
		case 0xFF43:
			lcd_write_scroll_x(i);
		break;
		case 0xFF44:
			lcd_reset();
		break;
		case 0xFF45:
			lcd_set_ly_compare(i);
		break;
		case 0xFF46: { /* OAM DMA */
			/* Check if address overlaps with RAM or ROM */
			unsigned short addr = i * 0x100;
			const unsigned char* src = mem;
			if (addr >= 0x4000 && addr < 0x8000) {
				src = rombank;
				addr -= 0x4000;
			}
			else if (addr >= 0xA000 && addr < 0xC000) {
				src = rambank;
				addr -= 0xA000;
			}
			
			/* Copy 0xA0 bytes from source to OAM */
			memcpy(&mem[0xFE00], &src[addr], 0xA0);
			DMA_pending = cpu_get_cycles();
		}
		break;
		case 0xFF47:
			lcd_write_bg_palette(i);
		break;
		case 0xFF48:
			lcd_write_spr_palette1(i);
		break;
		case 0xFF49:
			lcd_write_spr_palette2(i);
		break;
		case 0xFF4A:
			lcd_set_window_y(i);
		break;
		case 0xFF4B:
			lcd_set_window_x(i);
		break;
		case 0xFF50:
			memcpy(&mem[0x0000], &rom[0x0000], 0x100);
		break;
		case 0xFFFF:
			IE = i;
		break;
	}
	
	mem[d] = i;
}

bool mmu_init(const unsigned char* bootrom)
{
	mem = (unsigned char *)calloc(1, 0x10000);
	if (!mem)
		return false;
	
	if (!mbc_init())
		return false;
	
	rom = rom_getbytes();
	
	if (bootrom) {
		memcpy(&mem[0x0000], &bootrom[0x0000], 0x100);
		memcpy(&mem[0x0100], &rom[0x0100], 0x4000 - 0x100);
		usebootrom = true;
		return true;
	}
	
	memcpy(&mem[0x0000], &rom[0x0000], 0x4000);

	mem[0xFF10] = 0x80;
	mem[0xFF11] = 0xBF;
	mem[0xFF12] = 0xF3;
	mem[0xFF14] = 0xBF;
	mem[0xFF16] = 0x3F;
	mem[0xFF19] = 0xBF;
	mem[0xFF1A] = 0x7F;
	mem[0xFF1B] = 0xFF;
	mem[0xFF1C] = 0x9F;
	mem[0xFF1E] = 0xBF;
	mem[0xFF20] = 0xFF;
	mem[0xFF23] = 0xBF;
	mem[0xFF24] = 0x77;
	mem[0xFF25] = 0xF3;
	mem[0xFF26] = 0xF1;
	mem[0xFF40] = 0x91;
	mem[0xFF47] = 0xFC;
	mem[0xFF48] = 0xFF;
	mem[0xFF49] = 0xFF;
	
	return true;
}
