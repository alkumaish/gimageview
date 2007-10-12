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

#include <string.h>
#include <gtk/gtk.h>
#include "fileutil.h"
#include "fr-process.h"
#include "fr-command.h"
#include "gimv_image_info.h"
#include "gtk2-compat.h"


static void fr_command_class_init  (FRCommandClass *class);
static void fr_command_init        (FRCommand *afile);
static void fr_command_destroy     (GtkObject *object);


enum {
   START,
   DONE,
   LAST_SIGNAL
};


static GtkObjectClass *parent_class = NULL;
static guint fr_command_signals[LAST_SIGNAL] = { 0 };


static void
base_fr_command_list (FRCommand *comm)
{
}


static void
base_fr_command_add (FRCommand *comm,
                     GList *file_list,
                     gchar *base_dir,
                     gboolean update)
{
}


static void
base_fr_command_delete (FRCommand *comm,
                        GList *file_list)
{
}


static void
base_fr_command_extract (FRCommand *comm,
                         GList *file_list,
                         char *dest_dir,
                         gboolean overwrite,
                         gboolean skip_older,
                         gboolean junk_paths)
{
}


static void 
fr_command_class_init (FRCommandClass *class)
{
   GtkObjectClass *object_class;

   parent_class = gtk_type_class (GTK_TYPE_OBJECT);
   object_class = (GtkObjectClass*) class;

   fr_command_signals[START] =
      gtk_signal_new ("start",
                      GTK_RUN_LAST,
                      GTK_CLASS_TYPE (object_class),
                      GTK_SIGNAL_OFFSET (FRCommandClass, start),
                      gtk_marshal_NONE__INT,
                      GTK_TYPE_NONE, 1,
                      GTK_TYPE_INT);
   fr_command_signals[DONE] =
      gtk_signal_new ("done",
                      GTK_RUN_LAST,
                      GTK_CLASS_TYPE (object_class),
                      GTK_SIGNAL_OFFSET (FRCommandClass, done),
                      gtk_marshal_NONE__INT_INT,
                      GTK_TYPE_NONE, 2,
                      GTK_TYPE_INT,
                      GTK_TYPE_INT);
   gtk_object_class_add_signals (object_class, fr_command_signals, 
                                 LAST_SIGNAL);

   object_class->destroy = fr_command_destroy;

   class->list        = base_fr_command_list;
   class->add         = base_fr_command_add;
   class->delete      = base_fr_command_delete;
   class->extract     = base_fr_command_extract;

   class->start       = NULL;
   class->done        = NULL;
}


static void 
fr_command_start (FRProcess *process,
                  gpointer data)
{
   FRCommand *comm = FR_COMMAND (data);
   gtk_signal_emit (GTK_OBJECT (comm), fr_command_signals[START], 
                    comm->action);
}


static void 
fr_command_done (FRProcess *process,
                 FRProcError error, 
                 gpointer data)
{
   FRCommand *comm = FR_COMMAND (data);
   comm->file_list = g_list_reverse (comm->file_list);
   gtk_signal_emit (GTK_OBJECT (comm), fr_command_signals[DONE], 
                    comm->action, 
                    error);
}


static void 
fr_command_init (FRCommand *comm)
{
   comm->filename = NULL;
   comm->file_list = NULL;

#ifdef USE_GTK2
   gtk_object_ref (GTK_OBJECT (comm));
   gtk_object_sink (GTK_OBJECT (comm));
#endif
}


static void 
fr_command_destroy (GtkObject *object)
{
   FRCommand* comm;

   g_return_if_fail (object != NULL);
   g_return_if_fail (IS_FR_COMMAND (object));
  
   comm = FR_COMMAND (object);
   if (comm->filename != NULL)
      g_free (comm->filename);

   if (comm->file_list != NULL) {
      g_list_foreach (comm->file_list, 
                      (GFunc) gimv_image_info_unref_with_archive, 
                      NULL);
      g_list_free (comm->file_list);
   }

   gtk_signal_disconnect_by_data (GTK_OBJECT (comm->process), comm);
   gtk_object_unref (GTK_OBJECT (comm->process));

   /* Chain up */
   if (GTK_OBJECT_CLASS (parent_class)->destroy)
      (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}


GtkType
fr_command_get_type ()
{
   static guint fr_command_type = 0;

   if (!fr_command_type) {
      GtkTypeInfo fr_command_info = {
         "FRCommand",
         sizeof (FRCommand),
         sizeof (FRCommandClass),
         (GtkClassInitFunc) fr_command_class_init,
         (GtkObjectInitFunc) fr_command_init,
         /* reserved_1 */ NULL,
         /* reserved_2 */ NULL,
         (GtkClassInitFunc) NULL,
      };
      fr_command_type = gtk_type_unique (gtk_object_get_type(),
                                         &fr_command_info);
   }

   return fr_command_type;
}


void
fr_command_construct (FRCommand *comm,
                      FRProcess *process,
                      const char *fr_command_name)
{
   fr_command_set_filename (comm, fr_command_name);

   gtk_object_ref (GTK_OBJECT (process));
   comm->process = process;
   gtk_signal_connect (GTK_OBJECT (comm->process), "start",
                       GTK_SIGNAL_FUNC (fr_command_start),
                       comm);
   gtk_signal_connect (GTK_OBJECT (comm->process), "done",
                       GTK_SIGNAL_FUNC (fr_command_done),
                       comm);
}


void
fr_command_set_filename (FRCommand *comm,
                         const char *filename)
{
   g_return_if_fail (IS_FR_COMMAND (comm));

   if (comm->filename != NULL)
      g_free (comm->filename);

   if (!g_path_is_absolute (filename)) {
      char *current_dir;
      current_dir = g_get_current_dir ();
      comm->filename = g_strconcat (current_dir, 
                                    "/", 
                                    filename, 
                                    NULL);
      g_free (current_dir);
   } else {
      comm->filename = g_strdup (filename);
   }
}


void
fr_command_list (FRCommand *comm)
{
   g_return_if_fail (IS_FR_COMMAND (comm));

   if (comm->file_list != NULL) {
      g_list_foreach (comm->file_list, (GFunc) gimv_image_info_unref, NULL);
      g_list_free (comm->file_list);
      comm->file_list = NULL;
   }

   comm->action = FR_ACTION_LIST;
   FR_COMMAND_CLASS (GTK_OBJECT_GET_CLASS (comm))->list (comm);
}


void
fr_command_add (FRCommand *comm,
                GList *file_list,
                gchar *base_dir,
                gboolean update)
{
   comm->action = FR_ACTION_ADD;
   FR_COMMAND_CLASS (GTK_OBJECT_GET_CLASS (comm))->add (comm, 
                                                        file_list,
                                                        base_dir,
                                                        update);
}


void
fr_command_delete (FRCommand *comm,
                   GList *file_list)
{
   gboolean free_file_list = FALSE;

   comm->action = FR_ACTION_DELETE;

   /* file_list == NULL means all files in archive. */

   if (file_list == NULL) {
      GList *scan_fdata;

      free_file_list = TRUE;

      for (scan_fdata = comm->file_list; scan_fdata; scan_fdata = scan_fdata->next) {
         GimvImageInfo *fdata = scan_fdata->data;
         file_list = g_list_prepend (file_list, g_strdup (fdata->filename));
      }

   } 

   FR_COMMAND_CLASS (GTK_OBJECT_GET_CLASS (comm))->delete (comm, file_list);

   if (free_file_list) {
      g_list_foreach (file_list, (GFunc) g_free, NULL);
      g_list_free (file_list);
   }
}


void
fr_command_extract (FRCommand *comm,
                    GList *file_list,
                    char *dest_dir,
                    gboolean overwrite,
                    gboolean skip_older,
                    gboolean junk_paths)
{
   comm->action = FR_ACTION_EXTRACT;
   FR_COMMAND_CLASS (GTK_OBJECT_GET_CLASS (comm))->extract (comm, 
                                                            file_list, 
                                                            dest_dir,
                                                            overwrite,
                                                            skip_older,
                                                            junk_paths);
}
