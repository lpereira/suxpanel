#include "suxpanel.h"
#include "config.h"
#include "settings.h"

#include <string.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

static SuxModule *selected = NULL;
gboolean prefs_open = FALSE;

static void
settings_about(GtkWidget * wid, gpointer data)
{
	if (selected->about)
		selected->about();
}

static void
settings_prefs(GtkWidget * wid, gpointer data)
{
	/*
	 * This "prefs_open" stuff sucks... but I was unable to fix
	 * these problems with a better approach... :/
	 */
	if (selected->prefs) {
		prefs_open = selected->prefs_open = TRUE;
		selected->prefs(selected);
		
		while (selected->prefs_open) {
			g_usleep(1);
			while(gtk_events_pending())
				gtk_main_iteration();
		}
		prefs_open = FALSE;
	}
}

static void
settings_cancel(GtkWidget * wid, gpointer data)
{
	Settings *set = (Settings *) data;

	if (set->window) {
		gtk_widget_destroy(set->window);
		g_free(set);
	}
		
}

enum {
	PIXBUF_COLUMN,
	NAME_COLUMN,
	NUM_COLUMNS
};

static void
create_view_columns(GtkTreeView * tv, GtkTreeModel * tm)
{
	GtkCellRenderer *cr_text, *cr_pbuf;
	GtkTreeViewColumn *column;
	
	column = gtk_tree_view_column_new();
	gtk_tree_view_append_column(GTK_TREE_VIEW(tv), column);
	
	cr_pbuf = gtk_cell_renderer_pixbuf_new();
	cr_text = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, cr_pbuf, FALSE);
	gtk_tree_view_column_pack_start(column, cr_text, TRUE);
	
	gtk_tree_view_column_add_attribute(column, cr_pbuf, "pixbuf", PIXBUF_COLUMN);
	gtk_tree_view_column_add_attribute(column, cr_text, "markup", NAME_COLUMN);
	
}

static void
plugin_selected(GtkTreeSelection * ts, gpointer data)
{
	Settings *settings = (Settings *) data;
	GList *plug;
	GtkTreeIter iter;
	GtkTreeModel *model;
	SuxModule *sm = NULL;
	gchar *yuck;

	model = GTK_TREE_MODEL(settings->plugins_model);

	if (!gtk_tree_selection_get_selected(ts, &model, &iter))
		return;

	yuck = (gchar *) malloc(256);
	gtk_tree_model_get(model, &iter, NAME_COLUMN, &yuck, -1);
	
	if (strstr(yuck, "\n")) {
		gpointer start = yuck;
		
		while (*yuck && *yuck != '\n') yuck++;
		*yuck = 0;
		
		yuck = start;
	}

	/*
	 * Enquanto não encontrar maneira melhor de encontrar o
	 * ponteiro para o SuxModule referente ao plugin selecionado,
	 * vai assim mesmo!
	 *
	 * FIXME: Tá, encontrei maneira melhor. Vou implementar isso
	 *        logo... hehe
	 */
	for (plug = plugin_list; plug; plug = plug->next) {
		sm = plug->data;

		if (!strncmp(sm->name, yuck, strlen(yuck)))
			break;

		sm = NULL;
	}

	if (sm) {
		gtk_widget_set_sensitive(GTK_WIDGET(settings->about_btn),
					 sm->about ? TRUE : FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(settings->prefs_btn),
					 sm->prefs ? TRUE : FALSE);
	}

	selected = sm;

	g_free(yuck);
}

static void
list_fill(Settings * settings)
{
	GList *plug;
	GtkTreeSelection *sel;

	sel =
	    gtk_tree_view_get_selection(GTK_TREE_VIEW(settings->plugins_view));

	g_signal_connect(G_OBJECT(sel), "changed", (GCallback) plugin_selected,
			 settings);

	settings->plugins_model = gtk_tree_store_new(NUM_COLUMNS, 
		GDK_TYPE_PIXBUF, G_TYPE_STRING);

	gtk_tree_view_set_model(settings->plugins_view,
				GTK_TREE_MODEL(settings->plugins_model));
	create_view_columns(settings->plugins_view,
			    GTK_TREE_MODEL(settings->plugins_model));

	for (plug = plugin_list; plug; plug = plug->next) {
		SuxModule *sm;

		sm = plug->data;
		settings_append_plugin(settings, sm->name, sm->icon, 
				       sm->description);
	}
}

void
settings_append_plugin(Settings * settings, const gchar *name,
		       GdkPixbuf *icon, const gchar *description)
{
	GtkTreeIter iter;
	gchar *tmp;

	if (description) {
		tmp = g_strdup_printf("%s\n<span color='#444'><i>%s</i></span>", name,
				      description);
        } else {
        	tmp = g_strdup(name);
        }
        
	gtk_tree_store_append(settings->plugins_model, &iter, NULL);
	gtk_tree_store_set(settings->plugins_model, &iter,
			   NAME_COLUMN, tmp, -1);
			   
        g_free(tmp);
        
        if (icon) {
		gtk_tree_store_set(settings->plugins_model, &iter,
				   PIXBUF_COLUMN, icon, -1);
        }
}

Settings *
settings_create(void)
{
	GtkWidget *settings;
	GtkWidget *vbox1;
	GtkWidget *hbox1;
	GtkWidget *scrolledwindow1;
	GtkWidget *treeview1;
	GtkWidget *vbuttonbox1;
	GtkWidget *button4;
	GtkWidget *button5;
	GtkWidget *alignment1;
	GtkWidget *hbox2;
	GtkWidget *image1;
	GtkWidget *label4;
	GtkWidget *hseparator1;
	GtkWidget *hbuttonbox1;
	GtkWidget *button2;
	Settings *set;

	set = g_new0(Settings, 1);

	settings = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(settings), 500, 360);
	gtk_window_set_title(GTK_WINDOW(settings), _("SuxPanel Settings"));

	set->window = settings;

	vbox1 = gtk_vbox_new(FALSE, 6);
	gtk_widget_show(vbox1);
	gtk_container_add(GTK_CONTAINER(settings), vbox1);
	gtk_container_set_border_width(GTK_CONTAINER(vbox1), 5);

	hbox1 = gtk_hbox_new(FALSE, 4);
	gtk_widget_show(hbox1);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox1, TRUE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(hbox1), 4);

	scrolledwindow1 = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolledwindow1),
					    GTK_SHADOW_IN);
	gtk_widget_show(scrolledwindow1);
	gtk_box_pack_start(GTK_BOX(hbox1), scrolledwindow1, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow1),
				       GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);

	treeview1 = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview1), FALSE);
	gtk_widget_show(treeview1);
	gtk_container_add(GTK_CONTAINER(scrolledwindow1), treeview1);
	set->plugins_view = GTK_TREE_VIEW(treeview1);

	vbuttonbox1 = gtk_vbutton_box_new();
	gtk_widget_show(vbuttonbox1);
	gtk_box_pack_start(GTK_BOX(hbox1), vbuttonbox1, FALSE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbuttonbox1), 3);
	gtk_button_box_set_layout(GTK_BUTTON_BOX(vbuttonbox1),
				  GTK_BUTTONBOX_START);
	gtk_box_set_spacing(GTK_BOX(vbuttonbox1), 5);

	button4 = gtk_button_new_from_stock("gtk-properties");
	gtk_widget_show(button4);
	gtk_container_add(GTK_CONTAINER(vbuttonbox1), button4);
	gtk_widget_set_sensitive(button4, FALSE);
	GTK_WIDGET_SET_FLAGS(button4, GTK_CAN_DEFAULT);
	set->prefs_btn = button4;

	button5 = gtk_button_new();
	gtk_widget_show(button5);
	gtk_container_add(GTK_CONTAINER(vbuttonbox1), button5);
	GTK_WIDGET_SET_FLAGS(button5, GTK_CAN_DEFAULT);
	gtk_widget_set_sensitive(button5, FALSE);
	set->about_btn = button5;

	alignment1 = gtk_alignment_new(0.5, 0.5, 0, 0);
	gtk_widget_show(alignment1);
	gtk_container_add(GTK_CONTAINER(button5), alignment1);

	hbox2 = gtk_hbox_new(FALSE, 2);
	gtk_widget_show(hbox2);
	gtk_container_add(GTK_CONTAINER(alignment1), hbox2);

	image1 =
	    gtk_image_new_from_stock("gtk-dialog-info", GTK_ICON_SIZE_BUTTON);
	gtk_widget_show(image1);
	gtk_box_pack_start(GTK_BOX(hbox2), image1, FALSE, FALSE, 0);

	label4 = gtk_label_new_with_mnemonic(_("_About"));
	gtk_widget_show(label4);
	gtk_box_pack_start(GTK_BOX(hbox2), label4, FALSE, FALSE, 0);
	gtk_label_set_justify(GTK_LABEL(label4), GTK_JUSTIFY_LEFT);

	label4 = gtk_label_new("<b>Note:</b> To add or remove a plugin, edit <i>~/.suxpanel/modules.ini</i>\n" \
	                       "and restart the panel.");
	gtk_label_set_use_markup(GTK_LABEL(label4), TRUE);
	gtk_widget_show(label4);
	gtk_box_pack_start(GTK_BOX(vbox1), label4, FALSE, FALSE, 0);	                       

	hseparator1 = gtk_hseparator_new();
	gtk_widget_show(hseparator1);
	gtk_box_pack_start(GTK_BOX(vbox1), hseparator1, FALSE, TRUE, 0);

	hbuttonbox1 = gtk_hbutton_box_new();
	gtk_widget_show(hbuttonbox1);
	gtk_box_pack_start(GTK_BOX(vbox1), hbuttonbox1, FALSE, TRUE, 0);
	gtk_button_box_set_layout(GTK_BUTTON_BOX(hbuttonbox1),
				  GTK_BUTTONBOX_END);
	gtk_box_set_spacing(GTK_BOX(hbuttonbox1), 5);

	button2 = gtk_button_new_from_stock("gtk-close");
	gtk_widget_show(button2);
	gtk_container_add(GTK_CONTAINER(hbuttonbox1), button2);
	GTK_WIDGET_SET_FLAGS(button2, GTK_CAN_DEFAULT);
	g_signal_connect(G_OBJECT(button2), "clicked",
			 (GCallback) settings_cancel, set);

	g_signal_connect(G_OBJECT(set->about_btn), "clicked",
			 (GCallback) settings_about, NULL);
	g_signal_connect(G_OBJECT(set->prefs_btn), "clicked",
			 (GCallback) settings_prefs, NULL);

	list_fill(set);
	gtk_widget_show(set->window);

	return set;
}
