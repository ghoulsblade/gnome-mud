/* GNOME-Mud - A simple Mud CLient
 * Copyright (C) 1998-2002 Robin Ericsson <lobbin@localhost.nu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gnome.h>
#include <stdio.h>

#include "gnome-mud.h"

static char const rcsid[] = "$Id$";

extern SYSTEM_DATA prefs;

struct _mudentry{
	gchar     *name;
	GList     *list;
};

struct _mudcode{
	gchar     *key;
	gchar     *value;
};

typedef struct _mudentry mudentry;
typedef struct _mudcode  mudcode;

static char *mudlist_fix_codebase(gchar *codebase)
{
	mudcode replacements[] = 
	{
		{ "Abermud",         "Aber"      },
		{ "Amylaar's",       "Amylaar"   },
		{ "Cold",            "ColdMud"   },
		{ "coldC",           "ColdMud"   },
		{ "Circlemud",       "Circle"    },
		{ "Circle3",         "Circle"    },
		{ "Circle3.0",       "Circle"    },
		{ "CircleMUD30pl12", "Circle"    },
		{ "Dikumud",         "Diku"      },
		{ "Envymud",         "Envy"      },
		{ "EW-too",          "EwToo"     },
		{ "Lambda",          "LambdaMOO" },
		{ "LPC",             "LP"        },
		{ "LPMud",           "LP"        },
		{ "Rom2.4",          "Rom"       },
		{ "ROM24b4",         "Rom"       },
		{ "SillyMUD",        "Silly"     },
		{ NULL,              NULL        }
	};

	gint i = 0;

	while(replacements[i].key)
	{
		if (!g_strcasecmp(replacements[i].key, codebase))
		{
			return replacements[i].value;
		}

		i++;
	}
	
	return codebase;
}

static void mudlist_tree_fill_subtree(gpointer item, gpointer tree)
{
	gtk_tree_append(GTK_TREE(tree), GTK_WIDGET(item));
	gtk_widget_show(item);
}

static void mudlist_tree_fill(gpointer e, gpointer tree)
{
	mudentry *entry = (mudentry *) e;
	GtkWidget *item;
	GtkWidget *subtree;
	
	item = gtk_tree_item_new_with_label(entry->name);
	gtk_tree_append(GTK_TREE(tree), item);

	subtree = gtk_tree_new();
	gtk_tree_item_set_subtree(GTK_TREE_ITEM(item), subtree);

	g_list_foreach(entry->list, mudlist_tree_fill_subtree, subtree);
	g_list_free(entry->list);
	g_free(entry->name);
	g_free(entry);
	
	gtk_widget_show_all(tree);
}

static gint mudlist_compare_char_struct(gconstpointer a, gconstpointer b)
{
	mudentry *e = (mudentry *) a;

	return (g_strcasecmp(e->name, b));
}

static gint mudlist_compare_structs(gconstpointer a, gconstpointer b)
{
	mudentry *c = (mudentry *) a;
	mudentry *d = (mudentry *) b;
	
	return g_strcasecmp(c->name, d->name);
}

static void mudlist_parse_hoststring(gchar *string, gchar *host, gchar *port)
{
	gchar f[1];
	gint  stat = 0;
	
	while(*string)
	{
		if (stat == 1)
		{
			if (*string == ' ')
				break;
			else
				*port++ = *string;
		}
		
		if (stat == 0)
		{
			if (*string == ' ')
				stat = 1;
			else
				*host++ = *string;
		}	

		*f = *string++;
	}

}

static void mudlist_button_connect_cb(GtkWidget *button, GtkWidget *entry)
{
	gchar host[1024] = "";
	gchar port[10] = "";
	gchar *d = gtk_entry_get_text(GTK_ENTRY(entry));

	mudlist_parse_hoststring(d, host, port);

	make_connection(host, port, "Default");
}

static void mudlist_button_import_cb(GtkWidget *widget, GtkWidget *button)
{
	gchar host[1024] = "";
	gchar port[10]   = "";
	gchar *d = gtk_entry_get_text(GTK_ENTRY(gtk_object_get_data(GTK_OBJECT(widget), "entry_telnet")));

	mudlist_parse_hoststring(d, host, port);

	gtk_entry_set_text(GTK_ENTRY(gtk_object_get_data(GTK_OBJECT(button), "entry_info_mud_title")), gtk_entry_get_text(GTK_ENTRY(gtk_object_get_data(GTK_OBJECT(widget), "entry_name"))));
	gtk_entry_set_text(GTK_ENTRY(gtk_object_get_data(GTK_OBJECT(button), "entry_info_mud_host")), host);
	gtk_entry_set_text(GTK_ENTRY(gtk_object_get_data(GTK_OBJECT(button), "entry_info_mud_port")), port);
}

static void mudlist_select_item_cb(GtkTreeItem *item, GtkTree *tree)
{
	FILE *fp;
	glong floc = (glong) gtk_object_get_data(GTK_OBJECT(item), "floc");
	gchar line[1024];
	
	fp = fopen(prefs.MudListFile, "r");
	fseek(fp, floc, SEEK_SET);
	while(fgets(line, 1024, fp))
	{
		if (!strncmp("Mud         :", line, 13))
		{
			GtkWidget *entry = gtk_object_get_data(GTK_OBJECT(tree), "entry_name");

			gtk_entry_set_text(GTK_ENTRY(entry), line + 14);
		}
		else if (!strncmp("Code Base   :", line, 13))
		{
			GtkWidget *entry = gtk_object_get_data(GTK_OBJECT(tree), "entry_codebase");

			gtk_entry_set_text(GTK_ENTRY(entry), line + 14);
		}
		else if (!strncmp("WWW         :", line, 13))
		{
			GtkWidget *href = gtk_object_get_data(GTK_OBJECT(tree), "href_www");
			
			if (!strncmp("None", line + 14, 4))
			{
				gtk_widget_set_sensitive(href, FALSE);
			}
			else
			{
				gtk_widget_set_sensitive(href, TRUE);
				gnome_href_set_url(GNOME_HREF(href), line + 14);
			}
		}
		else if (!strncmp("Telnet      :", line, 13))
		{
			GtkWidget *entry  = gtk_object_get_data(GTK_OBJECT(tree), "entry_telnet");
			GtkWidget *button = gtk_object_get_data(GTK_OBJECT(tree), "button_connect");
			
			gtk_entry_set_text(GTK_ENTRY(entry), line + 14);
			gtk_widget_set_sensitive(button, TRUE);
		}
		else if (!strncmp("Description :", line, 13))
		{
			GString *descr = g_string_new("");
			GtkWidget *text_desc = gtk_object_get_data(GTK_OBJECT(tree), "text_desc");
			
			while(fgets(line, 1024, fp))
			{
				if (!strncmp("                -", line, 17))
					break;
			
				g_string_append(descr, line);
			}

			gtk_text_freeze(GTK_TEXT(text_desc));
			gtk_text_set_point(GTK_TEXT(text_desc), 0);
			gtk_text_forward_delete(GTK_TEXT(text_desc), gtk_text_get_length(GTK_TEXT(text_desc)));
			gtk_text_insert(GTK_TEXT(text_desc), NULL, NULL, NULL, descr->str, -1);
			gtk_text_thaw(GTK_TEXT(text_desc));
				
			g_string_free(descr, TRUE);
			break;
		}
	}
	fclose(fp);
}

static void mudlist_parse(FILE *fp, GtkWidget *tree)
{
	//GHashTable *codehash;
	GList      *codelist = NULL;
	gchar       line[1024];
	gchar      *name = NULL;
	glong       floc = 0, floc_name = 0;
	
	rewind(fp);
	while(fgets(line, 1024, fp))
	{
		if (!strncmp("Mud         :", line, 13))
		{
			gchar tmp[1024] = "";
			gchar *p = line + 14;
			gchar *c = tmp;
			
			while(*p)
			{
				if (*p == '\n') break;

				 *c++ = *p++;
			}
		
			g_free(name);
			name = g_strdup(tmp);
			floc_name = floc;
		}
		else if (!strncmp("Code Base   :", line, 13))
		{
			GtkWidget *itemm;
			mudentry  *e = NULL;
			GList	  *subtree = NULL;
			gchar     *p = (line + 14);
			gchar      code[2048] = "";
			gchar     *realname;
			gchar     *c = code;
			gchar	   f[1];
			
			while(*p)
			{
				if (*p == ' ' || *p == '\n' || *p == '/' || *p == ',')
					break;

				switch (*p)
				{
					case '[':
					case ']':
						*f = *p++;
						continue;
				}
				*c++ = *p;

				if (*p == '%')
					*c++ = '%';
				
				*f = *p++;	
			}

			realname = mudlist_fix_codebase(code);
			
			if (codelist != NULL)
			{
				subtree = g_list_find_custom(codelist, realname, mudlist_compare_char_struct);
			}
			
			if (subtree == NULL)
			{
				e = g_malloc0(sizeof(mudentry));

				e->name = g_strdup(realname);

				codelist = g_list_append(codelist, e);
			}
			else
			{
				e = subtree->data;
			}

			itemm = gtk_tree_item_new_with_label(name);
			gtk_object_set_data(GTK_OBJECT(itemm), "floc", (gpointer) floc_name);
			gtk_signal_connect(GTK_OBJECT(itemm), "select", mudlist_select_item_cb, tree);
			e->list = g_list_append(e->list, itemm);
		}

		floc = ftell(fp);
	}

	codelist = g_list_sort(codelist, mudlist_compare_structs);
	g_list_foreach(codelist, mudlist_tree_fill, tree);
	g_list_free(codelist);
}

void window_mudlist (GtkWidget *widget, gboolean wizard)
{
	static GtkWidget *mudlist_window;
	GtkWidget  *table;
	GtkWidget  *label_name;
	GtkWidget  *entry_name;
	GtkWidget  *label_codebase;
	GtkWidget  *entry_codebase;
	GtkWidget  *label_telnet;
	GtkWidget  *entry_telnet;
	GtkWidget  *href_www;
	GtkWidget  *label_desc;
	GtkWidget  *scrolledwindow;
	GtkWidget  *scrolledwindow_tree;
	GtkWidget  *text_desc;
	GtkWidget  *mudtree;
	GtkWidget  *box;
	GtkWidget  *button_connect;
	FILE       *fp;
	
	if (mudlist_window != NULL)
	{
		gdk_window_raise(mudlist_window->window);
		gdk_window_show(mudlist_window->window);
		return;
	}
	
	fp = fopen(prefs.MudListFile, "r");
	if (!fp)
	{
		popup_window(_("Could not open MudList file for reading")); 
		return;
	}

	mudlist_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_object_set_data (GTK_OBJECT (mudlist_window), "mudlist_window", mudlist_window);
	gtk_window_set_title (GTK_WINDOW (mudlist_window), _("GNOME-Mud MudList"));
	
	table = gtk_table_new (7, 2, FALSE);
	gtk_widget_ref (table);
	gtk_object_set_data_full (GTK_OBJECT (mudlist_window), "table", table, (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (table);
	gtk_container_add (GTK_CONTAINER (mudlist_window), table);
	gtk_container_set_border_width (GTK_CONTAINER (table), 5);
	gtk_table_set_col_spacings (GTK_TABLE (table), 3);

	label_name = gtk_label_new (_("Mud Name :"));
	gtk_widget_ref (label_name);
	gtk_object_set_data_full (GTK_OBJECT (mudlist_window), "label_name", label_name, (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (label_name);
	gtk_table_attach (GTK_TABLE (table), label_name, 1, 2, 0, 1, (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (label_name), 0, 0.5);

	entry_name = gtk_entry_new ();
	gtk_widget_ref (entry_name);
	gtk_object_set_data_full (GTK_OBJECT (mudlist_window), "entry_name", entry_name, (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (entry_name);
	gtk_table_attach (GTK_TABLE (table), entry_name, 1, 2, 1, 2, (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), (GtkAttachOptions) (0), 0, 0);
	gtk_entry_set_editable (GTK_ENTRY (entry_name), FALSE);

	label_codebase = gtk_label_new (_("Codebase :"));
	gtk_widget_ref (label_codebase);
	gtk_object_set_data_full (GTK_OBJECT (mudlist_window), "label_codebase", label_codebase, (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (label_codebase);
	gtk_table_attach (GTK_TABLE (table), label_codebase, 1, 2, 2, 3, (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (label_codebase), 0, 0.5);

	entry_codebase = gtk_entry_new ();
	gtk_widget_ref (entry_codebase);
	gtk_object_set_data_full (GTK_OBJECT (mudlist_window), "entry_codebase", entry_codebase, (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (entry_codebase);
	gtk_table_attach (GTK_TABLE (table), entry_codebase, 1, 2, 3, 4, (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), (GtkAttachOptions) (0), 0, 0);
	gtk_entry_set_editable (GTK_ENTRY (entry_codebase), FALSE);

	label_telnet = gtk_label_new (_("Telnet Address :"));
	gtk_widget_show(label_telnet);
	gtk_table_attach(GTK_TABLE(table), label_telnet, 1, 2, 4, 5, (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label_telnet), 0, 0.5);

	entry_telnet = gtk_entry_new();
	gtk_widget_show(entry_telnet);
	gtk_table_attach(GTK_TABLE(table), entry_telnet, 1, 2, 5, 6, (GtkAttachOptions) (GTK_FILL), 0, 0, 0);
	gtk_entry_set_editable(GTK_ENTRY(entry_telnet), FALSE);
	
	box = gtk_hbox_new(FALSE, 10);
	gtk_widget_show(box);
	gtk_table_attach (GTK_TABLE (table), box, 1, 2, 6, 7, (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);

	if (!wizard)
	{
		button_connect = gtk_button_new_with_label(_("Connect to the mud"));
	}
	else
	{
		button_connect = gtk_button_new_with_label(_("Import and close"));
	}
	gtk_button_set_relief(GTK_BUTTON(button_connect), GTK_RELIEF_NONE);
	gtk_widget_show(button_connect);
	gtk_container_add(GTK_CONTAINER(box), button_connect);
	gtk_widget_set_sensitive(button_connect, FALSE);
	
	href_www = gnome_href_new ("", _("Go to webpage of the mud"));
	gtk_widget_ref (href_www);
	gtk_object_set_data_full (GTK_OBJECT (mudlist_window), "href_www", href_www, (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (href_www);
	gtk_widget_set_sensitive(href_www, FALSE);
	gtk_container_add(GTK_CONTAINER(box), href_www);
	
	label_desc = gtk_label_new (_("Description :"));
	gtk_widget_ref (label_desc);
	gtk_object_set_data_full (GTK_OBJECT (mudlist_window), "label_desc", label_desc, (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (label_desc);
	gtk_table_attach (GTK_TABLE (table), label_desc, 1, 2, 7, 8, (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (label_desc), 0, 0.5);

	scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_ref (scrolledwindow);
	gtk_object_set_data_full (GTK_OBJECT (mudlist_window), "scrolledwindow", scrolledwindow, (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (scrolledwindow);
	gtk_table_attach (GTK_TABLE (table), scrolledwindow, 1, 2, 8, 9, (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	text_desc = gtk_text_new (NULL, NULL);
	gtk_widget_ref (text_desc);
	gtk_object_set_data_full (GTK_OBJECT (mudlist_window), "text_desc", text_desc, (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_set_usize(text_desc, 225, 200);
	gtk_widget_show (text_desc);
	gtk_container_add (GTK_CONTAINER (scrolledwindow), text_desc);
	gtk_text_set_word_wrap(GTK_TEXT(text_desc), TRUE);
	
	scrolledwindow_tree = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_usize(scrolledwindow_tree, 150, 200);
	gtk_widget_ref(scrolledwindow_tree);
	gtk_widget_show(scrolledwindow_tree);
	gtk_table_attach (GTK_TABLE (table), scrolledwindow_tree, 0, 1, 0, 9, (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow_tree), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	
	mudtree = gtk_tree_new ();
	gtk_widget_ref (mudtree);
	gtk_object_set_data_full (GTK_OBJECT (mudlist_window), "mudtree", mudtree, (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (mudtree);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolledwindow_tree), mudtree);
	gtk_object_set_data(GTK_OBJECT(mudtree), "entry_name", entry_name); 
	gtk_object_set_data(GTK_OBJECT(mudtree), "entry_codebase", entry_codebase);
	gtk_object_set_data(GTK_OBJECT(mudtree), "href_www", href_www);
	gtk_object_set_data(GTK_OBJECT(mudtree), "entry_telnet", entry_telnet);
	gtk_object_set_data(GTK_OBJECT(mudtree), "text_desc", text_desc);
	gtk_object_set_data(GTK_OBJECT(mudtree), "button_connect", button_connect);
	
	/*
	 * Signals
	 */
	gtk_signal_connect(GTK_OBJECT(mudlist_window), "destroy", GTK_SIGNAL_FUNC(gtk_widget_destroyed), &mudlist_window);

	if (!wizard)
	{
		gtk_signal_connect(GTK_OBJECT(button_connect), "clicked", GTK_SIGNAL_FUNC(mudlist_button_connect_cb), entry_telnet);
	}
	else
	{
		gtk_object_set_data(GTK_OBJECT(button_connect), "entry_telnet", entry_telnet);
		gtk_object_set_data(GTK_OBJECT(button_connect), "entry_name",   entry_name);
		gtk_signal_connect(GTK_OBJECT(button_connect), "clicked", GTK_SIGNAL_FUNC(mudlist_button_import_cb), widget);
		gtk_signal_connect_object(GTK_OBJECT(button_connect), "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy), (gpointer) mudlist_window);
	}
	
	mudlist_parse(fp, mudtree);	
	gtk_widget_show(mudlist_window);

	fclose(fp);
}
