/* 
 *  SuxPanel Workspace Switcher, version 0.2
 *  Copyright (c) Dinesh Nadarajah, 2003
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "suxpanel.h"
#include <unistd.h>
#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>

static WnckScreen *screen = NULL;
static GtkWidget  *pager  = NULL;

static void load_pager_applet(void);
static void set_pager_width_and_height(GtkWidget * pager);
static void set_number_of_workspaces(WnckScreen * screen);
static void create_settings_window(SuxModule *sm);
static void num_workspace_changed(GtkWidget * widget, GtkSpinButton * spin);
#if 0
static void set_workspace_count(gint num);
#endif
static void close_btn_cb(GtkWidget * widget, gpointer data);

/* workspace height, width, num_workspaces */
static gint workspace_height = 16;	/* This value is fixed */
static gint workspace_width_per_space = 24;
static gint num_workspaces;

static void num_workspace_changed(GtkWidget * widget, GtkSpinButton * spin)
{
    num_workspaces = gtk_spin_button_get_value_as_int(spin);
    set_number_of_workspaces(screen);
    set_pager_width_and_height(pager);
}

static void close_btn_cb(GtkWidget * widget, gpointer data)
{
    SuxModule *module;
    
    module = (SuxModule*) g_object_get_data(G_OBJECT(data), "module");
    module->prefs_open = FALSE;
    
    gtk_widget_destroy(GTK_WIDGET(data));
}

static void create_settings_window(SuxModule *module)
{
    GtkWidget *win;
    GtkWidget *label;
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *num;
    GtkWidget *close_btn;

    win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(win), _("WorkSpace Preferences"));

    g_object_set_data(G_OBJECT(win), "module", module);

    vbox = gtk_vbox_new(FALSE, 4);
    gtk_widget_show(vbox);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);

    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    label = gtk_label_new(_("Number of Work Spaces: "));
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);

    num = gtk_spin_button_new_with_range(1, 15, 1);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(num), 0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(num), num_workspaces);
    gtk_widget_show(num);
    gtk_box_pack_start(GTK_BOX(hbox), num, TRUE, TRUE, 0);
    g_signal_connect(G_OBJECT(num), "value_changed",
		     G_CALLBACK(num_workspace_changed), (gpointer) num);

    close_btn = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
    gtk_widget_show(close_btn);
    /* gtk_box_pack_start(GTK_BOX(vbox), close_btn, FALSE, FALSE, 0); */
    gtk_container_add(GTK_CONTAINER(vbox), close_btn);
    GTK_WIDGET_SET_FLAGS(close_btn, GTK_CAN_DEFAULT);
    g_signal_connect(G_OBJECT(close_btn), "clicked",
		     G_CALLBACK(close_btn_cb), (gpointer) win);

    gtk_container_add(GTK_CONTAINER(win), vbox);

    gtk_widget_show(win);

}

static void load_pager_applet(void)
{
    if (pager || screen) 
    	return;
    	
    screen = wnck_screen_get_default();
    set_number_of_workspaces(screen);

    /* because the pager doesn't respond to signals at the moment */
    wnck_screen_force_update(screen);

    pager = wnck_pager_new(screen);

    wnck_pager_set_orientation(WNCK_PAGER(pager),
			       GTK_ORIENTATION_HORIZONTAL);
    wnck_pager_set_n_rows(WNCK_PAGER(pager), 1);
    wnck_pager_set_shadow_type(WNCK_PAGER(pager), GTK_SHADOW_IN);

    set_pager_width_and_height(pager);

    return;
}

static void set_pager_width_and_height(GtkWidget * pager)
{
    gtk_widget_set_size_request(pager,
				workspace_width_per_space * num_workspaces,
				workspace_height);
}

static void set_number_of_workspaces(WnckScreen * screen)
{
    wnck_screen_change_workspace_count(screen, num_workspaces);
}

#if 0
static void set_workspace_count(gint num)
{
    num_workspaces = num;
}
#endif

const gchar *sux_name(void)
{
    return "WorkSpace Switcher";
}

void sux_init(SuxModule * sm)
{

    num_workspaces = 4;

    load_pager_applet();

    sm->widget = pager;

}

void sux_fini(SuxModule * sm)
{
    gtk_widget_destroy(sm->widget);
}

void sux_prefs(SuxModule * sm)
{
    create_settings_window(sm);
}

void sux_about(void)
{
    GtkAbout *about;
    const gchar *msg[] = {
	">Written by:",
	"Dinesh Nadarajah (dinesh_list@sbcglobal.net)",
	">Disclaimer:",
	"This software is distributed under the BSD license.",
	NULL
    };

    about = gtk_about_new("WorkSpace Switcher", "0.2",
			  "Work Space Switcher Applet",
			  msg, IMG_PREFIX "sux-ws.png");
}

GdkPixbuf *sux_icon(void)
{
        return gdk_pixbuf_new_from_file(IMG_PREFIX "sux-ws.png", NULL);
}
