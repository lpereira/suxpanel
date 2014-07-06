/*
 * SuxPanel Plugin Template
 * Version 0.4
 */

#include "suxpanel.h"

void sux_init(SuxModule *sm, gchar *args)
{
	GtkLabel *lbl;

	lbl = gtk_label_new((args && strlen(args)) ? args : "Hello, world!");
	gtk_widget_show(lbl);

	sm->widget = lbl;
}

void sux_fini(SuxModule *sm)
{
	gtk_widget_destroy(sm->widget);
}

const gchar *sux_name(void)
{
	return "Hello World!";
}
