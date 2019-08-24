#ifndef _INPUT_NDS_H_
#define _INPUT_NDS_H_

#include <nds.h>
#include "akey.h"

bool NDS_IsControlPressed();

typedef struct
{
	u8 x, y, w, h;
	s16 keycode;
	u8 flags;
} touch_area_t;

#define TA_FLAG_SLANTED 1
#define TA_FLAG_PRESSED 2

#endif /* _INPUT_NDS_H */
