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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>

#include "gimageview.h"

#include "fr-command.h"
#include "fr-archive.h"
#include "gimv_image.h"
#include "prefs.h"
#include "fileload.h"
#include "gimv_icon_stock.h"
#include "gimv_image_info.h"
#include "gimv_image_win.h"
#include "gimv_thumb_view.h"
#include "gimv_thumb_win.h"
#include "utils_char_code.h"
#include "utils_file.h"
#include "utils_file_gtk.h"
#include "utils_gtk.h"


/* for filebrowser call back functions */
typedef struct _FileSel
{
   GtkWidget    *filebrowser;
   GimvThumbWin *tw;
} FileSel;


static void cb_file_load_cancel                (GtkWidget *widget, FilesLoader *files);
static void cb_file_load_stop                  (GtkWidget *widget, FilesLoader *files);
static void filebrowser_open_files             (FileSel *filesel, ImgWinType type);
static void cb_filebrowser_open_selected_files (GtkWidget *widget, FileSel *filesel);
static void cb_filebrowser_add_thumbnail       (GtkWidget *widget, FileSel *filesel);
static void cb_filebrowser_ok_sel              (GtkWidget *widget, FileSel *filesel);
static void cb_filebrowser_close               (GtkWidget *widget, gpointer parent);


GList *files_loader_list = NULL;
gboolean loading_stop = FALSE;


FilesLoader *
files_loader_new (void)
{
   FilesLoader *files;
   gboolean reset_sensitive;

   files = g_new0 (FilesLoader, 1);
   files->filelist        = NULL;
   files->dirlist         = NULL;
   files->dirname         = NULL;
   files->archive         = NULL;
   files->window          = NULL;
   files->progressbar     = NULL;
   files->thumb_load_type = LOAD_CACHE;
   files->status          = GETTING_FILELIST;
   files->now_file        = NULL;
   files->pos             = 0;
   files->num             = 0;

   reset_sensitive = !files_loader_list;
   files_loader_list = g_list_append (files_loader_list, files);

   if (reset_sensitive) {
      GList *node = gimv_thumb_win_get_list();

      loading_stop = FALSE;

      while (node) {
         GimvThumbWin *tw = node->data;
         gimv_thumb_win_set_sensitive (tw, GIMV_THUMB_WIN_STATUS_LOADING);
         node = g_list_next (node);
      }
   }

   return files;
}


void
files_loader_delete (FilesLoader *files)
{
   g_return_if_fail (files);

   g_list_foreach (files->filelist, (GFunc) g_free, NULL);
   g_list_foreach (files->dirlist, (GFunc) g_free, NULL);

   g_list_free (files->filelist);
   g_list_free (files->dirlist);

   files_loader_list = g_list_remove (files_loader_list, files);

   if (!files_loader_list) {
      GList *node = gimv_thumb_win_get_list();
      while (node) {
         GimvThumbWin *tw = node->data;
         gimv_thumb_win_set_sensitive (tw, GIMV_THUMB_WIN_STATUS_NORMAL);
         node = g_list_next (node);
      }
   }

   if (files->archive)
      g_object_unref (G_OBJECT (files->archive));

   loading_stop = FALSE;

   g_free (files);
}
 
 
gboolean
files_loader_query_loading (void)
{
   if (files_loader_list)
      return TRUE;
   else
      return FALSE;
}


void
files_loader_stop (void)
{
   GList *node = files_loader_list;

   if (!files_loader_list) return;

   while (node) {
      FilesLoader *files = node->data;
      files->status = STOP;
      if (files->archive
          && files->archive->process
          && files->archive->process->running)	 
      {
         fr_process_stop (files->archive->process);
      }
      node = g_list_next (node);
   }

   get_dir_stop ();

   loading_stop = TRUE;
}


static void
cb_file_load_cancel (GtkWidget *widget, FilesLoader *files)
{
   files->status = CANCEL;
   if (files_loader_query_loading ())
      files_loader_stop ();
}


static void
cb_file_load_stop   (GtkWidget *widget, FilesLoader *files)
{
   files->status = STOP;
}


void
files_loader_create_progress_window (FilesLoader *files)
{
   GtkWidget *window;
   GtkWidget *vbox, *hbox;
   GtkWidget *label;
   GtkWidget *progressbar;
   GtkWidget *button;

   /* create dialog window */
   window = gtk_window_new(GTK_WINDOW_POPUP);
   gtk_widget_realize (window);
   gtk_container_border_width (GTK_CONTAINER (window), 3);
   gtk_window_set_title (GTK_WINDOW (window), _("*Loading Image Files* - GImageView - "));
   gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);
   vbox = gtk_vbox_new (FALSE, 0);
   gtk_container_add (GTK_CONTAINER(window), vbox);

   /* label */
   label = gtk_label_new (_("Now Opening Image Files..."));
   gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

   /* progress bar */
   progressbar = gtk_progress_bar_new();
   gtk_progress_set_show_text(GTK_PROGRESS(progressbar), TRUE);
   gtk_box_pack_start (GTK_BOX (vbox), progressbar, FALSE, FALSE, 0);

   /* hbox */
   hbox = gtk_hbox_new (TRUE, 0);
   gtk_container_add (GTK_CONTAINER(vbox), hbox);

   /* cancel button */
   button = gtk_button_new_with_label (_("Skip"));
   gtk_container_border_width (GTK_CONTAINER (button), 5);
   gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
   g_signal_connect (G_OBJECT(button), "clicked",
                     G_CALLBACK(cb_file_load_cancel), files);
 
   /* stop button */
   button = gtk_button_new_with_label (_("Stop"));
   gtk_container_border_width (GTK_CONTAINER (button), 5);
   gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
   g_signal_connect (G_OBJECT(button), "clicked",
                     G_CALLBACK(cb_file_load_stop), files);


   files->window = window;
   files->progressbar = progressbar;

   gtk_widget_show_all (window);
}


void
files_loader_destroy_progress_window (FilesLoader *files)
{
   g_return_if_fail (files);

   if (files->window)
      gtk_widget_destroy (files->window);

   files->window      = NULL;
   files->progressbar = NULL;
}


void
files_loader_progress_update (FilesLoader *files,
                              gfloat progress,
                              const gchar *text)
{
   g_return_if_fail (files);

   if (files->progressbar) {
      gdk_window_raise (files->window->window);
      gtk_progress_bar_update (GTK_PROGRESS_BAR(files->progressbar), progress);
      gtk_progress_set_format_string(GTK_PROGRESS(files->progressbar), text);
   }
}


gint
open_image_files_in_image_view (FilesLoader *files)
{
   gint listnum, pos;
   gint retval = FALSE;
   gfloat progress;
   gchar buf[32];
   GList *node;

   while (gtk_events_pending()) gtk_main_iteration();

   listnum = g_list_length (g_list_first(files->filelist));

   if (listnum < 1) {
      return FALSE;
   }

   retval = TRUE;

   if (listnum > 2) {
      files_loader_create_progress_window (files);
   } else {
      files->window = NULL;
      files->progressbar = NULL;
   }

   node = files->filelist;
   while (node) {
      gchar *filename = node->data;

      node = g_list_next (node);

      if (files->status == CANCEL || files->status == STOP) {
         retval = files->status;
         break;
      }

      /* Load Image */
      if (conf.disp_filename_stdout)
         g_print (_("filename = %s\n"), filename);

      /* Open Archive File */
      if (fr_archive_utils_get_file_name_ext (filename)) {
         g_print (_("The file is archive: %s\n"), filename);
         open_archive_images (filename, NULL, NULL, LOAD_CACHE);

         /* open iamge window */
      } else {
         GimvImageInfo *info = gimv_image_info_get (filename);
         if (info)
            gimv_image_win_open_window (info);
      }

      pos = g_list_position (files->filelist, node) + 1;
      progress = (gfloat) pos / (gfloat) listnum;
      g_snprintf (buf, 32, "%d/%d files", pos, listnum);
      files_loader_progress_update (files, progress, buf);
   }

   files_loader_destroy_progress_window (files);

   return retval;
}


static GimvThumbWin *
select_thumbnail_window (GimvThumbWin *tw)
{
   GimvThumbWin *retval;
   GList *node;

   if (tw && g_list_find (gimv_thumb_win_get_list(), tw)) {
      retval = (GimvThumbWin *) tw;
   }
   else if (g_list_length (gimv_thumb_win_get_list()) < 1) {
      retval = gimv_thumb_win_open_window ();
   } else {
      node = g_list_last (gimv_thumb_win_get_list());
      retval = node->data;
   }

   return retval;
}


gint
open_image_files_in_thumbnail_view (FilesLoader *files,
                                    GimvThumbWin *tw, GimvThumbView *tv_org)
{
   GimvThumbWin  *tw_tmp;
   GimvThumbView *tv;

   if (g_list_length(files->filelist) < 1) {
      return FALSE;
   }

   /* select thumbnail window */
   tw_tmp = select_thumbnail_window (tw);
   if (conf.thumbwin_raise_window)
      gdk_window_raise (GTK_WIDGET (tw_tmp)->window);

   /* select notebook page and open files */
   tv = gimv_thumb_win_find_thumbtable (tw_tmp, GIMV_THUMB_WIN_CURRENT_PAGE);

   if (!tv_org && tv && tv->mode == GIMV_THUMB_VIEW_MODE_COLLECTION) {
      gimv_thumb_view_append_thumbnail (tv, files, FALSE);
   } else {
      if (tv_org)
         tv = tv_org;
      else
         tv = gimv_thumb_win_create_new_tab (tw_tmp);
      gimv_thumb_view_reload (tv, files, GIMV_THUMB_VIEW_MODE_COLLECTION);

      if (files->status == STOP || files->status < 0)
         return files->status;
   }

   gimv_thumb_win_set_statusbar_page_info (tw_tmp, GIMV_THUMB_WIN_CURRENT_PAGE);

   return TRUE;
}


gint
open_image_files (FilesLoader *files)
{
   if (g_list_length(files->filelist) < 1) {
      return FALSE;
   }

   if (conf.default_file_open_window == THUMBNAIL_WINDOW)
      return open_image_files_in_thumbnail_view (files, NULL, NULL);
   else
      return open_image_files_in_image_view (files);
}


gint
open_dir_images (const gchar   *dir,
                 GimvThumbWin  *tw,
                 GimvThumbView *tv_org,
                 ThumbLoadType  type,
                 ScanSubDirType scan_subdir)
{
   FilesLoader *files;
   GimvThumbWin *tw_tmp = NULL;
   GimvThumbView *tv;
   gint retval = FALSE, flags;
   gchar *dirname;
   gboolean open_in_one_tab = FALSE;

   g_return_val_if_fail (dir && *dir, THUMB_LOAD_DONE);

   if (!(!tw && conf.default_dir_open_window == IMAGE_VIEW_WINDOW)
       && scan_subdir == SCAN_SUB_DIR_ONE_TAB)
   {
      open_in_one_tab = TRUE;
   }

   dirname = add_slash (dir);

   if (access (dirname, R_OK)) {
      gchar error_message[BUF_SIZE], *dir_internal;
      GtkWindow *window = tw ? GTK_WINDOW (tw) : NULL;

      dir_internal = charset_to_internal (dirname,
                                          conf.charset_filename,
                                          conf.charset_auto_detect_fn,
                                          conf.charset_filename_mode);
      g_snprintf (error_message, BUF_SIZE,
                  _("Permission denied: %s"),
                  dir_internal);
      gtkutil_message_dialog (_("Error!!"), error_message, window);

      g_free (dir_internal);
      g_free (dirname);
      return NO_OPEN_FILE;
   }

   tv = gimv_thumb_view_find_opened_dir (dirname);
   if (!tv_org && tv && (tw || (!tw && conf.default_dir_open_window == THUMBNAIL_WINDOW))
       && scan_subdir != SCAN_SUB_DIR_ONE_TAB)
   {
      gint pagenum;
      tw_tmp = tv->tw;
      gdk_window_raise (GTK_WIDGET (tw_tmp)->window);
      pagenum = gtk_notebook_page_num (GTK_NOTEBOOK (tw_tmp->notebook),
                                       tv->container);
      gtk_notebook_set_page (GTK_NOTEBOOK (tw_tmp->notebook), pagenum);
      if (!scan_subdir) {
         g_free (dirname);
         return THUMB_LOAD_DONE;
      }
   }

   /* allocate FilesLoader structure for this direcory */
   files = files_loader_new ();
   files->dirname = dirname;
   files->thumb_load_type = type;

   /* get files & directories list */
   flags = GETDIR_ENABLE_CANCEL;
   if (!scan_subdir || conf.recursive_follow_link)
      flags = flags | GETDIR_FOLLOW_SYMLINK;
   if (conf.read_dotfile)
      flags = flags | GETDIR_READ_DOT;
   if (conf.detect_filetype_by_ext)
      flags = flags | GETDIR_DETECT_EXT;
   if (conf.thumbview_show_archive)
      flags = flags | GETDIR_GET_ARCHIVE;
   if (conf.disp_filename_stdout)
      flags = flags | GETDIR_DISP_STDOUT;
   if (open_in_one_tab)
      flags = flags | GETDIR_RECURSIVE;

   get_dir (dirname, flags, &files->filelist, &files->dirlist);

   if (loading_stop) goto FUNC_END;

   if (tv && scan_subdir && scan_subdir != SCAN_SUB_DIR_ONE_TAB)
      goto OPEN_SUB_DIR;

   if (!conf.thumbwin_force_open_tab && g_list_length(files->filelist) < 1) {
      if (conf.disp_filename_stdout)
         g_print (_("No image files in this directory: %s\n"), dirname);
      goto OPEN_SUB_DIR;
   }

   files->filelist = g_list_first (files->filelist);
   if (!tw && conf.default_dir_open_window == IMAGE_VIEW_WINDOW) {
      /* (!tw && conf.default_dir_open_window == THUMBNAIL_WINDOW)) { */
      retval = open_image_files_in_image_view (files);
   } else {
      tw_tmp = select_thumbnail_window (tw);
      if (conf.thumbwin_raise_window)
         gdk_window_raise (GTK_WIDGET (tw_tmp)->window);

      if (tv_org)
         tv = tv_org;
      else
         tv = gimv_thumb_win_create_new_tab (tw_tmp);

      if (open_in_one_tab) {
         gimv_thumb_view_reload (tv, files, GIMV_THUMB_VIEW_MODE_RECURSIVE_DIR);
      } else {
         gimv_thumb_view_reload (tv, files, GIMV_THUMB_VIEW_MODE_DIR);
      }

      /* error handling */
      if (files->status == STOP || files->status < 0) {
         retval = files->status;
      } else {
         retval = THUMB_LOAD_DONE;
      }
   }

   /* open directories recursively */
OPEN_SUB_DIR:
   if (scan_subdir && g_list_length(files->dirlist) > 0
       && files->status != STOP && files->status >= 0
       && !open_in_one_tab)
   {
      retval = open_dirs (files, tw_tmp, type, scan_subdir);
   }

FUNC_END:
   /* free files (allocated at head of this function) */
   files_loader_delete (files);

   g_free (dirname);

   return retval;
}
/* END FIXME!! */



static gint
progress_timeout (gpointer data)
{
   gfloat new_val;
   GtkAdjustment *adj;

   adj = GTK_PROGRESS (data)->adjustment;

   new_val = adj->value + 1;
   if (new_val > adj->upper)
      new_val = adj->lower;

   gtk_progress_set_value (GTK_PROGRESS (data), new_val);

   return (TRUE);
}



static void
cb_archive_action_started (FRArchive *archive,
                           FRAction action, 
                           gpointer data)
{
}


static void
cb_archive_action_performed (FRArchive *archive,
                             FRAction action, 
                             FRProcError error, 
                             gpointer data)
{
   GimvThumbWin *tw = data;
   GtkWindow *window = tw ? GTK_WINDOW (tw) : NULL;

   if (error != FR_PROC_ERROR_NONE) {
      if (error == FR_PROC_ERROR_COMMAND_NOT_FOUND) {
         gtkutil_message_dialog (_("Error!!"),
                                 _("Command not found!!\n"),
                                 window);
      } else if (error == FR_PROC_ERROR_STOPPED) {
         gtkutil_message_dialog (_("Canceled"),
                                 _("Processing the archive file was\n"
                                   "canceled by user."),
                                 window);
      } else {
         g_print (_("An error occured while processing archive file...\n"));
         gtkutil_message_dialog (_("Error!!"),
                                 _("An error occured while processing\n"
                                   "archive file..."),
                                 window);
      }
      goto ERROR;
   }

   switch (action) {
   case FR_ACTION_LIST:
      break;

   default:
      break;
   }

ERROR:
   gtk_main_quit ();
}


static void
cb_archive_destroy (FRArchive *archive,
                    gpointer data)
{
   gchar *temp_dir;
   temp_dir = g_object_get_data (G_OBJECT (archive), "temp-dir");

   if (!temp_dir || !*temp_dir) return;

   /* remove temporary files */
   if (isdir (temp_dir)) {
      remove_dir (temp_dir);
   }
}


gint
open_archive_images (const gchar *filename,
                     GimvThumbWin *tw, GimvThumbView *tv_org,
                     ThumbLoadType type)
{
   FRArchive *archive;
   GimvThumbWin *tw_tmp;
   GimvThumbView *tv;
   gboolean success;
   gchar *ext, *temp_dir;
   FilesLoader *files;
   GList *node;
   guint timer = 0;

   g_return_val_if_fail (filename, THUMB_LOAD_DONE);

   tv = gimv_thumb_view_find_opened_archive (filename);
   if (!tv_org && tv) {
      gint pagenum;
      tw_tmp = tv->tw;
      gdk_window_raise (GTK_WIDGET (tw_tmp)->window);
      pagenum = gtk_notebook_page_num (GTK_NOTEBOOK (tw_tmp->notebook),
                                       tv->container);
      gtk_notebook_set_page (GTK_NOTEBOOK (tw_tmp->notebook), pagenum);
      return THUMB_LOAD_DONE;
   }

   node = files_loader_list;
   while (node) {
      FilesLoader *fl = node->data;
      if (fl && fl->archive && !strcmp (fl->archive->filename, filename))
         return THUMB_LOAD_DONE;
      node = g_list_next (node);
   }

   ext = fr_archive_utils_get_file_name_ext (filename);
   g_return_val_if_fail (ext, THUMB_LOAD_DONE);

   archive = fr_archive_new ();
   g_return_val_if_fail (archive, THUMB_LOAD_DONE);

   /* set progress bar */
   if (tw) {
      gtk_progress_set_activity_mode (GTK_PROGRESS (tw->progressbar), TRUE);
      timer = gtk_timeout_add (50, (GtkFunction)progress_timeout, tw->progressbar);
   }

   g_signal_connect (G_OBJECT (archive),
                     "start",
                     G_CALLBACK (cb_archive_action_started),
                     NULL);
   g_signal_connect (G_OBJECT (archive),
                     "done",
                     G_CALLBACK (cb_archive_action_performed),
                     tw);
   g_signal_connect (G_OBJECT (archive),
                     "destroy",
                     G_CALLBACK (cb_archive_destroy),
                     NULL);

   temp_dir = g_strconcat (get_temp_dir_name (),
                           FR_ARCHIVE (archive)->filename,
                           NULL);
   g_object_set_data_full (G_OBJECT (archive), "temp-dir", temp_dir,
                           (GtkDestroyNotify) g_free);

   files = files_loader_new ();
   files->thumb_load_type = type;
   files->archive = archive;

   tw_tmp = select_thumbnail_window (tw);
   if (!tw_tmp) goto ERROR;

   if (tv_org)
      tv = tv_org;
   else
      tv = gimv_thumb_win_create_new_tab (tw_tmp);

   /* get file list from archive */
   success = fr_archive_load (archive, filename);
   if (!success) {
      GtkWindow *window = tw_tmp ? GTK_WINDOW (tw_tmp) : NULL;
      g_object_unref (G_OBJECT (archive));
      /* gtk_object_remove_data (GTK_OBJECT (archive), "progress-bar"); */
      gtkutil_message_dialog (_("Error!!"),
                              _("Cannot load this archive file.\n"),
                              window);
      goto ERROR;
   }

   gtk_main ();   /* wait */

   if (archive->process->error == FR_PROC_ERROR_NONE) {
      gimv_thumb_view_reload (tv, files, GIMV_THUMB_VIEW_MODE_ARCHIVE);
   }

ERROR:
   files_loader_delete (files);      

   /* unset progress bar */
   if (tw) {
      if (timer)
         gtk_timeout_remove (timer);
      gtk_progress_set_activity_mode (GTK_PROGRESS (tw->progressbar), FALSE);
      gtk_progress_bar_update (GTK_PROGRESS_BAR(tw->progressbar), 0.0);
   }
   if (tw_tmp) {
      gimv_thumb_win_set_statusbar_page_info (tw_tmp,
                                              GIMV_THUMB_WIN_CURRENT_PAGE);
   }
   return THUMB_LOAD_DONE;
}


gint
open_dirs (FilesLoader *files, GimvThumbWin *tw,
           ThumbLoadType type, gboolean scan_subdir)
{
   gint i;
   gint listnum;
   gint retval = FALSE;
   LoadStatus status;

   if (g_list_length (files->dirlist) > 0) {
      listnum = g_list_length (g_list_first(files->dirlist));
      for (i = 0; i < listnum; i++) {

         while (gtk_events_pending()) gtk_main_iteration();

         status = open_dir_images (files->dirlist->data, tw, NULL,
                                   type, scan_subdir);

         if (files->status == STOP || files->status < 0
             || status == STOP || status < 0
             || loading_stop)
         {
            return files->status = STOP;
         } else {
            retval = status || retval;
         }

         files->dirlist = g_list_next (files->dirlist);
      }
   }

   return retval;
}


gint
open_images_dirs (GList *list, GimvThumbWin *tw,
                  ThumbLoadType type, gboolean scan_subdir)
{
   FilesLoader *files;
   GList *node;
   gchar *path;
   LoadStatus retval = FALSE;

   files = files_loader_new ();
   files->thumb_load_type = type;

   node = list;
   while (node) {
      path = node->data;
      if (isdir (path)) {
         files->dirlist = g_list_append (files->dirlist, g_strdup (path));
      } else if (file_exists (path)
                 && (!conf.detect_filetype_by_ext
                     || gimv_image_detect_type_by_ext (path)
                     || fr_archive_utils_get_file_name_ext (path)))
      {
         files->filelist = g_list_append (files->filelist, g_strdup (path));
      }
      node = g_list_next (node);
   }

   if (files->filelist) {
      if (tw)
         retval = open_image_files_in_thumbnail_view (files, tw, NULL);
      else
         retval = open_image_files (files);
   }

   if (files->dirlist)
      retval = open_dirs (files, tw, type, scan_subdir) || retval;

   files_loader_delete (files);

   return retval;
}


static void
filebrowser_open_files (FileSel *filesel, ImgWinType type)
{
   GtkFileSelection *fsel = GTK_FILE_SELECTION(filesel->filebrowser);
   FilesLoader *files;
   gint i;
   gchar **path = gtk_file_selection_get_selections (fsel);

   if (!path) return;

   files = files_loader_new ();

   for (i = 0; path[i]; i++) {
      if (!(conf.detect_filetype_by_ext)
          || gimv_image_detect_type_by_ext (path[i])
          || fr_archive_utils_get_file_name_ext (path[i]))
      {
         files->filelist = g_list_append (files->filelist, g_strdup (path[i]));
      }
   }

   if (type == THUMBNAIL_WINDOW) {
      files->status = THUMB_LOADING;
      open_image_files_in_thumbnail_view (files, filesel->tw, NULL);
   } else {
      files->status = IMAGE_LOADING;
      open_image_files_in_image_view (files);
   }

   files_loader_delete (files);

   g_strfreev (path);
}


static void
cb_filebrowser_open_selected_files(GtkWidget *widget, FileSel *filesel)
{
   /* GtkFileSelection *fsel = GTK_FILE_SELECTION(filesel->filebrowser); */

   filebrowser_open_files (filesel, IMAGE_VIEW_WINDOW);

#if 0
   /* FIXME!! If filebrowser is destoryed before loading complete,
      this will cause segmentation fault */
   gtk_clist_unselect_all(GTK_CLIST (fsel->file_list));
   gtk_entry_set_text(GTK_ENTRY(fsel->selection_entry), "");
#endif
}


static void
cb_filebrowser_add_thumbnail (GtkWidget *widget, FileSel *filesel)
{
   /* GtkFileSelection *fsel = GTK_FILE_SELECTION(filesel->filebrowser); */

   filebrowser_open_files (filesel, THUMBNAIL_WINDOW);

#if 0
   /* FIXME!! If filebrowser is destoryed before loading complete,
      this will cause segmentation fault */
   gtk_clist_unselect_all(GTK_CLIST (fsel->file_list));
   gtk_entry_set_text(GTK_ENTRY (fsel->selection_entry), "");
#endif
}


static void
cb_filebrowser_ok_sel (GtkWidget *widget, FileSel *filesel)
{
   gchar *filename;
   struct stat st;

   filename = g_strdup(gtk_file_selection_get_filename
                       (GTK_FILE_SELECTION (filesel->filebrowser)));

   if (!filename) {
      g_print (_("File name not specified!!\n"));
      return;
   }

   if (!stat(filename, &st)) {
      /* if path is directory */
      if (S_ISDIR(st.st_mode)) {
         open_dir_images(filename, filesel->tw, NULL, LOAD_CACHE,
                         conf.scan_dir_recursive);

         /* if path is archvie file */
      } else if (fr_archive_utils_get_file_name_ext (filename)) {
         open_archive_images (filename, filesel->tw, NULL, LOAD_CACHE);

         /* if path is file */
      } else if (!(conf.detect_filetype_by_ext)
                 || gimv_image_detect_type_by_ext (filename))
      {
         GimvImageInfo *info = gimv_image_info_get (filename);
         if (info)
            gimv_image_win_open_window_auto (info);
      } else {
         g_print (_("Not an image (or unsupported) file!!\n"));
      }
   }

   g_free (filename);
}


/*
 *  cb_filebrowser_ok_sel:
 *     @ Callback function for filebrowser "Close" button.
 *
 *  widget :
 *  parent :
 */
static void
cb_filebrowser_close (GtkWidget *widget, gpointer parent)
{
   GimvThumbWin *tw = parent;

   if (tw && g_list_find (gimv_thumb_win_get_list(), tw)) {
      tw->open_dialog = NULL;
   }
}


static void
cb_cancel_button_clicked(GtkWidget *widget, gpointer data)
{
   gtk_widget_destroy(GTK_WIDGET(data));
}


GtkWidget *
create_filebrowser (gpointer parent)
{
   GtkWidget *filebrowser, *bbox, *add_selected, *add_all;
   FileSel *filesel;

   filebrowser = gtk_file_selection_new(_("Load file(s)"));
   g_signal_connect (G_OBJECT (filebrowser), "destroy",
                     G_CALLBACK(cb_filebrowser_close), parent);

   filesel = g_new0 (FileSel, 1);
   filesel->filebrowser = filebrowser;
   filesel->tw          = (GimvThumbWin *) parent;
   g_object_set_data_full (G_OBJECT (filebrowser), "filesel",
                           filesel, (GtkDestroyNotify) g_free);

   if (filesel->tw)
      gtk_window_set_transient_for (GTK_WINDOW (filebrowser),
                                    GTK_WINDOW (filesel->tw));

   gtk_file_selection_set_select_multiple (GTK_FILE_SELECTION(filebrowser),
                                           TRUE);
   /*
     g_signal_connect(
        G_OBJECT(GTK_FILE_SELECTION(filebrowser)->selection_entry),
        "changed", G_CALLBACK(filebrowser_changed), filebrowser);
   */
   g_signal_connect(
      G_OBJECT(GTK_FILE_SELECTION(filebrowser)->ok_button),
      "clicked",
      G_CALLBACK(cb_filebrowser_ok_sel),
      filesel);
   g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(filebrowser)->cancel_button),
                    "clicked", G_CALLBACK(cb_cancel_button_clicked),
                    filebrowser);

   bbox = gtk_hbutton_box_new();
   gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
   gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), 0);
   gtk_box_pack_end(GTK_BOX(GTK_FILE_SELECTION(filebrowser)->action_area),
                    bbox, TRUE, TRUE, 0);

   add_selected  = gtk_button_new_with_label(_("Open selected files"));
   gtk_box_pack_start(GTK_BOX(bbox), add_selected, FALSE, FALSE, 0);
   g_signal_connect(G_OBJECT(add_selected),
                    "clicked",
                    G_CALLBACK(cb_filebrowser_open_selected_files),
                    filesel);

   add_all = gtk_button_new_with_label(_("Thumbnail for selected files"));
   gtk_box_pack_start(GTK_BOX(bbox), add_all, FALSE, FALSE, 0);
   g_signal_connect(G_OBJECT(add_all),
                    "clicked",
                    G_CALLBACK (cb_filebrowser_add_thumbnail),
                    filesel);
   gtk_widget_show_all(bbox);

   gtk_widget_show(filebrowser);

   gimv_icon_stock_set_window_icon (filebrowser->window, "nfolder");

   return filebrowser;
}



#ifdef GIMV_DEBUG
void
file_disp_loading_status (FilesLoader *files)
{
   g_print ("Loading ID: %p   ", files);
   g_print ("status ID: %d   ", files->status);

   switch (files->status) {
   case NO_OPEN_FILE:
      g_print ("Loading status: NO_OPEN_FILE\n");
      break;
   case END:
      g_print ("Loading status: END\n");
      break;
   case GETTING_FILELIST:
      g_print ("Loading status: GETTING_FILELIST\n");
      break;
   case IMAGE_LOADING:
      g_print ("Loading status: IMAGE_LOADING\n");
      break;
   case IMAGE_LOAD_DONE:
      g_print ("Loading status: IMAGE_LOAD_DONE\n");
      break;
   case THUMB_LOADING:
      g_print ("Loading status: THUMB_LOADING\n");
      break;
   case THUMB_LOAD_DONE:
      g_print ("Loading status: THUMB_LOAD_DONE\n");
      break;
   case CANCEL:
      g_print ("Loading status: CANCEL\n");
      break;
   case STOP:
      g_print ("Loading status: STOP\n");
      break;
   case CONTAINER_DESTROYED:
      g_print ("Loading status: CONTAINER_DESTROYED\n");
      break;
   case WINDOW_DESTROYED:
      g_print ("Loading status: WINDOW_DESTROYED\n");
      break;
   default:
      break;
   }
}
#endif /* GIMV_DEBUG */
