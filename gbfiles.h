#ifndef INTERNAL_H
#define INTERNAL_H

/* Uncomment this to include a fallback ROM */
// #define USE_INTERNAL_ROM
#define USE_INTERNAL_BIOS
#define USE_INTERNAL_BORDER

#ifdef USE_INTERNAL_ROM
	#include "gbrom.h"
#else
	const uint8_t* gb_rom = nullptr;
	const uint32_t gb_rom_size = 0;
#endif

#ifdef USE_INTERNAL_BIOS
	#include "gbbios.h"
#else
	const uint8_t* gb_bios = nullptr;
	const uint32_t gb_bios_size = 0;
#endif

#ifdef USE_INTERNAL_BORDER
	#include "gbborder.h"
#else
	const uint8_t* gb_border = nullptr;
	const uint32_t gb_border_size = 0;
#endif

#endif
