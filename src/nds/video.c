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
#include "ui.h"
#include "util.h"
#include "video.h"
#include "videomode.h"

#include "keyboard.h"

VIDEOMODE_MODE_t NDS_VIDEO_mode;

extern int NDS_ShouldDrawKeyboard;
void NDS_DrawKeyboard(u8 *dst, u8 *src, u8 *tmp);

static s16 oldSx = 0, oldSy = 0;
static int oldVsol = 0;
static int oldVsot = 0;

s8 video_scaler_mode = VIDEO_SCALER_MODE_BLENDED;

static void vblankHandler(void)
{
	s16 sx = VIDEOMODE_src_width;
	s16 sy = VIDEOMODE_src_height * 4 / 3;
	int offset_left = VIDEOMODE_src_offset_left;
	int offset_top = VIDEOMODE_src_offset_top;

	if (video_scaler_mode > 0) {
		REG_DISPCNT = MODE_5_2D | DISPLAY_BG2_ACTIVE | DISPLAY_BG3_ACTIVE;
#ifdef BITPL_SCR
		if (((u32) Screen_atari) == 0x06020000) {
			REG_BG2CNT |= 0x10;
			REG_BG3CNT |= 0x10;
		} else {
			REG_BG2CNT &= ~0x10;
			REG_BG3CNT &= ~0x10;
		}
#endif
	} else {
		REG_DISPCNT = MODE_5_2D | DISPLAY_BG2_ACTIVE;
#ifdef BITPL_SCR
		if (((u32) Screen_atari) == 0x06020000) {
			REG_BG2CNT |= 0x10;
		} else {
			REG_BG2CNT &= ~0x10;
		}
#endif
	}

	if (sx != oldSx || sy != oldSy || offset_left != oldVsol || offset_top != oldVsot)
	{
		oldSx = sx;
		oldSy = sy;
		oldVsol = offset_left;
		oldVsot = offset_top;

		REG_BG2PA = sx;
		REG_BG2PB = 0;
		REG_BG2PC = 0;
		REG_BG2PD = sy;

		REG_BG3PA = sx;
		REG_BG3PB = 0;
		REG_BG3PC = 0;
		REG_BG3PD = sy;

		REG_BG2X = 0 + (offset_left * 256);
		REG_BG3X = 64 + (offset_left * 256);
		REG_BG2Y = 0 + (offset_top * 256);
		REG_BG3Y = 64 + (offset_top * 256);
	}

	if (NDS_ShouldDrawKeyboard) {
		u8 *dst = (u8*) (0x06200000 + 256*192 - 256*64);
		u8 *src = (u8*) 0x06200000;
		u8 *tmp = (u8*) (0x06200000 + 256*384 - 256*128);
		NDS_DrawKeyboard(dst, src, tmp);
	}
}

void NDS_InitVideo(void)
{
	// power on 2D engine, power off 3D engine
	powerOn(POWER_ALL_2D);
	powerOff(POWER_3D_CORE | POWER_MATRIX);

	// init main engine
	REG_DISPCNT = MODE_5_2D | DISPLAY_BG2_ACTIVE | DISPLAY_BG3_ACTIVE;
	vramSetPrimaryBanks(
		VRAM_A_MAIN_BG_0x06000000,
		VRAM_B_MAIN_BG_0x06020000,
		VRAM_C_SUB_BG,
		VRAM_D_LCD
	);

	REG_BG2CNT = BG_PRIORITY_0 | BG_BMP8_512x256;
	REG_BG3CNT = BG_PRIORITY_1 | BG_BMP8_512x256;

	REG_BLDCNT = BLEND_ALPHA | BLEND_SRC_BG2 | BLEND_DST_BG3;
	REG_BLDALPHA = 0x808;

	// init sub engine
	videoSetModeSub(MODE_3_2D | DISPLAY_BG3_ACTIVE);

	REG_BG3CNT_SUB = BG_PRIORITY_0 | BG_BMP8_256x256;
	REG_BG3PA_SUB = 256;
	REG_BG3PB_SUB = 0;
	REG_BG3PC_SUB = 0;
	REG_BG3PD_SUB = 256;
	REG_BG3X_SUB = 0;
	REG_BG3Y_SUB = 0;

	for (int i = 0; i < keyboardPalLen>>1; i++)
		BG_PALETTE_SUB[i] = keyboardPal[i];

	decompress(keyboardBitmap, (u8*) 0x06200000, LZ77Vram);

	// set IRQs
	irqSet(IRQ_VBLANK, vblankHandler);
	irqEnable(IRQ_VBLANK);
}

#define NDS_ATARI_COLOR(i) \
	( ((Colours_table[(i)] >> 19) & 0x1F) | \
	  ((Colours_table[(i)] >> (11 - 5)) & (0x1F << 5)) | \
	  ((Colours_table[(i)] << (-(3 - 10))) & (0x1F << 10)) )

void PLATFORM_PaletteUpdate(void)
{
	for (int i = 0; i < 256; i++)
		BG_PALETTE[i] = NDS_ATARI_COLOR(i);
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
		target[i] = NDS_ATARI_COLOR(i);
}

void PLATFORM_SetVideoMode(VIDEOMODE_resolution_t const *res, int windowed, VIDEOMODE_MODE_t mode, int rotate90)
{
	NDS_VIDEO_mode = mode;

	PLATFORM_PaletteUpdate();
	PLATFORM_DisplayScreen();
}

VIDEOMODE_resolution_t *PLATFORM_AvailableResolutions(unsigned int *size)
{
	VIDEOMODE_resolution_t *resolutions = Util_malloc(sizeof(VIDEOMODE_resolution_t));
	resolutions->width = 384;
	resolutions->height = 240;
	*size = 1;
	return resolutions;
}

VIDEOMODE_resolution_t *PLATFORM_DesktopResolution(void)
{
	VIDEOMODE_resolution_t *resolutions = Util_malloc(sizeof(VIDEOMODE_resolution_t));
	resolutions->width = 384;
	resolutions->height = 240;
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

void PLATFORM_DisplayScreen(void)
{
#ifdef NDS_SCREEN_IN_VRAM
	if (UI_is_active) {
		while (DMA_CR(3) & DMA_BUSY);

#ifdef BITPL_SCR
		memcpy(Screen_atari_b, Screen_atari, Screen_WIDTH*Screen_HEIGHT);
#endif
		swiWaitForVBlank();

		DMA_SRC(3) = (u32) Screen_atari_ui;
		DMA_DEST(3) = 0x06000000;
		DMA_CR(3) = DMA_ENABLE | DMA_32_BIT | DMA_START_VBL | (Screen_HEIGHT << 7);
#ifdef BITPL_SCR
	} else {
		unsigned int *Screen_atari_old = Screen_atari;
		Screen_atari = Screen_atari_b;
		Screen_atari_b = Screen_atari_old;
#endif
	}
#else
	while (DMA_CR(3) & DMA_BUSY);

	DMA_SRC(3) = (u32) Screen_atari;
	DMA_DEST(3) = 0x06000000;
	DMA_CR(3) = DMA_ENABLE | DMA_32_BIT | DMA_START_NOW | (Screen_HEIGHT << 7);

#ifndef BITPL_SCR
	while (DMA_CR(3) & DMA_BUSY);
#endif
#endif
}
