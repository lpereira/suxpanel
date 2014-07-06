/*
 * Mailbox check applet for SuxPanel
 * Version 0.1
 *
 * Copyright (c) 2004 Giuseppe Coviello (slash@crux-it.org)
 *
 * This is free software; you can modify and/or distributed
 * under the terms of GNU GPL version 2. See http://www.fsf.org
 * for more information.
 */

#include "suxpanel.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

static gboolean update_lbl(gpointer label);
static gchar *mailbox_txt;
static gchar *delay_txt;
static gchar *client_txt;

typedef struct _MailSettings MailSettings;
struct _MailSettings {
    GtkWidget *window;
    GtkWidget *mailbox;
    GtkWidget *delay;
    GtkWidget *client;
    SuxModule *module;
};

static void mail_prefs_close(GtkWidget * wid, gpointer data)
{
    MailSettings *set = (MailSettings *) data;

    gtk_widget_destroy(set->window);
    set->module->prefs_open = FALSE;
}

static void mail_prefs_set(GtkWidget * wid, gpointer data)
{
    MailSettings *set = (MailSettings *) data;
    const gchar *newmailbox;
    const gchar *newdelay;
    const gchar *newclient;

    newmailbox = gtk_entry_get_text(GTK_ENTRY(set->mailbox));
    newdelay = g_strdup_printf("%d", gtk_spin_button_get_value_as_int
			       (GTK_SPIN_BUTTON(set->delay)));
    newclient = gtk_entry_get_text(GTK_ENTRY(set->client));

    if (strlen(newmailbox) && strlen(newclient)) {
	FILE *conf;
	gchar *buf;

	g_free(mailbox_txt);
	g_free(delay_txt);
	g_free(client_txt);
	mailbox_txt = g_strdup(newmailbox);
	delay_txt = g_strdup(newdelay);
	client_txt = g_strdup(newclient);

	gtk_widget_destroy(set->window);

	buf = g_strdup_printf("%s/.suxpanel/mail.ini", g_get_home_dir());
	unlink(buf);

	conf = fopen(buf, "w+");
	if (!conf)
	    return;


	fprintf(conf, "%s\n", mailbox_txt);
	fprintf(conf, "%s\n", delay_txt);
	fprintf(conf, "%s\n", client_txt);
	fclose(conf);

	g_free(buf);
    }
}

static MailSettings *mail_settings_create(SuxModule * module)
{
    MailSettings *set;
    GtkWidget *mail_prefs;
    GtkWidget *vb_main;
    GtkWidget *vb_label;
    GtkWidget *vb_entry;
    GtkWidget *hb_main;
    GtkWidget *hseparator;
    GtkWidget *hbuttonbox;
    GtkWidget *lbl_mailbox;
    GtkWidget *lbl_delay;
    GtkWidget *lbl_client;
    GtkWidget *_mailbox;
    GtkWidget *_delay;
    GtkWidget *_client;
    GtkWidget *btn_ok;
    GtkWidget *btn_cancel;

    set = g_new0(MailSettings, 1);

    set->module = module;

    mail_prefs = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    set->window = mail_prefs;
    gtk_window_set_title(GTK_WINDOW(mail_prefs),
			 _("Mail Check Preferences"));

    vb_main = gtk_vbox_new(FALSE, 6);
    gtk_widget_show(vb_main);
    gtk_container_add(GTK_CONTAINER(mail_prefs), vb_main);
    gtk_container_set_border_width(GTK_CONTAINER(vb_main), 5);

    hb_main = gtk_hbox_new(FALSE, 5);
    gtk_widget_show(hb_main);
    gtk_box_pack_start(GTK_BOX(vb_main), hb_main, TRUE, TRUE, 0);

    vb_label = gtk_vbox_new(TRUE, 6);
    gtk_widget_show(vb_label);
    gtk_box_pack_start(GTK_BOX(hb_main), vb_label, TRUE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(vb_label), 5);

    vb_entry = gtk_vbox_new(FALSE, 6);
    gtk_widget_show(vb_entry);
    gtk_box_pack_start(GTK_BOX(hb_main), vb_entry, FALSE, FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(vb_entry), 5);

    lbl_mailbox = gtk_label_new(_("Mailbox path:"));
    gtk_widget_show(lbl_mailbox);
    gtk_box_pack_start(GTK_BOX(vb_label), lbl_mailbox, FALSE, FALSE, 0);
    gtk_label_set_justify(GTK_LABEL(lbl_mailbox), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(lbl_mailbox), 0, 0.5);

    _mailbox = gtk_entry_new();
    set->mailbox = _mailbox;
    gtk_widget_show(_mailbox);
    gtk_entry_set_text(GTK_ENTRY(_mailbox), mailbox_txt);
    gtk_box_pack_start(GTK_BOX(vb_entry), _mailbox, TRUE, TRUE, 0);

    lbl_delay = gtk_label_new(_("Delay:"));
    gtk_widget_show(lbl_delay);
    gtk_box_pack_start(GTK_BOX(vb_label), lbl_delay, FALSE, FALSE, 0);
    gtk_label_set_justify(GTK_LABEL(lbl_delay), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(lbl_delay), 0, 0.5);

    _delay = gtk_spin_button_new_with_range(0, 1000, 1);
    set->delay = _delay;
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(_delay), atoi(delay_txt));
    gtk_widget_show(_delay);
    gtk_box_pack_start(GTK_BOX(vb_entry), _delay, FALSE, FALSE, 0);

    lbl_client = gtk_label_new(_("Mail Client:"));
    gtk_widget_show(lbl_client);
    gtk_box_pack_start(GTK_BOX(vb_label), lbl_client, FALSE, FALSE, 0);
    gtk_label_set_justify(GTK_LABEL(lbl_client), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(lbl_client), 0, 0.5);

    _client = gtk_entry_new();
    set->client = _client;
    gtk_widget_show(_client);
    gtk_entry_set_text(GTK_ENTRY(_client), client_txt);
    gtk_box_pack_start(GTK_BOX(vb_entry), _client, TRUE, TRUE, 0);

    hseparator = gtk_hseparator_new();
    gtk_widget_show(hseparator);
    gtk_box_pack_start(GTK_BOX(vb_main), hseparator, FALSE, TRUE, 0);

    hbuttonbox = gtk_hbutton_box_new();
    gtk_widget_show(hbuttonbox);
    gtk_box_pack_start(GTK_BOX(vb_main), hbuttonbox, FALSE, TRUE, 0);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(hbuttonbox),
			      GTK_BUTTONBOX_END);
    gtk_box_set_spacing(GTK_BOX(hbuttonbox), 5);

    btn_cancel = gtk_button_new_from_stock("gtk-cancel");
    gtk_widget_show(btn_cancel);
    gtk_container_add(GTK_CONTAINER(hbuttonbox), btn_cancel);
    GTK_WIDGET_SET_FLAGS(btn_cancel, GTK_CAN_DEFAULT);

    btn_ok = gtk_button_new_from_stock("gtk-ok");
    gtk_widget_show(btn_ok);
    gtk_container_add(GTK_CONTAINER(hbuttonbox), btn_ok);
    GTK_WIDGET_SET_FLAGS(btn_ok, GTK_CAN_DEFAULT);

    g_signal_connect(G_OBJECT(btn_cancel), "clicked",
		     (GCallback) mail_prefs_close, set);
    g_signal_connect(G_OBJECT(btn_ok), "clicked",
		     (GCallback) mail_prefs_set, set);

    gtk_widget_show(set->window);
    return set;

}

static void avvia_client(GtkWidget * wid, gpointer data)
{
    pid_t pid = fork();
    if (pid == 0) {
	execvp(client_txt, 0);
	_exit(127);
    }
}

static gboolean update_lbl(gpointer label)
{
    gchar buf[256];
    FILE *file;

    if (!label)
	return FALSE;

    strcpy(buf, "No mail");

    if ((file = fopen(mailbox_txt, "r"))) {
	char riga[255];
	if (fgets(riga, 255, file))
	    strcpy(buf, "You have mail");
	fclose(file);
    }

    gtk_label_set_text(GTK_LABEL(label), buf);
    return TRUE;
}

const gchar *sux_name(void)
{
    return "Mail Check";
}

void sux_init(SuxModule * sm)
{
    GtkWidget *button;
    GtkWidget *label;
    gchar *conf_path;
    gchar riga[255];
    FILE *conf;

    mailbox_txt = g_strdup_printf("/var/mail/%s", g_get_user_name());
    delay_txt = g_strdup("60");
    client_txt = g_strdup("");

    conf_path =
	g_strdup_printf("%s/.suxpanel/mail.ini", g_get_home_dir());

    if ((conf = fopen(conf_path, "r"))) {
	if (fgets(riga, 255, conf)) {
	    mailbox_txt = g_strdup(riga);
	    mailbox_txt[strlen(mailbox_txt) - 1] = '\0';
	}
	if (fgets(riga, 255, conf))
	    delay_txt = g_strdup(riga);
	if (fgets(riga, 255, conf)) {
	    client_txt = g_strdup(riga);
	    client_txt[strlen(client_txt) - 1] = '\0';
	}
	fclose(conf);
    }

    button = gtk_button_new();
    label = gtk_label_new("");
    gtk_container_add(GTK_CONTAINER(button), label);
    gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
    gtk_widget_show(label);
    g_signal_connect(G_OBJECT(button), "clicked", (GCallback) avvia_client,
		     NULL);
    g_timeout_add((atoi(delay_txt) * 1000), update_lbl, label);
    update_lbl(label);
    sm->widget = button;
}

void sux_fini(SuxModule * sm)
{
    gtk_widget_destroy(sm->widget);
}

void sux_prefs(SuxModule * sm)
{
    mail_settings_create(sm);
}

void sux_about(void)
{
    GtkAbout *about;
    const gchar *msg[] = {
	">Written by:",
	"Giuseppe Coviello (slash@crux-it.org)",
	">Disclaimer:",
	"This is free software; you can modify and/or distributed",
	"under the terms of GNU GPL version 2. See http://www.fsf.org",
	"for more information.",
	NULL
    };

    about = gtk_about_new("Mail", "0.1", "Mailbox check applet", msg,
			  IMG_PREFIX "mail.png");
}

GdkPixbuf *sux_icon(void)
{
    return gdk_pixbuf_new_from_file(IMG_PREFIX "mail.png", NULL);
}
