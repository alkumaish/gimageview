/* -*- Mode: C; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 3 -*- */

/*
 * GImageView
 * Copyright (C) 2001 Takuro Ashie
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Id$
 */

#ifndef __PCX_H__
#define __PCX_H__

#include <gtk/gtk.h>
#include "gimv_image.h"
#include "gimv_image_loader.h"

typedef struct
{
   gint width;
   gint height;
   gint ncolors;
} pcx_info;

gboolean   pcx_get_header (GimvIO          *gio,
                           pcx_info        *info);
GimvImage *pcx_load       (GimvImageLoader *loader,
                           gpointer         data);

#endif /* __PCX_H__ */

