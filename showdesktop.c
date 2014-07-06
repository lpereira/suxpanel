/*
 * Show Desktop Button for SuxPanel
 * Version 0.1
 * 
 * Copyright (c) 2004 Leandro Pereira <leandro@linuxmag.com.br>
 *
 * Based on "Show Desktop" applet for GNOME by Havoc Pennington
 * Copyright (c) 2002 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */
#include "suxpanel.h"

#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/screen.h>

typedef struct _ShowDesktop ShowDesktop;
struct _ShowDesktop {
        GtkWidget *button;
        WnckScreen *screen;
	GtkTooltips *tips;
        
        gboolean is_showing_desktop;
        gboolean lock;
};

static void update_button_state(ShowDesktop *sd, gboolean setting);

static void show_desktop(ShowDesktop *sd, gboolean setting)
{
	wnck_screen_toggle_showing_desktop(sd->screen, setting);
	sd->is_showing_desktop = setting;
}

static void update_tooltip(ShowDesktop *sd)
{
        gchar *tip;
        
        if (sd->is_showing_desktop) {
		tip = "Click here to restore hidden windows.";
        } else {
		tip = "Click here to hide all windows and show the desktop.";
        }
        
	gtk_tooltips_set_tip(sd->tips, sd->button, tip, NULL);
}

static gboolean toggle_show_desktop(GtkWidget *widget, gpointer data)
{
        ShowDesktop *sd = (ShowDesktop *) data;
        gboolean show_desktop_flag;
        
        show_desktop_flag = !sd->is_showing_desktop;
        
        if (!sd->lock) {
                update_button_state(sd, show_desktop_flag);
                show_desktop(sd, show_desktop_flag);
        	update_tooltip(sd);
        }

	return TRUE;
}

static void update_button_state(ShowDesktop *sd, gboolean setting)
{
         gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (sd->button), setting);
}

static void show_desktop_changed(WnckScreen *screen)
{
        ShowDesktop *sd;

        sd = (ShowDesktop *) g_object_get_data(G_OBJECT(screen), "sd");
        
        g_signal_handlers_block_by_func(G_OBJECT(screen),
                                        G_CALLBACK(show_desktop_changed),
                                        NULL);
       
        sd->lock = TRUE;
        
        sd->is_showing_desktop = wnck_screen_get_showing_desktop(screen);
        update_button_state(sd, sd->is_showing_desktop);
        update_tooltip(sd);
        
        sd->lock = FALSE;
        
        g_signal_handlers_unblock_by_func(G_OBJECT(screen),
                                          G_CALLBACK(show_desktop_changed),
                                          NULL);
}

void sux_init(SuxModule *sm, gchar *args)
{
        GtkWidget *button, *image;
        WnckScreen *screen;
        ShowDesktop *sd;

        sd = g_new0(ShowDesktop, 1);
        
        button = gtk_toggle_button_new();
        gtk_widget_show(button);
        
        image = gtk_image_new_from_file_scaled(IMG_PREFIX "showdesktop.png", 24, 24);
        gtk_widget_show(image);
        gtk_container_add(GTK_CONTAINER(button), image);

        screen = wnck_screen_get_default();
        g_object_set_data(G_OBJECT(screen), "sd", sd);

	sd->tips = gtk_tooltips_new();
        sd->screen = screen;
        sd->is_showing_desktop = wnck_screen_get_showing_desktop(sd->screen);
        sd->button = button;

        update_tooltip(sd);
        
        g_signal_connect(G_OBJECT(button), "toggled",
                         G_CALLBACK(toggle_show_desktop), sd);
        g_signal_connect(G_OBJECT(screen), "showing_desktop_changed",
                         G_CALLBACK(show_desktop_changed), sd);
	
	sm->widget = sd->button;
}

void sux_fini(SuxModule *sm)
{
	gtk_widget_destroy(sm->widget);
}

const gchar *sux_name(void)
{
	return "Show Desktop";
}

GdkPixbuf *sux_icon(void)
{
        return gdk_pixbuf_new_from_file(IMG_PREFIX "showdesktop_icon.png", NULL);
}

void sux_about (void)
{
        GtkAbout *about;
        const gchar *msg[] = {
                ">Written by:",
                        "Leandro Pereira (leandro@linuxmag.com.br)",
                ">Based on:",
                        "Show Desktop Button, by Havoc Pennington",
                        "Copyright (c) 2002 Red Hat, Inc.",
                ">Disclaimer:",
                        "This is free software; you can modify and/or distributed",
                        "under the terms of GNU GPL version 2. See http://www.fsf.org",
                        "for more information.",
                NULL
        };

        about = gtk_about_new("Show Desktop", "0.1",
                        "Lets you hide all windows and show the desktop.",
                        msg, IMG_PREFIX "showdesktop_icon.png");
}

const gchar *sux_description(void)
{
        return "Hide application windows and show the desktop.";
}
