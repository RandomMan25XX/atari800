/*
 * nds/main.c - Nintendo DS port main code
 *
 * Copyright (c) 2001-2002 Jacek Poplawski
 * Copyright (C) 2001-2016 Atari800 development team (see DOC/CREDITS)
 *
 * This file is part of the Atari800 emulator project which emulates
 * the Atari 400, 800, 800XL, 130XE, and 5200 8-bit computers.
 *
 * Atari800 is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Atari800 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Atari800; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <nds.h>
#include <fat.h>
#include <maxmod9.h>
#include <stdio.h>
#include <string.h>

/* Atari800-nds includes */
#include "video.h"

/* Atari800 includes */
#include "atari.h"
#include "config.h"
#include "../input.h"
#include "log.h"
#include "monitor.h"
#include "platform.h"
#include "pokeysnd.h"
#ifdef SOUND
#include "../sound.h"
#endif
#include "videomode.h"

extern int dpad_as_keyboard;

int PLATFORM_Configure(char *option, char *parameters)
{
	if (strcmp(option, "NDS_DPAD_MODE") == 0)
	{
		sscanf(parameters, "%d", &dpad_as_keyboard);
		return 1;
	}
	return 0;
}

void PLATFORM_ConfigSave(FILE *fp)
{
	fprintf(fp, "NDS_DPAD_MODE=%d\n", dpad_as_keyboard);
}

int PLATFORM_Initialise(int *argc, char *argv[])
{
	TIMER0_CR &= ~TIMER_ENABLE;
	TIMER1_CR &= ~TIMER_ENABLE;

	TIMER0_DATA = 0;
	TIMER1_DATA = 0;

	TIMER0_CR = TIMER_DIV_256 | TIMER_ENABLE;
	TIMER1_CR = TIMER_CASCADE | TIMER_ENABLE;

	NDS_InitVideo();
	fatInitDefault();

	mm_ds_system sys;
	sys.mod_count = 0;
	sys.samp_count = 0;
	sys.mem_bank = 0;
	sys.fifo_channel = FIFO_MAXMOD;
	mmInit(&sys);

	return TRUE;
}

void PLATFORM_Sleep(double s)
{
	double time = PLATFORM_Time() + s;
	while (PLATFORM_Time() < time);
}

double PLATFORM_Time(void)
{
	u32 time = TIMER0_DATA | (TIMER1_DATA << 16);
	return (time / (double) (BUS_CLOCK / 256.0));
}

int PLATFORM_Exit(int run_monitor)
{
	Log_flushlog();

	if (run_monitor) {
		return 1;
	} else {
		// TODO
	}

	return 0;
}

int main(int argc, char **argv)
{
	if (!Atari800_Initialise(&argc, argv))
	{
		printf("Atari initialisation failed!");
		return 0;
	}

	while (1) {
		INPUT_key_code = PLATFORM_Keyboard();
		Atari800_Frame();
		if (Atari800_display_screen)
			PLATFORM_DisplayScreen();
	}
}

/*
vim:ts=4:sw=4:
*/
