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
#include <gtk/gtktree.h>
#include <stdio.h>

#include "gnome-mud.h"

static char const rcsid[] = "$Id$";

extern SYSTEM_DATA prefs;

struct _mudentry{
	gchar     *name;
	glong     floc;
	GList     *list;
};

struct _mudcode{
	gchar     *key;
	gchar     *value;
};

struct _muddata{
	GtkWidget *name;
	GtkWidget *codebase;
	GtkWidget *www;
	GtkWidget *description;
	GtkWidget *button;
	GtkWidget *telnet;
};

typedef struct _mudentry mudentry;
typedef struct _mudcode  mudcode;
typedef struct _muddata  muddata;

muddata wd;

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

static void mudlist_tree_fill_subtree(gpointer item, GtkTreeStore *tree, GtkTreeIter *iter)
{
	GtkTreeIter child;
	mudentry *e = (mudentry *) item;

	gtk_tree_store_append(tree, &child, iter);
	gtk_tree_store_set(tree, &child, 0, e->name, 1, e->floc, -1);
}

static void mudlist_tree_fill(gpointer e, gpointer data)
{
	GList *list;
	GtkTreeStore *tree = GTK_TREE_STORE(data);
	GtkTreeIter iter;
	mudentry *entry = (mudentry *) e;
	
	gtk_tree_store_append(tree, &iter, NULL);
	gtk_tree_store_set(tree, &iter, 0, entry->name, -1);

	for (list = g_list_first(entry->list); list != NULL; list = g_list_next(list))
	{
		mudlist_tree_fill_subtree(list->data, tree, &iter);
	}

	g_list_free(entry->list);
	g_free(entry->name);
	g_free(entry);
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

static void mudlist_parse_hoststring(const gchar *string, gchar *host, gchar *port)
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
	const gchar *d = gtk_entry_get_text(GTK_ENTRY(entry));

	mudlist_parse_hoststring(d, host, port);

	make_connection(host, port, "Default");
}

static void mudlist_button_import_cb(GtkWidget *widget, GtkWidget *button)
{
	gchar host[1024] = "";
	gchar port[10]   = "";
	const gchar *d = gtk_entry_get_text(GTK_ENTRY(gtk_object_get_data(GTK_OBJECT(widget), "entry_telnet")));

	mudlist_parse_hoststring(d, host, port);

	gtk_entry_set_text(GTK_ENTRY(gtk_object_get_data(GTK_OBJECT(button), "entry_info_mud_title")), gtk_entry_get_text(GTK_ENTRY(gtk_object_get_data(GTK_OBJECT(widget), "entry_name"))));
	gtk_entry_set_text(GTK_ENTRY(gtk_object_get_data(GTK_OBJECT(button), "entry_info_mud_host")), host);
	gtk_entry_set_text(GTK_ENTRY(gtk_object_get_data(GTK_OBJECT(button), "entry_info_mud_port")), port);
}

static void mudlist_select_item_cb(GtkTreeSelection *selection, gpointer data)
{
	GtkTreeIter iter;
	GtkTreeModel *model;

	FILE *fp;
	gchar line[1024], tmp[1024];

	glong floc;

	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gtk_tree_model_get(model, &iter, 1, &floc, -1);
	}

	if (floc != 0)
	{
		fp = fopen(prefs.MudListFile, "r");
		fseek(fp, floc, SEEK_SET);
		while(fgets(line, 1024, fp))
		{
			if (!strncmp("Mud         :", line, 13))
			{
				GtkWidget *entry = wd.name;

				g_snprintf(tmp, strlen(line + 14), line + 14);
				gtk_entry_set_text(GTK_ENTRY(entry), tmp);
			}
			else if (!strncmp("Code Base   :", line, 13))
			{
				GtkWidget *entry = wd.codebase;

				g_snprintf(tmp, strlen(line + 14), line + 14);
				gtk_entry_set_text(GTK_ENTRY(entry), tmp);
			}
			else if (!strncmp("WWW         :", line, 13))
			{
				GtkWidget *href = wd.www;
			
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
				GtkWidget *entry  = wd.telnet;
				GtkWidget *button = wd.button;
			
				g_snprintf(tmp, strlen(line + 14), line + 14);
				gtk_entry_set_text(GTK_ENTRY(entry), tmp);
				gtk_widget_set_sensitive(button, TRUE);
			}
			else if (!strncmp("Description :", line, 13))
			{
				GtkTextBuffer *buffer;
				GString *descr = g_string_new("");
				GtkWidget *text_desc = wd.description;
				
				while(fgets(line, 1024, fp))
				{
					if (!strncmp("                -", line, 17))
						break;
				
					g_string_append(descr, line);
				}

				buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_desc));
				gtk_text_buffer_set_text(buffer, descr->str, -1);

				g_string_free(descr, TRUE);
				break;
			}
		}
		fclose(fp);
	}
}

static void mudlist_parse(FILE *fp, GtkTreeStore *tree)
{
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
			mudentry  *e = NULL, *ee;
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

			ee = g_malloc0(sizeof(mudentry));
			ee->name = g_strdup(name);
			ee->floc = floc_name;

			e->list = g_list_append(e->list, ee);
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
	
	GtkTreeStore *mudtree_store;
	GtkTreeViewColumn *tree_column;
	GtkTreeSelection *selection;
	GtkCellRenderer *renderer;

	FILE       *fp;
	
	if (mudlist_window != NULL)
	{
		gtk_window_present (GTK_WINDOW (mudlist_window));
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

	wd.name = entry_name = gtk_entry_new ();
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

	wd.codebase = entry_codebase = gtk_entry_new ();
	gtk_widget_ref (entry_codebase);
	gtk_object_set_data_full (GTK_OBJECT (mudlist_window), "entry_codebase", entry_codebase, (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (entry_codebase);
	gtk_table_attach (GTK_TABLE (table), entry_codebase, 1, 2, 3, 4, (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), (GtkAttachOptions) (0), 0, 0);
	gtk_entry_set_editable (GTK_ENTRY (entry_codebase), FALSE);

	label_telnet = gtk_label_new (_("Telnet Address :"));
	gtk_widget_show(label_telnet);
	gtk_table_attach(GTK_TABLE(table), label_telnet, 1, 2, 4, 5, (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label_telnet), 0, 0.5);

	wd.telnet = entry_telnet = gtk_entry_new();
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
	wd.button = button_connect;
	gtk_button_set_relief(GTK_BUTTON(button_connect), GTK_RELIEF_NONE);
	gtk_widget_show(button_connect);
	gtk_container_add(GTK_CONTAINER(box), button_connect);
	gtk_widget_set_sensitive(button_connect, FALSE);
	
	wd.www = href_www = gnome_href_new ("", _("Go to webpage of the mud"));
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
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow), GTK_SHADOW_IN);

	wd.description = text_desc = gtk_text_view_new();
	gtk_widget_ref (text_desc);
	gtk_object_set_data_full (GTK_OBJECT (mudlist_window), "text_desc", text_desc, (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_set_usize(text_desc, 350, 200);
	gtk_widget_show (text_desc);
	gtk_container_add (GTK_CONTAINER (scrolledwindow), text_desc);

	scrolledwindow_tree = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_usize(scrolledwindow_tree, 150, 200);
	gtk_widget_ref(scrolledwindow_tree);
	gtk_widget_show(scrolledwindow_tree);
	gtk_table_attach (GTK_TABLE (table), scrolledwindow_tree, 0, 1, 0, 9, (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow_tree), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	mudtree_store = gtk_tree_store_new(2, G_TYPE_STRING, G_TYPE_LONG);
	mudtree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(mudtree_store));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(mudtree), FALSE);
	g_object_unref(mudtree_store);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(mudtree));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
	g_signal_connect(selection, "changed", G_CALLBACK(mudlist_select_item_cb), NULL);

	renderer = gtk_cell_renderer_text_new();
	g_object_set(G_OBJECT(renderer), "foreground", "black", NULL);

	tree_column = gtk_tree_view_column_new_with_attributes(NULL, renderer, "text", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(mudtree), tree_column);

	gtk_widget_ref (mudtree);
	gtk_object_set_data_full (GTK_OBJECT (mudlist_window), "mudtree", mudtree, (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (mudtree);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolledwindow_tree), mudtree);
	
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
	
	mudlist_parse(fp, GTK_TREE_STORE(mudtree_store));
	gtk_widget_show(mudlist_window);

	fclose(fp);
}
