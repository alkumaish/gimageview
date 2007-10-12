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

#ifndef __PREFS_UI_IMAGEWIN_H__
#define __PREFS_UI_IMAGEWIN_H__

GtkWidget *prefs_imagewin_page       (void);
GtkWidget *prefs_imagewin_image_page (void);
GtkWidget *prefs_imagewin_mouse_page (void);

gboolean   prefs_ui_imagewin_image_apply   (GimvPrefsWinAction action);

#endif /* __PREFS_UI_IMAGEWIN_H__ */
