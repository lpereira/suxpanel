/* 
 *  SuxPanel Task List Applet, version 0.2
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

#include <unistd.h>
#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>
#include <stdio.h>
#include <string.h>
#include "suxpanel.h"


#define TASKLIST_ICON_FILE_NAME IMG_PREFIX "sux-tasklist-icon.png"
#define TASKLIST_NAME_LENGTH 50

static WnckScreen *screen;
static GList *winlist;
static gint active_wspace;
static GtkWidget *menu_bar = NULL;
static GtkWidget *root_menu;
static gint mini_only = 0;
static gint all_workspace = 0;

static GtkWidget *task_list_menu;

static void create_menu_bar(void);
static void create_root_menu_and_attach_to_menu_bar(void);
static void build_tasklist_menu(void);
static void set_screen_callback_funcs(void);
static void fetch_updated_window_list(void);
static void any_screen_event_cb(WnckScreen * lscreen,
				WnckWindow * lwindow, gpointer data);
static void any_window_event_cb(WnckWindow * lwindow, gpointer data);
static void any_window_state_event_cb(WnckWindow * lwindow,
				      WnckWindowState changed,
				      WnckWindowState new, gpointer data);
static void task_selection_cb(WnckWindow * lwindow);
static void build_tasklist_menu_items(void);
static void load_configuration(void);
static void write_configuration(void);
static void display_config_window(void);
static gboolean conf_window_delete_cb(GtkWidget * widget,
				      GdkEvent * event, gpointer data);
static void conf_window_destroy_cb(GtkWidget * widget, gpointer data);
static void closebtn_cb(GtkWidget * widget, gpointer data);
static void minicheck_cb(GtkWidget * widget, gpointer data);
static void allcheck_cb(GtkWidget * widget, gpointer data);

static void create_menu_bar(void)
{
    static gchar *menu_rc = "style 'menubar-style'\n"
	"{\n"
	"GtkMenuBar::shadow-type = none\n"
	"GtkMenuBar::internal-padding = 0\n"
	"}\n" "widget '*.sux-menu-bar' style 'menubar-style'";

    if (menu_bar && GTK_IS_MENU_SHELL(menu_bar)) { 
    	gtk_widget_destroy(menu_bar);
    }

    menu_bar = gtk_menu_bar_new();
    gtk_widget_set_name(GTK_WIDGET(menu_bar), "sux-menu-bar");
    gtk_widget_show(menu_bar);

    gtk_rc_parse_string(menu_rc);

    return;
}

static void create_root_menu_and_attach_to_menu_bar(void)
{
    GtkWidget *task_icon;

    task_icon = gtk_image_new_from_file(TASKLIST_ICON_FILE_NAME);
    gtk_widget_show(task_icon);

    root_menu = gtk_image_menu_item_new();
    gtk_widget_show(root_menu);
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(root_menu),
				  task_icon);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), root_menu);

    return;
}

static void build_tasklist_menu(void)
{
    screen = wnck_screen_get_default();
    
    if (screen && !WNCK_IS_SCREEN(screen)) return;
    
    set_screen_callback_funcs();
    load_configuration();
    wnck_screen_force_update(screen);

    create_menu_bar();
    create_root_menu_and_attach_to_menu_bar();

    build_tasklist_menu_items();

    return;
}


static void set_screen_callback_funcs(void)
{
    if (!WNCK_IS_SCREEN(screen)) return;

    g_signal_connect(G_OBJECT(screen), "window_opened",
		     G_CALLBACK(any_screen_event_cb), NULL);
    g_signal_connect(G_OBJECT(screen), "window_closed",
		     G_CALLBACK(any_screen_event_cb), NULL);
    g_signal_connect(G_OBJECT(screen), "application_opened",
		     G_CALLBACK(any_screen_event_cb), NULL);
    g_signal_connect(G_OBJECT(screen), "application_closed",
		     G_CALLBACK(any_screen_event_cb), NULL);
    g_signal_connect(G_OBJECT(screen), "workspace_created",
		     G_CALLBACK(any_screen_event_cb), NULL);
    g_signal_connect(G_OBJECT(screen), "workspace_created",
		     G_CALLBACK(any_screen_event_cb), NULL);
    g_signal_connect(G_OBJECT(screen), "active_workspace_changed",
		     G_CALLBACK(any_screen_event_cb), NULL);

    return;
}



static void fetch_updated_window_list(void)
{
    WnckWorkspace *workspace;

    winlist = wnck_screen_get_windows(screen);
    workspace = wnck_screen_get_active_workspace(screen);

    if (!workspace) return;
    	
    active_wspace = wnck_workspace_get_number(workspace);

    return;
}

static void build_tasklist_menu_items(void)
{
    if (!root_menu) return;

    fetch_updated_window_list();

    task_list_menu = gtk_menu_new();

    while (winlist != NULL) {
	WnckWindow *tmpwin;
	WnckWorkspace *tmpws;
	char *tmpname;
	int tmpnum = -1;
	gint mini = 0, pager = 0;
	GtkWidget *task;
	GtkWidget *mini_icon;

	tmpwin = winlist->data;

	if (tmpwin == NULL) {
	    g_warning("NULL winlist");
	    return;
	}

	tmpws = wnck_window_get_workspace(tmpwin);
	tmpname =
	    g_strndup(wnck_window_get_name(tmpwin), TASKLIST_NAME_LENGTH);
	mini_icon =
	    gtk_image_new_from_pixbuf(wnck_window_get_mini_icon(tmpwin));
	gtk_widget_show(mini_icon);

	if (tmpws)
	    tmpnum = wnck_workspace_get_number(tmpws);

	if (wnck_window_is_minimized(tmpwin))
	    mini = 1;

	if (wnck_window_is_skip_pager(tmpwin))
	    pager = 1;
	
	switch (all_workspace) {

	case 0:

	    switch (mini_only) {

	    case 0:
		if (tmpnum == active_wspace) {
		    task = gtk_image_menu_item_new_with_label(tmpname);
		    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM
						  (task), mini_icon);
		    gtk_widget_show(task);
		    gtk_menu_shell_append(GTK_MENU_SHELL(task_list_menu),
					  task);
		    g_signal_connect_swapped(G_OBJECT(task), "activate",
					     G_CALLBACK(task_selection_cb),
					     (gpointer) tmpwin);
		}
		break;

	    case 1:
		if (tmpnum == active_wspace && mini == 1) {
		    task = gtk_image_menu_item_new_with_label(tmpname);
		    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM
						  (task), mini_icon);
		    gtk_widget_show(task);
		    gtk_menu_shell_append(GTK_MENU_SHELL(task_list_menu),
					  task);
		    g_signal_connect_swapped(G_OBJECT(task), "activate",
					     G_CALLBACK(task_selection_cb),
					     (gpointer) tmpwin);
		}
		break;

	    default:
		break;
	    }
	    break;

	case 1:

	    switch (mini_only) {

	    case 0:
		task = gtk_image_menu_item_new_with_label(tmpname);
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(task),
					      mini_icon);
		gtk_widget_show(task);
		gtk_menu_shell_append(GTK_MENU_SHELL(task_list_menu),
				      task);
		g_signal_connect_swapped(G_OBJECT(task), "activate",
					 G_CALLBACK(task_selection_cb),
					 (gpointer) tmpwin);
		break;

	    case 1:
		if (mini == 1) {
		    task = gtk_image_menu_item_new_with_label(tmpname);
		    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM
						  (task), mini_icon);
		    gtk_widget_show(task);
		    gtk_menu_shell_append(GTK_MENU_SHELL(task_list_menu),
					  task);
		    g_signal_connect_swapped(G_OBJECT(task), "activate",
					     G_CALLBACK(task_selection_cb),
					     (gpointer) tmpwin);
		}
		break;

	    default:
		break;
	    }
	    break;

	default:
	    break;

	}

	g_free(tmpname);

	winlist = winlist->next;
    }

    gtk_menu_item_set_submenu(GTK_MENU_ITEM(root_menu), task_list_menu);

    return;
}



static void any_screen_event_cb(WnckScreen * lscreen,
				WnckWindow * lwindow, gpointer data)
{
    build_tasklist_menu_items();
    
    if (!WNCK_IS_WINDOW(lwindow)) return;

    g_signal_connect(G_OBJECT(lwindow), "state_changed",
		     G_CALLBACK(any_window_state_event_cb), NULL);
    g_signal_connect(G_OBJECT(lwindow), "name_changed",
		     G_CALLBACK(any_window_event_cb), NULL);
    g_signal_connect(G_OBJECT(lwindow), "workspace_changed",
		     G_CALLBACK(any_window_event_cb), NULL);
    g_signal_connect(G_OBJECT(lwindow), "icon_changed",
		     G_CALLBACK(any_window_event_cb), NULL);

    return;
}

static void any_window_event_cb(WnckWindow * lwindow, gpointer data)
{
    if (!lwindow) return;
    
    build_tasklist_menu_items();

    return;
}

static void any_window_state_event_cb(WnckWindow * lwindow,
				      WnckWindowState changed,
				      WnckWindowState new, gpointer data)
{
    if (!lwindow) return;
    
    build_tasklist_menu_items();

    return;
}

static void task_selection_cb(WnckWindow * lwindow)
{
    if (WNCK_IS_WINDOW(lwindow))
	wnck_window_activate(lwindow, 0);
}

static void load_configuration(void)
{
    gchar *buf;
    char tmpline[100];
    char tmpvar[100];
    int tmpstate;
    FILE *conf;

    buf = g_strdup_printf("%s/.suxpanel/tasklist.ini", g_get_home_dir());
    conf = fopen(buf, "r");
    g_free(buf);

    if (conf == NULL)
	return;

    while (!feof(conf)) {
	fgets(tmpline, 100, conf);

	if (strstr(tmpline, "MINIMIZED_ONLY")) {
	    sscanf(tmpline, "%s %d", tmpvar, &tmpstate);
	    if (tmpstate > 1)
		tmpstate = 1;
	    if (tmpstate < 0)
		tmpstate = 0;
	    mini_only = tmpstate;
	}

	if (strstr(tmpline, "ALL_WORKSPACES")) {
	    sscanf(tmpline, "%s %d", tmpvar, &tmpstate);
	    if (tmpstate > 1)
		tmpstate = 1;
	    if (tmpstate < 0)
		tmpstate = 0;
	    all_workspace = tmpstate;
	}
    }

    fclose(conf);

    return;
}

static void write_configuration(void)
{
    gchar *buf;
    FILE *conf;

    buf = g_strdup_printf("%s/.suxpanel/tasklist.ini", g_get_home_dir());
    conf = fopen(buf, "w");
    g_free(buf);

    if (conf == NULL)
	return;

    fprintf(conf, "MINIMIZED_ONLY %d\n", mini_only);
    fprintf(conf, "ALL_WORKSPACES %d\n", all_workspace);

    fclose(conf);

    return;
}

static void display_config_window(void)
{
    GtkWidget *confwin;
    GtkWidget *vbox;
    GtkWidget *closebtn;
    GtkWidget *minicheck, *allcheck;


    confwin = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(confwin),
			 "sux-Tasklist Configuration");
    gtk_widget_show(confwin);
    /* Sets the border width of the window. */
    gtk_container_set_border_width(GTK_CONTAINER(confwin), 10);

    g_signal_connect(G_OBJECT(confwin), "destroy",
		     G_CALLBACK(conf_window_destroy_cb), NULL);

    g_signal_connect(G_OBJECT(confwin), "delete_event",
		     G_CALLBACK(conf_window_delete_cb), NULL);

    vbox = gtk_vbox_new(FALSE, 4);
    gtk_widget_show(vbox);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
    gtk_container_add(GTK_CONTAINER(confwin), vbox);

    minicheck =
	gtk_check_button_new_with_label("Display Minimized Windows Only");
    gtk_widget_show(minicheck);
    gtk_container_add(GTK_CONTAINER(vbox), minicheck);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(minicheck), mini_only);
    g_signal_connect(G_OBJECT(minicheck), "toggled",
		     G_CALLBACK(minicheck_cb), NULL);

    allcheck =
	gtk_check_button_new_with_label("Show All Workspace Windows");
    gtk_widget_show(allcheck);
    gtk_container_add(GTK_CONTAINER(vbox), allcheck);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(allcheck),
				 all_workspace);
    g_signal_connect(G_OBJECT(allcheck), "toggled",
		     G_CALLBACK(allcheck_cb), NULL);

    closebtn = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
    gtk_widget_show(closebtn);
    gtk_container_add(GTK_CONTAINER(vbox), closebtn);
    GTK_WIDGET_SET_FLAGS(closebtn, GTK_CAN_DEFAULT);
    g_signal_connect(G_OBJECT(closebtn), "clicked",
		     G_CALLBACK(closebtn_cb), (gpointer) confwin);

    return;
}

static void minicheck_cb(GtkWidget * widget, gpointer data)
{
    mini_only = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
}

static void allcheck_cb(GtkWidget * widget, gpointer data)
{
    all_workspace = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
}

static gboolean conf_window_delete_cb(GtkWidget * widget,
				      GdkEvent * event, gpointer data)
{
    return FALSE;
}


static void conf_window_destroy_cb(GtkWidget * widget, gpointer data)
{
    write_configuration();
    if (data && GTK_IS_WIDGET(data)) 
    	gtk_widget_destroy(data);
}

static void closebtn_cb(GtkWidget * widget, gpointer data)
{
    if (data && GTK_IS_WIDGET(data)) 
	gtk_widget_destroy(data);
}

const gchar *sux_name(void)
{
    return "Tasklist Viewer";
}


void sux_init(SuxModule * sm)
{
    build_tasklist_menu();

    sm->widget = menu_bar;
}


void sux_fini(SuxModule * sm)
{
    gtk_widget_destroy(sm->widget);
}


void sux_prefs(SuxModule * sm)
{
    display_config_window();
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

    about = gtk_about_new("Tasklist Viewer", "0.2",
			  "Tasklist Viewer Applet",
			  msg, IMG_PREFIX "sux-tasklist-logo.png");
}

GdkPixbuf *sux_icon(void)
{
        return gdk_pixbuf_new_from_file(IMG_PREFIX "sux-tasklist-logo.png", NULL);
}
