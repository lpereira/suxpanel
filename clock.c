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

#include "suxpanel.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

static gboolean update_lbl(gpointer label);
static gchar *clock_fmt;
static SuxModule *clock_sm;

typedef struct _ClockSettings ClockSettings;
struct _ClockSettings {
	GtkWidget *window;
	GtkWidget *entry;
	
	SuxModule *module;
};

static void
clock_prefs_close(GtkWidget * wid, gpointer data)
{
	ClockSettings *set = (ClockSettings *) data;

	gtk_widget_destroy(set->window);
	set->module->prefs_open = FALSE;
}

static void
clock_prefs_set(GtkWidget * wid, gpointer data)
{
	ClockSettings *set = (ClockSettings *) data;
	const gchar *newval;

	newval = gtk_entry_get_text(GTK_ENTRY(set->entry));
	if (strlen(newval)) {
		FILE *conf;
		gchar *buf;
					
		g_free(clock_fmt);
		clock_fmt = g_strdup(newval);
		
		gtk_widget_destroy(set->window);

		buf = g_strdup_printf("%s/.suxpanel/clock.ini", g_get_home_dir());
		conf = fopen(buf, "w+");
		if (!conf)
			return;
			
		fprintf(conf, "%s", clock_fmt);
		fclose(conf);
		
		g_free(buf);
	} else {
		GtkWidget *dialog;

		dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
						GTK_MESSAGE_ERROR,
						GTK_BUTTONS_CLOSE,
						"Remove the clock if you don't need it... :P");
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
	}
}

static ClockSettings *
clock_settings_create(SuxModule *module)
{
	ClockSettings *set;
	GtkWidget *clock_prefs;
	GtkWidget *vbox1;
	GtkWidget *hbox4;
	GtkWidget *image1;
	GtkWidget *label2;
	GtkWidget *hbox3;
	GtkWidget *label3;
	GtkWidget *entry2;
	GtkWidget *hseparator1;
	GtkWidget *hbuttonbox1;
	GtkWidget *button1;
	GtkWidget *button2;

	set = g_new0(ClockSettings, 1);

        set->module = module;

	clock_prefs = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	set->window = clock_prefs;
	gtk_window_set_title(GTK_WINDOW(clock_prefs), _("Clock Preferences"));

	vbox1 = gtk_vbox_new(FALSE, 4);
	gtk_widget_show(vbox1);
	gtk_container_add(GTK_CONTAINER(clock_prefs), vbox1);
	gtk_container_set_border_width(GTK_CONTAINER(vbox1), 5);

	hbox4 = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox4);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox4, TRUE, TRUE, 0);

	image1 =
	    gtk_image_new_from_stock("gtk-dialog-info", GTK_ICON_SIZE_DIALOG);
	gtk_widget_show(image1);
	gtk_box_pack_start(GTK_BOX(hbox4), image1, FALSE, TRUE, 0);

	label2 =
	    gtk_label_new(_
			  ("You can use any token recognized by <b>date(1)</b>. "));
	gtk_widget_show(label2);
	gtk_box_pack_start(GTK_BOX(hbox4), label2, FALSE, FALSE, 0);
	gtk_label_set_use_markup(GTK_LABEL(label2), TRUE);
	gtk_label_set_justify(GTK_LABEL(label2), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment(GTK_MISC(label2), 0, 0.5);

	hbox3 = gtk_hbox_new(FALSE, 5);
	gtk_widget_show(hbox3);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox3, TRUE, TRUE, 0);

	label3 = gtk_label_new(_("Clock format:"));
	gtk_widget_show(label3);
	gtk_box_pack_start(GTK_BOX(hbox3), label3, FALSE, FALSE, 0);
	gtk_label_set_justify(GTK_LABEL(label3), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment(GTK_MISC(label3), 0, 0.5);

	entry2 = gtk_entry_new();
	set->entry = entry2;
	gtk_widget_show(entry2);
	gtk_entry_set_text(GTK_ENTRY(entry2), clock_fmt);
	gtk_box_pack_start(GTK_BOX(hbox3), entry2, TRUE, TRUE, 0);

	hseparator1 = gtk_hseparator_new();
	gtk_widget_show(hseparator1);
	gtk_box_pack_start(GTK_BOX(vbox1), hseparator1, FALSE, TRUE, 0);

	hbuttonbox1 = gtk_hbutton_box_new();
	gtk_widget_show(hbuttonbox1);
	gtk_box_pack_start(GTK_BOX(vbox1), hbuttonbox1, FALSE, TRUE, 0);
	gtk_button_box_set_layout(GTK_BUTTON_BOX(hbuttonbox1),
				  GTK_BUTTONBOX_END);
	gtk_box_set_spacing(GTK_BOX(hbuttonbox1), 5);

	button1 = gtk_button_new_from_stock("gtk-cancel");
	gtk_widget_show(button1);
	gtk_container_add(GTK_CONTAINER(hbuttonbox1), button1);
	GTK_WIDGET_SET_FLAGS(button1, GTK_CAN_DEFAULT);

	button2 = gtk_button_new_from_stock("gtk-ok");
	gtk_widget_show(button2);
	gtk_container_add(GTK_CONTAINER(hbuttonbox1), button2);
	GTK_WIDGET_SET_FLAGS(button2, GTK_CAN_DEFAULT);

	g_signal_connect(G_OBJECT(button1), "clicked", (GCallback)clock_prefs_close, set);
	g_signal_connect(G_OBJECT(button2), "clicked", (GCallback)clock_prefs_set, set);

	gtk_widget_show(set->window);
	return set;
}

static void
show_calendar(GtkWidget *widget, gpointer data)
{
	static GtkWidget *window = NULL;
	GtkWidget        *calendar;
	GtkAllocation     alloc;
	gint              x_pos, y_pos, width;

	if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) {
		if (window)
			gtk_widget_destroy(window);
		return;
	}

	alloc = sux_module_get_allocation(clock_sm);
	x_pos = alloc.x;
	y_pos = alloc.y + alloc.height;

	window = gtk_window_new(GTK_WINDOW_POPUP);
	
	calendar = gtk_calendar_new();
	gtk_widget_show(calendar);
	gtk_container_add(GTK_CONTAINER(window), calendar);
	
	gtk_widget_show(window);

	gtk_window_get_size(GTK_WINDOW(window), &width, NULL);

	if (x_pos + width > gdk_screen_get_width(gdk_screen_get_default()))
		x_pos = gdk_screen_get_width(gdk_screen_get_default()) - width;

        gtk_widget_set_uposition (GTK_WIDGET(window), x_pos, y_pos);
        	
}

static gboolean
update_lbl(gpointer label)
{
	time_t t;
	gchar buf[256];

	if (!label)
		return FALSE;

	t  = time(NULL);

	strftime(buf, 256, clock_fmt, localtime(&t));
	gtk_label_set_markup(GTK_LABEL(label), buf);

	return TRUE;
}

const gchar *
sux_name(void)
{
	return "Clock";
}

void
sux_init(SuxModule * sm)
{
	FILE *conf;
	gchar *buf;
	GtkWidget *label, *btn;

	buf = g_strdup_printf("%s/.suxpanel/clock.ini", g_get_home_dir());
		
	conf = fopen(buf, "r");
	if (!conf)
		clock_fmt = g_strdup("%I:%M");
	else {
		gchar temp[256];
		
		fgets(temp, 256, conf);
		fclose(conf);
		
		clock_fmt = g_strdup(temp);
		clock_fmt = g_strstrip(clock_fmt);
	}
	g_free(buf);

	btn = gtk_toggle_button_new();
	gtk_button_set_relief(GTK_BUTTON(btn), GTK_RELIEF_NONE);
	gtk_widget_show(btn);

	label = gtk_label_new("");
	gtk_widget_show(label);

	gtk_container_add(GTK_CONTAINER(btn), label);

	g_timeout_add(1000, update_lbl, label);
	update_lbl(label);
	
	g_signal_connect(G_OBJECT(btn), "toggled",
			 (GCallback)show_calendar, NULL);
	
	sm->widget = btn;
	clock_sm   = sm;
}

void
sux_fini(SuxModule * sm)
{
	gtk_widget_destroy(sm->widget);
}

void
sux_prefs(SuxModule * sm)
{
	clock_settings_create(sm);
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

	about = gtk_about_new("Clock", "0.3",
			      "Clock and calendar applet",
			      msg, IMG_PREFIX "clock.png");
}

GdkPixbuf *sux_icon(void)
{
        return gdk_pixbuf_new_from_file(IMG_PREFIX "clock.png", NULL);
}

const gchar *sux_description(void)
{
        return "Clock and calendar";
}
