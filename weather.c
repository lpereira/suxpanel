/*
 * SuxPanel version 0.4
 * Copyright (c) 2003-2005 Leandro Pereira <leandro@linuxmag.com.br>
 *
 * Weather module
 * Inspired by Per Liden's Temperature.app
 *
 * This program may be distributed under the terms of GNU General
 * Public License version 2. You should have received a copy of the
 * license with this program; if not, please consult http://www.fsf.org/.
 *
 * This program comes with no warranty. Use at your own risk.
 *
 */

#include "suxpanel.h"
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

typedef struct _WeatherModule	WeatherModule;
struct _WeatherModule {
	GtkWidget *label;
	
	gboolean fetching;
	
	gint temp;
};

static gboolean fetch_data(WeatherModule *wm);
gchar *icao_code;

WeatherModule *wm;

static gpointer
fetch_data_real(WeatherModule *wm)
{
	FILE *metar;
	gchar buf[64], *url, *tmpfile;

	if (wm->fetching)
		return NULL;
	else
		wm->fetching = TRUE;

	tmpfile = g_strdup_printf("%s/.suxpanel/weather.tmp", g_get_home_dir());
	metar = fopen(tmpfile, "w+");
	fclose(metar);
		
	url = g_strdup_printf("http://weather.noaa.gov/pub/data/observations/metar/decoded/%s.TXT",
			      icao_code);

	{
		gchar *sux;
		
		sux = g_strdup_printf("wget --cache=off --tries=0 --quiet -O %s %s",
				      tmpfile, url);

		system(sux);
		g_free(sux);
	}
	
	metar = fopen(tmpfile, "r");
	while (fgets(buf, 64, metar)) {
		if(!strncmp(buf, "Temperature", 11)) {
			gchar *tmp = buf;
				
			while (*tmp && *(tmp++) != '(');
			wm->temp = atoi(tmp);
		
			break;
		}
	}
	fclose(metar);

	unlink(tmpfile);
	g_free(url);
	g_free(tmpfile);

	wm->fetching = FALSE;

	return NULL;
}

static gboolean
fetch_data(WeatherModule *wm)
{
	fetch_data_real(wm);
				
	return TRUE;
}

static gboolean
update_lbl(WeatherModule *wm)
{
	gchar *tmp;
	

	if (wm->fetching)
		return TRUE;
			
	tmp = g_strdup_printf("%d\302\260C", wm->temp);
	gtk_label_set_text(GTK_LABEL(wm->label), tmp);

	g_free(tmp);
	
	return TRUE;
}

const gchar *
sux_name(void)
{
	return "Weather";
}

void
sux_init(SuxModule * sm)
{
	FILE *conf;
	gchar *buf;

	buf = g_strdup_printf("%s/.suxpanel/weather.ini", g_get_home_dir());
		
	conf = fopen(buf, "r");
	if (!conf)
		icao_code = "SBKP";
	else {
		gchar temp[256];
		
		fgets(temp, 256, conf);
		fclose(conf);
		
		icao_code = g_strdup(temp);
		icao_code = g_strstrip(icao_code);
	}

	wm = g_new0(WeatherModule, 1);
	wm->label = gtk_label_new("");
	gtk_widget_show(wm->label);
	sm->widget = wm->label;

	g_timeout_add(20000, (GSourceFunc)update_lbl, wm);
	fetch_data(wm);
	
	g_timeout_add(1000 * 60 * 10, (GSourceFunc)fetch_data, wm);

	update_lbl(wm);
	
	g_free(buf);
}

void
sux_fini(SuxModule * sm)
{
	gtk_widget_destroy(sm->widget);
}

void sux_about (void)
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

        about = gtk_about_new("Weather Information", "0.2",
                        "Temperature information applet",
                        msg, IMG_PREFIX "weather.png");
}

GdkPixbuf *sux_icon(void)
{
        return gdk_pixbuf_new_from_file(IMG_PREFIX "weather.png", NULL);
}
