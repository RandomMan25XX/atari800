/*
 * nds/sound.c - Nintendo DS audio backend
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
#include <maxmod9.h>
#include <stdlib.h>
#include <string.h>

#include "atari.h"
#include "log.h"
#include "platform.h"
#include "pokeysnd.h"
#include "sound.h"

u32	NDS_bufferSize, NDS_sampleSize;
Sound_setup_t *NDS_sound;
mm_stream NDS_stream;

mm_word NDS_AudioCallback(mm_word length, mm_addr dest, mm_stream_formats format);

int PLATFORM_SoundSetup(Sound_setup_t *setup)
{
	POKEYSND_enable_new_pokey = FALSE;
	POKEYSND_bienias_fix = FALSE;

	if (setup->freq > 32768)
		setup->freq = 32768;
	else if (setup->freq < 1024)
		setup->freq = 1024;

	if (setup->sample_size > 2)
		return FALSE;
	if (setup->channels > 2)
		return FALSE;

	if (setup->buffer_frames < (setup->freq / 32))
		setup->buffer_frames = (setup->freq / 32);
	setup->buffer_frames = (setup->buffer_frames + 3) & (~0x03);

	NDS_bufferSize = setup->buffer_frames;
	NDS_sampleSize = setup->sample_size * setup->channels;

	NDS_stream.sampling_rate = setup->freq;
	NDS_stream.buffer_length = NDS_bufferSize;
	NDS_stream.callback = NDS_AudioCallback;
	NDS_stream.format = ((setup->channels == 2) ? 1 : 0) | ((setup->sample_size == 2) ? 2 : 0);
	NDS_stream.timer = MM_TIMER3;
	NDS_stream.manual = false;

	NDS_sound = setup;
	mmStreamOpen(&NDS_stream);

	return TRUE;
}

DTCM_DATA static int audio_paused = 0;

mm_word NDS_AudioCallback(mm_word length, mm_addr dest, mm_stream_formats format)
{
	if (audio_paused) {
		memset(dest, 0, length * NDS_sampleSize);
	} else {
		Sound_Callback(dest, length * NDS_sampleSize);
	}
	return length;
}

void PLATFORM_SoundLock(void)
{
}

void PLATFORM_SoundUnlock(void)
{
}

void PLATFORM_SoundExit(void)
{
}

void PLATFORM_SoundPause(void)
{
	audio_paused = 1;
}

void PLATFORM_SoundContinue(void)
{
	audio_paused = 0;
}
