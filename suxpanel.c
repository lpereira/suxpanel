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

#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gmodule.h>

#include "about.h"
#include "suxpanel.h"
#include "settings.h"
#include "gtkmisc.h"

#define PANEL_HEIGHT 30

GList *plugin_list = NULL;

GtkAllocation sux_module_get_allocation(SuxModule *sm)
{
	return GTK_WIDGET(sm->widget)->allocation;
}

/**
 * description: Loads a module, and add it to the plugin list GList.
 * parameters:  - sp (pointer to the SuxPanel instance)
 *              - filename (the plugin to load, with full path)
 * returns:     a SuxModule instance
 */
SuxModule *sux_module_load_module(SuxPanel *sp, gchar *filename)
{
	SuxModule *sm;
	GModule *dll;
	const gchar *(*sux_text_func) (void);
	GdkPixbuf *(*sux_icon_func) (void);
	gpointer sux_init, sux_fini, sux_about, sux_prefs, sux_name,
		 sux_icon, sux_description;

	dll = g_module_open(filename, G_MODULE_BIND_MASK);
	if (!dll) {
		g_warning(g_module_error());
		g_return_val_if_reached(NULL);
	}

	sm = g_new0(SuxModule, 1);

	/*
	 * We need some symbols in the plugin file:
	 * - sux_init,
	 * - sux_fini,
	 * - sux_name
	 * They are needed to configure and finalize a plugin,
	 * and also display the plugin's name to the user.
	 */
	if (!g_module_symbol(dll, "sux_init", &sux_init) ||
	    !g_module_symbol(dll, "sux_fini", &sux_fini) ||
	    !g_module_symbol(dll, "sux_name", &sux_name)) {
		g_warning("Error loading module: %s\n", filename);
		g_return_val_if_reached(NULL);
	}

	sm->module_init = sux_init;
	sm->module_fini = sux_fini;

	/*
	 * Plugins can also have  "about" and "preferences" dialogs
	 */
	if (g_module_symbol(dll, "sux_about", &sux_about))
		sm->about = sux_about;
	if (g_module_symbol(dll, "sux_prefs", &sux_prefs))
		sm->prefs = sux_prefs;

	/*
	 * Can also have a description and an icon...
	 */
	if (g_module_symbol(dll, "sux_icon", &sux_icon)) {
		sux_icon_func = sux_icon;
		sm->icon = sux_icon_func();
	} else {
		sm->icon = gdk_pixbuf_new_from_file(IMG_PREFIX "apps.png", NULL);
	}

	if (g_module_symbol(dll, "sux_description", &sux_description)) {
		sux_text_func = sux_description;
		sm->description = sux_text_func();
	} else {
		sm->description = NULL;
	}
	
	sux_text_func = sux_name;
	sm->name = sux_text_func();
		
	sm->dll = dll;
	sm->sp  = sp;

	plugin_list = g_list_append(plugin_list, sm);

	return sm;
}

/**
 * description: initializes and packs a plugin in the panel
 * parameters:  - sp (pointer to a SuxPanel instance)
 *              - sm (pointer to a SuxModule instance)
 *              - align (the alignment, currently left or right only)
 *              - args (the argument list)
 * returns:     void
 */
void sux_panel_pack_module(SuxPanel *sp, SuxModule *sm,
			     SuxAlign align, gchar *args)
{
	g_return_if_fail(sp != NULL);
	g_return_if_fail(sm != NULL);
	
	gtkm_widget_set_cursor(sp->window, GDK_WATCH);
	sm->module_init(sm, args);
	g_return_if_fail(sm->widget != NULL);
		
	switch (align) {
		default:	/* fallthrough */
			g_warning("SuxAlign %d not supported, left-packing", align);
		case ALIGN_LEFT:
			gtk_box_pack_start(GTK_BOX(sp->box), sm->widget,
				FALSE, FALSE, 0);
			break;
		case ALIGN_RIGHT:
			gtk_box_pack_end(GTK_BOX(sp->box), sm->widget,
				FALSE, FALSE, 0);
			break;
	}
	
	gtk_widget_show(sm->widget);
	
	gtkm_widget_set_cursor(sp->window, GDK_LEFT_PTR);
}

static GtkWidget *round_new(gchar *file)
{
	GtkWidget *pix, *box;
	
	box = gtk_vbox_new(0, FALSE);
	gtk_widget_show(box);
	
	pix = gtk_image_new_from_file(file);
	gtk_widget_show(pix);
	gtk_box_pack_start(GTK_BOX(box), pix, FALSE, FALSE, 0);
	
	return box;
}

static void ev_popup_settings(GtkWidget *wid, gpointer data)
{
	settings_create();
}

static void ev_popup_about(GtkWidget *wid, gpointer data)
{
	const gchar *msg[] = {
		">Written by:",
			"Leandro Pereira (leandro@linuxmag.com.br)",
		">Disclaimer:",
			"This is free software; you can modify and/or distribute it",
			"under the terms of GNU GPL version 2. See http://www.fsf.org/",
			"for more information.",
                NULL
	};
	
	gtk_about_new("SuxPanel", VERSION,
	   	      "",
		      msg, IMG_PREFIX "icon.png");
}

static GtkItemFactoryEntry panel_popup[]= {
	{ "/Settings...",		NULL,	ev_popup_settings,	0,	"<StockItem>", GTK_STOCK_PREFERENCES },
	{ "/About SuxPanel...",	NULL,	ev_popup_about,		0,	"<Item>"}
};

static gboolean ev_popup (GtkWidget *wid, GdkEventButton *ev, SuxPanel *sp)
{
	static GtkItemFactory *factory = NULL;

	if (ev->button != 3)	/* only allow right-mouse button */
		return FALSE;
	
	if (!factory) {
		factory = gtk_item_factory_new(GTK_TYPE_MENU, "<main>", NULL);
		gtk_item_factory_create_items(factory, G_N_ELEMENTS(panel_popup),
				panel_popup, sp);
	}
	
	gtk_item_factory_popup_with_data(factory, sp, NULL, ev->x, ev->y,
			ev->button, gtk_get_current_event_time());
	
	return FALSE;
}

SuxPanel *sux_panel_create()
{
	SuxPanel *sp;
	GtkWidget *window, *hbox, *pix, *box, *ev;
	
	sp = g_new0(SuxPanel, 1);
	
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_type_hint(GTK_WINDOW(window), GDK_WINDOW_TYPE_HINT_DOCK);
	gtk_window_stick(GTK_WINDOW(window));
	
	gtk_window_set_default_size(GTK_WINDOW(window), gdk_screen_width(), PANEL_HEIGHT);
	gtk_widget_set_size_request(window, gdk_screen_width(), PANEL_HEIGHT);
	gtk_container_set_border_width(GTK_CONTAINER(window), 0);
	
	ev = gtk_event_box_new();
	gtk_widget_show(ev);
	gtk_container_add(GTK_CONTAINER(window), ev);
	
	hbox = gtk_hbox_new(FALSE, 1);
	gtk_widget_show(hbox);
	gtk_container_add(GTK_CONTAINER(ev), hbox);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 0);
	
	pix = round_new(IMG_PREFIX "side-left.xpm");
	gtk_widget_show(pix);
	gtk_box_pack_start(GTK_BOX(hbox), pix, FALSE, FALSE, 0);

	pix = round_new(IMG_PREFIX "side-right.xpm");
	gtk_widget_show(pix);
	gtk_box_pack_end(GTK_BOX(hbox), pix, FALSE, FALSE, 0);
	
	box = gtk_hbox_new(FALSE, 2);
	gtk_widget_show(box);
	gtk_container_set_border_width(GTK_CONTAINER(box), 0);
	gtk_box_pack_start(GTK_BOX(hbox), box, TRUE, TRUE, 0);
	
	g_signal_connect(G_OBJECT(ev), "button-press-event",
		(GCallback) ev_popup, sp);

	gtk_widget_show(window);
	gtk_window_set_icon_from_file(GTK_WINDOW(window), IMG_PREFIX "icon.png", NULL);
	gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
	gtk_window_set_resizable(GTK_WINDOW(window), FALSE);

	gtk_window_set_skip_pager_hint(GTK_WINDOW(window), TRUE);
	gtk_window_set_skip_taskbar_hint(GTK_WINDOW(window), TRUE);

	gtk_window_move(GTK_WINDOW(window), 0, 0);
	gtk_widget_set_uposition(GTK_WIDGET(window), 0, 0);

	gtk_window_stick(GTK_WINDOW(window));
	
	
	{
	       long wm_strut[] =  {   0,	/* Left		*/
	       			      0,	/* Right	*/
       				      29,	/* Top		*/
       				      0, 	/* Bottom	*/
				   };
	       GdkAtom strut, card;
       
	       strut = gdk_atom_intern("_NET_WM_STRUT", FALSE);
	       card  = gdk_atom_intern("CARDINAL",      FALSE);

	       gdk_property_change(GDK_WINDOW(window->window), strut, card,
        	                   32, GDK_PROP_MODE_REPLACE,
                	           (guchar *) (&wm_strut), 4);
	}

	sp->window = window;
	sp->box    = box;
	
	return sp;
}

gboolean sux_panel_load_modules(SuxPanel *sp)
{
	gchar *buf;
	gboolean ret = FALSE;
	
	if (!sp)
		return FALSE;
	
	buf = g_strdup_printf("%s/.suxpanel/modules.ini", g_get_home_dir());
	
	if (g_file_test(buf, G_FILE_TEST_EXISTS)) {
		/*
		 * modules.ini format:
		 *
		 * l=/path/to/plugin.so		<pack plugin to left>
		 * r=/path/to/plugin.so		<pack plugin to right>
		 *
		 * lines starting with ';' are comments.
		 */
		 FILE *modules;
		 gchar buffer[128];
		 SuxModule *sm;
		 SuxAlign align;
		 gboolean load_module = FALSE;
		 
		 modules = fopen(buf, "r");
		 if (!modules) {
		 	goto end;
		 	ret = FALSE;
		 }
		 
		 while (fgets(buffer, 128, modules)) {
			gchar *tmp = buffer;
		 	
		 	if (*tmp == ';') continue;
		 	
		 	switch (*tmp) {
		 		case 'l':
		 		case 'L':
		 			align = ALIGN_LEFT;
		 			load_module = TRUE;
		 			break;
		 		case 'r':
		 		case 'R':
					align = ALIGN_RIGHT;
					load_module = TRUE;
		 			break;		 
		 		default:
		 			load_module = FALSE;	
		 	}

			if (load_module) {
				gpointer start;
				gchar *args = NULL;
			
	 			tmp+=2;
	 			
	 			tmp = g_strstrip(tmp);
	 			
	 			/* Extract arguments from line; a bit messy,
				   I know :) */
	 			start = tmp;
	 			while ((*tmp != 0) && (*tmp != '(')) tmp++;
	 			if (*tmp == '(' && *(tmp + strlen(tmp) - 1) == ')') {
					*(tmp + strlen(tmp) - 1) = 0;

					tmp++;
					args = g_strdup(tmp);
					
					*(tmp-2) = 0;
				}
				tmp = start;
				
 				sm = sux_module_load_module(sp, tmp);
 				sux_panel_pack_module(sp, sm, align, args);
 				
 				load_module = FALSE;
 				
 				g_free(args);
			}
		 }
		 
		 fclose(modules);
		 
		 ret = TRUE;
	} else {
		g_warning("Can't find ``%s''. Run suxpanel-install.sh\n", buf);
		ret = FALSE;
	}
	
    end:
	g_free(buf);

	return ret;
}

int main (int argc, char **argv)
{
	SuxPanel *sp;
		
	gtk_init(&argc, &argv);
	
	if (!g_module_supported()) {
		g_warning("GModule not supported; aborting");
		return 1;
	}
	
	sp = sux_panel_create();
	
	if (!sux_panel_load_modules(sp))
		return 1;		
	 
	gtk_main();
	g_free(sp);
	
	return 0;
}
