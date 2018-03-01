/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2018 ET:Legacy team <mail@etlegacy.com>
 *
 * This file is part of ET: Legacy - http://www.etlegacy.com
 *
 * ET: Legacy is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ET: Legacy is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ET: Legacy. If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, Wolfenstein: Enemy Territory GPL Source Code is also
 * subject to certain additional terms. You should have received a copy
 * of these additional terms immediately following the terms and conditions
 * of the GNU General Public License which accompanied the source code.
 * If not, please request a copy in writing from id Software at the address below.
 *
 * id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.
 */
/**
 * @file dl_local.h
 */

#ifndef INCLUDE_DL_LOCAL_H
#define INCLUDE_DL_LOCAL_H

#ifdef __GNUC__
#define _attribute(x) __attribute__(x)
#else
#define _attribute(x)
#endif

// system API
// only the restricted subset we need

int Com_DPrintf(const char *fmt, ...) _attribute((format(printf, 1, 2)));
int Com_Printf(const char *fmt, ...) _attribute((format(printf, 1, 2)));
void Com_Error(int code, const char *fmt, ...) _attribute((format(printf, 2, 3)));       // watch out, we don't define ERR_FATAL and stuff
void Cvar_SetValue(const char *var_name, float value);
void Cvar_Set(const char *varName, const char *value);
char *va(char *format, ...) _attribute((format(printf, 1, 2)));

#ifdef _WIN32
  #define Q_stricmp stricmp
#else
  #define Q_stricmp strcasecmp
#endif

extern int com_errorEntered;

#endif // #ifndef INCLUDE_DL_LOCAL_H
