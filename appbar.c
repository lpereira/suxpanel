/*
 * SuxPanel version 0.4
 * Copyright (c) 2003-2004 Leandro Pereira <leandro@linuxmag.com.br>
 *
 * This program may be distributed under the terms of GNU General
 * Public License version 2. You should have received a copy of the
 * license with this program; if not, please consult http://www.fsf.org/.
 *
 * This program comes with no warranty. Use at your own risk.
 *
 */

#include "suxpanel.h"
#include <sys/stat.h>
#include <dirent.h>

#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

static GtkWidget *box;
static GtkTooltips *appbar_tips;

static GtkWidget *btn_from_appdir(const gchar * path,
				  const gchar * dir_name);

static void build_appbar(gchar * dir_name)
{
    DIR *dir;
    GtkWidget *btn;
    struct dirent *sd;

    dir = opendir(dir_name);
    if (!dir)
	return;

    if (!box) {
	box = gtk_hbox_new(FALSE, 2);
	gtk_widget_show(box);
    }

    while ((sd = readdir(dir))) {
	gchar *buf;

	if (sd->d_name[0] == '.')
	    continue;

	buf = g_strdup_printf("%s/%s/.DirIcon", dir_name, sd->d_name);
	if (g_file_test(buf, G_FILE_TEST_EXISTS)) {
	    btn = btn_from_appdir(dir_name, sd->d_name);
	    gtk_tooltips_set_tip(appbar_tips, btn, sd->d_name, NULL);
	    gtk_box_pack_start(GTK_BOX(box), btn, FALSE, FALSE, 0);
	}
	g_free(buf);
    }

    closedir(dir);
}

static gboolean spawn_app(GtkWidget * widget, GdkEventButton * event,
			  gpointer data)
{
    gchar *app, *path;

    if (event->button != 1)
	return FALSE;

    path = g_object_get_data(G_OBJECT(widget), "path");
    app = g_strdup_printf("\"%s/%s/AppRun\"", path, (gchar *) data);

    if (!g_spawn_command_line_async(app, NULL)) {
	GtkWidget *dialog;

	dialog = gtk_message_dialog_new(NULL, 0, GTK_MESSAGE_ERROR,
					GTK_BUTTONS_CLOSE,
					"Cannot fork. Yay.");
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
    }

    g_free(app);

    return FALSE;
}

static gboolean enter_notify(GtkWidget * wid, gpointer data)
{
    gtk_widget_modify_bg(GTK_WIDGET(wid), GTK_STATE_NORMAL,
			 &(wid->style->bg[GTK_STATE_SELECTED]));

    return FALSE;
}

static gboolean leave_notify(GtkWidget * wid, gpointer data)
{
    gtk_widget_modify_bg(GTK_WIDGET(wid), GTK_STATE_NORMAL,
			 &(wid->parent->style->bg[GTK_STATE_NORMAL]));

    return FALSE;
}

static GtkWidget *btn_from_appdir(const gchar * path,
				  const gchar * dir_name)
{
    GtkWidget *btn;
    GtkWidget *pix;
    gchar *buf;

    btn = gtk_event_box_new();
    gtk_widget_show(btn);

    g_object_set_data(G_OBJECT(btn), "path", g_strdup(path));

    buf = g_strdup_printf("%s/%s/.DirIcon", path, dir_name);
    if (g_file_test(buf, G_FILE_TEST_EXISTS)) {
	pix = gtk_image_new_from_file_scaled(buf, 24, 24);
	gtk_widget_show(pix);
	gtk_container_add(GTK_CONTAINER(btn), pix);
    }
    g_free(buf);

    g_signal_connect(G_OBJECT(btn), "button_press_event",
		     (GCallback) spawn_app, g_strdup(dir_name));
    g_signal_connect(G_OBJECT(btn), "enter_notify_event",
		     (GCallback) enter_notify, NULL);
    g_signal_connect(G_OBJECT(btn), "leave_notify_event",
		     (GCallback) leave_notify, NULL);

    gtk_widget_set_size_request(GTK_WIDGET(btn), 28, 28);
    return btn;
}

const gchar *sux_name(void)
{
    return "Application Bar";
}

void sux_init(SuxModule * sm)
{
    gchar *buf;

    buf = g_strdup_printf("%s/.suxpanel/appbar", g_get_home_dir());
    if (g_file_test(buf, G_FILE_TEST_EXISTS)) {
	appbar_tips = gtk_tooltips_new();

	build_appbar(buf);

	sm->widget = box;
    } else {
	g_print("*** Did you ran suxpanel-install.sh?\n");
    }

    g_free(buf);
}

void sux_fini(SuxModule * sm)
{
    gtk_widget_destroy(sm->widget);
}

void sux_about(void)
{
    GtkAbout *about;
    const gchar *msg[] = {
	">Written by:",
	"Leandro Pereira (leandro@linuxmag.com.br)",
	">Disclaimer:",
	"This is free software; you can modify and/or distribute it",
	"under the terms of GNU GPL version 2. See http://www.fsf.org/",
	"for more information.",
	NULL
    };

    about = gtk_about_new("Application Bar", "0.2.3",
			  "Yay to the bar, baby!",
			  msg, IMG_PREFIX "apps.png");
}
