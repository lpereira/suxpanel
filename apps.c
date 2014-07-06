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

#include "suxpanel.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>

#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

static GtkItemFactory *factory;
static GtkTooltips *apps_tips;

static gchar *app_path;
static gchar *menu_name = "Programs";

/*
 * This list used to be quite large and hard-coded; it's now generated
 * automatically. I'll remove this soon...
 */
static GtkItemFactoryEntry menu_items[] = {
    {"/Computer", NULL, 0, 0, "<Branch>"},
};
static gint n_menu_items = sizeof(menu_items) / sizeof(menu_items[0]);

/*
 * Prototypes
 */
static void read_app_menu(const gchar * dir_name, GtkWidget * parent_item);
static void spawn_app(GtkWidget * widget, gpointer data);

/*
 * Although this list is hardcoded, if you don't have the entry's executable,
 * it's icon won't be shown. Don't add *everything* to this menu, it should
 * be simple yet efficient.
 */
static void menu_add_icons(void)
{
    gint i;
    GtkWidget *item, *img, *menu;
    static struct {
	gchar *program;
	gchar *label;
	gchar *icon;
	gchar *parameters;
	gboolean separator_after;
    } prgicons[] = {
	{ "hardinfo", "About this Computer...", IMG_PREFIX "apps/hardinfo.png", "", TRUE },
	{ "rox-filer", "Home", IMG_PREFIX "apps/home.png", "~", FALSE },
	{ "rox-filer", "Disks", IMG_PREFIX "apps/disks.png", "/mnt", FALSE },
	{ "gnome-cups-manager", "Printer Jobs",	IMG_PREFIX "apps/printer.png", "", TRUE },
	{ "rox-filer", "Edit Menu Items...", IMG_PREFIX "apps.png", "~/.suxpanel/apps", TRUE },
	{ "xterm", "Terminal", IMG_PREFIX "apps/terminal.png","-bg black -fg white", FALSE },
	{ "xfrun4", "Run Program...", IMG_PREFIX "apps/run.png", "", TRUE },
	{ "/usr/apps/ROX-Session/AppRun", "Logout", IMG_PREFIX "apps/logout.png", "", FALSE }
    };

    menu = gtk_item_factory_get_widget(factory, "<main>/Computer");
    for (i = 0; i <= G_N_ELEMENTS(prgicons); i++) {
	gchar *exec_tmp;

	exec_tmp = g_find_program_in_path(prgicons[i].program);
	if (exec_tmp) {
	    gchar *tmp;

	    item = gtk_image_menu_item_new_with_label(prgicons[i].label);
	    img = gtk_image_new_from_file_scaled(prgicons[i].icon, 24, 24);
	    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), img);
	    gtk_widget_show(GTK_WIDGET(img));
	    gtk_widget_show(item);

	    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	    if (prgicons[i].parameters[0] == '~') {
		/* substitute "~" to $HOME */
		gchar *param;

		param = g_strdup_printf("%s%s", g_get_home_dir(),
					prgicons[i].parameters + 1);
		tmp = g_strdup_printf("%s %s", exec_tmp, param);
		g_free(param);
	    } else {
		tmp = g_strdup_printf("%s %s", exec_tmp,
				      prgicons[i].parameters);
	    }

	    g_signal_connect(G_OBJECT(item), "activate",
			     (GCallback) spawn_app, tmp);

	    /* no need to free tmp */

	    if (prgicons[i].separator_after) {
		item = gtk_separator_menu_item_new();
		gtk_widget_show(GTK_WIDGET(item));

		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	    }

	    g_free(exec_tmp);
	}
    }
}

static void build_menu(SuxModule * sm)
{
    static gchar *menu_rc = "style 'menubar-style'\n"
	"{\n"
	"GtkMenuBar::shadow-type = none\n"
	"GtkMenuBar::internal-padding = 0\n"
	"}\n"
	"widget '*.sux-menu' style 'menubar-style'";
    GtkAccelGroup *group;
    GtkWidget *menu;

    group = gtk_accel_group_new();
    factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<main>", group);
    gtk_item_factory_create_items(factory, n_menu_items, menu_items, NULL);
    menu_add_icons();

    menu = gtk_item_factory_get_widget(factory, "<main>");
    gtk_widget_set_name(GTK_WIDGET(menu), "sux-menu");
    gtk_widget_show(menu);

    sm->widget = menu;

    gtk_rc_parse_string(menu_rc);
}

static gboolean inline is_appdir(const gchar * path,
				 const gchar * dir_name)
{
    gchar *buf;
    gboolean ret = FALSE;

    buf = g_strdup_printf("%s/%s/AppRun", path, dir_name);
    ret = g_file_test(buf, G_FILE_TEST_IS_EXECUTABLE);

    g_free(buf);

    return ret;
}

static gboolean inline is_groupdir(const gchar * path,
				   const gchar * dir_name)
{
    gchar *buf;
    gboolean retval;

    buf = g_strdup_printf("%s/%s", path, dir_name);
    retval = g_file_test(buf, G_FILE_TEST_IS_DIR);
    g_free(buf);

    return retval;
}

static gboolean event_handler(GtkWidget * wid, GdkEvent * ev,
			      gpointer data)
{
    if (ev->type != GDK_EXPOSE)
	return FALSE;

    if (gtk_menu_item_get_submenu(GTK_MENU_ITEM(data)) == NULL) {
	gchar *dir_name, *path, *buf;
	GtkWidget *mnu;

	dir_name = g_object_get_data(G_OBJECT(data), "dir");
	path = g_object_get_data(G_OBJECT(data), "path");

	mnu = gtk_menu_new();

	buf = g_strdup_printf("%s/%s", path ? path : app_path, dir_name);
	read_app_menu(buf, mnu);

	gtk_menu_item_set_submenu(GTK_MENU_ITEM(data), mnu);

	g_free(buf);
	g_free(dir_name);
    }

    return FALSE;
}

static void spawn_app(GtkWidget * widget, gpointer data)
{
    gchar *app, *path;

    app = NULL;
    path = (gchar *) g_object_get_data(G_OBJECT(widget), "path");

    if (path) {
	app =
	    g_strdup_printf("\"%s/%s/AppRun\"", path ? path : app_path,
			    (gchar *) data);
    } else {
	app = g_strdup((gchar *) data);
    }

    if (!g_spawn_command_line_async(app, NULL)) {
	GtkWidget *dialog;

	dialog = gtk_message_dialog_new(NULL, 0, GTK_MESSAGE_ERROR,
					GTK_BUTTONS_CLOSE,
					"Cannot fork. Yay.");
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
    }

    g_free(app);
}

/*
 * Why use a full blown XML parser? ;P
 */
static gchar *appdir_get_purpose(const gchar * path,
				 const gchar * dir_name)
{
    gchar *buf;

    buf = g_strdup_printf("%s/%s/AppInfo.xml", path, dir_name);
    if (g_file_test(buf, G_FILE_TEST_EXISTS)) {
	FILE *xml;
	gchar tmp[64], *ptr;

	xml = fopen(buf, "r");
	if (!xml) {
	    g_free(buf);
	    return NULL;
	}

	while (fgets(tmp, 64, xml)) {
	    if (strstr(tmp, "<Purpose>")) {
		gpointer first;

		ptr = tmp;

		while (*ptr && *ptr != '>')
		    ptr++;
		first = ptr + 1;
		while (*ptr && *ptr != '<')
		    ptr++;
		*ptr = 0;
		ptr = first;

		g_free(buf);
		return g_strdup(ptr);
	    }
	}
    }

    /* 
     * If no purpose is found... just return NULL.
     */
    g_free(buf);
    return NULL;
}

static GtkWidget *menu_item_from_appdir(const gchar * path,
					const gchar * dir_name)
{
    GtkWidget *item;
    GtkWidget *pix;
    gchar *buf, *purpose;

    item = gtk_image_menu_item_new_with_label(dir_name);
    gtk_widget_show(item);

    purpose = appdir_get_purpose(path, dir_name);
    if (purpose) {
	gtk_tooltips_set_tip(apps_tips, item, purpose, NULL);
	g_free(purpose);
    }

    buf = g_strdup_printf("%s/%s/.DirIcon", path, dir_name);
    if (g_file_test(buf, G_FILE_TEST_EXISTS)) {
	pix = gtk_image_new_from_file_scaled(buf, 24, 24);
	gtk_widget_show(pix);

	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), pix);
    }

    g_free(buf);

    g_signal_connect(G_OBJECT(item), "activate", (GCallback) spawn_app,
		     g_strdup(dir_name));

    return item;
}

static GtkWidget *menu_item_from_dir(const gchar * path,
				     const gchar * dir_name)
{
    GtkWidget *item, *pix;
    gchar *buf;

    item = gtk_image_menu_item_new_with_label(dir_name);
    gtk_widget_show(item);

    buf = g_strdup_printf("%s/%s/.DirIcon", path, dir_name);
    if (g_file_test(buf, G_FILE_TEST_EXISTS)) {
	pix = gtk_image_new_from_file_scaled(buf, 24, 24);
	gtk_widget_show(pix);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), pix);
    } else {
#ifdef GTK_STOCK_DIRECTORY
	pix = gtk_image_new_from_stock(GTK_STOCK_DIRECTORY, GTK_ICON_SIZE_MENU);
#else
	pix = gtk_image_new_from_stock(GTK_STOCK_OPEN, GTK_ICON_SIZE_MENU);
#endif
	gtk_widget_show(pix);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), pix);
    }

    g_free(buf);

    return item;
}

static void read_app_menu(const gchar * dir_name, GtkWidget * parent_item)
{
    DIR *dir;
    GList *dir_list = NULL, *file_list = NULL, *node = NULL;
    GtkWidget *itm;
    struct dirent *sd;

    dir = opendir(dir_name);
    if (!dir)
	return;

    while ((sd = readdir(dir))) {
	gchar *buf;

	if (sd->d_name[0] == '.')
	    continue;

	buf = g_strdup_printf("%s/%s", dir_name, sd->d_name);
	if (is_groupdir(dir_name, sd->d_name) &&
	    !is_appdir(dir_name, sd->d_name))
	    dir_list = g_list_append(dir_list, g_strdup(sd->d_name));
	else
	    file_list = g_list_append(file_list, g_strdup(sd->d_name));

	g_free(buf);
    }

    closedir(dir);

    node = g_list_sort(dir_list, (GCompareFunc) g_strcasecmp);
    for (; node; node = node->next) {
	itm = menu_item_from_dir(dir_name, node->data);

	g_object_set_data(G_OBJECT(itm), "dir", g_strdup(node->data));
	g_object_set_data(G_OBJECT(itm), "path", g_strdup(dir_name));
	gtk_menu_shell_append(GTK_MENU_SHELL(parent_item), itm);

	g_signal_connect(G_OBJECT(itm), "event", (GCallback)
			 event_handler, itm);

	g_free((gchar *) node->data);
    }
    g_list_free(node);

    if (file_list && dir_list) {
	itm = gtk_menu_item_new();
	gtk_widget_show(itm);
	gtk_menu_shell_append(GTK_MENU_SHELL(parent_item), itm);
    }

    node = g_list_sort(file_list, (GCompareFunc) g_strcasecmp);
    for (; node; node = node->next) {
	itm = menu_item_from_appdir(dir_name, node->data);

	g_object_set_data(G_OBJECT(itm), "path", g_strdup(dir_name));
	gtk_menu_shell_append(GTK_MENU_SHELL(parent_item), itm);

	g_free((gchar *) node->data);
    }
    g_list_free(node);

    g_list_free(file_list);
    g_list_free(dir_list);
}

static void build_apps_menu(const gchar * dir_name)
{
    GtkWidget *item, *img, *mnu;

    item = gtk_image_menu_item_new_with_label(menu_name);
    img = gtk_image_new_from_file_scaled(IMG_PREFIX "apps.png", 22, 22);
    gtk_widget_show(img);
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), img);
    gtk_widget_show(item);

    mnu = gtk_item_factory_get_widget(factory, "<main>");
    gtk_menu_shell_insert(GTK_MENU_SHELL(mnu), item, 0);

    if (app_path)
	g_free(app_path);

    app_path = g_strdup(dir_name);

    mnu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), mnu);

    read_app_menu(dir_name, mnu);
}

G_MODULE_EXPORT const gchar *sux_name(void)
{
    return "Application Menu";
}

G_MODULE_EXPORT void sux_init(SuxModule * sm, gchar * args)
{
    gchar *buf;

    if (args) {
	menu_name = g_strdup(g_path_get_basename(args));
    }

    buf = g_strdup_printf("%s/.suxpanel/%s", g_get_home_dir(), args ? args : "apps");
    if (g_file_test(buf, G_FILE_TEST_IS_DIR)) {
	apps_tips = gtk_tooltips_new();

	build_menu(sm);
	build_apps_menu(buf);
    } else {
	g_print("*** Did you run suxpanel-install.sh?\n");
    }

    g_free(buf);
}

G_MODULE_EXPORT void sux_fini(SuxModule * sm)
{
    gtk_widget_destroy(sm->widget);
}

G_MODULE_EXPORT void sux_about(void)
{
    GtkAbout *about;
    const gchar *msg[] = {
	">Written by:",
	"Leandro Pereira (leandro@linuxmag.com.br)",
	">Disclaimer:",
	"This is free software; you can modify and/or distribute it",
	"under the terms of GNU GPL version 2. See http://www.fsf.org/",
	"for more information.",
	NULL
    };

    about = gtk_about_new("Application Menu", "0.4",
			  "GNOME-like application menu",
			  msg, IMG_PREFIX "apps.png");
}
