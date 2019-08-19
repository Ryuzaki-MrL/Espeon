#include "timer.h"
#include "rom.h"
#include "mem.h"
#include "cpu.h"
#include "lcd.h"
#include "espeon.h"
#include "menu.h"

/* Uncomment this to include a fallback ROM */
// #define USE_INTERNAL_ROM
#define USE_INTERNAL_BIOS

#ifdef USE_INTERNAL_ROM
	#include "gbrom.h"
#else
	const uint8_t* gb_rom = nullptr;
#endif

#ifdef USE_INTERNAL_BIOS
	#include "gbbios.h"
#else
	const uint8_t* gb_bios = nullptr;
#endif

void setup()
{
	espeon_init();
	
	menu_init();
	menu_loop();
	
	const uint8_t* rom = espeon_load_rom(menu_get_rompath());
	if (!rom) rom = (const uint8_t*)gb_rom;
	
	if (!rom_init(rom))
		espeon_faint("rom_init failed.");
	
	if (!mmu_init(gb_bios))
		espeon_faint("mmu_init failed.");
	
	if (!lcd_init())
		espeon_faint("lcd_init failed.");
	
	cpu_init();
	
	espeon_render_border();
	
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
