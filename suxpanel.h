/*
 * SuxPanel version 0.4
 * Copyright (c) 2003-2005 Leandro Pereira <leandro@linuxmag.com.br>
 *
 * This program may be distributed under the terms of GNU General
 * Public License version 2. You should have received a copy of the
 * license with this program; if not, please consult http://www.fsf.org/.
 *
 * This program comes with no warranty. Use at your own risk.
 *
 */

#ifndef __SUXPANEL_H__
#define __SUXPANEL_H__

#include <gtk/gtk.h>
#include <gmodule.h>

#include "config.h"
#include "gtkmisc.h"
#include "about.h"

typedef struct _SuxPanel	SuxPanel;
typedef struct _SuxModule	SuxModule;
typedef enum   _SuxAlign	SuxAlign;

enum _SuxAlign {
	ALIGN_LEFT,
	ALIGN_RIGHT,
	ALIGN_FREE
};

struct _SuxPanel {
	GtkWidget *window;
	
	GtkWidget *box;
};

struct _SuxModule {
	SuxPanel    *sp;
	gint	     pos;
	gboolean     prefs_open;
	
	GtkWidget   *widget;
	GModule     *dll;
	const gchar *name;
	const gchar *description;
	GdkPixbuf   *icon;

	void	(*module_init)	(SuxModule *sm, gchar *args);
	void	(*module_fini)	(SuxModule *sm);
	
	void	(*about)	(void);
	void	(*prefs)	(SuxModule *sm);
};

extern GList *plugin_list;

/*
 * Ugly hack... :/
 */ 
extern gboolean prefs_open;

extern GtkAllocation sux_module_get_allocation(SuxModule *sm);

#endif	/* __SUXPANEL_H__ */
