/* Debug Logger - A UI for Log Messages
 * debug-log.c
 * Copyright (C) 2009 Les Harris <lharris@gnome.org>
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
#include  "config.h"
#endif

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <glade/glade-xml.h>
#include <string.h>
#include <stdio.h>

#include "debug-logger.h"

struct _DebugLoggerPrivate
{
    GtkVBox *vbox;
    GtkWindow *window;
    GtkNotebook *notebook;

    GtkWidget *toolbar_save;
    GtkWidget *toolbar_copy;
    GtkWidget *toolbar_select;

    GSList *domains;
};

typedef struct DomainHandler
{
    guint handler_id;
    gchar *name;
    gboolean default_domain;

    GtkTreeView *view;
} DomainHandler;

enum
{
    MSG_COLUMN,
    COLOR_COLUMN,
    N_COLUMNS
};

G_DEFINE_TYPE(DebugLogger, debug_logger, G_TYPE_OBJECT);

static void debug_logger_init(DebugLogger *self);
static void debug_logger_class_init(DebugLoggerClass *klass);
static void debug_logger_finalize(GObject *object);

static gboolean debug_logger_window_delete(GtkWidget *widget,
                                           GdkEvent *event,
                                           DebugLogger *self);

static guint debug_logger_insert_handler(DebugLogger *logger,
                                         const gchar *domain);

static DomainHandler *debug_logger_get_handler_by_name(DebugLogger *logger,
                                                       const gchar *name);

static void debug_logger_log_func (const gchar *log_domain,
                                   GLogLevelFlags log_level,
                                   const gchar *message,
                                   DebugLogger *logger);

static void debug_logger_save_clicked(GtkWidget *widget, DebugLogger *logger);
static void debug_logger_copy_clicked(GtkWidget *widget, DebugLogger *logger);
static void debug_logger_select_clicked(GtkWidget *widget, DebugLogger *logger);

/* Class Functions */
static void
debug_logger_init(DebugLogger *self)
{
    GladeXML *glade;

    self->priv = DEBUG_LOGGER_GET_PRIVATE(self);
    self->priv->domains = NULL;

#ifdef ENABLE_DEBUG_LOGGER
    glade = glade_xml_new(GLADEDIR "/main.glade", "log_window", NULL);

    self->priv->window = GTK_WINDOW(glade_xml_get_widget(glade, "log_window"));
    self->priv->vbox = GTK_VBOX(glade_xml_get_widget(glade, "vbox"));
    self->priv->toolbar_save = glade_xml_get_widget(glade, "toolbar_save");
    self->priv->toolbar_copy = glade_xml_get_widget(glade, "toolbar_copy");
    self->priv->toolbar_select = glade_xml_get_widget(glade, "toolbar_selectall");

    self->priv->notebook = GTK_NOTEBOOK(gtk_notebook_new());

    gtk_box_pack_end(GTK_BOX(self->priv->vbox), 
                     GTK_WIDGET(self->priv->notebook),
                     TRUE, TRUE, 0);

    g_signal_connect(self->priv->window, "delete-event",
                     G_CALLBACK(debug_logger_window_delete),
                     self);

    g_signal_connect(self->priv->toolbar_save, "clicked",
                     G_CALLBACK(debug_logger_save_clicked),
                     self);

    g_signal_connect(self->priv->toolbar_copy, "clicked",
                     G_CALLBACK(debug_logger_copy_clicked),
                     self);

    g_signal_connect(self->priv->toolbar_select, "clicked",
                     G_CALLBACK(debug_logger_select_clicked),
                     self);

    gtk_widget_show_all(GTK_WIDGET(self->priv->window));

    g_object_unref(glade);
#endif
}

static void
debug_logger_class_init(DebugLoggerClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->finalize = debug_logger_finalize;

    g_type_class_add_private(klass, sizeof(DebugLoggerPrivate));
}

static void
debug_logger_finalize(GObject *object)
{
    DebugLogger *self = DEBUG_LOGGER(object);
    GObjectClass *parent_class;

    parent_class = g_type_class_peek_parent(G_OBJECT_GET_CLASS(object));
    parent_class->finalize(object);
}

/* Signal Callbacks */
static gboolean
debug_logger_window_delete(GtkWidget *widget,
                           GdkEvent *event,
                           DebugLogger *self)
{
    return TRUE;
}

static void
debug_logger_save_clicked(GtkWidget *widget, DebugLogger *logger)
{
    GladeXML *glade;
    GtkWidget *dialog;
    gint result;
    GList *selected_rows = NULL, *entry;
    GtkTreeView *view;
    GtkTreeModel *model;
    GtkTreeSelection *selection;
    gint current_page;
    GString *copy_data;

    glade = glade_xml_new(GLADEDIR "/main.glade", "save_dialog", NULL);
    dialog = glade_xml_get_widget(glade, "save_dialog");

    copy_data = g_string_new(NULL);

    gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), "debug.log");
    result = gtk_dialog_run(GTK_DIALOG(dialog));

    if(result == GTK_RESPONSE_OK)
    {
        gchar *filename;

        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        current_page = gtk_notebook_get_current_page(logger->priv->notebook);

        view = 
            GTK_TREE_VIEW(gtk_bin_get_child(
                        GTK_BIN(gtk_notebook_get_nth_page(
                                GTK_NOTEBOOK(logger->priv->notebook), 
                                current_page))));

        model = gtk_tree_view_get_model(view);
        selection = gtk_tree_view_get_selection(view);

        gtk_tree_selection_select_all(selection);

        selected_rows =
            gtk_tree_selection_get_selected_rows(selection, &model);

        for(entry = selected_rows; entry != NULL; entry = g_list_next(entry))
        {
            GtkTreeIter iter;
            gchar *message;

            gtk_tree_model_get_iter(model, &iter, entry->data);
            gtk_tree_model_get(model, &iter, MSG_COLUMN, &message, -1);

            copy_data = g_string_append(copy_data, message);
            copy_data = g_string_append_c(copy_data, '\n');

            g_free(message);
        }

        gtk_tree_selection_unselect_all(selection);

        g_file_set_contents(filename, copy_data->str, copy_data->len, NULL);

        g_free(filename);
    }

    g_string_free(copy_data, TRUE);

    g_list_foreach(selected_rows, (GFunc)gtk_tree_path_free, NULL);
    g_list_free(selected_rows);

    gtk_widget_destroy(dialog);
    g_object_unref(glade);
}

static void
debug_logger_copy_clicked(GtkWidget *widget, DebugLogger *logger)
{
    GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    GList *selected_rows = NULL, *entry;
    GtkTreeView *view;
    GtkTreeModel *model;
    GtkTreeSelection *selection;
    gint current_page;
    GString *copy_data;

    copy_data = g_string_new(NULL);

    current_page = gtk_notebook_get_current_page(logger->priv->notebook);
    
    view = 
        GTK_TREE_VIEW(gtk_bin_get_child(
                GTK_BIN(gtk_notebook_get_nth_page(
                        GTK_NOTEBOOK(logger->priv->notebook), 
                        current_page))));

    model = gtk_tree_view_get_model(view);
    selection = gtk_tree_view_get_selection(view);

    selected_rows =
        gtk_tree_selection_get_selected_rows(selection, &model);

    for(entry = selected_rows; entry != NULL; entry = g_list_next(entry))
    {
        GtkTreeIter iter;
        gchar *message;

        gtk_tree_model_get_iter(model, &iter, entry->data);
        gtk_tree_model_get(model, &iter, MSG_COLUMN, &message, -1);

        copy_data = g_string_append(copy_data, message);
        copy_data = g_string_append_c(copy_data, '\n');

        g_free(message);
    }

    gtk_clipboard_set_text(clipboard, copy_data->str, copy_data->len);

    g_string_free(copy_data, TRUE);

    g_list_foreach(selected_rows, (GFunc)gtk_tree_path_free, NULL);
    g_list_free(selected_rows);
}

static void
debug_logger_select_clicked(GtkWidget *widget, DebugLogger *logger)
{
    GtkTreeView *view;
    GtkTreeSelection *selection;
    gint current_page;

    current_page = gtk_notebook_get_current_page(logger->priv->notebook);
    
    view = 
        GTK_TREE_VIEW(gtk_bin_get_child(
                GTK_BIN(gtk_notebook_get_nth_page(
                        GTK_NOTEBOOK(logger->priv->notebook), 
                        current_page))));

    selection = gtk_tree_view_get_selection(view);

    gtk_tree_selection_select_all(selection);
}

/* Private Methods */
static void 
debug_logger_log_func (const gchar *log_domain,
                       GLogLevelFlags log_level,
                       const gchar *message,
                       DebugLogger *logger)
{
#ifdef ENABLE_DEBUG_LOGGER
    GtkTreeIter iter;
    GtkListStore *store;
    DomainHandler *handler =
        debug_logger_get_handler_by_name(logger, log_domain);

    g_return_if_fail(handler != NULL);

    store = GTK_LIST_STORE(gtk_tree_view_get_model(handler->view));
    gtk_list_store_append(store, &iter);

    switch(log_level)
    {
        case G_LOG_LEVEL_CRITICAL:
            gtk_list_store_set(store, &iter, 
                    MSG_COLUMN, message,
                    COLOR_COLUMN, "#FF0000",
                    -1);
            break;

        case G_LOG_LEVEL_WARNING:
            gtk_list_store_set(store, &iter,
                    MSG_COLUMN, message,
                    COLOR_COLUMN, "#FF9C00",
                    -1);
            break;

        case G_LOG_LEVEL_MESSAGE:
            gtk_list_store_set(store, &iter,
                    MSG_COLUMN, message,
                    COLOR_COLUMN, "#000000",
                    -1);
            break;

        case G_LOG_LEVEL_INFO:
        case G_LOG_LEVEL_DEBUG:
            gtk_list_store_set(store, &iter,
                    MSG_COLUMN, message,
                    COLOR_COLUMN, "#1E8DFF",
                    -1);
            break;
    }

#else
    switch(log_level)
    {
        case G_LOG_LEVEL_CRITICAL:
            g_printf("CRITICAL ERROR: %s\n", message);
            break;

        case G_LOG_LEVEL_WARNING:
            g_printf("Warning: %s\n", message);
            break;
        
        default:
            break;
    }    
#endif
}

static guint 
debug_logger_insert_handler(DebugLogger *logger, const gchar *domain)
{
    return g_log_set_handler(domain, G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL |
           G_LOG_FLAG_RECURSION, (GLogFunc)debug_logger_log_func, logger);
}

static DomainHandler *
debug_logger_get_handler_by_name(DebugLogger *logger, const gchar *name)
{
    GSList *entry;

    for(entry = logger->priv->domains; entry != NULL; entry = g_slist_next(entry))
    {
        DomainHandler *handler = entry->data;

        if(name == NULL)
        {
            if(handler->default_domain)
                return handler;
        }
        else if (strcmp(handler->name, name) == 0)
            return handler;
    }

    return NULL;
}

/* Public Methods */
void
debug_logger_add_domain(DebugLogger *logger,
                        const gchar *domain_name,
                        gboolean default_domain)
{
    GtkWidget *tab_label, *scrolled_window,
              *treeview;
    GtkListStore *list_store;
    GtkTreeViewColumn *column;
    GtkCellRenderer *renderer;
    GtkTreeSelection *selection;
    DomainHandler *new_handler;

    g_return_if_fail(IS_DEBUG_LOGGER(logger));

    new_handler = g_new0(DomainHandler, 1);
    new_handler->view = NULL;

#ifdef ENABLE_DEBUG_LOGGER
    tab_label = gtk_label_new(domain_name);
    scrolled_window = gtk_scrolled_window_new(NULL, NULL);

    g_object_set(G_OBJECT(scrolled_window),
                 "hscrollbar-policy", GTK_POLICY_AUTOMATIC,
                 "vscrollbar-policy", GTK_POLICY_AUTOMATIC,
                 NULL);

    list_store = gtk_list_store_new(N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING);
    treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(list_store));
    g_object_unref(list_store);

    g_object_set(G_OBJECT(treeview),
                 "rubber-banding", TRUE,
                 NULL);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes ("Message",
            renderer,
            "text", MSG_COLUMN,
            "foreground", COLOR_COLUMN,
            NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);

    gtk_container_add(GTK_CONTAINER(scrolled_window), treeview);
    gtk_notebook_append_page(logger->priv->notebook, scrolled_window, tab_label);

    gtk_widget_show_all(GTK_WIDGET(logger->priv->notebook));

    new_handler->view = GTK_TREE_VIEW(treeview);
#endif

    new_handler->handler_id = 
        debug_logger_insert_handler(logger,
                (default_domain) ? NULL : domain_name);

    new_handler->name = g_strdup(domain_name);
    new_handler->default_domain = default_domain;

    logger->priv->domains = g_slist_append(logger->priv->domains, new_handler);
}

void
debug_logger_remove_domain(DebugLogger *logger,
                           const gchar *domain_name)
{
    GSList *entry;
    gint i, num_pages;

    g_return_if_fail(IS_DEBUG_LOGGER(logger));

    for(entry = logger->priv->domains; entry != NULL; entry = g_slist_next(entry))
    {
        DomainHandler *handler = entry->data;

        if(strcmp( domain_name, handler->name) == 0)
        {
            g_log_remove_handler(
                    (handler->default_domain) ? NULL : handler->name,
                    handler->handler_id);

#ifdef ENABLE_DEBUG_LOGGER
            num_pages = gtk_notebook_get_n_pages(logger->priv->notebook);
            for(i = 0; i < num_pages; ++i)
            {
                GtkWidget *current_page =
                    gtk_notebook_get_nth_page(logger->priv->notebook, i);

                if( strcmp(domain_name,
                           gtk_notebook_get_tab_label_text(logger->priv->notebook, 
                               current_page)) == 0)
                {
                    gtk_notebook_remove_page(logger->priv->notebook, i);
                    break;
                }
            }
#endif

            if(handler->name)
                g_free(handler->name);

            logger->priv->domains = g_slist_remove(logger->priv->domains, handler);
        }
    }
}

