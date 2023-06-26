/*
 * 3ds/video.c - Nintendo 3DS video backend
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

#include <3ds.h>

#include "artifact.h"
#include "atari.h"
#include "colours.h"
#include "config.h"
#include "input.h"
#include "input_3ds.h"
#include "log.h"
#ifdef PAL_BLENDING
#include "pal_blending.h"
#endif /* PAL_BLENDING */
#include "platform.h"
#include "screen.h"
#include "util.h"
#include "videomode.h"

#include "grapefruit.h"
#include "shader_shbin.h"

static C3D_Tex tex, kbd_display;
static C3D_Mtx proj_top, proj_bottom;
static C3D_RenderTarget *target_top, *target_bottom;
static struct ctr_shader_data shader;

static u32 *texBuf;
VIDEOMODE_MODE_t N3DS_VIDEO_mode;
static int ctable[256];

static bool vsync_active = true;
static u32 vsync_counter;
static bool old_h_total_stored = false;
static u32 old_h_total;

static aptHookCookie apt_cookie;

static void set_refresh_rate(u32 h_total) {
	if (!old_h_total_stored) {
		gspWaitForVBlank();
		GSPGPU_ReadHWRegs(0x400424, &old_h_total, 4);
		old_h_total_stored = true;
	}
	GSPGPU_WriteHWRegs(0x400424, &h_total, 4);
}

static void restore_refresh_rate(void) {
	if (old_h_total_stored) {
		set_refresh_rate(old_h_total);
	}
}

void N3DS_SetVsync(bool value) {
	vsync_active = value;
	vsync_counter = 0;

	// 50Hz mode
	u32 h_total = (Atari800_tv_mode == Atari800_TV_PAL && vsync_active) ? 495 : 413;
	set_refresh_rate(h_total);
}

void N3DS_ToggleVsync(void) {
	N3DS_SetVsync(!vsync_active);
}

bool N3DS_IsVsyncEnabled(void) {
	return vsync_active;
}

static void apt_hook_cb(APT_HookType hookType, void *param) {
	if (hookType == APTHOOK_ONSUSPEND || hookType == APTHOOK_ONEXIT) {
		restore_refresh_rate();
	} else if (hookType == APTHOOK_ONRESTORE || hookType == APTHOOK_ONWAKEUP) {
		N3DS_SetVsync(vsync_active); // changes refresh rate
	}
}

//

static void N3DS_RenderNormal(u8 *src, u32 *dest)
{
	int x, y;
	int spitch = Screen_WIDTH - VIDEOMODE_src_width;
	int dpitch = 512 - VIDEOMODE_src_width;

	for (y = 0; y < VIDEOMODE_src_height; y++) {
		for (x = 0; x < VIDEOMODE_src_width; x++) {
			*(dest++) = ctable[*(src++)];
		}
		src += spitch;
		dest += dpitch;
	}
}

void N3DS_InitVideo(void)
{
	C3D_TexEnv* texEnv;

	gfxInitDefault();
	gfxSet3D(false);
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);

	aptHook(&apt_cookie, apt_hook_cb, NULL);

	target_top = C3D_RenderTargetCreate(240, 400, GPU_RB_RGB8, GPU_RB_DEPTH16);
	target_bottom = C3D_RenderTargetCreate(240, 320, GPU_RB_RGB8, GPU_RB_DEPTH16);
	C3D_RenderTargetClear(target_top, C3D_CLEAR_ALL, 0, 0);
	C3D_RenderTargetClear(target_bottom, C3D_CLEAR_ALL, 0, 0);
	C3D_RenderTargetSetOutput(target_top, GFX_TOP, GFX_LEFT,
		GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGB8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8));
	C3D_RenderTargetSetOutput(target_bottom, GFX_BOTTOM, GFX_LEFT,
		GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGB8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8));

	C3D_TexInitVRAM(&tex, 512, 256, GPU_RGBA8);
	texBuf = linearAlloc(512 * 256 * 4);

	ctr_load_t3x(&kbd_display, "romfs:/kbd_display.t3x", TEXTURE_TARGET_VRAM);

	ctr_init_shader(&shader, shader_shbin, shader_shbin_size);
	AttrInfo_AddLoader(&(shader.attr), 0, GPU_FLOAT, 3); // v0 = position
	AttrInfo_AddLoader(&(shader.attr), 1, GPU_FLOAT, 2); // v1 = texcoord
	ctr_bind_shader(&shader);

	Mtx_OrthoTilt(&proj_top, 0.0, 400.0, 0.0, 240.0, -1.0, 1.0, true);
	Mtx_OrthoTilt(&proj_bottom, 0.0, 320.0, 0.0, 240.0, -1.0, 1.0, true);

	texEnv = C3D_GetTexEnv(0);
	C3D_TexEnvSrc(texEnv, C3D_Both, GPU_TEXTURE0, 0, 0);
	C3D_TexEnvOpRgb(texEnv, 0, 0, 0);
	C3D_TexEnvOpAlpha(texEnv, 0, 0, 0);
	C3D_TexEnvFunc(texEnv, C3D_Both, GPU_MODULATE);

	C3D_DepthTest(true, GPU_GEQUAL, GPU_WRITE_ALL);

	N3DS_SetVsync(vsync_active);
}

void N3DS_ExitVideo(void)
{
	linearFree(texBuf);

	C3D_TexDelete(&kbd_display);
	C3D_TexDelete(&tex);

	C3D_RenderTargetDelete(target_top);
	C3D_RenderTargetDelete(target_bottom);

	aptUnhook(&apt_cookie);
	restore_refresh_rate();

	C3D_Fini();
	gfxExit();
}

void PLATFORM_PaletteUpdate(void)
{
#ifdef PAL_BLENDING
	if (N3DS_VIDEO_mode == VIDEOMODE_MODE_NORMAL && ARTIFACT_mode == ARTIFACT_PAL_BLEND)
		PAL_BLENDING_UpdateLookup();
#endif
	PLATFORM_MapRGB(ctable, Colours_table, 256);
}

void PLATFORM_GetPixelFormat(PLATFORM_pixel_format_t *format)
{
	format->bpp = 32;
	format->rmask = 0xFF000000;
	format->gmask = 0x00FF0000;
	format->bmask = 0x0000FF00;
}

void PLATFORM_MapRGB(void *dest, int const *palette, int size)
{
	int i;
	u32* target = (u32*) dest;
	for (i = 0; i < size; i++)
	{
		target[i] = (palette[i] << 8) | 0xFF;
	}
}

void PLATFORM_SetVideoMode(VIDEOMODE_resolution_t const *res, int windowed, VIDEOMODE_MODE_t mode, int rotate90)
{
	N3DS_VIDEO_mode = mode;

	C3D_RenderTargetClear(target_top, C3D_CLEAR_ALL, 0, 0);

	N3DS_SetVsync(vsync_active); // changes refresh rate
	PLATFORM_PaletteUpdate();
	PLATFORM_DisplayScreen();
}

VIDEOMODE_resolution_t *PLATFORM_AvailableResolutions(unsigned int *size)
{
	VIDEOMODE_resolution_t *resolutions;
	resolutions = Util_malloc(1 * sizeof(VIDEOMODE_resolution_t));
	resolutions[0].width = 400;
	resolutions[0].height = 240;
	*size = 1;
	return resolutions;
}

VIDEOMODE_resolution_t *PLATFORM_DesktopResolution(void)
{
	VIDEOMODE_resolution_t *resolutions;
	resolutions = Util_malloc(1 * sizeof(VIDEOMODE_resolution_t));
	resolutions[0].width = 400;
	resolutions[0].height = 240;
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

void N3DS_DrawTexture(C3D_Tex* tex, int x, int y, int tx, int ty, int width, int height) {
	float txmin, tymin, txmax, tymax;
	txmin = (float) tx / tex->width;
	tymax = 1.0f - (float) ty / tex->height;
	txmax = (float) (tx+width) / tex->width;
	tymin = 1.0f - (float) (ty+height) / tex->height;

	C3D_TexBind(0, tex);
	C3D_ImmDrawBegin(GPU_TRIANGLE_STRIP);
		C3D_ImmSendAttrib((float) x, (float) 240 - y - height, 0.0f, 0.0f);
		C3D_ImmSendAttrib((float) txmin, (float) tymin, 0.0f, 0.0f);

		C3D_ImmSendAttrib((float) x + width, (float) 240 - y - height, 0.0f, 0.0f);
		C3D_ImmSendAttrib((float) txmax, (float) tymin, 0.0f, 0.0f);

		C3D_ImmSendAttrib((float) x, (float) 240 - y, 0.0f, 0.0f);
		C3D_ImmSendAttrib((float) txmin, (float) tymax, 0.0f, 0.0f);

		C3D_ImmSendAttrib((float) x + width, (float) 240 - y, 0.0f, 0.0f);
		C3D_ImmSendAttrib((float) txmax, (float) tymax, 0.0f, 0.0f);
	C3D_ImmDrawEnd();
}

void PLATFORM_DisplayScreen(void)
{
	u8 *src;
	float xmin, ymin, xmax, ymax, txmin, tymin, txmax, tymax;

	if (!gspHasGpuRight())
		return;

	src = (u8*) Screen_atari;
	src += Screen_WIDTH * VIDEOMODE_src_offset_top + VIDEOMODE_src_offset_left;

#ifdef PAL_BLENDING
	if (N3DS_VIDEO_mode == VIDEOMODE_MODE_NORMAL && ARTIFACT_mode == ARTIFACT_PAL_BLEND)
	{
		PAL_BLENDING_Blit32((ULONG*) texBuf, src, tex.width, VIDEOMODE_src_width, VIDEOMODE_src_height, VIDEOMODE_src_offset_top % 2);
	}
	else
#endif
	{
		N3DS_RenderNormal(src, texBuf);
	}

	GSPGPU_FlushDataCache(texBuf, 512 * 256 * 4);
	C3D_SyncDisplayTransfer(texBuf, GX_BUFFER_DIM(512, 256), tex.data, GX_BUFFER_DIM(tex.width, tex.height),
		(GX_TRANSFER_FLIP_VERT(1) | GX_TRANSFER_OUT_TILED(1) | GX_TRANSFER_RAW_COPY(0) |
		GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGBA8) |
		GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))
	);

	if (vsync_active) {
		u32 curr_frame = C3D_FrameCounter(0);
		if (curr_frame > vsync_counter) {
			// ticking took >1 frame's worth
			C3D_FrameBegin(0);
		} else {
			C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
		}
	} else {
		C3D_FrameBegin(0);
	}

	C3D_FrameDrawOn(target_bottom);
	C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, shader.proj_loc, &proj_bottom);
	C3D_TexBind(0, &kbd_display);
	N3DS_DrawKeyboard(&kbd_display);

	xmin = (400 - VIDEOMODE_dest_width) / 2.0f;
	ymin = (240 - VIDEOMODE_dest_height) / 2.0f;
	xmax = xmin + VIDEOMODE_dest_width;
	ymax = ymin + VIDEOMODE_dest_height;
	txmax = ((float) VIDEOMODE_src_width / tex.width);
	txmin = 0.0f;
	tymin = ((float) VIDEOMODE_src_height / tex.height);
	tymax = 0.0f;

	C3D_FrameDrawOn(target_top);
	C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, shader.proj_loc, &proj_top);

	C3D_TexBind(0, &tex);
	C3D_ImmDrawBegin(GPU_TRIANGLE_STRIP);
		C3D_ImmSendAttrib(xmin, ymin, 0.0f, 0.0f);
		C3D_ImmSendAttrib(txmin, tymin, 0.0f, 0.0f);

		C3D_ImmSendAttrib(xmax, ymin, 0.0f, 0.0f);
		C3D_ImmSendAttrib(txmax, tymin, 0.0f, 0.0f);

		C3D_ImmSendAttrib(xmin, ymax, 0.0f, 0.0f);
		C3D_ImmSendAttrib(txmin, tymax, 0.0f, 0.0f);

		C3D_ImmSendAttrib(xmax, ymax, 0.0f, 0.0f);
		C3D_ImmSendAttrib(txmax, tymax, 0.0f, 0.0f);
	C3D_ImmDrawEnd();

	C3D_FrameEnd(0);

	vsync_counter = C3D_FrameCounter(0);
}