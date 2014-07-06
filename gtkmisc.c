/*
 * SuxPanel version 0.4b
 * Copyright (c) 2003-2005 Leandro Pereira <leandro@linuxmag.com.br>
 *
 * This program may be distributed under the terms of GNU General
 * Public License version 2. You should have received a copy of the
 * license with this program; if not, please consult http://www.fsf.org/.
 *
 * This program comes with no warranty. Use at your own risk.
 *
 */

#include <gtk/gtk.h>

GtkWidget *gtk_image_new_from_file_scaled(const gchar *file, gint width,
			gint height)
{
	GtkWidget *pix;
	
	if (g_file_test(file, G_FILE_TEST_EXISTS)) {		
		GError *err = NULL;
		GdkPixbuf *pb;
		
		pb = gdk_pixbuf_new_from_file(file, &err);
		if (err || !pb) {
			g_error_free(err);
		} else {
			GdkPixbuf *pb_scaled;

 			pb_scaled = gdk_pixbuf_scale_simple(pb, width, height,
							GDK_INTERP_BILINEAR);
		
			pix = gtk_image_new_from_pixbuf(pb_scaled);			
		
			gdk_pixbuf_unref(pb);
			gdk_pixbuf_unref(pb_scaled);

			return pix;
		}
	}

	pix = gtk_image_new_from_stock(GTK_STOCK_MISSING_IMAGE,
			   	       GTK_ICON_SIZE_BUTTON);

	return pix;	
}

void
gtkm_widget_set_cursor(GtkWidget *widget, GdkCursorType cursor_type)
{
        GdkCursor *cursor;
        
        cursor = gdk_cursor_new(cursor_type);
        gdk_window_set_cursor(GDK_WINDOW(widget->window), cursor);
        gdk_cursor_unref(cursor);
        
        while(gtk_events_pending())
                gtk_main_iteration();        
}

