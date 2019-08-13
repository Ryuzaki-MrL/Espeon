#include "timer.h"
#include "rom.h"
#include "mem.h"
#include "cpu.h"
#include "lcd.h"
#include "espeon.h"
#include "menu.h"
#include "gbrom.h"
#include "gbbios.h"

void setup()
{
	espeon_init();
	
	menu_init();
	menu_loop();
	
	const unsigned char* rom = espeon_load_rom(menu_get_rompath());
	if (!rom) rom = (const unsigned char*)gb_rom;
	
	if (!rom_init(rom))
		espeon_faint("rom_init failed.");
	
	if (!mmu_init(gb_bios))
		espeon_faint("mmu_init failed.");
	
	if (!lcd_init())
		espeon_faint("lcd_init failed.");
	
	cpu_init();
	
	espeon_render_border();
	
	while(true) {
		unsigned int cycles = cpu_cycle();
		espeon_update();
		lcd_cycle(cycles);
		timer_cycle(cycles);
	}
}

void loop()
{
	unsigned int cycles = cpu_cycle();
	espeon_update();
	lcd_cycle(cycles);
	timer_cycle(cycles);
}
