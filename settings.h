#ifndef __SETINGS_H__
#define __SETINGS_H__

#include <gtk/gtk.h>

typedef struct _Settings Settings;

struct _Settings {
	GtkWidget *window;

	GtkWidget *about_btn;
	GtkWidget *prefs_btn;

	GtkTreeStore *plugins_model;
	GtkTreeView *plugins_view;
};

Settings *settings_create(void);
void settings_append_plugin(Settings *settings, const gchar *name,
			    GdkPixbuf *icon, const gchar *description);

#endif				/* __SETTINGS_H__ */
