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

#ifndef __HULIB__
#define __HULIB__

// We are referring to patches.
#include "r_defs.h"

// font stuff
#define HU_CHARERASE            KEY_BACKSPACE

#define HU_MAXLINES             4
#define HU_MAXLINELENGTH        512

//
// Typedefs of widgets
//

// Text Line widget
//  (parent of Scrolling Text and Input Text widgets)
typedef struct
{
    // left-justified position of scrolling text window
    int         x;
    int         y;

    patch_t     **f;                            // font
    int         sc;                             // start character
    char        l[HU_MAXLINELENGTH + 1];        // line of text
    int         len;                            // current line length

    // whether this line needs to be udpated
    int         needsupdate;
} hu_textline_t;

// Scrolling Text window widget
//  (child of Text Line widget)
typedef struct
{
    hu_textline_t       l[HU_MAXLINES];         // text lines to draw
    int                 h;                      // height in lines
    int                 cl;                     // current line number

    // pointer to boolean stating whether to update window
    boolean             *on;
    boolean             laston;                 // last value of *->on.
} hu_stext_t;

// Input Text Line widget
//  (child of Text Line widget)
typedef struct
{
    hu_textline_t       l;                      // text line to input on

    // left margin past which I am not to delete characters
    int                 lm;

    // pointer to boolean stating whether to update window
    boolean             *on;
    boolean             laston;                 // last value of *->on;
} hu_itext_t;

//
// Widget creation, access, and update routines
//

//
// textline code
//

// clear a line of text
void HUlib_clearTextLine(hu_textline_t *t);

void HUlib_initTextLine(hu_textline_t *t, int x, int y, patch_t **f, int sc);

// returns success
boolean HUlib_addCharToTextLine(hu_textline_t *t, char ch);

// returns success
boolean HUlib_delCharFromTextLine(hu_textline_t *t);

// draws tline
void HUlib_drawTextLine(hu_textline_t *l);

// erases text line
void HUlib_eraseTextLine(hu_textline_t *l);

//
// Scrolling Text window widget routines
//

// ?
void HUlib_initSText(hu_stext_t *s, int x, int y, int h, patch_t **font, int startchar, boolean *on);

// add a new line
void HUlib_addLineToSText(hu_stext_t *s);

// ?
void HUlib_addMessageToSText(hu_stext_t *s, char *prefix, char *msg);

// draws stext
void HUlib_drawSText(hu_stext_t *s);

// erases all stext lines
void HUlib_eraseSText(hu_stext_t *s);

extern boolean  STCFN034;
extern boolean  idbehold;
extern byte     *tempscreen;
extern byte     *tinttab33;
extern byte     *tinttab50;
extern int      screensize;

#endif
