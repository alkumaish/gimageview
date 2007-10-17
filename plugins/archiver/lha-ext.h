/* -*- Mode: C; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 3 -*- */

/*
 *  File-Roller
 *
 *  Copyright (C) 2001 The Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 *
 *  $Id$
 */

#ifndef FR_COMMAND_LHA_H
#define FR_COMMAND_LHA_H


#include <glib-object.h>
#include "fr-command.h"
#include "fr-process.h"

#define FR_TYPE_COMMAND_LHA            (fr_command_lha_get_type ())
#define FR_COMMAND_LHA(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), FR_TYPE_COMMAND_LHA, FRCommandLha))
#define FR_COMMAND_LHA_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), FR_TYPE_COMMAND_LHA, FRCommandLhaClass))
#define FR_IS_COMMAND_LHA(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FR_TYPE_COMMAND_LHA))
#define FR_IS_COMMAND_LHA_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), FR_TYPE_COMMAND_LHA))
#define FR_COMMAND_LHA_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), FR_TYPE_COMMAND_LHA, FRCommandLhaClass))

typedef struct _FRCommandLha       FRCommandLha;
typedef struct _FRCommandLhaClass  FRCommandLhaClass;

struct _FRCommandLha
{
	FRCommand  __parent;
};

struct _FRCommandLhaClass
{
	FRCommandClass __parent_class;
};

GType        fr_command_lha_get_type        (void);

FRCommand*   fr_command_lha_new             (FRProcess  *process,
                                             const char *filename,
                                             FRArchive  *archive);

#endif /* FR_COMMAND_LHA_H */
