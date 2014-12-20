/*
========================================================================

                               DOOM RETRO
         The classic, refined DOOM source port. For Windows PC.

========================================================================

  Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
  Copyright (C) 2013-2015 Brad Harding.

  DOOM RETRO is a fork of CHOCOLATE DOOM by Simon Howard.
  For a complete list of credits, see the accompanying AUTHORS file.

  This file is part of DOOM RETRO.

  DOOM RETRO is free software: you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the
  Free Software Foundation, either version 3 of the License, or (at your
  option) any later version.

  DOOM RETRO is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with DOOM RETRO. If not, see <http://www.gnu.org/licenses/>.

  DOOM is a registered trademark of id Software LLC, a ZeniMax Media
  company, in the US and/or other countries and is used without
  permission. All other trademarks are the property of their respective
  holders. DOOM RETRO is in no way affiliated with nor endorsed by
  id Software LLC.

========================================================================
*/

#ifndef __HU_STUFF_H__
#define __HU_STUFF_H__

#include "d_event.h"

//
// Globally visible constants.
//
#define HU_FONTSTART    '!'     // the first font characters
#define HU_FONTEND      '_'     // the last font characters

// Calculate # of glyphs in font.
#define HU_FONTSIZE     (HU_FONTEND - HU_FONTSTART + 1)

#define HU_BROADCAST    5

#define HU_MSGX         (automapactive && fullscreen && !widescreen ? 0 : 3)
#define HU_MSGY         2
#define HU_MSGWIDTH     64      // in characters
#define HU_MSGHEIGHT    1       // in lines

#define HU_MSGTIMEOUT   (4 * TICRATE)

//
// HEADS UP TEXT
//
void HU_Init(void);
void HU_Start(void);

void HU_Ticker(void);
void HU_Drawer(void);
void HU_Erase(void);

void HU_clearMessages(void);

extern char mapnumandtitle[133];
extern char automaptitle[133];
extern int fullscreen;

#endif
