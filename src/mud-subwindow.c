/* GNOME-Mud - A simple Mud Client
 * mud-subwindow.c
 * Copyright (C) 2005-2009 Les Harris <lharris@gnome.org>
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
#include "config.h"
#endif

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <vte/vte.h>
#include <glade/glade-xml.h>
#include <gconf/gconf-client.h>
#include <gdk/gdkkeysyms.h>

#include "gnome-mud.h"
#include "gnome-mud-marshallers.h"
#include "mud-connection-view.h"
#include "mud-subwindow.h"

struct _MudSubwindowPrivate
{
    gchar *title;
    gchar *identifier;

    guint width;
    guint height;

    gboolean visible;
    gboolean input_enabled;

    GQueue *history;
    gint current_history_index;

    gint pixel_width;
    gint pixel_height;

    GtkWidget *window;
    GtkWidget *entry;
    GtkWidget *terminal;
    GtkWidget *scroll;
    GtkWidget *vbox;

    MudConnectionView *parent_view;
};

enum MudSubwindowHistoryDirection
{
    SUBWINDOW_HISTORY_UP,
    SUBWINDOW_HISTORY_DOWN
};

/* Property Identifiers */
enum
{
    PROP_MUD_SUBWINDOW_0,
    PROP_PARENT,
    PROP_TITLE,
    PROP_IDENT,
    PROP_WIDTH,
    PROP_HEIGHT,
    PROP_VISIBLE,
    PROP_INPUT
};

/* Signal Indices */
enum
{
    RESIZED,
    INPUT,
    LAST_SIGNAL
};

/* Signal Identifier Map */
static guint mud_subwindow_signal[LAST_SIGNAL] = { 0, 0 };

/* Create the Type */
G_DEFINE_TYPE(MudSubwindow, mud_subwindow, G_TYPE_OBJECT);

/* Class Functions */
static void mud_subwindow_init (MudSubwindow *self);
static void mud_subwindow_class_init (MudSubwindowClass *klass);
static void mud_subwindow_finalize (GObject *object);
static GObject *mud_subwindow_constructor (GType gtype,
                                           guint n_properties,
                                           GObjectConstructParam *properties);
static void mud_subwindow_set_property(GObject *object,
                                       guint prop_id,
                                       const GValue *value,
                                       GParamSpec *pspec);
static void mud_subwindow_get_property(GObject *object,
                                       guint prop_id,
                                       GValue *value,
                                       GParamSpec *pspec);

/* Callback Functions */
static gboolean mud_subwindow_delete_event_cb(GtkWidget *widget,
		                   	      GdkEvent *event,
			                      MudSubwindow *self);

static gboolean mud_subwindow_entry_keypress_cb(GtkWidget *widget,
                                                GdkEventKey *event,
                                                MudSubwindow *self);

static gboolean mud_subwindow_configure_event_cb(GtkWidget *widget,
                                                 GdkEventConfigure *event,
                                                 gpointer user_data);

/* Private Methods */
static void mud_subwindow_set_size_force_grid (MudSubwindow *window,
                                               VteTerminal *screen,
                                               gboolean        even_if_mapped,
                                               int             force_grid_width,
                                               int             force_grid_height);

const gchar *mud_subwindow_get_history_item(MudSubwindow *self,
                                            enum MudSubwindowHistoryDirection direction);

static void mud_subwindow_update_geometry (MudSubwindow *window);
static void mud_subwindow_set_terminal_colors(MudSubwindow *self);
static void mud_subwindow_set_terminal_scrollback(MudSubwindow *self);
static void mud_subwindow_set_terminal_scrolloutput(MudSubwindow *self);
static void mud_subwindow_set_terminal_font(MudSubwindow *self);

/* MudSubwindow class functions */
static void
mud_subwindow_class_init (MudSubwindowClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    /* Override base object constructor */
    object_class->constructor = mud_subwindow_constructor;

    /* Override base object's finalize */
    object_class->finalize = mud_subwindow_finalize;

    /* Override base object property methods */
    object_class->set_property = mud_subwindow_set_property;
    object_class->get_property = mud_subwindow_get_property;

    /* Add private data to class */
    g_type_class_add_private(klass, sizeof(MudSubwindowPrivate));

    /* Install Properties */
    g_object_class_install_property(object_class,
            PROP_PARENT,
            g_param_spec_object("parent-view",
                "Parent View",
                "The parent MudSubwindow",
                MUD_TYPE_CONNECTION_VIEW,
                G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY));

    g_object_class_install_property(object_class,
            PROP_TITLE,
            g_param_spec_string("title",
                "Title",
                "The visible Title of the subwindow.",
                NULL,
                G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY));

    g_object_class_install_property(object_class,
            PROP_IDENT,
            g_param_spec_string("identifier",
                "Ident",
                "The identifier of the subwindow.",
                NULL,
                G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY));

    g_object_class_install_property(object_class,
            PROP_WIDTH,
            g_param_spec_uint("width",
                "Width",
                "The width of the terminal in columns.",
                1,
                1024,
                40,
                G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY));

    g_object_class_install_property(object_class,
            PROP_HEIGHT,
            g_param_spec_uint("height",
                "Height",
                "The height of the terminal in rows.",
                1,
                1024,
                40,
                G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY));

    g_object_class_install_property(object_class,
            PROP_VISIBLE,
            g_param_spec_boolean("visible",
                "Visible",
                "True if subwindow is visible.",
                TRUE,
                G_PARAM_READWRITE));

    g_object_class_install_property(object_class,
            PROP_INPUT,
            g_param_spec_boolean("input-enabled",
                "Input Enabled",
                "True if subwindow accepts input.",
                TRUE,
                G_PARAM_READWRITE));

    /* Register Signals */
    mud_subwindow_signal[RESIZED] =
        g_signal_new("resized",
                     G_TYPE_FROM_CLASS(object_class),
                     G_SIGNAL_RUN_LAST | G_SIGNAL_NO_HOOKS,
                     0,
                     NULL,
                     NULL,
                     gnome_mud_cclosure_VOID__UINT_UINT,
                     G_TYPE_NONE,
                     2,
                     G_TYPE_UINT,
                     G_TYPE_UINT);

    mud_subwindow_signal[INPUT] =
        g_signal_new("input-received",
                     G_TYPE_FROM_CLASS(object_class),
                     G_SIGNAL_RUN_LAST | G_SIGNAL_NO_HOOKS,
                     0,
                     NULL,
                     NULL,
                     g_cclosure_marshal_VOID__STRING,
                     G_TYPE_NONE,
                     1,
                     G_TYPE_STRING); 

}

static void
mud_subwindow_init (MudSubwindow *self)
{
    /* Get our private data */
    self->priv = MUD_SUBWINDOW_GET_PRIVATE(self);

    /* Set Defaults */
    self->priv->parent_view = NULL;
    self->priv->title = NULL;
    self->priv->identifier = NULL;
    self->priv->visible = TRUE;
    self->priv->input_enabled = TRUE;
    self->priv->history = g_queue_new();
    self->priv->current_history_index = 0;
    self->priv->width = 0;
    self->priv->height = 0;

    self->priv->window = NULL;
    self->priv->entry = NULL;
    self->priv->scroll = NULL;
    self->priv->terminal = NULL;
    self->priv->vbox = NULL;
}

static GObject *
mud_subwindow_constructor (GType gtype,
                           guint n_properties,
                           GObjectConstructParam *properties)
{
    GtkWidget *term_box;

    MudSubwindow *self;
    GObject *obj;
    MudSubwindowClass *klass;
    GObjectClass *parent_class;

    GladeXML *glade;

    /* Chain up to parent constructor */
    klass = MUD_SUBWINDOW_CLASS( g_type_class_peek(MUD_TYPE_SUBWINDOW) );
    parent_class = G_OBJECT_CLASS( g_type_class_peek_parent(klass) );
    obj = parent_class->constructor(gtype, n_properties, properties);

    self = MUD_SUBWINDOW(obj);

    if(!self->priv->parent_view)
    {
        g_printf("ERROR: Tried to instantiate MudSubwindow without passing parent view\n");
        g_error("Tried to instantiate MudSubwindow without passing parent view");
    }

    if(!self->priv->title)
    {
        g_printf("ERROR: Tried to instantiate MudSubwindow without passing title\n");
        g_error("Tried to instantiate MudSubwindow without passing title.");
    }

    if(!self->priv->identifier)
    {
        g_printf("ERROR: Tried to instantiate MudSubwindow without passing identifier\n");
        g_error("Tried to instantiate MudSubwindow without passing identifier.");
    }

    if(self->priv->width == 0 || self->priv->height == 0)
    {
        g_printf("ERROR: Tried to instantiate MudSubwindow without passing valid width/height\n");
        g_error("Tried to instantiate MudSubwindow without passing valid width/height.");
    }

    /* start glading */
    glade = glade_xml_new(GLADEDIR "/main.glade", "subwindow", NULL);

    self->priv->window = glade_xml_get_widget(glade, "subwindow");

    g_object_unref(glade);

    self->priv->vbox = gtk_vbox_new(FALSE, 0);
    self->priv->entry = gtk_entry_new();

    self->priv->terminal = vte_terminal_new();
    self->priv->scroll = gtk_vscrollbar_new(NULL);
    term_box = gtk_hbox_new(FALSE, 0);

    vte_terminal_set_encoding(VTE_TERMINAL(self->priv->terminal),
                              "ISO-8859-1");

    vte_terminal_set_emulation(VTE_TERMINAL(self->priv->terminal),
                               "xterm");

    gtk_box_pack_start(GTK_BOX(term_box),
                       self->priv->terminal,
                       TRUE,
                       TRUE,
                       0);

    gtk_box_pack_end(GTK_BOX(term_box),
                     self->priv->scroll, 
                     FALSE,
                     FALSE,
                     0);

    gtk_box_pack_start(GTK_BOX(self->priv->vbox), term_box, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(self->priv->vbox), self->priv->entry, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(self->priv->window), self->priv->vbox);

    gtk_range_set_adjustment(
            GTK_RANGE(self->priv->scroll),
            VTE_TERMINAL(self->priv->terminal)->adjustment);

    gtk_widget_show_all(self->priv->window);

    mud_subwindow_update_geometry (self);

    vte_terminal_set_size(VTE_TERMINAL(self->priv->terminal),
                          self->priv->width,
                          self->priv->height);

    mud_subwindow_set_size_force_grid(self,
                                      VTE_TERMINAL(self->priv->terminal),
                                      TRUE,
                                      -1,
                                      -1);

    if(self->priv->input_enabled)
        gtk_widget_show(self->priv->entry);
    else
        gtk_widget_hide(self->priv->entry);

    gtk_window_set_title(GTK_WINDOW(self->priv->window), self->priv->title);

    gtk_window_get_size(GTK_WINDOW(self->priv->window),
                        &self->priv->pixel_width,
                        &self->priv->pixel_height);

    g_signal_connect(self->priv->window,
                     "delete-event",
                     G_CALLBACK(mud_subwindow_delete_event_cb),
                     self);

    g_signal_connect(self->priv->window,
                     "configure-event",
                     G_CALLBACK(mud_subwindow_configure_event_cb),
                     self);

    g_signal_connect(self->priv->entry,
                     "key_press_event",
                     G_CALLBACK(mud_subwindow_entry_keypress_cb),
                     self);

    mud_subwindow_reread_profile(self);

    return obj;
}

static void
mud_subwindow_finalize (GObject *object)
{
    MudSubwindow *self;
    GObjectClass *parent_class;
    gchar *history_item;

    self = MUD_SUBWINDOW(object);

    if(self->priv->history && !g_queue_is_empty(self->priv->history))
        while((history_item = (gchar *)g_queue_pop_head(self->priv->history)) != NULL)
            g_free(history_item);

    if(self->priv->history)
        g_queue_free(self->priv->history);

    gtk_widget_destroy(self->priv->window);

    parent_class = g_type_class_peek_parent(G_OBJECT_GET_CLASS(object));
    parent_class->finalize(object);
}

static void
mud_subwindow_set_property(GObject *object,
                      guint prop_id,
                      const GValue *value,
                      GParamSpec *pspec)
{
    MudSubwindow *self;
    gchar *new_string;
    gboolean new_boolean;
    guint new_uint;

    self = MUD_SUBWINDOW(object);

    switch(prop_id)
    {
        case PROP_INPUT:
            new_boolean = g_value_get_boolean(value);

            if(new_boolean != self->priv->input_enabled)
                self->priv->input_enabled = new_boolean;

            if(self->priv->entry)
                if(self->priv->input_enabled)
                    gtk_widget_show(self->priv->entry);
                else
                    gtk_widget_hide(self->priv->entry);
            break;

        case PROP_VISIBLE:
            new_boolean = g_value_get_boolean(value);

            if(new_boolean != self->priv->visible)
                self->priv->visible = new_boolean;
            break;

        case PROP_HEIGHT:
            new_uint = g_value_get_uint(value);

            if(new_uint != self->priv->height)
                self->priv->height = new_uint;
            break;

        case PROP_WIDTH:
            new_uint = g_value_get_uint(value);

            if(new_uint != self->priv->width)
                self->priv->width = new_uint;
            break;

        case PROP_IDENT:
            new_string = g_value_dup_string(value);

            if(!self->priv->identifier)
                self->priv->identifier = g_strdup(new_string);
            else if(!g_str_equal(self->priv->identifier, new_string))
            {
                g_free(self->priv->identifier);
                self->priv->identifier = g_strdup(new_string);
            }

            g_free(new_string);

            break;

        case PROP_TITLE:
            new_string = g_value_dup_string(value);

            if(!self->priv->title)
                self->priv->title = g_strdup(new_string);
            else if(!g_str_equal(self->priv->title, new_string))
            {
                g_free(self->priv->title);
                self->priv->title = g_strdup(new_string);
            }

            if(GTK_IS_WINDOW(self->priv->window))
            {
                g_printf("window\n");
            }

            g_free(new_string);

            break;

        case PROP_PARENT:
            self->priv->parent_view = MUD_CONNECTION_VIEW(g_value_get_object(value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void
mud_subwindow_get_property(GObject *object,
                      guint prop_id,
                      GValue *value,
                      GParamSpec *pspec)
{
    MudSubwindow *self;

    self = MUD_SUBWINDOW(object);

    switch(prop_id)
    {
        case PROP_TITLE:
            g_value_set_string(value, self->priv->title);
            break;

        case PROP_IDENT:
            g_value_set_string(value, self->priv->identifier);
            break;

        case PROP_WIDTH:
            g_value_set_uint(value, self->priv->width);
            break;

        case PROP_HEIGHT:
            g_value_set_uint(value, self->priv->height);
            break;

        case PROP_VISIBLE:
            g_value_set_boolean(value, self->priv->visible);
            break;

        case PROP_INPUT:
            g_value_set_boolean(value, self->priv->input_enabled);
            break;

        case PROP_PARENT:
            g_value_take_object(value, self->priv->parent_view);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

/* Private Methods */
const gchar *
mud_subwindow_get_history_item(MudSubwindow *self,
                               enum MudSubwindowHistoryDirection direction)
{
    gchar *history_item;

    if(direction == SUBWINDOW_HISTORY_DOWN)
        if( !(self->priv->current_history_index <= 0) )
            self->priv->current_history_index--;

    if(direction == SUBWINDOW_HISTORY_UP)
        if(self->priv->current_history_index < 
                (gint)g_queue_get_length(self->priv->history) - 1)
            self->priv->current_history_index++;

    history_item = (gchar *)g_queue_peek_nth(self->priv->history,
            self->priv->current_history_index);

    return history_item;
}

static void
mud_subwindow_set_terminal_colors(MudSubwindow *self)
{
    g_return_if_fail(MUD_IS_SUBWINDOW(self));

    vte_terminal_set_colors(VTE_TERMINAL(self->priv->terminal),
            &self->priv->parent_view->profile->preferences->Foreground,
            &self->priv->parent_view->profile->preferences->Background,
            self->priv->parent_view->profile->preferences->Colors, C_MAX);
}

static void
mud_subwindow_set_terminal_scrollback(MudSubwindow *self)
{
    g_return_if_fail(MUD_IS_SUBWINDOW(self));

    vte_terminal_set_scrollback_lines(VTE_TERMINAL(self->priv->terminal),
            self->priv->parent_view->profile->preferences->Scrollback);
}

static void
mud_subwindow_set_terminal_scrolloutput(MudSubwindow *self)
{
    g_return_if_fail(MUD_IS_SUBWINDOW(self));

    if(self->priv->terminal)
        vte_terminal_set_scroll_on_output(VTE_TERMINAL(self->priv->terminal),
                self->priv->parent_view->profile->preferences->ScrollOnOutput);
}

static void
mud_subwindow_set_terminal_font(MudSubwindow *self)
{
    PangoFontDescription *desc = NULL;
    char *name;

    g_return_if_fail(MUD_IS_SUBWINDOW(self));

    name = self->priv->parent_view->profile->preferences->FontName;

    if(name)
        desc = pango_font_description_from_string(name);

    if(!desc)
        desc = pango_font_description_from_string("Monospace 10");

    vte_terminal_set_font(VTE_TERMINAL(self->priv->terminal), desc);
}

static void
mud_subwindow_set_size_force_grid (MudSubwindow *window,
                                   VteTerminal *screen,
                                   gboolean        even_if_mapped,
                                   int             force_grid_width,
                                   int             force_grid_height)
{
    /* Owen's hack from gnome-terminal */
    GtkWidget *widget;
    GtkWidget *app;
    GtkRequisition toplevel_request;
    GtkRequisition widget_request;
    int w, h;
    int char_width;
    int char_height;
    int grid_width;
    int grid_height;
    int xpad;
    int ypad;

    g_return_if_fail(MUD_IS_SUBWINDOW(window));

    /* be sure our geometry is up-to-date */
    mud_subwindow_update_geometry (window);
    widget = GTK_WIDGET (screen);

    app = window->priv->window;

    gtk_widget_size_request (app, &toplevel_request);
    gtk_widget_size_request (widget, &widget_request);

    w = toplevel_request.width - widget_request.width;
    h = toplevel_request.height - widget_request.height;

    char_width = VTE_TERMINAL(screen)->char_width;
    char_height = VTE_TERMINAL(screen)->char_height;

    grid_width = VTE_TERMINAL(screen)->column_count;
    grid_height = VTE_TERMINAL(screen)->row_count;

    if (force_grid_width >= 0)
        grid_width = force_grid_width;
    if (force_grid_height >= 0)
        grid_height = force_grid_height;

    vte_terminal_get_padding (VTE_TERMINAL (screen), &xpad, &ypad);

    w += xpad * 2 + char_width * grid_width;
    h += ypad * 2 + char_height * grid_height;

    if (even_if_mapped && GTK_WIDGET_MAPPED (app)) {
        gtk_window_resize (GTK_WINDOW (app), w, h);
    }
    else {
        gtk_window_set_default_size (GTK_WINDOW (app), w, h);
    }
}

static void
mud_subwindow_update_geometry (MudSubwindow *window)
{
    GtkWidget *widget = window->priv->terminal;
    GdkGeometry hints;
    gint char_width;
    gint char_height;
    gint xpad, ypad;

    char_width = VTE_TERMINAL(widget)->char_width;
    char_height = VTE_TERMINAL(widget)->char_height;

    vte_terminal_get_padding (VTE_TERMINAL (window->priv->terminal), &xpad, &ypad);

    hints.base_width = xpad;
    hints.base_height = ypad;

#define MIN_WIDTH_CHARS 4
#define MIN_HEIGHT_CHARS 2

    hints.width_inc = char_width;
    hints.height_inc = char_height;

    /* min size is min size of just the geometry widget, remember. */
    hints.min_width = hints.base_width + hints.width_inc * MIN_WIDTH_CHARS;
    hints.min_height = hints.base_height + hints.height_inc * MIN_HEIGHT_CHARS;

    gtk_window_set_geometry_hints (GTK_WINDOW (window->priv->window),
            widget,
            &hints,
            GDK_HINT_RESIZE_INC |
            GDK_HINT_MIN_SIZE |
            GDK_HINT_BASE_SIZE);
}

/* MudSubwindow Callbacks */
static gboolean
mud_subwindow_delete_event_cb(GtkWidget *widget,
			      GdkEvent *event,
			      MudSubwindow *self)
{
    gtk_widget_hide(self->priv->window);
    self->priv->visible = FALSE;

    return TRUE;
}

static gboolean
mud_subwindow_configure_event_cb(GtkWidget *widget,
                                 GdkEventConfigure *event,
                                 gpointer user_data)
{
    MudSubwindow *self = MUD_SUBWINDOW(user_data);

    if(event->width != self->priv->pixel_width ||
       event->height != self->priv->pixel_height)
    {
        self->priv->pixel_width = event->width;
        self->priv->pixel_height = event->height;

        self->priv->width = VTE_TERMINAL(self->priv->terminal)->column_count;
        self->priv->height = VTE_TERMINAL(self->priv->terminal)->row_count;

        g_signal_emit(self,
                      mud_subwindow_signal[RESIZED],
                      0,
                      self->priv->width,
                      self->priv->height);
    }

    gtk_widget_grab_focus(self->priv->entry);

    return FALSE;
}

static gboolean
mud_subwindow_entry_keypress_cb(GtkWidget *widget,
                                GdkEventKey *event,
                                MudSubwindow *self)
{
    const gchar *history;
    GConfClient *client = gconf_client_get_default();

    if ((event->keyval == GDK_Return || event->keyval == GDK_KP_Enter) &&
            (event->state & gtk_accelerator_get_default_mod_mask()) == 0)
    {
        gchar *head = g_queue_peek_head(self->priv->history);
        const gchar *text = gtk_entry_get_text(GTK_ENTRY(self->priv->entry));

        if( (head && !g_str_equal(head, text) && head[0] != '\n') 
                || g_queue_is_empty(self->priv->history))
            g_queue_push_head(self->priv->history,
                    (gpointer)g_strdup(text));

        self->priv->current_history_index = -1;

        g_signal_emit(self,
                      mud_subwindow_signal[INPUT],
                      0,
                      text);

        if (gconf_client_get_bool(client,
                    "/apps/gnome-mud/functionality/keeptext", NULL) == FALSE)
            gtk_entry_set_text(GTK_ENTRY(self->priv->entry), "");
        else
            gtk_editable_select_region(GTK_EDITABLE(self->priv->entry), 0, -1);

        g_object_unref(client);

        return TRUE;
    }

    g_object_unref(client);

    if(event->keyval == GDK_Up)
    {
        history = 
            mud_subwindow_get_history_item(self, 
                                           SUBWINDOW_HISTORY_UP);

        if(history)
        {
            gtk_entry_set_text(GTK_ENTRY(self->priv->entry), history);
            gtk_editable_select_region(GTK_EDITABLE(self->priv->entry), 0, -1);
        }

        return TRUE;
    }

    if(event->keyval == GDK_Down)
    {
        history = 
            mud_subwindow_get_history_item(self, 
                                           SUBWINDOW_HISTORY_DOWN);

        if(history)
        {
            gtk_entry_set_text(GTK_ENTRY(self->priv->entry), history);
            gtk_editable_select_region(GTK_EDITABLE(self->priv->entry), 0, -1);
        }

        return TRUE;
    }

    return FALSE;
}

/* Public Methods */
void
mud_subwindow_set_size(MudSubwindow *self,
                       guint width,
                       guint height)
{
    g_return_if_fail(MUD_IS_SUBWINDOW(self));

    self->priv->width = width;
    self->priv->height = height;

    vte_terminal_set_size(VTE_TERMINAL(self->priv->terminal),
                          self->priv->width,
                          self->priv->height);

    mud_subwindow_set_size_force_grid(self,
                                      VTE_TERMINAL(self->priv->terminal),
                                      TRUE,
                                      -1,
                                      -1);
}

void
mud_subwindow_show(MudSubwindow *self)
{
    g_return_if_fail(MUD_IS_SUBWINDOW(self));

    gtk_widget_show(self->priv->window);
    self->priv->visible = TRUE;
}

void
mud_subwindow_reread_profile(MudSubwindow *self)
{
    g_return_if_fail(MUD_IS_SUBWINDOW(self));

    mud_subwindow_set_terminal_colors(self);
    mud_subwindow_set_terminal_scrollback(self);
    mud_subwindow_set_terminal_scrolloutput(self);
    mud_subwindow_set_terminal_font(self);
}

void
mud_subwindow_feed(MudSubwindow *self,
                   const gchar *data,
                   guint length)
{
    g_return_if_fail(MUD_IS_SUBWINDOW(self));

    vte_terminal_feed(VTE_TERMINAL(self->priv->terminal),
                      data,
                      length);
}

void
mud_subwindow_set_title(MudSubwindow *self,
                        const gchar *title)
{
    g_return_if_fail(MUD_IS_SUBWINDOW(self));

    gtk_window_set_title(GTK_WINDOW(self->priv->window), title);
}
