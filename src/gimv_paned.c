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

/*
 * These codes are taken from gThumb.
 * gThumb code Copyright (C) 2001 The Free Software Foundation, Inc.
 * gThumb author: Paolo Bacchilega
 */

#include "gimv_paned.h"

#ifndef USE_NORMAL_PANED

#ifndef GTK_OBJECT_GET_CLASS
#define GTK_OBJECT_GET_CLASS(object) \
    GTK_OBJECT (object)->klass
#endif

char gray50_bits[] = {0x02, 0x01};

enum {
   ARG_0,
   ARG_GUTTER_SIZE
};

static void gimv_paned_class_init      (GimvPanedClass *klass);
static void gimv_paned_init            (GimvPaned	   *paned);
static void gimv_paned_set_arg         (GtkObject	   *object,
                                        GtkArg	   *arg,
                                        guint	    arg_id);
static void gimv_paned_get_arg         (GtkObject	   *object,
                                        GtkArg	   *arg,
                                        guint	    arg_id);
static void gimv_paned_realize         (GtkWidget      *widget);
static void gimv_paned_map             (GtkWidget	   *widget);
static void gimv_paned_unmap           (GtkWidget	   *widget);
static void gimv_paned_unrealize       (GtkWidget      *widget);
static gint gimv_paned_expose          (GtkWidget	   *widget,
                                        GdkEventExpose *event);
static void gimv_paned_add             (GtkContainer   *container,
                                        GtkWidget	   *widget);
static void gimv_paned_remove          (GtkContainer   *container,
                                        GtkWidget	   *widget);
static void gimv_paned_forall          (GtkContainer   *container,
                                        gboolean	    include_internals,
                                        GtkCallback	    callback,
                                        gpointer	    callback_data);
static GtkType gimv_paned_child_type   (GtkContainer   *container);
static gint gimv_paned_motion          (GtkWidget      *widget,
                                        GdkEventMotion *event);


static GtkContainerClass *parent_class = NULL;


GtkType
gimv_paned_get_type (void)
{
   static GtkType paned_type = 0;
	
   if (!paned_type) {
      static const GtkTypeInfo paned_info = {
         "GimvPaned",
         sizeof (GimvPaned),
         sizeof (GimvPanedClass),
         (GtkClassInitFunc) gimv_paned_class_init,
         (GtkObjectInitFunc) gimv_paned_init,
         /* reserved_1 */ NULL,
         /* reserved_2 */ NULL,
         (GtkClassInitFunc) NULL,
      };
      paned_type = gtk_type_unique (GTK_TYPE_CONTAINER, &paned_info);
   }
	
   return paned_type;
}


static void
gimv_paned_virtual_xor_line (GimvPaned *paned) 
{
   g_warning ("gimv_paned_virtual_xor_line reached !");
}


static void
gimv_paned_class_init (GimvPanedClass *class)
{
   GtkObjectClass *object_class;
   GtkWidgetClass *widget_class;
   GtkContainerClass *container_class;
	
   object_class = (GtkObjectClass*) class;
   widget_class = (GtkWidgetClass*) class;
   container_class = (GtkContainerClass*) class;
	
   parent_class = gtk_type_class (GTK_TYPE_CONTAINER);
	
   object_class->set_arg = gimv_paned_set_arg;
   object_class->get_arg = gimv_paned_get_arg;
	
   widget_class->realize = gimv_paned_realize;
   widget_class->map = gimv_paned_map;
   widget_class->unmap = gimv_paned_unmap;
   widget_class->unrealize = gimv_paned_unrealize;
   widget_class->expose_event = gimv_paned_expose;
   widget_class->motion_notify_event = gimv_paned_motion;
	
   container_class->add = gimv_paned_add;
   container_class->remove = gimv_paned_remove;
   container_class->forall = gimv_paned_forall;
   container_class->child_type = gimv_paned_child_type;

   class->xor_line = gimv_paned_virtual_xor_line;
	
   gtk_object_add_arg_type ("GimvPaned::gutter_size", GTK_TYPE_UINT,
                            GTK_ARG_READWRITE, ARG_GUTTER_SIZE);
}


static GtkType
gimv_paned_child_type (GtkContainer *container)
{
   if (!GIMV_PANED (container)->child1 || !GIMV_PANED (container)->child2)
      return GTK_TYPE_WIDGET;
   else
      return GTK_TYPE_NONE;
}


static void
gimv_paned_init (GimvPaned *paned)
{
   GTK_WIDGET_UNSET_FLAGS (paned, GTK_NO_WINDOW);
	
   paned->child1 = NULL;
   paned->child2 = NULL;
   paned->handle = NULL;
   paned->xor_gc = NULL;
	
   paned->gutter_size = 6;
   paned->position_set = FALSE;
   paned->last_allocation = -1;
   paned->in_drag = FALSE;
	
   paned->handle_xpos = -1;
   paned->handle_ypos = -1;
	
   paned->horizontal = TRUE;

   paned->child1_minsize = 0;
   paned->child2_minsize = 0;
   paned->child1_use_minsize = FALSE;
   paned->child2_use_minsize = FALSE;

   paned->child_hidden = 0;
}


static void
gimv_paned_set_arg (GtkObject *object,
                    GtkArg    *arg,
                    guint      arg_id)
{
   GimvPaned *paned = GIMV_PANED (object);
	
   switch (arg_id)	{
   case ARG_GUTTER_SIZE:
      gimv_paned_set_gutter_size (paned, GTK_VALUE_UINT (*arg));
      break;
   }
}


static void
gimv_paned_get_arg (GtkObject *object,
                    GtkArg    *arg,
                    guint      arg_id)
{
   GimvPaned *paned = GIMV_PANED (object);
	
   switch (arg_id)	{
   case ARG_GUTTER_SIZE:
      GTK_VALUE_UINT (*arg) = paned->gutter_size;
      break;
   default:
      arg->type = GTK_TYPE_INVALID;
      break;
   }
}


static void
gimv_paned_realize (GtkWidget *widget)
{
   GimvPaned *paned;
   GdkWindowAttr attributes;
   gint attributes_mask;
	
   g_return_if_fail (widget != NULL);
   g_return_if_fail (GIMV_IS_PANED (widget));
	
   GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);
   paned = GIMV_PANED (widget);
	
   attributes.x = widget->allocation.x;
   attributes.y = widget->allocation.y;
   attributes.width = widget->allocation.width;
   attributes.height = widget->allocation.height;
   attributes.window_type = GDK_WINDOW_CHILD;
   attributes.wclass = GDK_INPUT_OUTPUT;
   attributes.visual = gtk_widget_get_visual (widget);
   attributes.colormap = gtk_widget_get_colormap (widget);
   attributes.event_mask = gtk_widget_get_events (widget) | GDK_EXPOSURE_MASK;
   attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
	
   widget->window = gdk_window_new (gtk_widget_get_parent_window (widget),
                                    &attributes, attributes_mask);
   gdk_window_set_user_data (widget->window, paned);
	
   attributes.x = paned->handle_xpos;
   attributes.y = paned->handle_ypos;
   if (paned->horizontal) {
      attributes.width = paned->gutter_size;
      attributes.height = widget->allocation.height;
      attributes.cursor = gdk_cursor_new (GDK_SB_H_DOUBLE_ARROW);
   } else {
      attributes.width = widget->allocation.width;
      attributes.height = paned->gutter_size;
      attributes.cursor = gdk_cursor_new (GDK_SB_V_DOUBLE_ARROW);
   }
   attributes.event_mask |= (GDK_BUTTON_PRESS_MASK |
                             GDK_BUTTON_RELEASE_MASK |
                             GDK_POINTER_MOTION_MASK |
                             GDK_POINTER_MOTION_HINT_MASK);
   attributes_mask |= GDK_WA_CURSOR;
	
   paned->handle = gdk_window_new (widget->window,
                                   &attributes, attributes_mask);
   gdk_window_set_user_data (paned->handle, paned);
   gdk_cursor_destroy (attributes.cursor);
  
   widget->style = gtk_style_attach (widget->style, widget->window);
	
   gtk_style_set_background (widget->style, widget->window, GTK_STATE_NORMAL);
   gtk_style_set_background (widget->style, paned->handle, GTK_STATE_NORMAL);
	
   gdk_window_set_back_pixmap (widget->window, NULL, TRUE);

   gdk_window_show (paned->handle);
}


static void
gimv_paned_map (GtkWidget *widget)
{
   GimvPaned *paned;

   g_return_if_fail (widget != NULL);
   g_return_if_fail (GIMV_IS_PANED (widget));
	
   GTK_WIDGET_SET_FLAGS (widget, GTK_MAPPED);
   paned = GIMV_PANED (widget);
	
   if (paned->child1 
       && GTK_WIDGET_VISIBLE (paned->child1) 
       && !GTK_WIDGET_MAPPED (paned->child1))
      gtk_widget_map (paned->child1);
   if (paned->child2 
       && GTK_WIDGET_VISIBLE (paned->child2) 
       && !GTK_WIDGET_MAPPED (paned->child2))
      gtk_widget_map (paned->child2);

   gdk_window_show (widget->window);
}


static void
gimv_paned_unmap (GtkWidget *widget)
{
   g_return_if_fail (widget != NULL);
   g_return_if_fail (GIMV_IS_PANED (widget));
	
   GTK_WIDGET_UNSET_FLAGS (widget, GTK_MAPPED);

   gdk_window_hide (widget->window);
}


static void
gimv_paned_unrealize (GtkWidget *widget)
{
   GimvPaned *paned;
	
   g_return_if_fail (widget != NULL);
   g_return_if_fail (GIMV_IS_PANED (widget));
	
   paned = GIMV_PANED (widget);
	
   if (paned->xor_gc) {
      gdk_gc_destroy (paned->xor_gc);
      paned->xor_gc = NULL;
   }
	
   if (paned->handle) {
      gdk_window_set_user_data (paned->handle, NULL);
      gdk_window_destroy (paned->handle);
      paned->handle = NULL;
   }
	
   if (GTK_WIDGET_CLASS (parent_class)->unrealize)
      (* GTK_WIDGET_CLASS (parent_class)->unrealize) (widget);
}


static gint
gimv_paned_expose (GtkWidget	  *widget, 
                   GdkEventExpose *event)
{
   GimvPaned *paned;
   GdkEventExpose child_event;

   g_return_val_if_fail (widget != NULL, FALSE);
   g_return_val_if_fail (GIMV_IS_PANED (widget), FALSE);
   g_return_val_if_fail (event != NULL, FALSE);
	
   if (GTK_WIDGET_DRAWABLE (widget)) {
      paned = GIMV_PANED (widget);

      /* An expose event for the handle */
      if (event->window == paned->handle) {
         gint width, height;

         gdk_window_get_size (paned->handle, &width, &height);
			
         gtk_paint_flat_box (widget->style, paned->handle,
                             GTK_WIDGET_STATE (widget),
                             GTK_SHADOW_NONE, 
                             &event->area, widget, "paned",
                             0, 0,
                             width, height);
      } else {
         child_event = *event;
         if (paned->child1 
             && GTK_WIDGET_NO_WINDOW (paned->child1) 
             && gtk_widget_intersect (paned->child1, 
                                      &event->area,
                                      &child_event.area))
            gtk_widget_event (paned->child1, 
                              (GdkEvent*) &child_event);

         if (paned->child2 
             && GTK_WIDGET_NO_WINDOW (paned->child2) 
             && gtk_widget_intersect (paned->child2, 
                                      &event->area,
                                      &child_event.area))
            gtk_widget_event (paned->child2, 
                              (GdkEvent*) &child_event);
      }
   }
   return FALSE;
}


void
gimv_paned_add1 (GimvPaned *paned, 
                 GtkWidget *widget)
{
   gimv_paned_pack1 (paned, widget, FALSE, TRUE);
}


void
gimv_paned_add2 (GimvPaned *paned, GtkWidget *widget)
{
   gimv_paned_pack2 (paned, widget, TRUE, TRUE);
}


void
gimv_paned_pack1 (GimvPaned *paned, 
                  GtkWidget *child, 
                  gboolean   resize, 
                  gboolean   shrink)
{
   g_return_if_fail (paned != NULL);
   g_return_if_fail (GIMV_IS_PANED (paned));
   g_return_if_fail (GTK_IS_WIDGET (child));
	
   if (!paned->child1) {
      paned->child1 = child;
      paned->child1_resize = resize;
      paned->child1_shrink = shrink;

      gtk_widget_set_parent (child, GTK_WIDGET (paned));
		
      if (GTK_WIDGET_REALIZED (child->parent))
         gtk_widget_realize (child);
		
      if (GTK_WIDGET_VISIBLE (child->parent) 
          && GTK_WIDGET_VISIBLE (child))
      {
         if (GTK_WIDGET_MAPPED (child->parent))
            gtk_widget_map (child);

         gtk_widget_queue_resize (child);
      }
   }
}


void
gimv_paned_pack2 (GimvPaned *paned, 
                  GtkWidget *child, 
                  gboolean   resize,
                  gboolean   shrink)
{
   g_return_if_fail (paned != NULL);
   g_return_if_fail (GIMV_IS_PANED (paned));
   g_return_if_fail (GTK_IS_WIDGET (child));
	
   if (!paned->child2) {
      paned->child2 = child;
      paned->child2_resize = resize;
      paned->child2_shrink = shrink;
		
      gtk_widget_set_parent (child, GTK_WIDGET (paned));
		
      if (GTK_WIDGET_REALIZED (child->parent))
         gtk_widget_realize (child);

      if (GTK_WIDGET_VISIBLE (child->parent) 
          && GTK_WIDGET_VISIBLE (child))
      {
         if (GTK_WIDGET_MAPPED (child->parent))
            gtk_widget_map (child);

         gtk_widget_queue_resize (child);
      }
   }
}


static void
gimv_paned_add (GtkContainer *container, 
                GtkWidget    *widget)
{
   GimvPaned *paned;
	
   g_return_if_fail (container != NULL);
   g_return_if_fail (GIMV_IS_PANED (container));
   g_return_if_fail (widget != NULL);
	
   paned = GIMV_PANED (container);
	
   if (!paned->child1)
      gimv_paned_add1 (GIMV_PANED (container),widget);
   else if (!paned->child2)
      gimv_paned_add2 (GIMV_PANED (container),widget);
}


static void
gimv_paned_remove (GtkContainer *container, 
                   GtkWidget	*widget)
{
   GimvPaned *paned;
   gboolean was_visible;
	
   g_return_if_fail (container != NULL);
   g_return_if_fail (GIMV_IS_PANED (container));
   g_return_if_fail (widget != NULL);
	
   paned = GIMV_PANED (container);
   was_visible = GTK_WIDGET_VISIBLE (widget);
	
   if (paned->child1 == widget) {
      gtk_widget_unparent (widget);

      paned->child1 = NULL;
		
      if (was_visible && GTK_WIDGET_VISIBLE (container))
         gtk_widget_queue_resize (GTK_WIDGET (container));
   } else if (paned->child2 == widget) {
      gtk_widget_unparent (widget);
		
      paned->child2 = NULL;
		
      if (was_visible && GTK_WIDGET_VISIBLE (container))
         gtk_widget_queue_resize (GTK_WIDGET (container));
   }
}


static void
gimv_paned_forall (GtkContainer *container, 
                   gboolean	 include_internals,
                   GtkCallback	 callback, 
                   gpointer	 callback_data)
{
   GimvPaned *paned;
  
   g_return_if_fail (container != NULL);
   g_return_if_fail (GIMV_IS_PANED (container));
   g_return_if_fail (callback != NULL);
  
   paned = GIMV_PANED (container);
  
   if (paned->child1)
      (* callback) (paned->child1, callback_data);
   if (paned->child2)
      (* callback) (paned->child2, callback_data);
}


void
gimv_paned_set_position (GimvPaned  *paned, 
                         gint position)
{
   g_return_if_fail (paned != NULL);
   g_return_if_fail (GIMV_IS_PANED (paned));
	
   if (position >= 0) {
      /* We don't clamp here - the assumption is that
       * if the total allocation changes at the same time
       * as the position, the position set is with reference
       * to the new total size. If only the position changes,
       * then clamping will occur in gimv_paned_compute_position()
       */
      paned->child1_size = position;
      paned->position_set = TRUE;

      paned->child_hidden = 0;
   } else
      paned->position_set = FALSE;

   gtk_widget_queue_resize (GTK_WIDGET (paned));
}


gint
gimv_paned_get_position (GimvPaned  *paned)
{	
   g_return_val_if_fail (paned != NULL, 0);
   g_return_val_if_fail (GIMV_IS_PANED (paned), 0);

   return paned->child1_size;
}


void
gimv_paned_set_gutter_size (GimvPaned *paned, 
                            guint16 size)
{
   g_return_if_fail (paned != NULL);
   g_return_if_fail (GIMV_IS_PANED (paned));
	
   paned->gutter_size = size;
	
   if (GTK_WIDGET_VISIBLE (GTK_WIDGET (paned)))
      gtk_widget_queue_resize (GTK_WIDGET (paned));
}


void
gimv_paned_xor_line (GimvPaned *paned)
{
   GIMV_PANED_CLASS (GTK_OBJECT_GET_CLASS (paned))->xor_line (paned);
}


/**
 * gimv_paned_child1_use_minsize: Set a minimum size for child 1.
 * @paned: The paned object.
 * @use_minsize: Whether or not to use the minimum size option.
 * @minsize: The minimun size to use if the option is enabled.
 * 
 * If @use_minsize = TRUE then set @minsize as the minimal size child 1 can 
 * be set to.
 * If @use_minsize = FALSE then disable this option, that is, child 1 can have
 * any size.
 **/
void
gimv_paned_child1_use_minsize (GimvPaned *paned,
                               gboolean	  use_minsize,
                               gint	  minsize)
{
   g_return_if_fail (paned != NULL);
   g_return_if_fail (GIMV_IS_PANED (paned));

   paned->child1_use_minsize = use_minsize;
   if (use_minsize) 
      paned->child1_minsize = minsize;
}


/**
 * gimv_paned_child1_use_minsize: Set a minimum size for child 2.
 * @paned: The paned object.
 * @use_minsize: Whether or not to use the minimum size option.
 * @minsize: The minimun size to use if the option is enabled.
 * 
 * If @use_minsize = TRUE then set @minsize as the minimal size child 2 can 
 * be set to.
 * If @use_minsize = FALSE then disable this option, that is, child 2 can have
 * any size.
 **/
void
gimv_paned_child2_use_minsize (GimvPaned *paned,
                               gboolean	  use_minsize,
                               gint	  minsize)
{
   g_return_if_fail (paned != NULL);
   g_return_if_fail (GIMV_IS_PANED (paned));

   paned->child2_use_minsize = use_minsize;
   if (use_minsize) 
      paned->child2_minsize = minsize;
}


void
gimv_paned_compute_position (GimvPaned *paned,
                             gint      allocation,
                             gint      child1_req,
                             gint      child2_req)
{
   gint child2_size;

   g_return_if_fail (paned != NULL);
   g_return_if_fail (GIMV_IS_PANED (paned));
  
   paned->min_position = paned->child1_shrink ? 0 : child1_req;

   paned->max_position = allocation;
   if (!paned->child2_shrink)
      paned->max_position = MAX (1, paned->max_position - child2_req);

   if (!paned->position_set) {
      if (paned->child1_resize && !paned->child2_resize)
         paned->child1_size = MAX (1, allocation - child2_req);
      else if (!paned->child1_resize && paned->child2_resize)
         paned->child1_size = child1_req;
      else if (child1_req + child2_req != 0)
         paned->child1_size = allocation * ((gdouble)child1_req / (child1_req + child2_req));
      else
         paned->child1_size = allocation * 0.5;
   } else {
      /* If the position was set before the initial allocation.
       * (paned->last_allocation <= 0) just clamp it and leave it.
       */
      if (paned->last_allocation > 0)	{
         if (paned->child1_resize && !paned->child2_resize)
            paned->child1_size += (allocation - paned->last_allocation);
         else if (!(!paned->child1_resize && paned->child2_resize))
            paned->child1_size = allocation * ((gdouble)paned->child1_size / (paned->last_allocation));
      }
   }
   
   paned->child1_size = CLAMP (paned->child1_size,
                               paned->min_position,
                               paned->max_position);

   child2_size = allocation - paned->child1_size;


   /* Ensure that minimum sizes, if enabled, are respected. */
   if (paned->child1_use_minsize) {
      if (paned->child1_size < paned->child1_minsize)
         paned->child1_size = paned->child1_minsize;
   }  else if (paned->child2_use_minsize) {
      if (child2_size < paned->child2_minsize)
			paned->child1_size = paned->child2_minsize;
   }

   if (paned->child_hidden == 1)
      paned->child1_size = 0;
   else if (paned->child_hidden == 2) {
      paned->child1_size = allocation;
      paned->child1_size += paned->gutter_size;
   }

	paned->last_allocation = allocation;
}


/**
 * gimv_paned_hide_child1: hide child 1.
 * @paned: The paned widget.
 * 
 * Hide the child number 1.
 **/
void
gimv_paned_hide_child1 (GimvPaned *paned)
{
   g_return_if_fail (paned != NULL);
   g_return_if_fail (GIMV_IS_PANED (paned));

   if (paned->child_hidden != 0)
      return;
	
   paned->child_hidden = 1;
   gtk_widget_queue_resize (GTK_WIDGET (paned));
}


/**
 * gimv_paned_hide_child2: collapse child 2.
 * @paned: The paned widget.
 * 
 * Hide the child number 2.
 **/
void
gimv_paned_hide_child2 (GimvPaned *paned)
{
   g_return_if_fail (paned != NULL);
   g_return_if_fail (GIMV_IS_PANED (paned));

   if (paned->child_hidden != 0)
      return;

   paned->child_hidden = 2;
   gtk_widget_queue_resize (GTK_WIDGET (paned));
}


/**
 * gimv_paned_split: split the paned widget in two halves.
 * @paned: The paned widget.
 * 
 * Give both children the same size.
 **/
void
gimv_paned_split (GimvPaned *paned)
{
   g_return_if_fail (paned != NULL);
   g_return_if_fail (GIMV_IS_PANED (paned));

   gimv_paned_set_position (paned, paned->last_allocation / 2);
}


guint
gimv_paned_which_hidden (GimvPaned *paned)
{
   g_return_val_if_fail (paned != NULL, 0);
   g_return_val_if_fail (GIMV_IS_PANED (paned), 0);

   return paned->child_hidden;
}


static gint
gimv_paned_motion (GtkWidget	  *widget, 
                   GdkEventMotion *event)
{
   GimvPaned *paned;
   gint pos;
   static gint last_size = -1;

   g_return_val_if_fail (widget != NULL, FALSE);
   g_return_val_if_fail (GIMV_IS_PANED (widget), FALSE);
	
   paned = GIMV_PANED (widget);

   if (event->is_hint || event->window != widget->window) {
      if (paned->horizontal)
         gtk_widget_get_pointer (widget, &pos, NULL);
      else
         gtk_widget_get_pointer (widget, NULL, &pos);
   } else
      pos = (paned->horizontal) ? event->x : event->y;

   if (paned->in_drag) {
      gint child1_size, child2_size;

      child1_size = (pos - GTK_CONTAINER (paned)->border_width 
                     - paned->gutter_size / 2);
      child2_size = paned->last_allocation - child1_size;

      if (paned->child1_use_minsize 
          && (child1_size < paned->child1_minsize)) 
         child1_size = paned->child1_minsize;

      if (paned->child2_use_minsize 
          && (child2_size < paned->child2_minsize))
         child1_size = paned->last_allocation - paned->child2_minsize;

      /* Avoid blinking. */
      if (child1_size == last_size)
         return FALSE;

      last_size = child1_size;

      gimv_paned_xor_line (paned);
      paned->child1_size = CLAMP (child1_size,
                                  paned->min_position,
                                  paned->max_position);
      gimv_paned_xor_line (paned);
   }

   return FALSE;
}

#else

guint
gimv_paned_which_hidden (GimvPaned *paned)
{
   g_return_val_if_fail (GIMV_IS_PANED (paned), 0);
   g_return_val_if_fail (paned->child1 && paned->child2, 0);

   if (!GTK_WIDGET_VISIBLE (paned->child1)
       && GTK_WIDGET_VISIBLE (paned->child2))
   {
      return 1;

   } else if (GTK_WIDGET_VISIBLE (paned->child1)
              && !GTK_WIDGET_VISIBLE (paned->child2))
   {
      return 2;

   } else {
      return 0;
   }
}

#endif /* USE_NORMAL_PANED */
