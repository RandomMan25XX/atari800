/*
 * nds/video.c - Nintendo DS video backend
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

#include "artifact.h"
#include "atari.h"
#include "colours.h"
#include "config.h"
#include "input.h"
#include "input_nds.h"
#include "log.h"
#include "platform.h"
#include "screen.h"
#include "util.h"
#include "videomode.h"

VIDEOMODE_MODE_t NDS_VIDEO_mode;
int atariBackground;

void NDS_InitVideo(void)
{
	videoSetMode(MODE_5_2D);
	vramSetPrimaryBanks(
		VRAM_A_MAIN_BG_0x06000000,
		VRAM_B_MAIN_BG_0x06020000,
		VRAM_C_SUB_BG,
		VRAM_D_LCD
	);
	consoleDemoInit();
	atariBackground = bgInit(3, BgType_Bmp8, BgSize_B8_512x256, 0, 0);
}

void PLATFORM_PaletteUpdate(void)
{
	for (int i = 0; i < 256; i++)
		BG_PALETTE[i] =
			((Colours_table[i] >> 19) & 0x1F) |
			((Colours_table[i] >> (11 - 5)) & (0x1F << 5)) |
			((Colours_table[i] << (-(3 - 10))) & (0x1F << 10));
}

void PLATFORM_GetPixelFormat(PLATFORM_pixel_format_t *format)
{
	format->bpp = 16;
	format->rmask = 0x1F << 0;
	format->gmask = 0x1F << 5;
	format->bmask = 0x1F << 10;
}

void PLATFORM_MapRGB(void *dest, int const *palette, int size)
{
	int i;
	u16* target = (u16*) dest;
	for (i = 0; i < size; i++)
	{
		target[i] =
			((Colours_table[i] >> 19) & 0x1F) |
			((Colours_table[i] >> (11 - 5)) & (0x1F << 5)) |
			((Colours_table[i] << (-(3 - 10))) & (0x1F << 10));
	}
}

void PLATFORM_SetVideoMode(VIDEOMODE_resolution_t const *res, int windowed, VIDEOMODE_MODE_t mode, int rotate90)
{
	NDS_VIDEO_mode = mode;

	PLATFORM_PaletteUpdate();
	PLATFORM_DisplayScreen();
}

VIDEOMODE_resolution_t *PLATFORM_AvailableResolutions(unsigned int *size)
{
	VIDEOMODE_resolution_t *resolutions;
	resolutions = Util_malloc(1 * sizeof(VIDEOMODE_resolution_t));
	resolutions[0].width = 512;
	resolutions[0].height = 512;
	*size = 1;
	return resolutions;
}

VIDEOMODE_resolution_t *PLATFORM_DesktopResolution(void)
{
	VIDEOMODE_resolution_t *resolutions;
	resolutions = Util_malloc(1 * sizeof(VIDEOMODE_resolution_t));
	resolutions[0].width = 512;
	resolutions[0].height = 512;
	return resolutions;
}

int PLATFORM_SupportsVideomode(VIDEOMODE_MODE_t mode, int stretch, int rotate90)
{
	if (rotate90 != 0)
		return false;

	return mode == VIDEOMODE_MODE_NORMAL;
}

int PLATFORM_WindowMaximised(void)
{
	return 1;
}

static s32 oldSx, oldSy;

void PLATFORM_DisplayScreen(void)
{
	u8* dst = (u8*) 0x06000000;
	u8* src = (u8*) Screen_atari;
	src += VIDEOMODE_src_offset_left + (VIDEOMODE_src_offset_top * Screen_WIDTH);

	if (bgGetMapBase(atariBackground) == 0)
		dst += 512*256;

	s32 sx = VIDEOMODE_src_width * 256 / 256;
	s32 sy = VIDEOMODE_src_height * 256 / 192;

	if (sx != oldSx || sy != oldSy)
	{
		oldSx = sx;
		oldSy = sy;
		bgSetScale(atariBackground, sx, sy);
		bgUpdate();
	}

	for (int y = 0; y < VIDEOMODE_src_height; y++) {
		dmaCopy(src + (y * Screen_WIDTH), dst + (y * 512), VIDEOMODE_src_width);
	}

	if (bgGetMapBase(atariBackground) == 0)
		bgSetMapBase(atariBackground, 8);
	else
		bgSetMapBase(atariBackground, 0);
}
