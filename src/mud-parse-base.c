/* GNOME-Mud - A simple Mud Client
 * mud-parse-base.c
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
#  include "config.h"
#endif

#include <glib-object.h>
#include <glib/gi18n.h>
#include <stdlib.h>
#include <string.h>

#include "mud-regex.h"
#include "mud-parse-base.h"
#include "mud-parse-alias.h"
#include "mud-parse-trigger.h"
#include "mud-connection-view.h"
#include "mud-profile.h"

#define TOKEN_TYPE_REGISTER 0
#define TOKEN_TYPE_TEXT		1

#define PARSE_STATE_TEXT 	0
#define PARSE_STATE_INTEXT	1
#define PARSE_STATE_REGISTER 	2
#define PARSE_STATE_INREGISTER  3

typedef struct ParseObject
{
    gchar *data;
    gint type;
} ParseObject;

struct _MudParseBasePrivate
{
	MudParseAlias *alias;
	MudParseTrigger *trigger;
};

/* Property Identifiers */
enum
{
    PROP_MUD_PARSE_BASE_0,
    PROP_PARENT_VIEW,
    PROP_REGEX
};

/* Create the Type */
G_DEFINE_TYPE(MudParseBase, mud_parse_base, G_TYPE_OBJECT);

/* Class Functions */
static void mud_parse_base_init (MudParseBase *parse_base);
static void mud_parse_base_class_init (MudParseBaseClass *klass);
static void mud_parse_base_finalize (GObject *object);
static GObject *mud_parse_base_constructor (GType gtype,
                                            guint n_properties,
                                            GObjectConstructParam *properties);
static void mud_parse_base_set_property(GObject *object,
                                        guint prop_id,
                                        const GValue *value,
                                        GParamSpec *pspec);
static void mud_parse_base_get_property(GObject *object,
                                        guint prop_id,
                                        GValue *value,
                                        GParamSpec *pspec);

// MudParseBase class functions

static void
mud_parse_base_class_init (MudParseBaseClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    /* Override base object constructor */
    object_class->constructor = mud_parse_base_constructor;

    /* Override base object's finalize */
    object_class->finalize = mud_parse_base_finalize;

    /* Override base object property methods */
    object_class->set_property = mud_parse_base_set_property;
    object_class->get_property = mud_parse_base_get_property;

    /* Add private data to class */
    g_type_class_add_private(klass, sizeof(MudParseBasePrivate));

    /* Install Properties */
    g_object_class_install_property(object_class,
            PROP_PARENT_VIEW,
            g_param_spec_object("parent-view",
                "parent view",
                "parent mud connection view",
                MUD_TYPE_CONNECTION_VIEW,
                G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY));

    g_object_class_install_property(object_class,
            PROP_REGEX,
            g_param_spec_object("regex",
                "regex",
                "mud regex object",
                MUD_TYPE_REGEX,
                G_PARAM_READABLE));
}

static void
mud_parse_base_init (MudParseBase *self)
{
    /* Get our private data */
    self->priv = MUD_PARSE_BASE_GET_PRIVATE(self);

    /* set public members to defaults */
    self->parent_view = NULL;
    self->regex = NULL;
}

static GObject *
mud_parse_base_constructor (GType gtype,
                            guint n_properties,
                            GObjectConstructParam *properties)
{
    MudParseBase *self;
    GObject *obj;
    MudParseBaseClass *klass;
    GObjectClass *parent_class;

    /* Chain up to parent constructor */
    klass = MUD_PARSE_BASE_CLASS( g_type_class_peek(MUD_TYPE_PARSE_BASE) );
    parent_class = G_OBJECT_CLASS( g_type_class_peek_parent(klass) );
    obj = parent_class->constructor(gtype, n_properties, properties);

    self = MUD_PARSE_BASE(obj);

    if(!self->parent_view)
    {
        g_printf("ERROR: Tried to instantiate MudParseBase without passing parent mudconnectionview\n");
        g_error("Tried to instantiate MudParseBase without passing parent mudconnectionview");
    }

    self->regex = g_object_new(MUD_TYPE_REGEX, NULL);

    self->priv->alias = g_object_new(MUD_TYPE_PARSE_ALIAS,
                                     "parent-base", self,
                                     NULL);

    self->priv->trigger = g_object_new(MUD_TYPE_PARSE_TRIGGER,
                                       "parent-base", self,
                                       NULL);

    return obj;
}

static void
mud_parse_base_finalize (GObject *object)
{
    MudParseBase *parse_base;
    GObjectClass *parent_class;

    parse_base = MUD_PARSE_BASE(object);

    g_object_unref(parse_base->regex);
    g_object_unref(parse_base->priv->alias);
    g_object_unref(parse_base->priv->trigger);

    parent_class = g_type_class_peek_parent(G_OBJECT_GET_CLASS(object));
    parent_class->finalize(object);
}

static void
mud_parse_base_set_property(GObject *object,
                            guint prop_id,
                            const GValue *value,
                            GParamSpec *pspec)
{
    MudParseBase *self;

    self = MUD_PARSE_BASE(object);

    switch(prop_id)
    {
        case PROP_PARENT_VIEW:
            self->parent_view = MUD_CONNECTION_VIEW(g_value_get_object(value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void
mud_parse_base_get_property(GObject *object,
                            guint prop_id,
                            GValue *value,
                            GParamSpec *pspec)
{
    MudParseBase *self;

    self = MUD_PARSE_BASE(object);

    switch(prop_id)
    {
        case PROP_PARENT_VIEW:
            g_value_take_object(value, self->parent_view);
            break;

        case PROP_REGEX:
            g_value_take_object(value, self->regex);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

// MudParseBase Methods
gboolean
mud_parse_base_do_triggers(MudParseBase *self, gchar *data)
{
    if(!MUD_IS_PARSE_BASE(self))
        return FALSE;

    return mud_parse_trigger_do(self->priv->trigger, data);
}

gboolean
mud_parse_base_do_aliases(MudParseBase *self, gchar *data)
{
    if(!MUD_IS_PARSE_BASE(self))
        return FALSE;

    return mud_parse_alias_do(self->priv->alias, data);
}

void
mud_parse_base_parse(MudParseBase *self, const gchar *data, gchar *stripped_data, gint ovector[1020])
{
    gint i, state, len, reg_num, reg_len, startword, endword, replace_len, curr_char;
    gchar *replace_text;
    gchar charbuf[2];
    gboolean new_send_line = TRUE;
    gchar *send_line = NULL;
    ParseObject *po = NULL;
    GSList *parse_list, *entry;

    g_return_if_fail(MUD_IS_PARSE_BASE(self));

    parse_list = NULL;
    len = strlen(data);

    // Lexer/Tokenizer
    if(data[0] == '%' && len > 1 && g_ascii_isdigit(data[1]))
        state = PARSE_STATE_REGISTER;
    else
        state = PARSE_STATE_TEXT;

    for(i = 0; i < len; i++)
    {
        switch(state)
        {
            case PARSE_STATE_TEXT:
                po = g_malloc(sizeof(ParseObject));
                parse_list = g_slist_prepend(parse_list, (gpointer) po);

                g_snprintf(charbuf, 2, "%c", data[i]);

                po->data = g_strdup(charbuf);
                po->type = TOKEN_TYPE_TEXT;

                if(i + 1 <= len)
                {
                    if((data[i+1] == '%' &&
                         i + 2 <=len && 
                        !g_ascii_isdigit(data[i+2])) || data[i+1] != '%')
                        state = PARSE_STATE_INTEXT;
                    else
                        state = PARSE_STATE_REGISTER;
                }
                break;

            case PARSE_STATE_INTEXT:
                g_snprintf(charbuf, 2, "%c", data[i]);
                po->data = g_strconcat((const gchar *)po->data, charbuf, NULL);

                if(i + 2 <= len)
                    if(data[i+1] == '%' && g_ascii_isdigit(data[i+2])) // % by itself isn't a register.
                        state = PARSE_STATE_REGISTER;
                break;

            case PARSE_STATE_REGISTER:
                po = g_malloc(sizeof(ParseObject));
                parse_list = g_slist_prepend(parse_list, (gpointer)po);

                g_snprintf(charbuf, 2, "%%");
                po->data = g_strdup(charbuf);
                po->type = TOKEN_TYPE_REGISTER;

                state = PARSE_STATE_INREGISTER;
                break;

            case PARSE_STATE_INREGISTER:
                g_snprintf(charbuf, 2, "%c", data[i]);
                po->data = g_strconcat((const gchar *)po->data, charbuf, NULL);

                if(i + 1 <= len)
                {
                    if(data[i + 1] == '%' && g_ascii_isdigit(data[i+2]))
                        state = PARSE_STATE_REGISTER;
                    else if(!g_ascii_isdigit(data[i+1]))
                        state = PARSE_STATE_TEXT;
                }
                break;
        }
    }

    /* We prepend items to the list for speed but we need
       to reverse it back into the proper order */
    if(parse_list)
        parse_list = g_slist_reverse(parse_list);

    // Parse what our lexer/tokenizer gave us.

    for(entry = parse_list; entry != NULL; entry = g_slist_next(entry))
    {
        ParseObject *myParse;

        myParse = (ParseObject *)entry->data;

        switch(myParse->type)
        {
            case TOKEN_TYPE_TEXT:
                if(new_send_line)
                {
                    new_send_line = FALSE;
                    send_line = g_strdup(myParse->data);
                }
                else
                    send_line = g_strconcat((const gchar *)send_line, (const gchar *)myParse->data, NULL);
                break;

            case TOKEN_TYPE_REGISTER:
                reg_len = strlen((gchar *)myParse->data);

                /* If you need more than 510 registers, tough luck ;) -lh */
                if(reg_len < 512)
                {
                    gint k;
                    gint curr_digit;
                    gchar reg_buf[512];

                    for(k=0,curr_digit=1; k < strlen((gchar *)myParse->data)-1; ++k, ++curr_digit)
                        reg_buf[k] = (gchar)myParse->data[curr_digit];
                    reg_buf[strlen((gchar *)myParse->data)-1] = '\0';

                    reg_num = (gint)g_strtod(reg_buf, NULL);

                    startword = ovector[reg_num << 1];
                    endword = ovector[(reg_num << 1) + 1];

                    replace_len = endword - startword;

                    replace_text = malloc(replace_len * sizeof(gchar));

                    for(i = 0, curr_char = startword; i < replace_len; i++, curr_char++)
                        replace_text[i] = stripped_data[curr_char];
                    replace_text[replace_len] = '\0';

                    if(new_send_line)
                    {
                        new_send_line = FALSE;
                        send_line = g_strdup(replace_text);
                    }
                    else
                        send_line = g_strconcat((const gchar *)send_line, (const gchar *)replace_text, NULL);

                    g_free(replace_text);
                }
                else
                    g_warning("Register number exceeded maximum - 510.");
                break;
        }
    }

    // Free our memory
    for(entry = parse_list; entry != NULL; entry = g_slist_next(entry))
    {
        ParseObject *myParse;

        myParse = (ParseObject *)entry->data;

        g_free((myParse->data));
        g_free(myParse);
    }

    g_slist_free(parse_list);

    // We're done, send our parsed trigger actions!
    mud_connection_view_send(self->parent_view, (const gchar *)send_line);
    g_free(send_line);
}

