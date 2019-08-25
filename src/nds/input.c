/*
 * nds/input.c - Nintendo DS input device support
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

#include "config.h"
#include "akey.h"
#include "input.h"
#include "input_nds.h"
#include "log.h"
#include "platform.h"
#include "ui.h"
#include "video.h"

#include "keyboard.h"

int key_control;
int current_key_down = AKEY_NONE;
int dpad_as_keyboard = 0;
double key_down_time = 0;

#define WARMSTART_HOLD_TIME 3
#define AKEY_WAS_SHIFT_CTRL -992

touch_area_t NDS_touch_areas_key[] = {
	{ 0, 102, 18, 18, AKEY_ESCAPE, 0 },
	{ 17, 102, 18, 18, AKEY_1, 0 },
	{ 34, 102, 18, 18, AKEY_2, 0 },
	{ 51, 102, 18, 18, AKEY_3, 0 },
	{ 68, 102, 18, 18, AKEY_4, 0 },
	{ 85, 102, 18, 18, AKEY_5, 0 },
	{ 102, 102, 18, 18, AKEY_6, 0 },
	{ 119, 102, 18, 18, AKEY_7, 0 },
	{ 136, 102, 18, 18, AKEY_8, 0 },
	{ 153, 102, 18, 18, AKEY_9, 0 },
	{ 170, 102, 18, 18, AKEY_0, 0 },
	{ 187, 102, 18, 18, AKEY_LESS, 0 },
	{ 204, 102, 18, 18, AKEY_GREATER, 0 },
	{ 221, 102, 18, 18, AKEY_BACKSPACE, 0 },
	{ 238, 102, 18, 18, AKEY_BREAK, 0 },

	{ 0, 119, 28, 18, AKEY_TAB, 0 },
	{ 27, 119, 18, 18, AKEY_q, 0 },
	{ 44, 119, 18, 18, AKEY_w, 0 },
	{ 61, 119, 18, 18, AKEY_e, 0 },
	{ 78, 119, 18, 18, AKEY_r, 0 },
	{ 95, 119, 18, 18, AKEY_t, 0 },
	{ 112, 119, 18, 18, AKEY_y, 0 },
	{ 129, 119, 18, 18, AKEY_u, 0 },
	{ 146, 119, 18, 18, AKEY_i, 0 },
	{ 163, 119, 18, 18, AKEY_o, 0 },
	{ 180, 119, 18, 18, AKEY_p, 0 },
	{ 197, 119, 18, 18, AKEY_MINUS, 0 },
	{ 214, 119, 18, 18, AKEY_EQUAL, 0 },
	{ 231, 119, 25, 18, AKEY_RETURN, 0 },

	{ 0, 136, 31, 22, AKEY_CTRL, 0 },
	{ 30, 136, 18, 18, AKEY_a, 0 },
	{ 47, 136, 18, 18, AKEY_s, 0 },
	{ 64, 136, 18, 18, AKEY_d, 0 },
	{ 81, 136, 18, 18, AKEY_f, 0 },
	{ 98, 136, 18, 18, AKEY_g, 0 },
	{ 115, 136, 18, 18, AKEY_h, 0 },
	{ 132, 136, 18, 18, AKEY_j, 0 },
	{ 149, 136, 18, 18, AKEY_k, 0 },
	{ 166, 136, 18, 18, AKEY_l, 0 },
	{ 183, 136, 18, 18, AKEY_SEMICOLON, 0 },
	{ 200, 136, 18, 18, AKEY_PLUS, 0 },
	{ 217, 136, 18, 18, AKEY_ASTERISK, 0 },
	{ 234, 136, 22, 18, AKEY_CAPSTOGGLE, 0 },

	{ 0, 153, 35, 18, AKEY_SHFT, 0 },
	{ 34, 153, 18, 18, AKEY_z, 0 },
	{ 51, 153, 18, 18, AKEY_x, 0 },
	{ 68, 153, 18, 18, AKEY_c, 0 },
	{ 85, 153, 18, 18, AKEY_v, 0 },
	{ 102, 153, 18, 18, AKEY_b, 0 },
	{ 119, 153, 18, 18, AKEY_n, 0 },
	{ 136, 153, 18, 18, AKEY_m, 0 },
	{ 153, 153, 18, 18, AKEY_COMMA, 0 },
	{ 170, 153, 18, 18, AKEY_FULLSTOP, 0 },
	{ 187, 153, 18, 18, AKEY_SLASH, 0 },
	{ 204, 153, 35, 18, AKEY_SHFT, 0 },
	{ 238, 153, 18, 18, AKEY_ATARI, 0 },

	{ 50, 170, 154, 18, AKEY_SPACE, 0 },

	{ 113, 78, 29, 14, AKEY_HELP, TA_FLAG_SLANTED },
	{ 141, 78, 29, 14, AKEY_START, TA_FLAG_SLANTED },
	{ 169, 78, 29, 14, AKEY_SELECT, TA_FLAG_SLANTED },
	{ 197, 78, 29, 14, AKEY_OPTION, TA_FLAG_SLANTED },
	{ 225, 78, 29, 14, AKEY_WARMSTART, TA_FLAG_SLANTED }
};

#define NDS_TOUCH_AREA_KEY_MAX (sizeof(NDS_touch_areas_key) / sizeof(touch_area_t))

/* touch_area_t NDS_touch_areas_5200[] = {
	{ 95, 73, 38, 22, AKEY_5200_1, 0 },
	{ 141, 73, 38, 22, AKEY_5200_2, 0 },
	{ 187, 73, 38, 22, AKEY_5200_3, 0 },
	{ 95, 109, 38, 22, AKEY_5200_4, 0 },
	{ 141, 109, 38, 22, AKEY_5200_5, 0 },
	{ 187, 109, 38, 22, AKEY_5200_6, 0 },
	{ 95, 145, 38, 22, AKEY_5200_7, 0 },
	{ 141, 145, 38, 22, AKEY_5200_8, 0 },
	{ 187, 145, 38, 22, AKEY_5200_9, 0 },
	{ 95, 181, 38, 22, AKEY_5200_ASTERISK, 0 },
	{ 141, 181, 38, 22, AKEY_5200_0, 0 },
	{ 187, 181, 38, 22, AKEY_5200_HASH, 0 }
}; */

// #define NDS_TOUCH_AREA_5200_MAX (sizeof(NDS_touch_areas_5200) / sizeof(touch_area_t))

/* #define NDS_TOUCH_AREAS ((Atari800_machine_type == Atari800_MACHINE_5200 && !UI_is_active) \
		? NDS_touch_areas_5200 : NDS_touch_areas_key)
#define NDS_TOUCH_AREA_MAX ((Atari800_machine_type == Atari800_MACHINE_5200 && !UI_is_active) \
		? NDS_TOUCH_AREA_5200_MAX : NDS_TOUCH_AREA_KEY_MAX) */

#define NDS_TOUCH_AREAS NDS_touch_areas_key
#define NDS_TOUCH_AREA_MAX NDS_TOUCH_AREA_KEY_MAX

bool NDS_IsControlPressed()
{
	return key_control != 0;
}

static inline bool isKeyTouched(touchPosition* pos, touch_area_t* area)
{
	if (area->flags & TA_FLAG_SLANTED)
	{
		if (pos->py >= area->y && pos->py < (area->y + area->h))
		{
			int slx = pos->px + (pos->py - area->y);
			return slx >= area->x && slx < (area->x + area->w);
		}

		return false;
	} else
	{
		return pos->px >= area->x && pos->py >= area->y
			&& pos->px < (area->x + area->w)
			&& pos->py < (area->y + area->h);
	}
}

static void copyTexture(u8 *dst, u8 *src, int x, int y, int w, int h)
{
	x &= ~1;
	w &= ~1;

	src += (y << 8) + x;
	dst += (y << 8) + x;

	for (int iy = 0; iy < h; iy++, src += 256 - w, dst += 256 - w)
	for (int ix = 0; ix < w; ix+=2, src+=2, dst+=2)
		*(u16*)(dst) = *(u16*)(src);
}

DTCM_DATA int NDS_ShouldDrawKeyboard = 0;

void NDS_DrawKeyboard(u8 *dst, u8 *src, u8 *tmp)
{
	touch_area_t* keyTable = NDS_TOUCH_AREAS;
	int keyTableLen = NDS_TOUCH_AREA_MAX;

	touchPosition pos;
	bool isTouch = ((keysDown() | keysHeld()) & KEY_TOUCH) != 0;

	int key_down = current_key_down;
	if (key_down >= 0) key_down &= ~AKEY_SHFTCTRL;

	touchRead(&pos);
	for (int i = 0; i < keyTableLen; i++)
	{
		touch_area_t* area = &keyTable[i];
		if (key_down == area->keycode
                        || (INPUT_key_shift != 0 && area->keycode == AKEY_SHFT)
                        || (NDS_IsControlPressed() && area->keycode == AKEY_CTRL)
			|| (isTouch && (
				(area->keycode == AKEY_START) ||
				(area->keycode == AKEY_SELECT) ||
				(area->keycode == AKEY_OPTION)
			) && isKeyTouched(&pos, area))
		)
		{
			// pressed
			if (!(area->flags & TA_FLAG_PRESSED)) {
				area->flags |= TA_FLAG_PRESSED;
				if (area->flags & TA_FLAG_SLANTED)
					for (int i = 0; i < area->h; i++)
					{
						copyTexture(tmp, src, area->x - i, area->y + i, area->w, 1);
						copyTexture(src, dst, area->x - i, area->y + i, area->w, 1);
					}
				else
				{
					copyTexture(tmp, src, area->x, area->y, area->w, area->h);
					copyTexture(src, dst, area->x, area->y, area->w, area->h);
				}
			}
		} else if (area->flags & TA_FLAG_PRESSED)
		{
			// released
			area->flags &= ~TA_FLAG_PRESSED;
			if (area->flags & TA_FLAG_SLANTED)
				for (int i = 0; i < area->h; i++)
					copyTexture(src, tmp, area->x - i, area->y + i, area->w, 1);
			else
				copyTexture(src, tmp, area->x, area->y, area->w, area->h);
		}
	}
}

void PLATFORM_SetJoystickKey(int joystick, int direction, int value)
{
}

void PLATFORM_GetJoystickKeyName(int joystick, int direction, char *buffer, int bufsize)
{
}

int PLATFORM_GetRawKey(void)
{
	return 0;
}

int PLATFORM_Keyboard(void)
{
	u32 kDown, kHeld, kUp;
	touchPosition pos;
	int shiftctrl = (INPUT_key_shift != 0 ? AKEY_SHFT : 0) | (key_control != 0 ? AKEY_CTRL : 0);

	scanKeys();
	kDown = keysDown();
	kHeld = keysHeld();
	kUp = keysUp();

	// reset
	INPUT_key_consol = INPUT_CONSOL_NONE;

	if (kUp & KEY_TOUCH)
	{
		if (current_key_down != AKEY_WAS_SHIFT_CTRL)
		{
			INPUT_key_shift = 0;
			key_control = 0;
		}
		current_key_down = AKEY_NONE;
		Atari800_display_screen = TRUE;
		NDS_ShouldDrawKeyboard = 1;
	}

	if (UI_is_active)
	{
		if (kDown & KEY_A)
			return AKEY_RETURN;
		if (kDown & KEY_B)
			return AKEY_ESCAPE;
		if (kDown & KEY_X)
			return AKEY_SPACE;
		if (kDown & KEY_Y)
			return AKEY_BACKSPACE;
		if (kHeld & KEY_UP)
			return AKEY_UP;
		if (kHeld & KEY_LEFT)
			return AKEY_LEFT;
		if (kHeld & KEY_RIGHT)
			return AKEY_RIGHT;
		if (kHeld & KEY_DOWN)
			return AKEY_DOWN;
		if (kDown & KEY_L)
			return AKEY_COLDSTART;
		if (kDown & KEY_R)
			return AKEY_WARMSTART;
	} else if (Atari800_machine_type == Atari800_MACHINE_5200) {
		// second fire key
		INPUT_key_shift = (kDown & KEY_B) ? 1 : 0;

		if (kHeld & KEY_START)
			return AKEY_5200_START;
		if (kHeld & KEY_SELECT)
			return AKEY_5200_PAUSE;
		if (kHeld & KEY_R)
			return AKEY_5200_RESET;
		if (kHeld & KEY_L)
			return AKEY_UI;
	} else
	{
		if (kHeld & KEY_START)
			INPUT_key_consol &= ~INPUT_CONSOL_START;
		if (kHeld & KEY_SELECT)
			INPUT_key_consol &= ~INPUT_CONSOL_SELECT;
		if (kHeld & KEY_R)
			INPUT_key_consol &= ~INPUT_CONSOL_OPTION;
		if (kHeld & KEY_B)
			return AKEY_RETURN;
		if (kHeld & KEY_Y)
			return AKEY_SPACE;
		if (kHeld & KEY_L)
			return AKEY_UI;
		if (dpad_as_keyboard) {
			if (kHeld & KEY_UP)
				return AKEY_UP;
			if (kHeld & KEY_LEFT)
				return AKEY_LEFT;
			if (kHeld & KEY_RIGHT)
				return AKEY_RIGHT;
			if (kHeld & KEY_DOWN)
				return AKEY_DOWN;
		}
	}

	if ((kDown | kHeld) & KEY_TOUCH)
	{
		touchRead(&pos);
		touch_area_t* keyTable = NDS_TOUCH_AREAS;
		bool down = (kDown & KEY_TOUCH) != 0;
		bool touching = ((kDown | kHeld) & KEY_TOUCH) != 0;
		bool refresh = false;

		if (down) current_key_down = AKEY_NONE;

		for (int i = 0; i < NDS_TOUCH_AREA_MAX; i++)
		{
			touch_area_t* area = &keyTable[i];
			if (isKeyTouched(&pos, area))
			{
				refresh |= down;
				switch (area->keycode)
				{
					case AKEY_SHFT:
						if (down) Atari800_display_screen = TRUE;
						INPUT_key_shift = 1;
						current_key_down = AKEY_WAS_SHIFT_CTRL;
						return AKEY_NONE;
					case AKEY_CTRL:
						if (down) Atari800_display_screen = TRUE;
						key_control = 1;
						current_key_down = AKEY_WAS_SHIFT_CTRL;
						return AKEY_NONE;
					case AKEY_START:
						if (touching) INPUT_key_consol &= ~INPUT_CONSOL_START;
						break;
					case AKEY_SELECT:
						if (touching) INPUT_key_consol &= ~INPUT_CONSOL_SELECT;
						break;
					case AKEY_OPTION:
						if (touching) INPUT_key_consol &= ~INPUT_CONSOL_OPTION;
						break;
					case AKEY_WARMSTART:
						if (down)
						{
							key_down_time = PLATFORM_Time();
							current_key_down = AKEY_NONE;
							break;
						} else if (touching && (PLATFORM_Time() - key_down_time) > WARMSTART_HOLD_TIME)
						{
							Atari800_display_screen = TRUE;
							return AKEY_WARMSTART;
						}
					default:
						if (down) current_key_down = area->keycode >= 0
							? (area->keycode | shiftctrl)
							: area->keycode;
						break;
				}
				break;
			}
		}

		if (refresh) NDS_ShouldDrawKeyboard = 1;
		if (refresh) Atari800_display_screen = TRUE;
		return current_key_down;
	}

	return AKEY_NONE;
}

int PLATFORM_PORT(int num)
{
	int ret = 0xff;
	if (num == 0) {
		scanKeys();
		u32 kHeld = keysHeld();
		if ((!dpad_as_keyboard || Atari800_machine_type == Atari800_MACHINE_5200) && !UI_is_active)
		{
			if (kHeld & KEY_LEFT)
				ret &= 0xf0 | INPUT_STICK_LEFT;
			if (kHeld & KEY_RIGHT)
				ret &= 0xf0 | INPUT_STICK_RIGHT;
			if (kHeld & KEY_UP)
				ret &= 0xf0 | INPUT_STICK_FORWARD;
			if (kHeld & KEY_DOWN)
				ret &= 0xf0 | INPUT_STICK_BACK;
		}
	}
	return ret;
}

int PLATFORM_TRIG(int num)
{
	if (num == 0) {
		scanKeys();
		if (keysHeld() & KEY_A)
			return 0;
	}
	return 1;
}
