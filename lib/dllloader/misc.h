/*
 * misc.h -- miscellaneous routines header
 * (C)Copyright 2000, 2001, 2002 by Hiroshi Takekawa
 * This file is part of Enfle.
 *
 * Last Modified: Fri Feb  8 02:11:50 2002.
 * $Id$
 *
 * Enfle is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Enfle is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#ifndef _ENFLE_MISC_H
#define _ENFLE_MISC_H

char *misc_basename(char *);
char *misc_get_ext(const char *, int);
char *misc_trim_ext(const char *, const char *);
char *misc_replace_ext(char *, char *);
char *misc_canonical_pathname(char *);
char **misc_str_split(char *, char);
void misc_free_str_array(char **);
char *misc_str_tolower(char *);

#endif
