/*
 * SuxPanel Mount Applet version 0.1
 * Copyright (c) 2004 Jens Askengren <jensus@linux.nu>
 *
 * This program may be distributed under the terms of GNU General
 * Public License version 2. You should have received a copy of the
 * license with this program; if not, please consult http://www.fsf.org/.
 *
 * This program comes with no warranty. Use at your own risk.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <mntent.h>
#include <paths.h>
#include <string.h>
#include <signal.h>

#include "suxpanel.h"
#include "gtkmisc.h"

static SuxModule *mount_sm = NULL;
static gboolean run_command_on_mount = FALSE;
static gchar *on_mount_command = NULL;

static void popup_mount_menu(void);
static gboolean is_mounted(const char *path);
static void toggle_mount(GtkCheckMenuItem * i, gchar * data);

const gchar *sux_name(void)
{
    return "Disk mounting applet";
}

void sux_init(SuxModule * sm)
{

    GtkWidget *button;
    GtkWidget *img;
    GtkTooltips *tips;

    button = gtk_button_new();
    img = gtk_image_new_from_stock(GTK_STOCK_SAVE,
				   GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_widget_show(img);
    gtk_container_add(GTK_CONTAINER(button), img);
    gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
    g_signal_connect(G_OBJECT(button), "button_press_event",
		     (GCallback) popup_mount_menu, NULL);
    tips = gtk_tooltips_new();
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips), button,
			 "Mount and unmount removable disks", NULL);
    gtk_widget_show(button);

    sm->widget = button;
    mount_sm = sm;

    run_command_on_mount = TRUE;
    on_mount_command = g_strdup("nautilus");
}

void sux_fini(SuxModule * sm)
{
    gtk_widget_destroy(sm->widget);
}

/**
 * @return TRUE if the path is mounted according to /etc/mtab
 */
static gboolean is_mounted(const char *path)
{
    FILE *mtab;
    struct mntent me;
    char buffer[1024];
    gboolean r = FALSE;

    mtab = setmntent(_PATH_MOUNTED, "r");

    if (mtab == NULL) {
	return FALSE;
    }

    while (getmntent_r(mtab, &me, buffer, sizeof(buffer)) != NULL) {
	if (strcmp(me.mnt_dir, path) == 0) {
	    r = TRUE;
	    break;
	}
    }

    endmntent(mtab);

    return r;
}

/**
 * Toggle mounting of the selected menuitem
 */
static void toggle_mount(GtkCheckMenuItem * i, gchar * data)
{
    gchar *cmd;
    gint status;
    GError *err = NULL;
    GtkWidget *dialog = NULL;
    gboolean mount;
    gchar *mount_error = NULL;
    gchar *msg = NULL;
    gchar *p;
    struct sigaction sa;
    struct sigaction old_sa;

    mount = gtk_check_menu_item_get_active(i);

    if (mount) {
	cmd = g_strdup_printf("mount %s", data);
    } else {
	cmd = g_strdup_printf("umount %s", data);
    }

    sigaction(SIGCHLD, NULL, &old_sa);
    sa.sa_handler = SIG_DFL;
    sa.sa_flags = old_sa.sa_flags;
    sa.sa_mask = old_sa.sa_mask;
    sigaction(SIGCHLD, &sa, &old_sa);
    g_spawn_command_line_sync(cmd, NULL, &mount_error, &status, &err);
    sigaction(SIGCHLD, &old_sa, NULL);
    g_free(cmd);

    if (status != 0 || err != NULL) {

	if (mount_error != NULL) {

	    /* format mount output */
	    if (g_str_has_prefix(mount_error, "mount: ")) {
		msg = mount_error + 7;
	    } else if (g_str_has_prefix(mount_error, "umount: ")) {
		msg = mount_error + 8;
	    } else {
		msg = mount_error;
	    }

	    for (p = msg; *p; p++) {
		if (*p == '\n') {
		    *p = ' ';
		}
	    }

	} else {
	    msg = err->message;
	}

	dialog = gtk_message_dialog_new(NULL, 0, GTK_MESSAGE_ERROR,
					GTK_BUTTONS_OK,
					"Could not %s %s:\n%s",
					(mount ? "mount" : "unmount"),
					data, msg);

	g_signal_connect_swapped(GTK_OBJECT(dialog), "response",
				 G_CALLBACK(gtk_widget_destroy),
				 GTK_OBJECT(dialog));

	gtk_widget_show_all(dialog);

    } else {

	/* 
	 * (u)mount ok, run command if any 
	 */

	if (mount && run_command_on_mount && on_mount_command) {

	    cmd = g_strdup_printf("%s %s", on_mount_command, data);
	    g_spawn_command_line_async(cmd, &err);

	    if (err) {
		dialog = gtk_message_dialog_new(NULL, 0, GTK_MESSAGE_ERROR,
						GTK_BUTTONS_OK,
						"Could not run \"%s\":\n%s",
						cmd, err->message);

		g_signal_connect_swapped(GTK_OBJECT(dialog), "response",
					 G_CALLBACK(gtk_widget_destroy),
					 GTK_OBJECT(dialog));

		gtk_widget_show_all(dialog);
	    }

	    g_free(cmd);
	}

    }				/* if status ... */

    if (mount_error != NULL) {
	g_free(mount_error);
    }

    if (err != NULL) {
	g_error_free(err);
    }

    g_free(data);
}


static void popup_mount_menu(void)
{
    GtkWidget *menu;
    GtkWidget *item;
    GtkWidget *dialog;
    FILE *fstab;
    struct mntent me;
    char buffer[1024];

    fstab = setmntent(_PATH_MNTTAB, "r");
    if (fstab != NULL) {

	menu = gtk_menu_new();

	while (getmntent_r(fstab, &me, buffer, sizeof(buffer)) != NULL) {
	    if (hasmntopt(&me, "user")) {
		item = gtk_check_menu_item_new_with_label(me.mnt_dir);
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item),
					       is_mounted(me.mnt_dir));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "toggled",
				 (GCallback) toggle_mount,
				 g_strdup(me.mnt_dir));
	    }
	}

	endmntent(fstab);

	gtk_widget_show_all(menu);
	gtk_menu_popup(GTK_MENU(menu), NULL, mount_sm->widget, NULL,
		       NULL, GDK_LEFTBUTTON, gtk_get_current_event_time());
	gtk_object_sink(GTK_OBJECT(menu));
    } else {
	/* Can not read /etc/fstab */
	dialog = gtk_message_dialog_new(NULL, 0, GTK_MESSAGE_ERROR,
					GTK_BUTTONS_OK,
					"Could not read " _PATH_MNTTAB);

	g_signal_connect_swapped(GTK_OBJECT(dialog), "response",
				 G_CALLBACK(gtk_widget_destroy),
				 GTK_OBJECT(dialog));

	gtk_widget_show_all(dialog);
    }

}


static void command_toggled(GtkToggleButton * togglebutton,
			    GtkWidget * command)
{
    run_command_on_mount = gtk_toggle_button_get_active(togglebutton);
    gtk_widget_set_sensitive(command, run_command_on_mount);
}

/* 
  TODO:
    - Option to add custom (non fstab) mount commands to the menu
    - Use gconf to save settings
 */
void sux_prefs(SuxModule * sm)
{
    GtkWidget *d;
    GtkWidget *table;
    GtkWidget *checkbtn;
    GtkWidget *command;

    d = gtk_dialog_new_with_buttons("Mount applet preferences", NULL,
				    0, GTK_STOCK_CLOSE,
				    GTK_RESPONSE_ACCEPT, NULL);

    table = gtk_table_new(2, 1, FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(table), 4);
    gtk_table_set_col_spacings(GTK_TABLE(table), 4);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(d)->vbox), table, TRUE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(table), 12);
    checkbtn =
	gtk_check_button_new_with_mnemonic
	("_Run command when a disk is mounted:");
    gtk_table_attach_defaults(GTK_TABLE(table), checkbtn, 0, 1, 0, 1);
    command = gtk_entry_new();
    if (on_mount_command != NULL) {
	gtk_entry_set_text(GTK_ENTRY(command), on_mount_command);
    }
    gtk_table_attach_defaults(GTK_TABLE(table), command, 0, 1, 1, 2);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbtn),
				 run_command_on_mount);
    gtk_widget_set_sensitive(command, run_command_on_mount);

    g_signal_connect(GTK_OBJECT(checkbtn), "toggled",
		     G_CALLBACK(command_toggled), GTK_OBJECT(command));

    gtk_widget_show_all(table);
    gtk_dialog_run(GTK_DIALOG(d));

    if (on_mount_command) {
	g_free(on_mount_command);
    }
    on_mount_command = g_strdup(gtk_entry_get_text(GTK_ENTRY(command)));

    gtk_widget_destroy(d);
}


void sux_about(void)
{
    GtkAbout *about;
    const gchar *msg[] = {
	">Written by:",
	"Jens Askengren (jensus@linux.nu)",
	">Disclaimer:",
	"This is free software; you can modify and/or distribute it",
	"under the terms of GNU GPL version 2. See http://www.fsf.org",
	"for more information.",
	NULL
    };

    about = gtk_about_new("Disk mount", "0.1",
			  "Disk mounting applet",
			  msg, IMG_PREFIX "mount.png");
}

GdkPixbuf *sux_icon(void)
{
        return gdk_pixbuf_new_from_file(IMG_PREFIX "mount.png", NULL);
}
