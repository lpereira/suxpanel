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

#ifndef __GTK__MISC_H__
#define __GTK__MISC_H__

GtkWidget *gtk_image_new_from_file_scaled(const gchar *file, gint width,
			gint height);
void gtkm_widget_set_cursor(GtkWidget *widget, GdkCursorType cursor);

#endif	/* __GTK__MISC_H__ */
