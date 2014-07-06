/*
 * SuxPanel ROX-Panel Applet Compatibility Layer
 * Copyright (c) 2003 Leandro Pereira <leandro@linuxmag.com.br>
 */

#include <string.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "suxpanel.h"

static void spawn_app(gchar *apppath, GdkNativeWindow gid)
{
        gchar *app;
        
        app = g_strdup_printf("\"%s\" %lu", apppath, (gulong) gid);

	if (!g_spawn_command_line_async(app, NULL)) {
	    GtkWidget *dialog;
	    
	    dialog = gtk_message_dialog_new(NULL, 0, GTK_MESSAGE_ERROR,
	                                    GTK_BUTTONS_CLOSE,
	                                    "Cannot fork. Yay.");
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);                                    
	}
} 

static void applet_realize(GtkWidget *widget, gpointer data)
{
	GdkNativeWindow gid;
	gchar *args = (gchar*) data;
	static gboolean realized = FALSE;
	
	if (realized)
	    return;

	gid = (GdkNativeWindow)gtk_socket_get_id(GTK_SOCKET(widget));
	spawn_app(args, gid);
	
	realized = TRUE;
}


void sux_init(SuxModule * sm, gchar *args)
{
	GtkWidget *socket;
	
	if (!args) {
	    sm->widget = NULL;
	    return;
	}

	socket = gtk_socket_new();
	gtk_widget_show(socket);

	g_signal_connect(G_OBJECT(socket), "realize",
			 G_CALLBACK(applet_realize), args);			 

	sm->widget = socket;
}

void sux_fini(SuxModule * sm)
{
    gtk_widget_destroy(sm->widget);
}

const gchar *sux_name(void)
{
    return "ROX Applet Swallower";
}

void
sux_about(void)
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

        about = gtk_about_new("ROX Applet Swallower", "0.1",
                              "Swallows ROX panel applets in SuxPanel",
                              msg, IMG_PREFIX "apps.png");
}
