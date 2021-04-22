#include "timer.h"
#include "rom.h"
#include "mem.h"
#include "cpu.h"
#include "lcd.h"
#include "espeon.h"
#include "menu.h"

#include "gbfiles.h"

void setup()
{
	espeon_init();
	
	menu_init();
	menu_loop();
	
	const uint8_t* rom = espeon_load_rom(menu_get_rompath());
	if (!rom) rom = (const uint8_t*)gb_rom;
	
	const uint8_t* bootrom = espeon_load_bootrom("/gb_bios.bin");
	if (!bootrom) bootrom = (const uint8_t*)gb_bios;
	
	if (!rom_init(rom))
		espeon_faint("rom_init failed.");
	
	if (!mmu_init(bootrom))
		espeon_faint("mmu_init failed.");
	
	if (!lcd_init())
		espeon_faint("lcd_init failed.");
	
	cpu_init();
	
	espeon_render_border((const uint8_t*)gb_border, gb_border_size);
	
	while(true) {
		uint32_t cycles = cpu_cycle();
		espeon_update();
		lcd_cycle(cycles);
		timer_cycle(cycles);
	}
}

void loop()
{
	uint32_t cycles = cpu_cycle();
	espeon_update();
	lcd_cycle(cycles);
	timer_cycle(cycles);
}
