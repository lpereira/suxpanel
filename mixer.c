/*
 * SuxPanel Mixer Applet version 0.3
 * Copyright (c) 2001-2004 Leandro Pereira <leandro@linuxmag.com.br>
 *
 * This program may be distributed under the terms of GNU General
 * Public License version 2. You should have received a copy of the
 * license with this program; if not, please consult http://www.fsf.org/.
 *
 * This program comes with no warranty. Use at your own risk.
 *
 */

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>

#include "suxpanel.h"

static SuxModule *vol_sm = NULL;
static int mixer_fd = 0;

static int vol_set_vol (GtkAdjustment * adj, gpointer data);
static void popup_volume_window(GtkWidget *widget);

/*
 *  Call an ioctl to change the master OSS volume.
 *  Stolen from GnomeRadio :)
 */
static int vol_set_vol (GtkAdjustment * adj, gpointer data)
{
        int i_vol;

        if (mixer_fd < 0)
                return -1;
                
        i_vol  = (gint) adj->value;
        i_vol += (gint) adj->value << 8;

        if ((ioctl (mixer_fd, SOUND_MIXER_WRITE_VOLUME, &i_vol)) < 0)
                return 0;

  	return TRUE;
}

const gchar *sux_name (void)
{
	return "Volume Control";
}

void sux_init (SuxModule *sm)
{
	mixer_fd = open("/dev/mixer", O_RDWR);

	if (mixer_fd > 0) {
		GtkWidget *button, *img;

		button = gtk_toggle_button_new();
		gtk_widget_show (button);
                
		img = gtk_image_new_from_file_scaled(IMG_PREFIX "mixer_applet.png", 24, 24);
                gtk_widget_show (img);

                gtk_container_add (GTK_CONTAINER (button), img);
                gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);

                g_signal_connect (G_OBJECT (button), "toggled",
                                           (GCallback) popup_volume_window, NULL);
                                         
		sm->widget = button;
		vol_sm = sm;
	} else {
		GtkWidget *dialog;
                
                dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
                        GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
                        "Can't open the mixer device.");
                gtk_dialog_run(GTK_DIALOG(dialog));
                gtk_widget_destroy(dialog);                
	}
}

void sux_fini (SuxModule *sm)
{
	gtk_widget_destroy(sm->widget);
}

static void popup_volume_window(GtkWidget *widget)
{
        GtkWidget *frame, *vscale;
        static GtkWidget *vol_wnd;
        GtkAdjustment *adj;
        int vol, x_pos, y_pos;
        GtkAllocation alloc;

	if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) {
		if (vol_wnd)
			gtk_widget_destroy(vol_wnd);
		return;
	}

        if (mixer_fd < 0)
                return;

	alloc = sux_module_get_allocation(vol_sm);
	x_pos = alloc.x;
	y_pos = alloc.y + alloc.height;

        ioctl (mixer_fd, SOUND_MIXER_READ_VOLUME, &vol);

        vol_wnd = gtk_window_new (GTK_WINDOW_POPUP);
                 
        frame = gtk_frame_new (NULL);
        gtk_widget_show (frame);
        gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
        gtk_container_add (GTK_CONTAINER (vol_wnd), frame);

        adj = (GtkAdjustment*)gtk_adjustment_new (100, 0, 101, 1, 10, 1);
        gtk_adjustment_set_value (adj, vol >> 8);

        vscale = gtk_vscale_new (adj);
	gtk_range_set_inverted(GTK_RANGE(vscale), TRUE);
        gtk_scale_set_digits (GTK_SCALE (vscale), 0);
        gtk_scale_set_draw_value (GTK_SCALE (vscale), FALSE);
        gtk_widget_set_usize(GTK_WIDGET(vscale), 0, 100);
        gtk_widget_show (vscale);
	gtk_container_add(GTK_CONTAINER(frame), vscale);

	gtk_widget_show(vol_wnd);
	gtk_widget_set_uposition (GTK_WIDGET(vol_wnd), x_pos, y_pos);

        g_signal_connect (G_OBJECT (adj), "value_changed",
                            (GCallback)vol_set_vol, adj);
}

void sux_about (void)
{
        GtkAbout *about;
        const gchar *msg[] = {
                ">Written by:",
                        "Leandro Pereira (leandro@linuxmag.com.br)",
                ">Disclaimer:",
                        "This is free software; you can modify and/or distributed",
                        "under the terms of GNU GPL version 2. See http://www.fsf.org",
                        "for more information.",
                NULL
        };

        about = gtk_about_new("Volume Control", "0.3",
                        "Simple OSS mixer applet",
                        msg, IMG_PREFIX "mixer.png");
}

GdkPixbuf *sux_icon(void)
{
        return gdk_pixbuf_new_from_file(IMG_PREFIX "mixer.png", NULL);
}

const gchar *sux_description(void)
{
        return "Controls the OSS Master volume";
}
