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

#if !defined(__M_MISC__)
#define __M_MISC__

#include <stdio.h>
#include <stdlib.h>

#include "doomtype.h"

boolean M_WriteFile(char *name, void *source, int length);
int M_ReadFile(char *name, byte **buffer);
void M_MakeDirectory(char *dir);
char *M_TempFile(char *s);
boolean M_FileExists(char *file);
long M_FileLength(FILE *handle);
char *M_ExtractFolder(char *path);
boolean M_StrToInt(const char *str, int *result);
char *M_StrCaseStr(char *haystack, char *needle);
boolean M_StringCopy(char *dest, char *src, size_t dest_size);
char *M_StringReplace(char *haystack, char *needle, char *replacement);
char *M_StringJoin(char *s, ...);
boolean M_StringStartsWith(char *s, char *prefix);
boolean M_StringEndsWith(char *s, char *suffix);
int M_vsnprintf(char *buf, size_t buf_len, const char *s, va_list args);
int M_snprintf(char *buf, size_t buf_len, const char *s, ...);
char *uppercase(char *str);
char *commify(double value);
boolean wildcard(char *input, char *pattern);

#endif
