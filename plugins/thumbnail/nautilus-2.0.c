/* -*- Mode: C; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 3 -*- */

/*
 * Nautilus thumbnail support plugin for GImageView
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

#include <string.h>
#include <gtk/gtk.h>
#include <gmodule.h>

#include "fileutil.h"
#include "gfileutil.h"
#include "gimv_plugin.h"
#include "gimv_thumb_cache.h"


#ifndef BUF_SIZE
#define BUF_SIZE 4096
#endif

#ifndef MAX_PATH_LEN
#define MAX_PATH_LEN 1024
#endif

/* under home directory */
#define NAUTILUS2_THUMNAIL_DIRECTORY ".nautilus/thumbnails"
#define URI_FILE_PREFIX         "file://"
#define URI_FILE_ESCAPED_PREFIX "file:%2F%2F"

static GimvImage *load_thumb (const gchar *filename,
                              const gchar *cache_type,
                              GimvImageInfo *info);
static GimvImage *save_thumb (const gchar *filename,
                              const gchar *cache_type,
                              GimvImage   *image,
                              GimvImageInfo *info);
static gchar     *get_path   (const gchar *filename,
                              const gchar *cache_type);
static gboolean   get_size   (gint         width,
                              gint         height,
                              const gchar *cache_type,
                              gint        *width_ret,
                              gint        *height_ret);


static GimvThumbCacheLoader plugin_impl[] =
{
   {GIMV_THUMB_CACHE_LOADER_IF_VERSION,
    N_("Nautilus-2.0"),
    load_thumb, save_thumb,
    get_path, get_size,
    NULL, NULL, NULL, NULL, NULL},
};

GIMV_PLUGIN_GET_IMPL(plugin_impl, GIMV_PLUGIN_THUMB_CACHE)

GimvPluginInfo gimv_plugin_info =
{
   if_version:    GIMV_PLUGIN_IF_VERSION,
   name:          N_("Nautilus-2.0 thumbnail support"),
   version:       "0.5.0",
   author:        N_("Takuro Ashie"),
   description:   NULL,
   get_implement: gimv_plugin_get_impl,
   get_mime_type: NULL,
   get_prefs_ui:  NULL,
};


gchar *
g_module_check_init (GModule *module)
{
   return NULL;
}


void
g_module_unload (GModule *module)
{
   return;
}


static GimvImage *
load_thumb (const gchar *filename, const gchar *cache_type,
            GimvImageInfo *info)
{
   GimvImage *image;
   gchar *thumb_file;

   g_return_val_if_fail (filename, NULL);
   g_return_val_if_fail (cache_type, NULL);

   thumb_file = get_path (filename, cache_type);

   g_return_val_if_fail (thumb_file, NULL);

   image = gimv_image_load_file (thumb_file, FALSE);

   g_free (thumb_file);

   return image;
}


static GimvImage *
save_thumb (const gchar *filename, const gchar *cache_type,
            GimvImage *image, GimvImageInfo *info)
{
   GimvImage *imcache;
   gchar *thumb_file;
   gint im_width = -1, im_height = -1, width = -1, height = -1;
   gboolean success;

   /* check args */
   g_return_val_if_fail (filename, NULL);
   g_return_val_if_fail (image, NULL);
   g_return_val_if_fail (cache_type, NULL);

   thumb_file = get_path (filename, cache_type);
   g_return_val_if_fail (thumb_file, NULL);

   /* get image width & height */
   gimv_image_get_size (image, &im_width, &im_height);
   if (im_width < 1 || im_height < 1) return NULL;

   /* get thumnail width & height */
   success = get_size (im_width, im_height, cache_type, &width, &height);
   if (!success || width < 1 || height < 1) return NULL;

   /* create cache directory if not found */
   success = mkdirs (thumb_file);
   if (!success) return NULL;

   /* scale image */
   imcache = gimv_image_scale (image, width, height);

   /* save cache to disk */
   if (imcache) {
      g_print ("save cache: %s\n", thumb_file);
      gimv_image_save_file (imcache, thumb_file, "png");
   }

   g_free (thumb_file);
   return imcache;
}


static gchar *
escape_slashes (gchar *string)
{
#define HEX_ESCAPE '%'

   const gchar hex[16] = "0123456789ABCDEF";
   const gchar *p;
   gchar *q;
   gchar *result;
   gchar c;
   gint unacceptable;

   if (!string) return NULL;
        
   unacceptable = 0;
   for (p = string; *p != '\0'; p++) {
      c = *p;
      if (c == '/') {
         unacceptable++;
      }
   }

   result = g_malloc (p - string + unacceptable * 2 + 1);

   for (q = result, p = string; *p != '\0'; p++){
      c = *p;

      if (c == '/') {
         *q++ = HEX_ESCAPE; /* means hex coming */
         *q++ = hex[c >> 4];
         *q++ = hex[c & 15];
      } else {
         *q++ = c;
      }
   }

   *q = '\0';

   return result;
}


static gchar *
get_path (const gchar *filename, const gchar *cache_type)
{
   const gchar *image_name;
   gchar *abspath, *image_dir, *escaped_dir;
   gchar buf[MAX_PATH_LEN];

   g_return_val_if_fail (filename, NULL);
   g_return_val_if_fail (cache_type, NULL);

   g_return_val_if_fail (!strcmp (cache_type, plugin_impl[0].label), NULL);

   {
      gchar *tmpstr = relpath2abs (filename);
      abspath = link2abs (tmpstr);
      g_free (tmpstr);
   }

   /* get filename */
   image_name = g_basename (abspath);
   if (!image_name) goto ERROR0;

   /* get dir name */
   image_dir = g_dirname (abspath);
   if (!image_dir) goto ERROR0;

   /* escate slashes */
   escaped_dir = escape_slashes (image_dir);
   if (!escaped_dir) goto ERROR1;

   if (fileutil_extension_is (image_name, ".png")) {
      g_snprintf (buf, MAX_PATH_LEN, "%s/%s/%s%s/%s",
                  g_get_home_dir(),
                  NAUTILUS2_THUMNAIL_DIRECTORY, URI_FILE_ESCAPED_PREFIX,
                  escaped_dir, image_name);
   } else {
      g_snprintf (buf, MAX_PATH_LEN, "%s/%s/%s%s/%s"".png",
                  g_get_home_dir(),
                  NAUTILUS2_THUMNAIL_DIRECTORY, URI_FILE_ESCAPED_PREFIX,
                  escaped_dir, image_name);
   }

   g_free (abspath);
   g_free (image_dir);
   g_free (escaped_dir);

   return g_strdup (buf);

ERROR1:
   g_free (image_dir);
ERROR0:
   g_free (abspath);
   return NULL;
}


static gboolean
get_size (gint width, gint height, const gchar *cache_type,
          gint *width_ret, gint *height_ret)
{
   const gint max_size = 96;

   /* check args */
   g_return_val_if_fail (width_ret && height_ret, FALSE);
   *width_ret = *height_ret = -1;

   if (width < 1 || height < 1) return FALSE;

   /* no need to scale */
   if (width < max_size && height < max_size) {
      *width_ret = width;
      *height_ret = height;
      return TRUE;
   }

   /* calculate width & height of thumbnail */
   if (width > height) {
      *width_ret = max_size;
      *height_ret = (gfloat) height * (gfloat) max_size / (gfloat) width;
   } else {
      *width_ret = (gfloat) width * (gfloat) max_size / (gfloat) height;
      *height_ret = max_size;
   }

   return TRUE;
}
