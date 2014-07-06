/*
 * SuxPanel Clipboard Manager
 * Copyright (c) 2003 Leandro Pereira <leandro@linuxmag.com.br>
 *
 * This is a port of XFCE Clipboard Manager
 * Copyright (c) 2003 Eduard Roccatello (master@spine-group.org)
 *
 * Based on xfce4-sample-plugin:
 * Copyright (c) 2003 Benedikt Meurer <benedikt.meurer@unix-ag.uni-siegen.de>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <string.h>
#include <gtk/gtk.h>
#include <glib.h>

#include "suxpanel.h"

#define MAXHISTORY 6

typedef struct {
    GtkWidget *ebox;
    GtkWidget *button;
    GtkWidget *img;
    GString *content[MAXHISTORY];
    guint iter;
} t_clipman;

typedef struct {
    t_clipman *clip;
    guint idx;
} t_action;

static GtkClipboard *primaryClip, *defaultClip;

static gboolean isThere(t_clipman * clip, gchar * txt);
static gchar *filterLFCR(gchar * txt);

static gboolean isThere(t_clipman * clip, gchar * txt)
{
    gint i;

    for (i = 0; i < MAXHISTORY; i++) {
	if (strcmp(clip->content[i]->str, txt) == 0)
	    return TRUE;
    }
    return FALSE;
}

static gchar *filterLFCR(gchar * txt)
{
    guint i = 0;

    while (txt[i] != '\0') {
	if (txt[i] == '\n' || txt[i] == '\r' || txt[i] == '\t')
	    txt[i] = ' ';
	i++;
    }
    return txt;
}

static void
clearClipboard (GtkWidget *widget, gpointer data)
{
    gint i;
    t_clipman *clipman = (t_clipman *)data;

    /* Clear History */
    for (i=0; i<MAXHISTORY; i++)
        g_string_assign(clipman->content[i], "");

    /* Clear Clipboard */
    gtk_clipboard_set_text(defaultClip, "", -1);
    gtk_clipboard_set_text(primaryClip, "", -1);

    /* Set iterator to the first element of the array */
    clipman->iter = 0;

}

static void clicked_menu(GtkWidget * widget, gpointer data)
{
    t_action *act = (t_action*) data;

    gtk_clipboard_set_text(defaultClip, act->clip->content[act->idx]->str,
			   -1);
    gtk_clipboard_set_text(primaryClip, act->clip->content[act->idx]->str,
			   -1);
}

static void clicked_cb(GtkWidget * button, gpointer data)
{
    GtkMenu *menu = NULL;
    GtkWidget *mi;
    t_clipman *clipman = (t_clipman*)data;
    t_action *action = NULL;
    gboolean hasOne = FALSE;
    guint i;

    menu = GTK_MENU(gtk_menu_new());

    for (i = 0; i < MAXHISTORY; i++) {
	if (clipman->content[i]->str != NULL
	    && (strcmp(clipman->content[i]->str, "") != 0)) {
	    gchar *temp, *temp2;

	    temp = g_strndup(clipman->content[i]->str, 40);
	    temp2 = g_strdup_printf("%d. %s", i + 1, filterLFCR(temp));

	    mi = gtk_menu_item_new_with_label(temp2);
	    gtk_widget_show(mi);
	    action = g_new(t_action, 1);
	    action->clip = clipman;
	    action->idx = i;
	    g_signal_connect(G_OBJECT(mi), "activate",
			     G_CALLBACK(clicked_menu), (gpointer) action);
	    gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);
	    hasOne = TRUE;
	    
	    g_free(temp);
	    g_free(temp2);
	}
    }

    if (!hasOne) {
	mi = gtk_menu_item_new_with_label("< Clipboard Empty >");
	gtk_widget_show(mi);
	gtk_widget_set_sensitive(mi, FALSE);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);
    } else {
        GtkWidget *img;

        mi = gtk_image_menu_item_new_with_label ("Clear clipboard");
    	gtk_widget_show (mi);        
	img = gtk_image_new_from_stock(GTK_STOCK_CLEAR, GTK_ICON_SIZE_MENU);
	gtk_widget_show(img);
        
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(mi), img);
	g_signal_connect (G_OBJECT (mi), "activate", G_CALLBACK (clearClipboard), (gpointer)clipman);
	gtk_menu_shell_insert (GTK_MENU_SHELL (menu), mi, 0);
        
	mi = gtk_separator_menu_item_new();
	gtk_widget_show(mi);
	gtk_widget_set_sensitive(mi, FALSE);
	gtk_menu_shell_insert (GTK_MENU_SHELL (menu), mi, 1);
    }

    gtk_menu_popup(menu, NULL, NULL, NULL, NULL, 0,
		   gtk_get_current_event_time());
}

static void checkClipReal(GtkClipboard * clipboard, const gchar * text,
			  gpointer data)
{
    t_clipman *clipman = (t_clipman *) data;
    gchar *txt = NULL;

    if (!text)
	return;

    txt = g_strdup(text);
    
    if (txt != NULL && !isThere(clipman, txt)) {
	g_string_assign(clipman->content[clipman->iter], txt);
	if (clipman->iter < (MAXHISTORY - 1))
	    clipman->iter++;
	else
	    clipman->iter = 0;
    }
    if (txt != NULL && txt) {
	g_free(txt);
	txt = NULL;
    }

    /* Check for text in default clipboard */
    txt = gtk_clipboard_wait_for_text(defaultClip);
    if (txt != NULL && !isThere(clipman, txt)) {
	g_string_assign(clipman->content[clipman->iter], txt);
	if (clipman->iter < (MAXHISTORY - 1))
	    clipman->iter++;
	else
	    clipman->iter = 0;
    }
    if (txt != NULL && txt) {
	g_free(txt);
	txt = NULL;
    }

}

static gboolean checkClip(gpointer data)
{
    t_clipman *clipman = (t_clipman*) data;

    if (prefs_open)
    	return TRUE;

    gtk_clipboard_request_text(primaryClip, checkClipReal, clipman);
 
    return TRUE;
}

static t_clipman *clipman_new(void)
{
    GtkTooltips *tips;
    t_clipman *clipman;
    gint i;

    clipman = g_new(t_clipman, 1);

    tips = gtk_tooltips_new();
    clipman->ebox = gtk_event_box_new();
    gtk_widget_show(clipman->ebox);

    clipman->button = gtk_button_new();
    gtk_button_set_relief(GTK_BUTTON(clipman->button), GTK_RELIEF_NONE);
    gtk_widget_show(clipman->button);

    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips), clipman->button,
                         "Clipboard Manager", NULL);

    gtk_container_add(GTK_CONTAINER(clipman->ebox), clipman->button);

    clipman->img =
	gtk_image_new_from_stock("gtk-paste", GTK_ICON_SIZE_BUTTON);
    gtk_widget_show(clipman->img);
    gtk_container_add(GTK_CONTAINER(clipman->button), clipman->img);

    /* Element to be modified */
    clipman->iter = 0;

    for (i = 0; i < MAXHISTORY; i++) {
	clipman->content[i] = g_string_new("");
    }
    defaultClip = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    primaryClip = gtk_clipboard_get(GDK_SELECTION_PRIMARY);

    checkClip(clipman);
    g_timeout_add(512, checkClip, clipman);
    g_signal_connect(clipman->button, "clicked", G_CALLBACK(clicked_cb),
		     clipman);

    return (clipman);
}

void sux_init(SuxModule * sm)
{
    t_clipman *clipman;

    clipman = clipman_new();

    sm->widget = clipman->ebox;
}

void sux_fini(SuxModule * sm)
{
    gtk_widget_destroy(sm->widget);
}

const gchar *sux_name(void)
{
    return "Clipboard Manager";
}

void sux_about(void)
{
    const gchar *msg[] = {
	">SuxPanel port by:",
	"Leandro Pereira (leandro@linuxmag.com.br)",
	">Original XFCE applet by:",
	"Eduard Roccatello (master@spine-group.org)",
	NULL
    };

    gtk_about_new("Clipboard Manager", "0.2",
		  "A clipboard history utility",
		  msg, IMG_PREFIX "clipboard.png");
}

GdkPixbuf *sux_icon(void)
{
        return gdk_pixbuf_new_from_file(IMG_PREFIX "clipboard.png", NULL);
}

