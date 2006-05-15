/* GNOME-Mud - A simple Mud CLient
 * Copyright (C) 1998-2006 Robin Ericsson <lobbin@localhost.nu>
 *
 * map.c is written by Paul Cameron <thrase@progsoc.uts.edu.au> with
 * modifications by Robin Ericsson to make it work with AMCL.
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

/*****************************************************************************
 *  Converted to a Plugin by Les Harris.  This is a bad example of a plugin  *
 *  as it relies heavily on some gmud internals.  Please do not use this     *
 *  as a good example of how to write a gnome-mud plugin ;)  I basically     *
 *  implemented this as a plugin for .11 so we can rip it out and replace    *
 *  it easier later on.							     *
 *									     *
 *****************************************************************************
 *								             *
 *		 ALL HOPE ABANDON, YE WHO ENTER HERE			     *
 *									     *
 *****************************************************************************/

#define __MODULE__
  
#include "../../config.h"

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <sys/stat.h>
#include <gmodule.h>
#include <glib.h>
#include <gconf/gconf-client.h>

#include <ctype.h>
#include <errno.h>
#include <libintl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "map.h"
#include "directions.h"
#include "../../src/modules_api.h"

#if 0
#define USE_DMALLOC
#include <glib.h>
#include <dmalloc.h>
#endif

#define _(string) gettext(string)


/* Basic idea:
 * Create a co-ordinate based graph, starting at (0,0) which is where the
 * graph begins.
 *
 * Assumptions/requirements:
 *   1) By default, all units of movement are of the same size
 *   2) Each time you move up and down, you are transported into a new unique map
 *   3) Unique maps can be combined (say, multiple entrances to a building)
 *   4) By moving to a node that already exists, the paths are 'joined'
 *   5) Joined paths can be unjoined, this would mean there would be two nodes
 *      with the same co-ordinates. This would occur if the units of
 *      movement weren't the same size
 *   6) Nodes can be dragged to different locations on the map (to perhaps
 *      indicate distance
 *   7) Portals to already mapped areas, are joined to form a single link
 *      between them. On a single map, this link may be represented with a
 *      line. They are joined like so:
 *         User clicks a join button
 *         User clicks the source node (usually where they currently are located)
 *         User navigates the map finding the destination.
 *         User selects the destination node
 *         User clicks on the join button again
 *         This will join the nodes ...
 *   8) User can delete nodes (in case of a navigation error), and move where they are
 *      location (in the case of a teleport spell ...)
 *   9) Nodes can be named (perhaps allow macros in AMCL to set the player
 *      location, for teleport spells and such ...)
 *
 * All graphs are stored in a linked list. Each trail on a graph requires a
 * starting place, all starting places are referenced on the linked listo
 *
 * Each graph contains references to nodes. Each node contains links to north,
 * north-east, east, south-east, south, south-west, west and north-west.
 */

/* Concepts for selecting nodes:
 * In the automap, contain a linked list of rectangles on the screen. If
 * someone clicks on the screen, check the rectangles list. Each rectangle
 * element has a link to the map node that it references.
 * ...
 */
extern GList *MapList, *AutoMapList;

static void init_plugin   (PLUGIN_OBJECT *, GModule *);

PLUGIN_INFO gnomemud_plugin_info =
{
    "Automapper",
    "Robin Ericsson, Remi Bonnet",
    "1.0",
    "An auto-mapper for Gnome-MUD.\nConverted to a plugin by Les Harris.",
    init_plugin,
};


#define RGB(r, g, b) { ((r)<<24) | ((g)<<16) | ((b)<<8), (r)<<8, (g)<<8, (b)<<8 }

GdkColor red = RGB(255, 0, 0);

typedef GdkPoint            Point;
typedef struct _RectNode    RectNode;
typedef struct _CreatePathData CreatePathData;
typedef struct _EnterPathData  EnterPathData;
typedef struct _SPVertex    SPVertex; /* Shortest path vertex */

char *direction[] = { "N", "NE", "E", "SE", "S", "SW", "W", "NW", "U", "D" };
char *direction_long[] = { "North", "Northeast", "East", "Southeast", "South", "Southwest", "West", "Northwest", "Up", "Down" };

/* Other button codes */
#define REMOVE   10
#define LOAD     11
#define SAVE     12

#define PIX_ZOOM    40
#define X_INIT_SIZE 200
#define Y_INIT_SIZE 200
#define X_PADDING   (X_INIT_SIZE / PIX_ZOOM / 2)
#define Y_PADDING   (Y_INIT_SIZE / PIX_ZOOM / 2)

AutoMapConfig* automap_config;
extern LinkMemory* actual; 
extern AutoMap *automap;

/* The join types.
 * LINE has no additional parameters, so needs no additional struct
 * ARC requires that the arc thingy be known. The rest is calculated
 */
struct _RectNode {

    Rectangle  rectangle;
    MapNode   *node;
};

struct _SPVertex {

    MapNode *node;
    int label;
    int working;
};

struct _CreatePathData {
	AutoMap* automap;
	gchar* path_name;
	enum { NEW_PATH, FOLLOW_PATH, SELECT_PATH } choice;
	gchar* map_name;
	gchar* follow_path_name;
	GtkWidget* new_choice;
	GtkWidget* follow_choice;
};
struct _EnterPathData {
	AutoMap* automap;
	enum {CREATE_PATH, USE_PATH} choice;
	gchar* new_path_name;
	gchar* exists_path_name;
	GtkWidget* new_choice;    // Used to select the radio button when the use modifies the data 
	GtkWidget* exists_choice; //
};
GList *AutoMapList = NULL;
GList *MapList = NULL;

AutoMap *auto_map_new(void);
Map *map_new(void);
void redraw_map (AutoMap *);
void draw_map (AutoMap *);

/* Local functions */
static void		 blit_nodes (AutoMap *, struct win_scale *,
				     MapNode *[]);
static void      create_path(MapNode*, MapNode*, gchar*);
static void		 draw_nodes (AutoMap *, struct win_scale *, MapNode *,
				     MapNode *, GHashTable *);
static void		 draw_selected (AutoMap *, struct win_scale *);
static void		 file_sel_cancel_cb (GtkWidget *, void *[]);
static void		 file_sel_delete_event (GtkWidget *, GdkEventAny *,
					        void *[]);
static void		 file_sel_ok_cb (GtkWidget *, void *[]);
static void		 fill_node_path (SPVertex *, MapNode *, GHashTable *,
					 GList **);
static void		 free_maps (AutoMap *);
static void 	 free_node (MapNode *);
static gboolean		 free_node_count (MapNode *, guint8 *, gpointer);
static gboolean		 free_vertexes (MapNode *, SPVertex *, gpointer);
static void		 get_nodes (GHashTable *, Map *, guint *);
static gboolean		 is_numeric (gchar *);
static void		 load_automap_from_file (const gchar *, AutoMap *);
struct win_scale	*map_coords (AutoMap *);
static void      move_player_real (AutoMap*, guint);
static void		 move_player (AutoMap *, guint);
static void		 node_break (AutoMap *, guint);
static void		 new_automap_with_node (void);
static void		 node_goto (AutoMap *, struct win_scale *, MapNode *);
static void      path_type_dialog(AutoMap*, const gchar*);
static void		 remove_map (Map *);
static gboolean		 remove_node (MapNode *, guint *, void *);
//static void      remove_path(MapNode*, gchar*);
static void		 remove_player_node (AutoMap *);
static void		 remove_selected (AutoMap *);
static void		 scrollbar_adjust(AutoMap *);
static void		 scrollbar_value_changed (GtkAdjustment *, AutoMap *);
static void      show_message(AutoMap *automap, gchar *message, guint time);
static gint		 sp_compare (SPVertex *, SPVertex *);
static void		 teleport_player(AutoMap*, MapNode*);
static void		 undraw_selected (AutoMap *, struct win_scale *);
	   void 	 user_command(AutoMap* automap, const gchar* command);
       void		 window_automap (GtkWidget *, gpointer);
static void		 write_node (MapNode *, guint *, void *[]);

MudConnectionView *cd;
GSList *dirlist;

void init_plugin(PLUGIN_OBJECT *plugin, GModule *context)
{
	GConfClient *client = gconf_client_get_default();
	gchar keyname[2048];
	gchar *exit;
	gchar **exits;
	GError *error = NULL;
	gint len;
	gint i;
	
	plugin_register_menu(context, "Automapper", "window_automap");
	plugin_register_data_incoming(context, "data_in_function");
	plugin_register_data_outgoing(context, "data_out_function");
	
	cd = NULL;
	
	/* Just as a note, Plugins shouldn't muck with gnome-muds gconf settings */

	automap_config = g_new(AutoMapConfig, 1);
	automap_config->unusual_exits = NULL;
	
	g_snprintf(keyname, 2048, "/apps/gnome-mud/mapper/unusual_exits");
	exit = gconf_client_get_string(client, keyname, &error);
	
	exits = g_strsplit(exit, ";", -1);
	len = g_strv_length(exits);
	
	for(i = 0; i < len; i++)
		automap_config->unusual_exits = g_list_append(automap_config->unusual_exits, exits[i]);
	
	g_snprintf(keyname, 2048, "/apps/gnome-mud/profiles/Default/directions");
	dirlist = gconf_client_get_list(client, keyname, GCONF_VALUE_STRING, &error);
}

void data_in_function(PLUGIN_OBJECT *plugin, gchar *data, MudConnectionView *view)
{
	cd = view;
}

void data_out_function(PLUGIN_OBJECT *plugin, gchar *data, MudConnectionView *view)
{
	GList *puck;
	
	for (puck = AutoMapList; puck != NULL; puck = puck->next)
		user_command(puck->data, data);
}

void hash_list_path(gchar* key, gpointer value, gpointer* list)
{
	// TODO: test if a path exists already
	(*list) = g_list_append((*list), key);
}

struct win_scale *map_coords (AutoMap *automap)
{
    static struct win_scale ws;

    memset(&ws, 0, sizeof(struct win_scale));

    ws.width = automap->draw_area->allocation.width;
    ws.height = automap->draw_area->allocation.height;

    ws.mapped_unit = PIX_ZOOM / automap->zoom;
    ws.mapped_width = ws.width / ws.mapped_unit;
    ws.mapped_height = ws.height / ws.mapped_unit;

    if (ws.mapped_width & 1) ws.mapped_width++;

    if (ws.mapped_height & 1) ws.mapped_height++;

    ws.mapped_x = automap->x - ws.mapped_width / 2;
    ws.mapped_y = automap->y - ws.mapped_height / 2;

    return &ws;
}

static inline
gboolean node_visible (MapNode *node, struct win_scale *ws)
{
    return (node->x >= ws->mapped_x &&
            node->x <= ws->mapped_x + ws->mapped_width &&
            node->y >= ws->mapped_y &&
            node->y <= ws->mapped_y + ws->mapped_height);
}

static inline
gboolean in_rectangle (gint x, gint y, Rectangle *rectangle)
{
    return (x >= rectangle->x && x <= rectangle->x + rectangle->width &&
            y >= rectangle->y && y <= rectangle->y + rectangle->height);
}

guint node_hash (MapNode *a)
{
    return (guint)(a->x << 16 | a->y);
}

gint node_comp (MapNode *a, MapNode *b)
{
    return a->x == b->x && a->y == b->y;
}

static inline void node_hash_prepend (GHashTable *hash, MapNode *node)
{
    GList *list = g_hash_table_lookup(hash, node);
    list = g_list_prepend(list, node);
    g_hash_table_insert(hash, node, list);
}

static inline void node_hash_remove (GHashTable *hash, MapNode *node)
{
    GList *list = g_hash_table_lookup(hash, node);
    list = g_list_remove(list, node);

    if (list)
        g_hash_table_insert(hash, list->data, list);
    else
        g_hash_table_remove(hash, node);
}


static gint expose_event (GtkWidget *widget, GdkEventExpose *event,
			 AutoMap *automap)
{
    gdk_draw_pixmap(automap->draw_area->window,
                    automap->draw_area->style->fg_gc[GTK_WIDGET_STATE (automap->draw_area)],
                    automap->pixmap,
                    event->area.x, event->area.y,
                    event->area.x, event->area.y,
                    event->area.width, event->area.height);

    draw_selected(automap, map_coords(automap));

    return TRUE; /* Terminate signal */
}

static void create_path(MapNode* from, MapNode* to, gchar* name)
{
	/* First, if we need this, create the hash tables */
	if (from->gates == NULL)
		from->gates = g_hash_table_new(g_str_hash, g_str_equal);
	if (to->in_gates == NULL)
		to->in_gates = g_hash_table_new(g_str_hash, g_str_equal);
	
	/* And now, create the paths */
	g_hash_table_insert(from->gates, g_strdup(name), to);
	g_hash_table_insert(to->in_gates, g_strdup(name), from);
}


static void remove_selected (AutoMap *automap)
{
    undraw_selected(automap, map_coords(automap));
    g_list_free(automap->selected);
    automap->selected = NULL;
}

//~ static void remove_path(MapNode* node, gchar* name)
//~ {
	//~ /* If there is no paths, there is nothing to do */
	//~ if (node->gates == NULL)
		//~ return;
	
	//~ MapNode* target;
	//~ target = g_hash_table_lookup(node->gates, name);
	
	//~ /* The path doesn't exist */
	//~ if (!target)
		//~ return;
	
	//~ g_hash_table_remove(node->gates, name);
	
	//~ /* If there is no more keys in the hash table, destroy it */
	//~ if (g_hash_table_size(node->gates) == 0)
		//~ g_hash_table_destroy(node->gates);
	
	//~ if (target->in_gates == NULL)
	//~ {
		//~ g_warning("Path %s exist but hasn't been registred in in_gates", name);
		//~ return;
	//~ }
	//~ else
	//~ {
		//~ if (g_hash_table_lookup(target->in_gates, name) == NULL)
		//~ {
			//~ g_warning("Path %s exist but hasn't been registred in in_gates", name);
			//~ return;
		//~ }
	//~ }
	
	//~ g_hash_table_remove(target->in_gates, name);

	//~ if (g_hash_table_size(target->in_gates) == 0)
		//~ g_hash_table_destroy(target->in_gates);
//~ }

/* Recurses the map node list, looking for nodes in the map node
 * list that are in the list of all unconnected node starting places
 * (map->nodelist).
 * 1) If the hash table 'global' is passed, then it will return
 *    a non-zero value if the node was found. It will also
 *    operate faster (by avoiding searching nodes in the
 *    hash table 'global', and returning a non-zero value
 * 2) If the hash table 'global' is not passed, then all nodes will
 *    be searched. If the starting node is found, it is returned.
 */
MapNode *connected_node_in_list (Map *map, MapNode *node,
					GHashTable *hash,
					GHashTable *global)
{
    int i;
    GList *puck;
    MapNode *ret;

    if (g_hash_table_lookup(hash, node))
        return NULL;

    if (global && g_hash_table_lookup(global, node))
        return (MapNode *)!NULL;

    g_hash_table_insert(hash, node, node);

    if (global)
        g_hash_table_insert(global, node, node);

    if ((puck = g_list_find(map->nodelist, node)))
        return (MapNode *)puck->data;

    for (i = 0; i <= 7; i++) {
        MapNode *next = node->connections[i].node;

        if (next == NULL || node->connections[i].node->map != map)
            continue;

        ret = connected_node_in_list(map, next,  hash, global);

        if (ret)
            return ret;
    }

    return NULL;
}

static void remove_player_node (AutoMap *automap)
{
    /* Removing the node the current player is on is a little
     * tricky ...
     */
    MapNode *curr = automap->player;
    GHashTable *hash;
    struct win_scale *ws;
    int i;

    /* Step 1:
     *
     * Check to see if removing this node is a valid action
     * Removing a node is invalid if:
     *   The node goes both up or down
     *  The node goes up or down, and is not the last node on the map
     *   The node doesn't go up or down, and is the last node on the map
     */

    if (curr->connections[UP].node || curr->connections[DOWN].node)
    {
        if (curr->connections[UP].node && curr->connections[DOWN].node)
            return;

        for (i = 0; i < 8; i++)
            if (curr->connections[i].node)
                return;

        if (g_list_length(automap->map->nodelist) != 1)
            return;
    } else {
        for (i = 0; i < 8; i++)
            if (curr->connections[i].node)
                break;

        if (i == 8)
            if (g_list_length(automap->map->nodelist) == 1)
                return;
    }

    /* Step 2:
     *
     * Initialise function state before proceeding
     */

    automap->player = NULL;
    automap->map->nodelist = g_list_remove(automap->map->nodelist, curr);
    hash = g_hash_table_new((GHashFunc)node_hash, (GCompareFunc)node_comp);
    ws = map_coords(automap);

    /* Step 3:
     *
     * Remove any reference that nodes are connected to this node
     */

    for (i = 0; i < 10; i++)
    {
        MapNode *this = curr->connections[i].node;

        if (this)
        {
            /* Maintain the node count unless this node goes up or down */
            if (i < 8)
			{
				maplink_delete(curr, i);
				curr->connections[i].node = this; // We need this for later 
			}
			if (i > 7)
			{
				this->connections[OPPOSITE(i)].node = NULL;
				this->conn--;
			}
			
            if (automap->player == NULL)
			{
				if (i > 7)
					maplink_change_map(this->map);
                automap->player = this;
        	}
		}
    }

    /* Step 4:
     *
     * If by removing this node, it becomes impossible to traverse nodes
     * connected to this one, add as many nodes to the adjacent nodelist
     * as it takes
     */

    if (automap->player)
    {
/*        for (i = 0; i < 8; i++) { // This is normally done by maplink_delete
            GHashTable *thishash;

            MapNode *this = curr->connections[i].node;

            if (this && !g_hash_table_lookup(hash, this))
            {
                thishash = g_hash_table_new((GHashFunc)node_hash, (GCompareFunc)node_comp );

                if (!connected_node_in_list(automap->map, this, thishash, hash))
                    automap->map->nodelist = g_list_prepend(automap->map->nodelist, this);

                g_hash_table_destroy(thishash);
            }
        }
*/    } else {

        /* This node had no connections, yet there were other nodes on the map */
        automap->player = automap->map->nodelist->data;
    }

    g_hash_table_destroy(hash);
	automap->map->nodelist = g_list_remove(automap->map->nodelist, curr);
    node_hash_remove(automap->map->nodes, curr);
    g_free(curr);

    /* If the map the closest node was on, is not the same map as the
     * removed node was on, then destroy the previous map, and set the
     * current map to the nodes map
     */

    if (automap->map != automap->player->map)
    {
        remove_map(automap->map);
        automap->map = automap->player->map;
    }


    /* Recalculate map size by reseting the map size before entering
     * redraw_map(). Recenter window if the node is offscreen
     */

    automap->map->min_x = automap->map->max_x = automap->player->x;
    automap->map->min_y = automap->map->max_y = automap->player->y;

    if ((automap->player->x <= ws->mapped_x) ||
        (automap->player->y <= ws->mapped_y) ||
        (automap->player->x >= ws->mapped_x + ws->mapped_width) ||
        (automap->player->y >= ws->mapped_y + ws->mapped_height)   )
    {
        automap->x = automap->player->x;
        automap->y = automap->player->y;
    }

    redraw_map(automap);
    scrollbar_adjust(automap);
}

static gint key_press_event (GtkWidget *widget, GdkEventKey *event, AutoMap *automap)
{

    switch (event->keyval)
    {
    case GDK_q:
        gtk_exit(0); break;
        
	case GDK_Escape:
		if (automap->create_link_data != NULL)
		{
			/* Free the create_link_data structure */
			show_message(automap, _("Ready."), 0);
			show_message(automap, _("Canceled."), 750);
			link_delete(automap->create_link_data->link);
			g_free(automap->create_link_data);
			automap->create_link_data = NULL;
		}
		break;
	
	case GDK_Return:
		if (automap->create_link_data != NULL)
		{
                    MapNode *my_start_node, *next_start_node;
                    GHashTable *hash;
		    struct win_scale *ws;

			Link* last = automap->create_link_data->link;
			while (last->next)
				last = last->next;
			
			if (2 * ((gint) (last->x / 2)) == last->x && 2 * ((gint) (last->y / 2)) == last->y)
			{
				/* Create a new node for testing */
				MapNode *node, *from = automap->create_link_data->from;
				GList *list;
				node = g_malloc0(sizeof(MapNode));
				node->x = last->x / 2;
				node->y = last->y / 2;
				
				/* Test if a node already exist */
				list = g_hash_table_lookup(automap->map->nodes, node);
				if (list)
				{
					g_free(node);
					node = list->data; /* We will create the link to the first node at this place */

					if (node->connections[OPPOSITE(automap->create_link_data->last_move)].node != NULL)
					{
						show_message(automap, _("A link already exists here!"), 2000);
						break;
					}
					
					/* I really like Ctrl-C Ctrl-V :) , and I am too lazy to create a new function :p */
					
                    /* If the node we're connecting to is part of a different trail
                     * (ie automap->map->nodelist start point is different to the start
                     * point of this node), then delete the start point of the other node
                     * (since we are connecting start points)
                     */

                    hash = g_hash_table_new((GHashFunc)node_hash, (GCompareFunc)node_comp);
                    my_start_node = connected_node_in_list(automap->map, from, hash, NULL);
                    g_hash_table_destroy(hash);

                    hash = g_hash_table_new((GHashFunc)node_hash, (GCompareFunc)node_comp);
                    next_start_node = connected_node_in_list(automap->map, node, hash, NULL);
                    g_hash_table_destroy(hash);

                    if (my_start_node != next_start_node)
                        automap->map->nodelist = g_list_remove(automap->map->nodelist, next_start_node);

				}
				else
				{
					/* Achieve to create the node */
					node->map = automap->map;
					
					/* Add the node to the hash */
					node_hash_prepend(automap->map->nodes, node);
				}
				
				/* And now, create the new connection */
				last->type = LK_NODE;
				last->node = node;
				from = automap->create_link_data->from;
				from->connections[automap->create_link_data->first_move].node = node;
				from->connections[automap->create_link_data->first_move].link = automap->create_link_data->link;
				node->connections[OPPOSITE(automap->create_link_data->last_move)].node = from;
				node->connections[OPPOSITE(automap->create_link_data->last_move)].link = last;
				(from->conn)++;
				(node->conn)++;

				/* Draw correctly the ending nodes */
				ws = map_coords(automap);
				maplink_draw(2 * node->x, 2 * node->y, ws);
				maplink_draw(last->prev->x, last->prev->y, ws);
				
				/* End the create_link */
				g_free(automap->create_link_data);
				automap->create_link_data = NULL;
				show_message(automap, _("Ready"), 0);
				show_message(automap, _("Link created"), 1500);
			}
			else
			{
				show_message(automap, _("Can't create a node here"), 3000); /* More explicit message ? */
			}
		}
		break;
	
    case GDK_Shift_L:
    case GDK_Shift_R:
        automap->shift = TRUE; break;

    case GDK_r:

        /* Delete the node the player is on */
        remove_player_node(automap); break;

    case GDK_b:
        automap->node_break = TRUE; break;

    case GDK_g:
        automap->node_goto = TRUE; break;

    case GDK_p:
        automap->print_coord = TRUE; break;

        /* Keys to do with movement
         * Hopefully I have gotten them all
         */

    case GDK_n:
    case GDK_Up:
    case GDK_KP_Up:
    case GDK_KP_8:
        move_player(automap, NORTH);     break;

    case GDK_KP_Page_Up:
    case GDK_KP_9:
        move_player(automap, NORTHEAST); break;

    case GDK_e:
    case GDK_Right:
    case GDK_KP_Right:
    case GDK_KP_6:
        move_player(automap, EAST);      break;

    case GDK_KP_Page_Down:
    case GDK_KP_3:
        move_player(automap, SOUTHEAST); break;

    case GDK_s:
    case GDK_Down:
    case GDK_KP_Down:
    case GDK_KP_2:
        move_player(automap, SOUTH);     break;

    case GDK_KP_End:
    case GDK_KP_1:
        move_player(automap, SOUTHWEST); break;

    case GDK_w:
    case GDK_Left:
    case GDK_KP_Left:
    case GDK_KP_4:
        move_player(automap, WEST);      break;

    case GDK_KP_Home:
    case GDK_KP_7:
        move_player(automap, NORTHWEST); break;

    case GDK_u:
        move_player(automap, UP);        break;

    case GDK_d:
        move_player(automap, DOWN);      break;

    default:
        return FALSE;
    }

    gtk_signal_emit_stop_by_name(GTK_OBJECT(widget), "key_press_event");

    return TRUE;
}

static gint
key_release_event (GtkWidget *widget, GdkEventKey *event, AutoMap *automap)
{

    switch (event->keyval)
    {
    case GDK_Shift_L:
    case GDK_Shift_R:
        automap->shift = FALSE;
        return TRUE;
    }

    return FALSE;
}

static void on_create_path(GtkDialog *widget, int id, CreatePathData* data)
{
	gtk_widget_destroy(GTK_WIDGET(widget));
	
	switch (data->choice)
	{
		case NEW_PATH:
		{
			GList* puck; 
			gboolean retry = TRUE;
			
			/* Create the initial node */
			MapNode* node = g_malloc0(sizeof(MapNode));
			Map* new_map;
			node->x = 0;
			node->y = 0;
			
			/* Verify that the name is unique */
			while (retry)
			{
				retry = FALSE;
				for (puck = MapList; puck != NULL; puck = puck->next)
				{
					Map *map = puck->data;
					gchar* tmp;
					if (strcmp(data->map_name, map->name))
						continue;
				
					retry = TRUE;
					tmp = g_malloc0(sizeof(char) * (strlen(data->map_name) + 5));
					strcpy(tmp, data->map_name);
					strcat(tmp, " (2)");
					g_free(data->map_name);
					data->map_name = tmp;
				}
			}
			
			/* Set the new map */
			new_map = map_new();
			new_map->name = g_strdup(data->map_name);
			node->map = new_map;
			data->automap->map = new_map;
			maplink_change_map(new_map);

			/* Add the initial node to the list and to the hash */
			new_map->nodelist = g_list_append(new_map->nodelist, node);
			node_hash_prepend(new_map->nodes, node);
			
			/* Create the path */
			create_path(data->automap->player, node, data->path_name);
			
			/* Set the automap data */
			data->automap->player = node;
			data->automap->x = 0;
			data->automap->y = 0;
			
			/* Redraw all */
			redraw_map(data->automap);
			
			break;
		}
		case FOLLOW_PATH:
		{
			/* Gets the target with the in_gates hash */
			MapNode* target = g_hash_table_lookup(data->automap->player->in_gates, data->follow_path_name);
			if (target == NULL)
			{
				g_warning("There is no entry in data->automap->player->in_gates which match %s", data->follow_path_name);
				return;
			}

			/* Create the path */
			create_path(data->automap->player, target, data->path_name);

			/* Update the automap data */
			data->automap->player = target;
			data->automap->map = target->map;
			data->automap->x = target->x;
			data->automap->y = target->y;
			
			/* Notify the map change and redraw */
			maplink_change_map(target->map);
			redraw_map(data->automap);
			break;
		}
		default:
			g_warning("data->choice doesn't match with anything");		
	}
	
	/* To finish, free the data */
	g_free(data->path_name);
	g_free(data->follow_path_name);
	g_free(data->map_name);
	g_free(data);
}


static inline void on_new_path_wanted(GtkWidget *widget, CreatePathData* data)
{
	data->choice = NEW_PATH;
}
static inline void on_follow_wanted(GtkWidget *widget, CreatePathData* data)
{
	data->choice = FOLLOW_PATH;
}
static inline void on_map_name_changed(GtkWidget *widget, CreatePathData* data)
{
	g_free(data->map_name);
	data->map_name = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->new_choice), TRUE);
	data->choice = NEW_PATH;	
}
static inline void on_follow_changed(GtkWidget *widget, CreatePathData* data)
{
	g_free(data->follow_path_name);
	data->follow_path_name = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->follow_choice), TRUE);
	data->choice = FOLLOW_PATH;	
}
void path_type_dialog(AutoMap* automap, const gchar* name)
{
	GtkWidget *dialog;             			// Main dialog
	GtkWidget *to_new_map_edit;             // Edit field
	GtkWidget *follow_path_combo;           // A combo box
	GtkWidget *to_new_map, *follow_path;	// Choices
	GtkWidget *vbox, *hbox_new_map, *hbox_follow;	 			// Containers

	guint result;

	/* Initialise the CreatePathData */
	CreatePathData* data;
	data = g_malloc0(sizeof(CreatePathData));
	data->map_name = g_strdup(_("New Map"));
	data->path_name = g_strdup(name);
	data->follow_path_name = g_strdup("");
	data->automap = automap;
	data->choice = NEW_PATH;
			
	/* Setting the dialog */
	dialog = gtk_dialog_new();
	/* Translator: "path" means "line of travel", ie "road" */
	gtk_window_set_title(GTK_WINDOW(dialog), _("Creating a path"));
	vbox = GTK_DIALOG(dialog)->vbox;

	/* First horizontal container */
	hbox_new_map = gtk_hbox_new(TRUE, 15);
	gtk_box_pack_start(GTK_BOX(vbox), hbox_new_map, TRUE, TRUE, 2);
	
	/* The radio button (new map)*/
	to_new_map = gtk_radio_button_new_with_label(NULL, _("Path lead to a new map"));
	data->new_choice = to_new_map;
	gtk_box_pack_start(GTK_BOX(hbox_new_map), to_new_map, FALSE, FALSE, 5);
	gtk_signal_connect(GTK_OBJECT(to_new_map), "clicked", GTK_SIGNAL_FUNC(on_new_path_wanted), data);

	/* The edit entry for the name of the new map */
	to_new_map_edit = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(to_new_map_edit), _("New map"));
	gtk_box_pack_start(GTK_BOX(hbox_new_map), to_new_map_edit, FALSE, FALSE, 5);
	gtk_signal_connect(GTK_OBJECT(to_new_map_edit), "changed", GTK_SIGNAL_FUNC(on_map_name_changed), data);
			
	/* If there is entrances, give anothers choices */
	if (automap->player->in_gates)
	{
		gpointer* path_list;

		/* Choose this node by default */
		data->choice = FOLLOW_PATH; 
				
		/* The container */
		hbox_follow =  gtk_hbox_new(TRUE, 15);
		gtk_box_pack_start(GTK_BOX(vbox), hbox_follow , TRUE, TRUE, 2);

		/* The radio button (default) */
		follow_path = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(to_new_map), _("Path follows an already existing path:"));
		data->follow_choice = follow_path;
		gtk_box_pack_start(GTK_BOX(hbox_follow), follow_path, FALSE, FALSE, 5);
		gtk_signal_connect(GTK_OBJECT(follow_path), "clicked", GTK_SIGNAL_FUNC(on_follow_wanted), data);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(follow_path), TRUE);
	
		/* And the dropdown list */
		follow_path_combo = gtk_combo_new();
		path_list = g_malloc0(sizeof(gpointer));
		g_hash_table_foreach(automap->player->in_gates, (GHFunc) hash_list_path, path_list);
		gtk_combo_set_popdown_strings(GTK_COMBO(follow_path_combo), (*path_list));
		gtk_editable_set_editable(GTK_EDITABLE(GTK_COMBO(follow_path_combo)->entry), FALSE);
		gtk_box_pack_start(GTK_BOX(hbox_follow), follow_path_combo, FALSE, FALSE, 5);
		gtk_signal_connect(GTK_OBJECT(GTK_COMBO(follow_path_combo)->entry), "changed", GTK_SIGNAL_FUNC(on_follow_changed), data);
		g_free(data->follow_path_name);
		data->follow_path_name = g_strdup(gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(follow_path_combo)->entry)));				
	}

			
	gtk_dialog_add_button(GTK_DIALOG(dialog), _("Create"), GTK_RESPONSE_ACCEPT);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
			
	/* and show all */
	gtk_widget_show_all(GTK_DIALOG(dialog)->vbox);
	result = gtk_dialog_run(GTK_DIALOG(dialog));

	/* When the dialog is destroyed, create the path */
	on_create_path(GTK_DIALOG(dialog), result, data);
}

void user_command(AutoMap* automap, const gchar* command)
{
	GList* puck;
	int i = 0;
	int out = -1;
	GSList *dir;
	
	command = g_ascii_strdown(command, -1);

	//	no way to use this?
	//	g_slist_foreach

	dir = dirlist;
	while(dir) {
		if(!g_ascii_strcasecmp(command,(gchar *) dir->data)) {
			out = i;
			break;
		}
		i++;
		dir = dir->next;
	}

	if (out != -1)
	{
		move_player_real(automap, out);
		return;
	}
	
	for (puck = automap_config->unusual_exits; puck != NULL; puck = puck->next)
	{
		if (!g_ascii_strcasecmp(command, puck->data))
		{
			MapNode* target = NULL;
			if (automap->player->gates)
				target = g_hash_table_lookup(automap->player->gates, command);
			
			if (target)
			{
				teleport_player(automap, target);
				return;
			}
			
			path_type_dialog(automap, command);			
			return;
		}
	}
	
	return;
}

static void on_enter_event(GtkWidget* widget, int id, EnterPathData* data)
{
	if (id == GTK_RESPONSE_ACCEPT)
	{
		switch (data->choice)
		{
			case CREATE_PATH:
			{
				gchar command[25];

				if (strcmp(data->new_path_name, "") == 0)
					return;

				if (g_list_find_custom(automap_config->unusual_exits, data->new_path_name, (GCompareFunc) g_str_equal) == NULL)
				{
					automap_config->unusual_exits = g_list_append(automap_config->unusual_exits, data->new_path_name);					
					//update_gconf_from_unusual_exits();
				}
				
				gtk_widget_destroy(widget);

				path_type_dialog(data->automap, data->new_path_name);

				g_snprintf(command, 25, "%s\r\n", data->new_path_name);
				if(cd)
					plugin_connection_send(command, cd);
				//connection_send(main_connection, command);

				g_free(data->new_path_name);
				g_free(data->exists_path_name);

				break;
			}
			case USE_PATH:
			{
				MapNode* target = g_hash_table_lookup(data->automap->player->gates, data->exists_path_name);
				gchar command[25];

				if (target == NULL)
				{
					g_warning("There is no entry in data->automap->player->gates which match %s", data->exists_path_name);
					return;
				}

			data->automap->player = target;
			data->automap->map = target->map;
			data->automap->x = target->x;
			data->automap->y = target->y;

			g_snprintf(command, 25, "%s\r\n", data->exists_path_name);

			if(cd)
				plugin_connection_send(command, cd);
			//connection_send(main_connection, command);
			
			gtk_widget_destroy(widget);
			maplink_change_map(data->automap->map);
			redraw_map(data->automap);
			}
		}
	}	
	
	gtk_widget_destroy(widget);
}

static inline void on_create_path_wanted(GtkWidget* widget, EnterPathData* data)
{
	data->choice = CREATE_PATH;
}
static inline void on_future_path_name_changed(GtkWidget* widget, EnterPathData* data)
{
	g_free(data->new_path_name);
	data->new_path_name = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));
	data->new_path_name = g_strstrip(data->new_path_name);
	
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->new_choice), TRUE);
	data->choice = CREATE_PATH;	
}
static inline void on_enter_wanted(GtkWidget* widget, EnterPathData* data)
{
	data->choice = USE_PATH;
}
static inline void on_exists_name_changed(GtkWidget* widget, EnterPathData* data)
{
	g_free(data->exists_path_name);
	data->exists_path_name = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));
	
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->exists_choice), TRUE);
	data->choice = USE_PATH;
}
static void path_enter_dialog(AutoMap* automap)
{
	GtkWidget* dialog;
	GtkWidget *new_choice, *exists_choice;
	GtkWidget *new_edit, *exists_combo;
	GtkWidget *vbox, *hbox;
	guint result;
	
	/* Create the data which will be passed to the callbacks */
	EnterPathData *data;
	data = g_malloc0(sizeof(EnterPathData));
	data->automap = automap;
	data->choice = CREATE_PATH;
	data->new_path_name = g_strdup("");
	data->exists_path_name = g_strdup("");
	
	/* Create the dialog */
	dialog = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(dialog), _("Enter in a path"));
	vbox = GTK_DIALOG(dialog)->vbox;
	
	/* First choice: create */
	hbox = gtk_hbox_new(FALSE, 12);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);
	
	new_choice = gtk_radio_button_new_with_label(NULL, _("Create a new path:"));
	data->new_choice = new_choice;
	gtk_box_pack_start(GTK_BOX(hbox), new_choice, FALSE, FALSE, 5);
	gtk_signal_connect(GTK_OBJECT(new_choice), "clicked", GTK_SIGNAL_FUNC(on_create_path_wanted), data);

	new_edit = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox), new_edit, FALSE, FALSE, 1);
	gtk_signal_connect(GTK_OBJECT(new_edit), "changed", GTK_SIGNAL_FUNC(on_future_path_name_changed), data);
	
	/* Second choice: enter in existing path */
	if (automap->player->gates)
	{
		gpointer* path_list;

		hbox = gtk_hbox_new(FALSE, 12);
		gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);
		
		exists_choice = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(new_choice), _("Enter in existing path:"));
		data->exists_choice = exists_choice;
		gtk_box_pack_start(GTK_BOX(hbox), exists_choice, FALSE, FALSE, 5);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(exists_choice), TRUE);
		gtk_signal_connect(GTK_OBJECT(exists_choice), "clicked", GTK_SIGNAL_FUNC(on_enter_wanted), data);

		exists_combo = gtk_combo_new();
		path_list = g_malloc0(sizeof(gpointer));
		g_hash_table_foreach(automap->player->gates, (GHFunc) hash_list_path, path_list);
		gtk_combo_set_popdown_strings(GTK_COMBO(exists_combo), (*path_list));
		gtk_editable_set_editable(GTK_EDITABLE(GTK_COMBO(exists_combo)->entry), FALSE);
		gtk_box_pack_start(GTK_BOX(hbox), exists_combo, FALSE, FALSE, 5);
		gtk_signal_connect(GTK_OBJECT(GTK_COMBO(exists_combo)->entry), "changed", GTK_SIGNAL_FUNC(on_exists_name_changed), data);
		g_free(data->exists_path_name);
		data->exists_path_name = g_strdup(gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(exists_combo)->entry)));						

		data->choice = USE_PATH;
	}
	
	gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_OK, GTK_RESPONSE_ACCEPT);
	gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_NONE);

	gtk_widget_show_all(GTK_DIALOG(dialog)->vbox);
	result = gtk_dialog_run(GTK_DIALOG(dialog));
	
	on_enter_event(dialog, result, data);
}
static void on_enter_item_activated(GtkWidget* widget, void* data[2])
{
	path_enter_dialog(data[0]);
	g_free(data);
}
static void on_teleport_item_activated(GtkWidget* widget, void* data[2])
{
	teleport_player(data[0], data[1]);
	g_free(data);
}

static void on_create_link_item_activated(GtkWidget* widget, void* data[2])
{
	int i;
	AutoMap* automap = data[0];
	MapNode* node = data[1];
	
	for(i=0; i<8; i++)
	{
		if (node->connections[i].node == NULL)
			break;
		
		if (i == 7)
		{
			GtkWidget* error = gtk_message_dialog_new(GTK_WINDOW(automap->window), 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("This node has already 8 links. Destroy one of these before trying to create a new one"));
			g_signal_connect_swapped(GTK_OBJECT(error), "response", GTK_SIGNAL_FUNC(gtk_widget_destroy), GTK_OBJECT(error));
			gtk_dialog_run(GTK_DIALOG(error));
		}
	}
	
	/* Change the automap state */
	show_message(automap, _("Enter to finish, Esc to quit"), 0);
	show_message(automap, _("Use move buttons to build the link."), 5000);
	automap->create_link_data = g_malloc0(sizeof(struct _create_link_data));
	automap->create_link_data->from = node;
	automap->create_link_data->first_move = -1; /* Means that the user hasn't move yet */
	
	/* Move the player to the node and initialise the new link */
	automap->player = node;
	automap->create_link_data->link = link_add(NULL, LK_NODE, 2 * node->x, 2 * node->y);
	automap->create_link_data->link->node = node;
	
	g_free(data);
}

static void on_zoom_in_item_activated(GtkWidget* widget, AutoMap* automap)
{
	if (automap->zoom > 0.25)
	{
  		automap->zoom /= 2;
		scrollbar_adjust(automap);
  		redraw_map(automap);
	}
}

static void on_zoom_out_item_activated(GtkWidget* widget, AutoMap* automap)
{
	if (automap->zoom < 4)
	{
		automap->zoom *= 2;
		scrollbar_adjust(automap);
		redraw_map(automap);
	}
}

static void on_config_item_activated(GtkWidget* widget, AutoMap* automap)
{
	//window_prefs(NULL, (gpointer*) 0x123456); // Window_prefs is a callback. This code will be broken if a release use its first argument
								  			  // The second argument tells to window_prefs that it is called from here
	return;
}

/*static void on_properties_item_activated(GtkWidget* widget, AutoMap* automap)
{
	return;
}*/

static gint button_press_event (GtkWidget *widget, GdkEventButton *event, AutoMap *automap)
{

    GList *puck;

    gint x = (gint)rint(event->x), y = (gint)rint(event->y);

    if (event->button == 1)
    {
        struct win_scale *ws = map_coords(automap);

        /* Remove all selections made unless the user is pressing shift
         */

        if (!automap->shift && automap->selected)
        {
            undraw_selected(automap, ws);
            remove_selected(automap);
        }

        /* See if the mouse clicked on a node */
        for (puck = automap->visible; puck != NULL; puck = puck->next)
        {
            RectNode *rn = puck->data;
            if (in_rectangle(x, y, &rn->rectangle))
            {
                /* DEBUG print the coordinate the node is on */
                if (automap->print_coord)
                {
                    automap->print_coord = FALSE;
                    
                    g_print("Mouse is at (%d, %d)\n",
     				rn->node->x, rn->node->y);

                    return TRUE;
                }

                /* If the player wants to go to this node ... */
                if (automap->node_goto)
                {
                    automap->node_goto = FALSE;
                    node_goto(automap, ws, rn->node);
                    return TRUE;
                }

                /* If the shift button has not been pressed, select this node
                 * as the player node
                 */
                if (!automap->shift)
                {
					teleport_player(automap, rn->node);
                    automap->state = NONE;
                    return TRUE;
                }

                /* Select this object, if it's not already in the list */
                if (!g_list_find(automap->selected, rn->node))
                    automap->selected = g_list_prepend(automap->selected, rn->node);

                automap->x_orig = x; automap->y_orig = y;
                automap->x_offset = automap->y_offset = 0;

                draw_selected(automap, map_coords(automap));
                automap->state = SELECTMOVE;

                return TRUE; /* Terminate signal */

            }
        }

        /* If this area was reached, then the user clicked in a blank area
         * If any nodes were selected, deselect them
         *
         * FIXME create selection boxes here
         */

        automap->state = BOXSELECT;
        automap->selection_box.x = x;
        automap->selection_box.y = y;
        automap->selection_box.width = automap->selection_box.height = 0;
    }

    if (event->button == 3)
    {
		MapNode* clicked_node = NULL;
	
		/* Test if we are in a normal state */
		if (automap->state != NONE)
	  		return FALSE;

		/* If there is a selection of more than one, do nothing! */
		if (g_list_length(automap->selected) > 1)
	  		return FALSE;

		/* Check now if there is a node clicked */
		if (g_list_length(automap->selected) == 1)
	  	{
	    	clicked_node = automap->selected->data;
	  	}
		else
	  	{
	    	GList* puck;
	    	for(puck = automap->visible; puck != NULL; puck = puck->next)
	    	{
				RectNode* rn = puck->data;
				struct win_scale *ws;
				Rectangle* rect = malloc(sizeof(Rectangle));
				ws = map_coords(automap);
				rect->x = rn->rectangle.x - (ws->mapped_unit / 4);
				rect->y = rn->rectangle.y - (ws->mapped_unit / 4);
				rect->width  = ws->mapped_unit / 2;
				rect->height = ws->mapped_unit / 2;
				if (in_rectangle(x, y, rect))
					clicked_node = rn->node;
	    	}
	  	}

		/* So test now if there is a node selected */
		if (clicked_node == NULL)
	  	{
			/* The user clicked on a blank area: display a menu:
			 * Zoom In: Increase the zoom factor
			 * Zoom Out: Decrease the zoom factor
			 * Configure Automap: Display a message box to configure automap
			 */
			
			GtkWidget* pop_menu;
			GtkWidget* zoom_in_item;
			GtkWidget* zoom_out_item;
			GtkWidget* config_item;
			
			/* Build menu */
			pop_menu = gtk_menu_new();
			zoom_in_item = gtk_menu_item_new_with_label(_("Zoom In"));
			zoom_out_item = gtk_menu_item_new_with_label(_("Zoom Out"));
			config_item = gtk_menu_item_new_with_label(_("Configure Automap"));
			gtk_menu_shell_append(GTK_MENU_SHELL(pop_menu), zoom_in_item);
			gtk_menu_shell_append(GTK_MENU_SHELL(pop_menu), zoom_out_item);
			gtk_menu_shell_append(GTK_MENU_SHELL(pop_menu), config_item);
			
			/* Connecting events */
			gtk_signal_connect(GTK_OBJECT(zoom_in_item), "activate", GTK_SIGNAL_FUNC(on_zoom_in_item_activated), automap);
			gtk_signal_connect(GTK_OBJECT(zoom_out_item), "activate", GTK_SIGNAL_FUNC(on_zoom_out_item_activated), automap);
			gtk_signal_connect(GTK_OBJECT(config_item), "activate", GTK_SIGNAL_FUNC(on_config_item_activated), automap);

			/* Show menu */
			gtk_menu_popup(GTK_MENU(pop_menu), NULL, NULL, NULL, NULL, 3, gtk_get_current_event_time());
			gtk_widget_show(zoom_in_item);
			gtk_widget_show(zoom_out_item);
			gtk_widget_show(config_item);
			
			return TRUE;
		}
		else
		{
			/* The user clicked on a single node: display a menu:
			 * Teleport here: Move instantly the player to this node.
			 * Create new link: Create a link with *no* restriction.
			 * Set node properties: Display a message box to set things like name, icon...		
			 * Enter in: Enter in a non-standard paths (only if the node is the player node)
	 		 */
		    void **data = g_malloc0(2 * sizeof(gpointer));
	  		GtkWidget *pop_menu;
	  		GtkWidget *teleport_item, *enter_item, *create_item;

			/* Create data */
			data[0] = automap;
			data[1] = clicked_node;
		
//	  		GtkWidget* properties_item;
			
			/* create the menu */
			pop_menu = gtk_menu_new();
			
			/* Teleport here */
	  		teleport_item = gtk_menu_item_new_with_label(_("Teleport here"));
	  		gtk_menu_shell_append(GTK_MENU_SHELL(pop_menu), teleport_item);
			gtk_signal_connect(GTK_OBJECT(teleport_item), "activate", GTK_SIGNAL_FUNC(on_teleport_item_activated), data);
//
//			/* Set node properties */
//	  		properties_item = gtk_menu_item_new_with_label(_("Set node properties"));
//			gtk_menu_shell_append(GTK_MENU_SHELL(pop_menu), properties_item);
//			gtk_signal_connect(GTK_OBJECT(properties_item), "activate", GTK_SIGNAL_FUNC(on_properties_item_activated), data);

			/* Create new link */
			create_item = gtk_menu_item_new_with_label(_("Create new link"));
			gtk_menu_shell_append(GTK_MENU_SHELL(pop_menu), create_item);
			gtk_signal_connect(GTK_OBJECT(create_item), "activate", GTK_SIGNAL_FUNC(on_create_link_item_activated), data);

			/* Enter in */
			if (automap->player == clicked_node)
			{	/* Translator: this is an action, not a key */
				enter_item = gtk_menu_item_new_with_label(_("Enter"));
				gtk_menu_shell_append(GTK_MENU_SHELL(pop_menu), enter_item);
				gtk_signal_connect(GTK_OBJECT(enter_item), "activate", GTK_SIGNAL_FUNC(on_enter_item_activated), data);
			}
			
			/* show the menu */
	  		gtk_menu_popup(GTK_MENU(pop_menu), NULL, NULL, NULL, NULL, 3, gtk_get_current_event_time());
	  		gtk_widget_show_all(pop_menu);
			
			return TRUE;
		}
	    
  	}

    return FALSE; /* Propogate signal */
}

static void undraw_hollow_rectangle (AutoMap *automap, gint x, gint y,
				     gint width, gint height)
{

    /* Redraw the pixmap over the old selection box */
    if (width < 0) {
        x += width;
        width = -width;
    }

    if (height < 0) {
        y += height;
        height = -height;
    }

    /* Top left to top right */
    gdk_draw_pixmap(automap->draw_area->window,
                    automap->draw_area->style->fg_gc[GTK_WIDGET_STATE (automap->draw_area)],
                    automap->pixmap, x, y, x, y, width + 1, 1);

    /* Bottom left to bottom right */
    gdk_draw_pixmap(automap->draw_area->window,
                    automap->draw_area->style->fg_gc[GTK_WIDGET_STATE (automap->draw_area)],
                    automap->pixmap, x, y + height, x, y + height, width + 1, 1);

    /* Top left to bottom left */
    gdk_draw_pixmap(automap->draw_area->window,
                    automap->draw_area->style->fg_gc[GTK_WIDGET_STATE (automap->draw_area)],
                    automap->pixmap, x, y, x, y, 1, height + 1);

    /* Top right to bottom right */
    gdk_draw_pixmap(automap->draw_area->window,
                    automap->draw_area->style->fg_gc[GTK_WIDGET_STATE (automap->draw_area)],
                    automap->pixmap, x + width, y, x + width, y, 1, height + 1);
}

static gint button_release_event (GtkWidget *widget, GdkEventButton *event,
				  AutoMap *automap)
{

    struct win_scale *ws = map_coords(automap);

    if (event->button == 1 && automap->state != NONE)
    {
        gint x_off = automap->x_offset / ws->mapped_unit;
        gint y_off = automap->y_offset / ws->mapped_unit;
        GList *puck = automap->selected;
        MapNode *node;

        if (automap->state == SELECTMOVE)
        {
            for (; puck != NULL; puck = puck->next)
            {
                node = puck->data;
                node_hash_remove(automap->map->nodes, node);
                node->x += x_off;
                node->y -= y_off;
                node_hash_prepend(automap->map->nodes, node);
            }

            automap->x_offset = automap->y_offset = 0;
            automap->x_orig = (gint16)rint(event->x);
            automap->y_orig = (gint16)rint(event->y);
            automap->map->min_x = automap->map->max_x = automap->x;
            automap->map->min_y = automap->map->max_y = automap->y;

            redraw_map(automap);
        } else {
            automap->selected = g_list_concat(automap->selected, automap->in_selection_box);
            automap->in_selection_box = NULL;

            undraw_hollow_rectangle(automap, automap->selection_box.x,
                                    automap->selection_box.y, automap->selection_box.width,
                                    automap->selection_box.height);
        }

        draw_selected(automap, ws);

        automap->state = NONE;
    }

    return FALSE; /* Propogate signal */
}

static gint motion_notify_event (GtkWidget *widget, GdkEventMotion *event,
				 AutoMap *automap)
{

    gint32 x, y;
    GdkModifierType state;

    if (event->is_hint)
    {
        gdk_window_get_pointer(event->window, &x, &y, &state);
    } else {
        x = event->x;
        y = event->y;
        state = event->state;
    }

    if (state & GDK_BUTTON1_MASK) {

        /* First mouse button is depressed
         */

        struct win_scale *ws = map_coords(automap);

        if (automap->state == SELECTMOVE)
        { /* check for automap->selected ? */
            if (x > automap->x_orig + automap->x_offset + ws->mapped_unit/2 ||
                x < automap->x_orig + automap->x_offset - ws->mapped_unit/2 ||
                y > automap->y_orig + automap->y_offset + ws->mapped_unit/2 ||
                y < automap->y_orig + automap->y_offset - ws->mapped_unit/2)
            {

                undraw_selected(automap, ws);

                if (x > automap->x_orig + automap->x_offset + ws->mapped_unit/2)
                    automap->x_offset += ws->mapped_unit;
                else if (x < automap->x_orig + automap->x_offset - ws->mapped_unit/2)
                    automap->x_offset -= ws->mapped_unit;

                if (y > automap->y_orig + automap->y_offset + ws->mapped_unit/2)
                    automap->y_offset += ws->mapped_unit;
                else if (y < automap->y_orig + automap->y_offset - ws->mapped_unit/2)
                    automap->y_offset -= ws->mapped_unit;

                draw_selected(automap, ws);
            }
        } else if (automap->state == BOXSELECT) {
            gint rx = automap->selection_box.x;
            gint ry = automap->selection_box.y;
            gint rwidth = automap->selection_box.width;
            gint rheight = automap->selection_box.height;
            GList *puck;

            undraw_selected(automap, ws);
            undraw_hollow_rectangle(automap, rx, ry, rwidth, rheight);

            rwidth = automap->selection_box.width = x - rx;
            rheight = automap->selection_box.height = y - ry;

            if (rwidth < 0)
            {
                rx += rwidth;
                rwidth = -rwidth;
            }

            if (rheight < 0)
            {
                ry += rheight;
                rheight = -rheight;
            }

            /* Draw the rectangle */
            gdk_draw_rectangle(automap->draw_area->window,
                               automap->draw_area->style->black_gc,
                               FALSE, rx, ry, rwidth, rheight);
            g_list_free(automap->in_selection_box);

            automap->in_selection_box = NULL;

            /* And now insert unselected nodes to go in the selected node list */
            for (puck = automap->visible; puck != NULL; puck = puck->next)
            {
                RectNode *rn = puck->data;
                Rectangle rect;

                rect.x = rx; rect.y = ry; rect.width = rwidth; rect.height = rheight;

                if (in_rectangle(rn->rectangle.x + rn->rectangle.width / 2,
                                 rn->rectangle.y + rn->rectangle.height / 2,
                                 &rect))
                {
                    /* Select this object, if it's not already in the list */
                    if (!g_list_find(automap->selected, rn->node))
                    {
                        automap->in_selection_box = g_list_prepend(automap->in_selection_box, rn->node);
                    }
                }
            }

            draw_selected(automap, map_coords(automap));
        }
    }

    return TRUE; /* Terminate */
}

static void scrollbar_adjust (AutoMap *automap)
{

    struct win_scale *ws = map_coords(automap);

    /* Can do with a little optimisation here, but too lazy
     */

    GtkAdjustment *hsb = GTK_ADJUSTMENT(automap->hsbdata);
    GtkAdjustment *vsb = GTK_ADJUSTMENT(automap->vsbdata);

    hsb->page_size = ws->mapped_width / 2;

    if (automap->map->max_x - automap->map->min_x < ws->mapped_width / 2)
    {
        hsb->lower = automap->map->min_x ;
        hsb->upper = automap->map->min_x + ws->mapped_width / 2;
        hsb->value = automap->map->min_x ;
    } else {
        hsb->lower = automap->map->min_x ;
        hsb->upper = automap->map->max_x + ws->mapped_width / 2;
        hsb->value = automap->x;
    }

    vsb->page_size = ws->mapped_height / 2;

    if (automap->map->max_y - automap->map->min_y < ws->mapped_height / 2)
    {
        vsb->lower = automap->map->min_y ;
        vsb->upper = automap->map->min_y + ws->mapped_height / 2;
        vsb->value = automap->map->min_y;
    } else {
        vsb->lower = automap->map->min_y;
        vsb->upper = automap->map->max_y + ws->mapped_height / 2;
        vsb->value = automap->map->max_y + automap->map->min_y - automap->y;
    }

#if DEBUG
    g_print("hsb->lower=%.4f, hsb->upper=%.4f\n", hsb->lower, hsb->upper);
    g_print("vsb->lower=%.4f, vsb->upper=%.4f\n", vsb->lower, vsb->upper);
    g_print("hsb->value=%.4f, vsb->value=%.4f\n", hsb->value, vsb->value);
    g_print("automap->x=%d, automap->y=%d\n", automap->x, automap->y);
    g_print("ws->mapped_x=%d, ws->mapped_y=%d\n\n", ws->mapped_x, ws->mapped_y);
#endif

    automap->modifying_coords = TRUE;

    gtk_signal_emit_by_name(GTK_OBJECT(hsb), "value_changed");
    gtk_signal_emit_by_name(GTK_OBJECT(hsb), "changed");
    gtk_signal_emit_by_name(GTK_OBJECT(vsb), "value_changed");
    gtk_signal_emit_by_name(GTK_OBJECT(vsb), "changed");

    automap->modifying_coords = FALSE;

    redraw_map(automap);
}

static gint configure_event (GtkWidget *widget, GdkEventConfigure *event,
			     AutoMap *automap)
{

    /* Check if the pixmap exists */

    if (automap->pixmap)
        gdk_pixmap_unref(automap->pixmap);

    /* Create the pixmap */
    automap->pixmap = gdk_pixmap_new(automap->draw_area->window,
                                     automap->draw_area->allocation.width,
                                     automap->draw_area->allocation.height,
                                     -1);

    if (automap->map)
    {
        /* Recenter the screen */
        automap->x = automap->player->x;
        automap->y = automap->player->y;
        redraw_map(automap);
        scrollbar_adjust(automap);
    } else {
        /* Clear the pixmap ... */
        gdk_draw_rectangle(automap->pixmap,
                           automap->draw_area->style->white_gc, TRUE, 0, 0,
                           automap->draw_area->allocation.width,
                           automap->draw_area->allocation.height);
    }

    return TRUE; /* Terminate */
}

static inline guint get_direction_type (char *text)
{
    /* Can easily be optimised ... but why bother ? */

	 /* Translator: "N" means "North" here */
         if (!strcasecmp(text, _("N")     )) return NORTH;
	 /* Translator: "NE" means "Northeast" here */
    else if (!strcasecmp(text, _("NE")    )) return NORTHEAST;
	 /* Translator: "E" means "East" here */
    else if (!strcasecmp(text, _("E")     )) return EAST;
	 /* Translator: "SE" means "Southeast" here */
    else if (!strcasecmp(text, _("SE")    )) return SOUTHEAST;
	 /* Translator: "S" means "South" here */
    else if (!strcasecmp(text, _("S")     )) return SOUTH;
	 /* Translator: "SW" means "Southwest" here */
    else if (!strcasecmp(text, _("SW")    )) return SOUTHWEST;
	 /* Translator: "W" means "West" here */
    else if (!strcasecmp(text, _("W")     )) return WEST;
	 /* Translator: "NW" means "Northwest" here */
    else if (!strcasecmp(text, _("NW")    )) return NORTHWEST;
    else if (!strcasecmp(text, _("Up")    )) return UP;
    else if (!strcasecmp(text, _("Down")  )) return DOWN;
    else if (!strcasecmp(text, _("Remove"))) return REMOVE;
    else if (!strcasecmp(text, _("Load")  )) return LOAD;
    else if (!strcasecmp(text, _("Save")  )) return SAVE;
         g_error("get_direction_type: unknown direction string: %s\n", text);

    gtk_exit(1);

    exit(1); /* Shut GCC up */
}

static inline gchar* get_direction_text(guint type)
{
	switch (type)
	{
		/*case NORTH:     return g_strdup(g_slist_nth_data(main_connection->profile->directions,DIRECTION_NORTH));
		case NORTHWEST: return g_strdup(g_slist_nth_data(main_connection->profile->directions,DIRECTION_NORTHWEST));
		case NORTHEAST: return g_strdup(g_slist_nth_data(main_connection->profile->directions,DIRECTION_NORTHEAST));
		case WEST:      return g_strdup(g_slist_nth_data(main_connection->profile->directions,DIRECTION_WEST));
		case EAST:      return g_strdup(g_slist_nth_data(main_connection->profile->directions,DIRECTION_EAST));
		case SOUTHWEST: return g_strdup(g_slist_nth_data(main_connection->profile->directions,DIRECTION_SOUTHWEST));
		case SOUTHEAST: return g_strdup(g_slist_nth_data(main_connection->profile->directions,DIRECTION_SOUTHEAST));
		case SOUTH:     return g_strdup(g_slist_nth_data(main_connection->profile->directions,DIRECTION_SOUTH));
		case UP:		return g_strdup(g_slist_nth_data(main_connection->profile->directions,DIRECTION_UP));
		case DOWN:		return g_strdup(g_slist_nth_data(main_connection->profile->directions,DIRECTION_DOWN));*/
		case NORTH:     return g_strdup(g_slist_nth_data(dirlist,DIRECTION_NORTH));
		case NORTHWEST: return g_strdup(g_slist_nth_data(dirlist,DIRECTION_NORTHWEST));
		case NORTHEAST: return g_strdup(g_slist_nth_data(dirlist,DIRECTION_NORTHEAST));
		case WEST:      return g_strdup(g_slist_nth_data(dirlist,DIRECTION_WEST));
		case EAST:      return g_strdup(g_slist_nth_data(dirlist,DIRECTION_EAST));
		case SOUTHWEST: return g_strdup(g_slist_nth_data(dirlist,DIRECTION_SOUTHWEST));
		case SOUTHEAST: return g_strdup(g_slist_nth_data(dirlist,DIRECTION_SOUTHEAST));
		case SOUTH:     return g_strdup(g_slist_nth_data(dirlist,DIRECTION_SOUTH));
		case UP:		return g_strdup(g_slist_nth_data(dirlist,DIRECTION_UP));
		case DOWN:		return g_strdup(g_slist_nth_data(dirlist,DIRECTION_DOWN));
	}

	return NULL;
}
static inline void node_reposition (guint type, guint *x, guint *y)
{
    switch(type) {
    case NORTH:             (*y)++; return;
    case NORTHEAST: (*x)++; (*y)++; return;
    case EAST:      (*x)++;         return;
    case SOUTHEAST: (*x)++; (*y)--; return;
    case SOUTH:             (*y)--; return;
    case SOUTHWEST: (*x)--; (*y)--; return;
    case WEST:      (*x)--;         return;
    case NORTHWEST: (*x)--; (*y)++; return;
    default:
        g_warning("node_reposition: invalid type(%s) passed to this function\n",
                  type == UP ? "up" : "down");
    }

    return;
}

/* Process for drawing the graphs
 *  Draw node draw only one node (if this node is visible)
 *  draw_map call draw_node for each entry in automap->nodes
 */

static inline
void translate (AutoMap *automap, struct win_scale *ws, Point *p)
{
    p->x = (gint)((p->x - automap->x) * ws->mapped_unit) + ws->width / 2;
    p->y = ws->height / 2 - (gint)((p->y - automap->y) * ws->mapped_unit);
}

inline gboolean adjust (struct win_scale *ws, Rectangle *rect)
{

    if (rect->x < 0)
    {
        if (rect->x + rect->width >= 0)
        {
            rect->width += rect->x;
            rect->x = 0;
        } else
            return FALSE;
    } else if (rect->x + rect->width >= ws->width)
    {
        if (rect->x < ws->width)
            rect->width = ws->width - rect->x;
        else
            return FALSE;
    }

    if (rect->y < 0)
    {
        if (rect->y + rect->height >= 0)
        {
            rect->height += rect->y;
            rect->y = 0;
        } else
            return FALSE;
    } else if (rect->y + rect->height >= ws->height)
    {
        if (rect->y < ws->height)
        {
            rect->height = ws->height - rect->y;
        } else
            return FALSE;
    }

    return TRUE;
}

static void teleport_player(AutoMap* automap, MapNode* target)
{
	struct win_scale *ws;
	ws = map_coords(automap);

	if (automap->map == target->map)
	{
		MapNode *nodelist[] = { automap->player, NULL };

    	draw_dot(automap, ws, automap->player);
    	blit_nodes(automap, ws, nodelist);

	    nodelist[0] = automap->player = target;
    	draw_player(automap, ws, automap->player);
    	blit_nodes(automap, ws, nodelist);
	}
	else
	{
		automap->player = target;
		automap->map = target->map;
		maplink_change_map(automap->map);
		redraw_map(automap);
	}
}

void draw_dot (AutoMap *automap, struct win_scale *ws, MapNode *node)
{
    /* nodewidth is the distance from the nodes centre, to the nodes
     * edge (not its diagonal edge)
     */
    gint nodewidth = ws->mapped_unit / 4 - 1;

    Point p = { node->x, node->y };
    translate(automap, ws, &p);

    /* Clear the area first */
    gdk_draw_rectangle(automap->pixmap,
                       automap->draw_area->style->white_gc, TRUE,
                       p.x - nodewidth, p.y - nodewidth,
                       nodewidth * 2, nodewidth * 2);

    /* Outline */
    gdk_draw_rectangle(automap->pixmap,
                       automap->draw_area->style->black_gc, FALSE,
                       p.x - nodewidth, p.y - nodewidth,
                       nodewidth * 2, nodewidth * 2);

    /* Going up ? Draw up arrow */
    if (node->connections[UP].node)
    {
        gdk_draw_line(automap->pixmap,
                      automap->draw_area->style->black_gc,
                      p.x + (nodewidth + 1)/2, p.y - (nodewidth + 1)/5*4,
                      p.x + (nodewidth + 1)/2, p.y + (nodewidth + 1)/5*4);

        gdk_draw_line(automap->pixmap,
                      automap->draw_area->style->black_gc,
                      p.x + (nodewidth + 1)/2, p.y - (nodewidth + 1)/5*4,
                      p.x + (nodewidth + 1)/5, p.y - (nodewidth + 1)/5*2);

        gdk_draw_line(automap->pixmap,
                      automap->draw_area->style->black_gc,
                      p.x + (nodewidth + 1)/2, p.y - (nodewidth + 1)/5*4,
                      p.x + (nodewidth + 1)/5*4, p.y - (nodewidth + 1)/5*2);
    }

    /* Going down ? Draw down arrow */
    if (node->connections[DOWN].node)
    {
        gdk_draw_line(automap->pixmap,
                      automap->draw_area->style->black_gc,
                      p.x - (nodewidth + 1)/2, p.y + (nodewidth + 1)/5*4,
                      p.x - (nodewidth + 1)/2, p.y - (nodewidth + 1)/5*4);

        gdk_draw_line(automap->pixmap,
                      automap->draw_area->style->black_gc,
                      p.x - (nodewidth + 1)/2, p.y + (nodewidth + 1)/5*4,
                      p.x - (nodewidth + 1)/5, p.y + (nodewidth + 1)/5*2);

        gdk_draw_line(automap->pixmap,
                      automap->draw_area->style->black_gc,
                      p.x - (nodewidth + 1)/2, p.y + (nodewidth + 1)/5*4,
                      p.x - (nodewidth + 1)/5*4, p.y + (nodewidth + 1)/5*2);
    }
	
	/* Paths? Draw a door */ 
	if (node->gates || node->in_gates)
	{
		int x = p.x; 	
		if (node->connections[UP].node)
			x = x - (nodewidth + 1) / 2;
		
		gdk_draw_line(automap->pixmap,
					  automap->draw_area->style->black_gc,
					  x + (nodewidth + 1)/3, p.y - (nodewidth + 1)/3,
					  x + (nodewidth + 1)/3, p.y - (nodewidth + 1)*2/3);
		
		gdk_draw_line(automap->pixmap,
					  automap->draw_area->style->black_gc,
					  x + (nodewidth + 1)*2/3, p.y - (nodewidth + 1)/3,
					  x + (nodewidth + 1)*2/3, p.y - (nodewidth + 1)*2/3);
		
		gdk_draw_line(automap->pixmap,
					  automap->draw_area->style->black_gc,
					  x + (nodewidth + 1)/3, p.y - (nodewidth + 1)/3,
					  x + (nodewidth + 1)*2/3, p.y - (nodewidth + 1)/3); 
	
		gdk_draw_arc(automap->pixmap,
			    	 automap->draw_area->style->black_gc, FALSE,
					 x + (nodewidth + 1)/3, p.y - (nodewidth + 1)*5/6,
					 (nodewidth + 1)/3, (nodewidth + 1)/3,
					 0, 180*64);		
	}
	
	/* Now insert the node into the visible nodes list (for selecting items)
     */
    if (node_visible (node, ws))
    {
        GList *puck = automap->visible;

        for (; puck != NULL; puck = puck->next)
        {
            if ( ((RectNode *)puck->data)->node == node )
                break;
        }

        if (puck == NULL)
        {
            /* Not in the list */
            RectNode *rn = g_malloc0(sizeof(RectNode));

            if (rn == NULL)
            {
                g_error("draw_nodes: g_malloc0 error: %s\n", strerror(errno));
                gtk_exit(1);
            }

            rn->rectangle.x = (node->x - automap->x) * ws->mapped_unit +
                ws->width / 2 - 3;

            rn->rectangle.y = ws->height / 2 - (gint)((node->y - automap->y) *
                                                      ws->mapped_unit) - 3;

            rn->rectangle.width = rn->rectangle.height = 6;
            rn->node = node;

            automap->visible = g_list_prepend(automap->visible, rn);
        }
    }
}

static void blit_nodes (AutoMap *automap, struct win_scale *ws,
			MapNode *nodelist[])
{

    /* nodewidth is the distance from the nodes centre, to the nodes
     * edge (not its diagonal edge)
     */
    gint nodewidth = ws->mapped_unit / 4;

    Point p = { nodelist[0]->x, nodelist[0]->y };
    Rectangle rect;
    gint x_lower, y_lower, x_higher, y_higher;
    int i;

    /* Set up bounds initially */
    translate(automap, ws, &p);

    x_lower = p.x - nodewidth;
    x_higher = p.x + nodewidth;
    y_lower = p.y - nodewidth;
    y_higher = p.y + nodewidth;

    for (i = 1; nodelist[i] != NULL; i++)
    {
        p.x = nodelist[i]->x; p.y = nodelist[i]->y;
        translate(automap, ws, &p);

        if (p.x - nodewidth < x_lower)
            x_lower = p.x - nodewidth;
        else if (p.x + nodewidth > x_higher)
            x_higher = p.x + nodewidth;

        if (p.y - nodewidth < y_lower)
            y_lower = p.y - nodewidth;
        else if (p.y + nodewidth > y_higher)
            y_higher = p.y + nodewidth;
    }

    rect.x = x_lower;
    rect.y = y_lower;
    rect.width = x_higher - x_lower + 1;
    rect.height = y_higher - y_lower + 1;

    if (adjust(ws, &rect))
    {
        gdk_draw_drawable(automap->draw_area->window,
                          automap->draw_area->style->fg_gc[GTK_WIDGET_STATE (automap->draw_area)],
                          automap->pixmap, rect.x, rect.y, rect.x, rect.y, rect.width, rect.height);
    }
}

static void blank_nodes (AutoMap *automap, struct win_scale *ws,
			 MapNode *nodelist[])
{

    /* nodewidth is the distance from the nodes centre, to the nodes
     * edge (not its diagonal edge)
     */
    gint nodewidth = ws->mapped_unit / 4 - 1;

    Point p = { nodelist[0]->x, nodelist[0]->y };
    Rectangle rect;
    gint x_lower, y_lower, x_higher, y_higher;
    int i;

    /* Set up bounds initially */
    translate(automap, ws, &p);

    x_lower = p.x - nodewidth;
    x_higher = p.x + nodewidth;
    y_lower = p.y - nodewidth;
    y_higher = p.y + nodewidth;

    for (i = 1; nodelist[i] != NULL; i++)
    {
        p.x = nodelist[i]->x; p.y = nodelist[i]->y;
        translate(automap, ws, &p);

        if (p.x - nodewidth < x_lower)
            x_lower = p.x - nodewidth;
        else if (p.x + nodewidth > x_higher)
            x_higher = p.x + nodewidth;

        if (p.y - nodewidth < y_lower)
            y_lower = p.y - nodewidth;
        else if (p.y + nodewidth > y_higher)
            y_higher = p.y + nodewidth;
    }

    rect.x = x_lower;
    rect.y = y_lower;
    rect.width = x_higher - x_lower + 1;
    rect.height = y_higher - y_lower + 1;

    if (adjust(ws, &rect)) {
        gdk_draw_rectangle(automap->pixmap,
                           automap->draw_area->style->white_gc, TRUE,
                           rect.x, rect.y, rect.width, rect.height);
    }
}

static void draw_selected (AutoMap *automap, struct win_scale *ws)
{
    gint16 radius = 5 * ws->mapped_unit / 40; /* ceil (sqrt (3*3 + 3*3)) */
    GList *list_start, *puck;
    MapNode *node;
    gint x, y;
    GdkGC *gc = gdk_gc_new(automap->draw_area->window);

    gdk_gc_set_foreground(gc, &red);

    if (automap->selected)
        list_start = puck = automap->selected;
    else if (automap->in_selection_box)
        list_start = puck = automap->in_selection_box;
    else
        return;

    for (;;)
    {
        node = (MapNode *)puck->data;
        x = (gint)((node->x - automap->x) * ws->mapped_unit) + ws->width / 2;
        x += automap->x_offset;
        y = ws->height / 2 - (gint)((node->y - automap->y) * ws->mapped_unit);
        y += automap->y_offset;

        if (node == automap->player)
        {
            gdk_draw_arc(automap->draw_area->window, gc, TRUE,
                         x - radius, y - radius, radius * 2, radius * 2, 0, 360*64);
        } else {
            gdk_draw_rectangle(automap->draw_area->window, gc, TRUE,
                               x - (3 * ws->mapped_unit / 40), y - (3 * ws->mapped_unit / 40), 6 * ws->mapped_unit / 40, 6 * ws->mapped_unit / 40);
        }

        puck = puck->next;

        if (puck == NULL)
        {
            if (list_start == automap->selected && automap->in_selection_box)
                list_start = puck = automap->in_selection_box;
            else
                break;
        }
    }

    gdk_gc_destroy(gc);
}

static void undraw_selected (AutoMap *automap, struct win_scale *ws)
{
    gint16 radius = 5 * ws->mapped_unit / 40; /* ceil (sqrt (3*3 + 3*3)) */
    GList *list_start, *puck;
    MapNode *node;
    Point p;
    Rectangle rect;

    if (automap->selected)
        list_start = puck = automap->selected;
    else if (automap->in_selection_box)
        list_start = puck = automap->in_selection_box;
    else
        return;

    for (;;)
    {
        node = (MapNode *)puck->data;
        p.x = node->x; p.y = node->y;
        translate(automap, ws, &p);
        p.x += automap->x_offset;
        p.y += automap->y_offset;

        if (node == automap->player)
        {
            rect.x = p.x - radius; rect.y = p.y - radius;
            rect.width = rect.height = radius * 2;
        } else {
            rect.x = p.x - (3 * ws->mapped_unit / 40); rect.y = p.y - (3 * ws->mapped_unit / 40);
            rect.width = rect.height = 6 * ws->mapped_unit / 40;
        }

        if (adjust(ws, &rect))
        {
            gdk_draw_pixmap(automap->draw_area->window,
                            automap->draw_area->style->fg_gc[GTK_WIDGET_STATE (automap->draw_area)],
                            automap->pixmap, rect.x, rect.y, rect.x, rect.y, rect.width, rect.height);
        }

        puck = puck->next;

        if (puck == NULL)
        {
            if (list_start == automap->selected && automap->in_selection_box)
                list_start = puck = automap->in_selection_box;
            else
                break;
        }
    }
}

void draw_player (AutoMap *automap, struct win_scale *ws, MapNode *node)
{
    gint16 radius = 5 * ws->mapped_unit / 40; /* ceil (sqrt (3*3 + 3*3)) */ // Dfaut of mapped_unit is 40
    Point p = { node->x, node->y };
    Rectangle rect;

    translate(automap, ws, &p);
	
	rect.x = p.x - radius;
    rect.y = p.y - radius;
    rect.width = rect.height = radius * 2;

    if (adjust(ws, &rect))
    {
        gdk_draw_arc(automap->pixmap,
                     automap->draw_area->style->black_gc, TRUE,
                     rect.x, rect.y, rect.width, rect.height, 0, 360*64);
    }
}

static void clear_block (AutoMap *automap, struct win_scale *ws, gint x,
			 gint y)
{
    guint half = ws->mapped_unit / 2;
    Point p = { x, y };
    Rectangle rect;

    translate(automap, ws, &p);
    rect.x = p.x - half;
    rect.y = p.y - half;
    rect.width = rect.height = ws->mapped_unit;

    if (adjust(ws, &rect))
    {
        gdk_draw_rectangle(automap->pixmap,
                           automap->draw_area->style->white_gc,
                           TRUE, rect.x , rect.y, rect.width, rect.height);
    }
}

static void draw_nodes (AutoMap *automap, struct win_scale *ws,
			MapNode *start, MapNode *parent, GHashTable *hash)
{
    int i;
    MapNode *next;

    if (hash)
    {
        GSList *unvisited = NULL, *puck;
        guint8 *count = g_hash_table_lookup(hash, start);
        guint8 *next_count;

        if (start->x < automap->map->min_x)
            automap->map->min_x = start->x;
        else if (start->x > automap->map->max_x)
            automap->map->max_x = start->x;

        if (start->y < automap->map->min_y)
            automap->map->min_y = start->y;
        else if (start->y < automap->map->min_y)
            automap->map->max_y = start->y;

        for (i = 0; i < 8; i++)
        {
            next = start->connections[i].node;

            if (!next) continue;
            next_count = g_hash_table_lookup(hash, next);

            if (!next_count)
            {
                next_count = g_malloc0(sizeof(guint8));
                g_hash_table_insert(hash, next, next_count);
                unvisited = g_slist_prepend(unvisited, next);
            } else if (*next_count == next->conn)
                continue;

            (*count)++;
            (*next_count)++;

#if DEBUG
            if (start->x <= next->x)
                g_print("Drawing from (%3d, %3d) to (%3d, %3d)\n",
                        start->x, start->y, next->x, next->y);
            else
                g_print("Drawing from (%3d, %3d) to (%3d, %3d)\n",
                        next->x, next->y, start->x, start->y);
#endif
        }

        for (puck = unvisited; puck != NULL; puck = puck->next)
            draw_nodes(automap, ws, (MapNode *)puck->data, start, hash);
        
        g_slist_free(unvisited);
    } else {
        if (!parent)
		{
            /* Clear this entire pixel block */
            clear_block(automap, ws, start->x, start->y);}
    	}

   draw_dot(automap, ws, start);
}

/* Wrapper function over draw_map, which fixes up the
 * pixmaps and copying scrollbars, drawing selected items
 * etc etc
 */
void redraw_map (AutoMap *automap)
{
    GList *puck;

    /* Clear the pixmap ... */
    gdk_draw_rectangle(automap->pixmap,
                       automap->draw_area->style->white_gc, TRUE, 0, 0,
                       automap->draw_area->allocation.width,
                       automap->draw_area->allocation.height);

    /* Remove the visible elements list */
    for (puck = automap->visible; puck != NULL; puck = puck->next)
        free(puck->data);

    g_list_free(automap->visible);
    automap->visible = NULL;

    /* Draw the map */
    draw_map(automap);

    /* Copy it to the drawable */
    gdk_draw_pixmap(automap->draw_area->window,
                    automap->draw_area->style->fg_gc[GTK_WIDGET_STATE (automap->draw_area)],
                    automap->pixmap, 0, 0, 0, 0,
                    automap->draw_area->allocation.width,
                    automap->draw_area->allocation.height
                   );

    draw_selected(automap, map_coords(automap));
}

static gboolean free_node_count (MapNode *key, guint8 *count,
				 gpointer user_data)
{
    free(count);
    return TRUE;
}

void draw_map (AutoMap *automap)
{
    struct win_scale *ws = map_coords(automap);
    GList *puck;

    /* Draw all links */
	maplink_draw_all(ws);

	/* And now, draw the nodes */
    for (puck = automap->map->nodelist; puck != NULL; puck = puck->next)
    {
        GHashTable *hash = g_hash_table_new((GHashFunc)node_hash, (GCompareFunc)node_comp );
        MapNode *start = puck->data;
        guint8 *count = g_malloc0(sizeof(guint8));

        if (!count)
        {
            g_error("draw_map: g_malloc0 error: %s\n", strerror(errno));
            gtk_exit(1);
        }

        g_hash_table_insert(hash, start, count);
        draw_nodes(automap, ws, start, NULL, hash);
        g_hash_table_foreach_remove(hash, (GHRFunc)free_node_count, NULL);
        g_hash_table_destroy(hash);
    }

    /* Draw the player */
    if (automap->player)
        draw_player(automap, ws, automap->player);
}

static gboolean restore_message(AutoMap *automap)
{
	gtk_label_set_text(GTK_LABEL(automap->hint), automap->last_unlimited_message);
	return FALSE; /* This functions must be called only one time */
}

static void show_message(AutoMap *automap, gchar *message, guint time)
{
	if (time == 0)
	{
		if (automap->last_unlimited_message)
			g_free(automap->last_unlimited_message);
		
		automap->last_unlimited_message = g_strdup(message);
		
		gtk_label_set_text(GTK_LABEL(automap->hint), automap->last_unlimited_message);
	}
	if (time > 0)
	{
		gtk_label_set_text(GTK_LABEL(automap->hint), message);
		gtk_timeout_add(time, (GtkFunction) restore_message, automap);
	}
}

static void get_node_through_path(gchar *key, MapNode *node, GHashTable *hash)
{
	if (!g_hash_table_lookup(hash, node))
	{
		guint *n = g_malloc0(sizeof(guint));
		*n = g_hash_table_size(hash);
		get_nodes(hash, node->map, n);
		g_free(n);
	}
}

static void get_node (GHashTable *hash, MapNode *node, guint *n)
{
    gint *num = g_malloc(sizeof(gint));
    gint i;

    if (!num)
    {
        g_error("get_node: g_malloc error: %s\n", strerror(errno));
        gtk_exit(1);
    }

    *num = (*n)++;
    g_hash_table_insert(hash, node, num);

    for (i = 0; i < 8; i++)
    {
        MapNode *next = node->connections[i].node;

        if (next && !g_hash_table_lookup(hash, next))
            get_node(hash, next, n);
    }

    for (i = 8; i < 10; i++)
    {
        MapNode *next = node->connections[i].node;

        if (next && !g_hash_table_lookup(hash, next))
            get_nodes(hash, next->map, n);
    }

	if (node->gates)	
		g_hash_table_foreach(node->gates, (GHFunc) get_node_through_path, hash);
	if (node->in_gates)
		g_hash_table_foreach(node->in_gates, (GHFunc) get_node_through_path, hash);
}

static void get_nodes (GHashTable *hash, Map *map, guint *n)
{
    GList *puck;
	
    for (puck = map->nodelist; puck != NULL; puck = puck->next)
    {
        MapNode *node = puck->data;

        if (g_hash_table_lookup(hash, node))
            continue;

        get_node(hash, node, n);
    }
}

static void write_link(Link* link, guint nbr, void *data[5])
{
	FILE *file = data[0];
	GHashTable *hash_maps = data[2];
	
	fprintf(file, "%d map %d ", nbr, (gint) g_hash_table_lookup(hash_maps, link->node->map->name));
	
	link = link_reorder(link);
	while (link)
	{
		fprintf(file, "(%d, %d) ", link->x, link->y);
		link = link->next;
	}
	
	fprintf(file, "\n");
}
static inline void write_path (gchar* key, MapNode* target, void *data[2])
{
	fprintf(data[0], "%s %d",
			key,
			*(gint*) g_hash_table_lookup(data[1], target));
	
}

static void write_node (MapNode *node, guint *num, void *data[5])
{
    FILE *file = data[0];
    GHashTable *hash_nodes = data[1];
	GHashTable *hash_maps = data[2];
	GHashTable *hash_links = data[3];
	guint *nbr_links = data[4];
    int i;

    fprintf(file, "%d (%d, %d) %d ", *num, node->x, node->y, (gint) g_hash_table_lookup(hash_maps, node->map->name));

    for (i = 0; i < 10; i++)
    {
        MapNode *next = node->connections[i].node;

		if (next == NULL)
		{
			fprintf(file, "%s -1 ", direction[i]);
		}
		else if (i>7)
		{
	        fprintf(file, "%s %d ", direction[i], *(gint *)g_hash_table_lookup(hash_nodes, next));
		}
		else if (link_is_standard(node, i))
		{
	        fprintf(file, "%s %d ", direction[i], *(gint *)g_hash_table_lookup(hash_nodes, next));
    	}
		else
		{
			guint index;
			Link *opposite = link_reorder(node->connections[i].link);
						
			
			/* Gets the opposite link to test if the link has already been registred */
			while (opposite->next)
				opposite = opposite->next;
			
			index = GPOINTER_TO_INT(g_hash_table_lookup(hash_links, opposite));
			
			if (!index)
			{
				(*nbr_links)++;
				index = *nbr_links;
				g_hash_table_insert(hash_links, node->connections[i].link, (gpointer) index); 
			}
			fprintf(file, "%s *%d ", direction[i], index);
		}
	}
	
	if (node->gates)
		g_hash_table_foreach(node->gates, (GHFunc) write_path, data);

    fputs("\n", file);
}

static gboolean remove_node (MapNode *node, guint *num, void *data)
{
    g_free(num);
    return TRUE;
}

static void save_maps (const gchar *filename, AutoMap *automap)
{
    GHashTable *hash_nodes;
	GHashTable *hash_maps;
	GHashTable *hash_links;
	guint *nbr_links = g_malloc0(sizeof(guint));
    GList *puck;
    Map *map;
    FILE *file;
    int n = 0;
    void *data[3];
    int nbr = 0;

    file = fopen(filename, "w");

    if (file == NULL)
    {
        g_warning("save_maps: Can't open %s for writing: %s\n",
                  filename, strerror(errno));
        return;
    }

	*nbr_links = 0;
	hash_links = g_hash_table_new(g_direct_hash, g_direct_equal);
    hash_nodes = g_hash_table_new(g_direct_hash, g_direct_equal);
	hash_maps = g_hash_table_new(g_str_hash, g_str_equal);
    get_nodes(hash_nodes, automap->map, &n);

    fprintf(file, "automap map %s ~map player %d zoom %.2f, center (%d, %d)\n",
            automap->map->name, *(gint *)g_hash_table_lookup(hash_nodes, automap->player),
            automap->zoom, automap->x, automap->y);

    /* For all maps known, if elements in the map's nodelist are
     * in the hash, then all nodes in that map are saved. We
     * check for this because it's possible (although not yet
     * with the code) for maps to be listed, yet not be referenced
     * by all nodes in the hash
     */
    for (puck = MapList; puck != NULL; puck = puck->next)
    {
        GList *inner;

        map = puck->data;

        if (!g_hash_table_lookup(hash_nodes, map->nodelist->data))
            continue;

		g_hash_table_insert(hash_maps, map->name, (void*) nbr);
		nbr++;
		fprintf(file, "map %s ~map min_x %d min_y %d max_x %d max_y %d nodelist ", map->name,
                map->min_x, map->min_y, map->max_x, map->max_y);
       
        for (inner = map->nodelist; inner != NULL; inner = inner->next)
            fprintf(file, "%d ", *(gint *)g_hash_table_lookup(hash_nodes, inner->data));

        fputs("\n", file);
    }

    data[0] = file;
	data[1] = hash_nodes;
    data[2] = hash_maps;
	data[3] = hash_links;
	data[4] = nbr_links;

	/* Write all nodes */
    g_hash_table_foreach(hash_nodes, (GHFunc)write_node, (gpointer)data);

	/* Write all links */
	fprintf(file, "--Links--\n");
	g_hash_table_foreach(hash_links, (GHFunc)write_link, data);
	
    g_hash_table_foreach_remove(hash_nodes, (GHRFunc)remove_node, NULL);
	g_hash_table_destroy(hash_links);
    g_hash_table_destroy(hash_nodes);
	g_hash_table_destroy(hash_maps);

    fclose(file);
}

static inline gboolean free_key (void *key, void *data, void *data2)
{
	g_free(key);
	
	return TRUE;
}

static void free_node (MapNode *node)
{	
	if (node->gates)
	{
		g_hash_table_foreach_remove(node->gates, (GHRFunc) free_key, NULL);
		g_hash_table_destroy(node->gates);
	}
	if (node->in_gates)
	{
		g_hash_table_foreach_remove(node->in_gates, (GHRFunc) free_key, NULL);
		g_hash_table_destroy(node->in_gates);
	}
	g_free(node);
}

static inline gboolean free_nodes_from_hash(MapNode *key, GList *list, void *data)
{
	free_node(key);
	g_list_free(list);
	return TRUE;
}

static void free_maps (AutoMap *automap)
{
    Map *map;
	GList *puck;

    while (MapList)
    {
        map = (Map *)MapList->data;
        MapList = g_list_remove_link(MapList, MapList);

        g_hash_table_foreach_remove(map->nodes, (GHRFunc)free_nodes_from_hash, NULL);
        g_hash_table_destroy(map->nodes);
        g_list_free(map->nodelist);
        g_free(map->name);
        g_free(map);
    }

	maplink_free();
	
    automap->player = NULL;
    automap->map = NULL;
    automap->x = automap->y = 0;
	for (puck = automap->visible; puck != NULL; puck = puck->next)
		g_free(puck->data);
    g_list_free(automap->visible);

    automap->visible = NULL;
    g_list_free(automap->selected);
    automap->selected = NULL;
    automap->state = NONE;
    /* automap->in_selection_box should be NULL here, hence ignore */
}

static void file_sel_ok_cb (GtkWidget *widget, void *ptr[])
{
    guint type = GPOINTER_TO_UINT(ptr[0]);
    AutoMap *automap = ptr[1];
    GtkFileSelection *find = GTK_FILE_SELECTION(ptr[2]);
    const gchar *filename = gtk_file_selection_get_filename(find);

    struct stat filestat;

    if (type == LOAD)
    {
        if (stat(filename, &filestat) != 0)
        {
            g_warning("file_sel_ok_cb: stat() error: %s\n", strerror(errno));

            return;
        }

        if (filestat.st_size == 0)
        {
            g_warning("file_sel_ok_cb: file %s is zero bytes in length\n", filename);

            return;
        }

        /* Bother checking for symlinks ? Don't know */

        if (! S_ISREG(filestat.st_mode))
        {
            g_warning("file_sel_ok_cb: file %s is not a regular file\n", filename);

            return;
        }
    }

    {
        /*
         * read/written to ...
         */

        FILE *file = fopen(filename, type == LOAD ? "r" : "w");

        if (file == NULL)
        {
            char *action = type == LOAD ? "reading" : "writing";
            g_warning("Can't open file %s for %s: %s\n",
                      filename, action, strerror(errno));
            return;
        }

        fclose(file);
    }

    if (type == LOAD)
    {
        load_automap_from_file(filename, automap);
    } else {
        save_maps(filename, automap);
    }

    /* When this widget is destroyed, filename disappears too (well, it does
     * when the gtk+/gdk/glib/whatever function is called)
     * So destroy it after load_automap_from_file is called! :-)
     */
    
    /* This should invoke file_sel_destroy */
    gtk_widget_destroy(GTK_WIDGET(find));
    g_free(ptr);
}

static void file_sel_cancel_cb (GtkWidget *widget, void *ptr[])
{
    gtk_widget_destroy(GTK_WIDGET(ptr[2]));
    g_free(ptr);
}

static void file_sel_delete_event (GtkWidget *widget, GdkEventAny *event,
				   void *ptr[])
{
    gtk_widget_destroy(GTK_WIDGET(ptr[2]));
    g_free(ptr);
}

static void button_cb (GtkWidget *widget, AutoMap *automap)
{
    GtkWidget *find;
    gchar *text = GTK_LABEL( GTK_BIN(widget)->child )->label, dir[256];
	const gchar *home;
    guint type = get_direction_type(text);

    struct stat dirstat;

    void **ptr;

    switch (type)
    {
    case REMOVE:
        remove_player_node(automap);
        break;

    case LOAD:
    case SAVE:

        ptr = g_malloc(sizeof(void *[3]));

        if (ptr == NULL)
        {
            g_error("button_cb: g_malloc error: %s\n", strerror(errno));
            return;
        }

        find = gtk_file_selection_new(type == LOAD ? _("Load map") : _("Save map"));

        ptr[0] = GUINT_TO_POINTER(type);
        ptr[1] = automap;
        ptr[2] = find;

        home = g_get_home_dir();

        if (home == NULL)
        {
            strcpy(dir, "default.map");
        } else {
            strcpy(dir, home);
            strcat(dir, "/.gnome-mud/");

            if (!stat(dir, &dirstat) && S_ISDIR(dirstat.st_mode)) {
                strcat(dir, "default.map");
            } else {
                strcpy(dir, "default.map");
            }
        }

        gtk_file_selection_set_filename(GTK_FILE_SELECTION(find), dir);
        gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION (find));
        
        gtk_signal_connect(GTK_OBJECT (GTK_FILE_SELECTION (find)->ok_button),
                           "clicked", GTK_SIGNAL_FUNC(file_sel_ok_cb), ptr);
        gtk_signal_connect(GTK_OBJECT (GTK_FILE_SELECTION (find)->cancel_button),
                           "clicked", GTK_SIGNAL_FUNC(file_sel_cancel_cb), ptr);
        gtk_signal_connect(GTK_OBJECT (find), "delete_event",
                           GTK_SIGNAL_FUNC(file_sel_delete_event), ptr);

        gtk_widget_show(find);
        break;

    default:
        move_player(automap, type);
    }

    return;
}

static gint can_reach_node (MapNode *current, MapNode *dest, GHashTable *hash)
{
    int i;

    if (g_hash_table_lookup(hash, current))
        return FALSE;

    g_hash_table_insert(hash, current, current);

    for (i = 0; i <= 9; i++)
    {
        MapNode *next = current->connections[i].node;

        if (next == NULL) continue;

        if (next == dest) return TRUE;

        if (can_reach_node(next, dest, hash)) return TRUE;
    }

    return FALSE;
}

static gint sp_compare (SPVertex *insert, SPVertex *listnode)
{
    return insert->working - listnode->working;
}

static void fill_node_path (SPVertex *cvertex, MapNode *dest,
			    GHashTable *hash, GList **list)
{
    gint i;
    MapNode *next;
    SPVertex *vertex;

    /* Step 1:
     *
     * Look at all connections, add them to the list
     * Cut some corners because the weighting is guaranteed
     * (for now) to be one
     */

    for (i = 0; i < 10; i++)
    {
        next = cvertex->node->connections[i].node;

        if (!next) continue;

        vertex = g_hash_table_lookup(hash, next);

        if (vertex == NULL)
        {
            vertex = g_malloc(sizeof(SPVertex));

            if (vertex == NULL)
            {
                g_error("fill_node_path: g_malloc error: %s\n", strerror(errno));
                gtk_exit(1);
            }

            vertex->node = next;
            vertex->label = -1;
            vertex->working = cvertex->label + 1;

            g_hash_table_insert(hash, next, vertex);
            *list = g_list_insert_sorted(*list, vertex, (GCompareFunc)sp_compare);

        } else if (vertex->label == -1) {

            if (vertex->working > cvertex->working + 1)
            {
                vertex->working = cvertex->working + 1;
                *list = g_list_remove(*list, vertex);
                *list = g_list_insert_sorted(*list, vertex, (GCompareFunc)sp_compare);
            }
        }
    }
}

static gboolean free_vertexes (MapNode *key, SPVertex *vertex,
			       gpointer user_data)
{
    g_free(vertex);
    return TRUE;
}

static void node_goto (AutoMap *automap, struct win_scale *ws, MapNode *dest)
{
    GHashTable *hash;
    GList *list = NULL, *puck;
    guint16 order[10] = { 0, 2, 4, 6, 1, 3, 5, 7, 9, 10 };
    SPVertex *vertex = g_malloc0(sizeof(SPVertex));

    if (vertex == NULL)
    {
        g_error("node_goto: g_malloc0 error: %s\n", strerror(errno));
        gtk_exit(1);
    }

    if (dest == automap->player) return;

    /* What makes this function interesting is that we must be able
     * to traverse up and down nodes too
     */
    hash = g_hash_table_new( (GHashFunc)g_direct_hash, (GCompareFunc)g_direct_equal);

    if (!can_reach_node(automap->player, dest, hash))
        return;

    g_hash_table_destroy(hash);

    /* Use Dijkstra's shortest path algorithm, with the weighting between
     * nodes set at a constant of 1
     */

    hash = g_hash_table_new( (GHashFunc)g_direct_hash, (GCompareFunc)g_direct_equal);

    vertex->node = automap->player;

    g_hash_table_insert(hash, automap->player, vertex);
    list = g_list_prepend(NULL, vertex);

    do {
        vertex = (SPVertex *)list->data;
        list = g_list_remove_link(list, list);
        vertex->label = vertex->working;

        if (vertex->node != dest)
            fill_node_path(vertex, dest, hash, &list);
    } while (list);

    /* Now start from the destination node, and work our way back to the start
     * node
     */

    vertex = g_hash_table_lookup(hash, dest);

    for (;;)
    {
        gint i;
        MapNode *node;
        SPVertex *next;

        for (i = 0; i < 10; i++)
        {
            node = vertex->node->connections[order[i]].node;

            if (!node) continue;

            next = g_hash_table_lookup(hash, node);

            if (next == NULL)
            {
                /* This can happen and is normal
                 * g_print("Hmm, next vertex is null");
                 * while (1);
                 */
                continue;
            }

            if (vertex->label - next->label == 1)
            {
                vertex = next;
                break;
#if 0
            } else {
                g_print("From (%d, %d): %d tried (%d, %d): %d\n",
                        vertex->node->x, vertex->node->y, vertex->label,
                        next->node->x, next->node->y, next->label);
#endif
            }
        }

        if (i == 10)
        {
            g_print("Currently at (%d, %d)\n", vertex->node->x, vertex->node->y);
            g_print("Hmm, no connections led to node\n");
            while(1);
        }

        list = g_list_prepend(list, direction_long[OPPOSITE(order[i])]);
        if (vertex->node == automap->player) break;
    }

    /* And now free the hash table and all vertexes with in */
    g_hash_table_foreach_remove(hash, (GHRFunc)free_vertexes, NULL);
    g_hash_table_destroy(hash);

    /* And do something with this list */
    for (puck = list; puck != NULL; puck = puck->next)
    {
        g_print(puck->next == NULL ? "%s\n" : "%s, ", (gchar *)puck->data);
    }

    g_list_free(list);
}

static void node_break (AutoMap *automap, guint type)
{
    MapNode *this = automap->player;
    MapNode *next = automap->player->connections[type].node;
    MapNode *nodelist[3] = { this, next, NULL };

    struct win_scale *ws = map_coords(automap);

    if (!next)
    {
        show_message(automap, _("No link existed in that direction"), 750);
        return;
    }

    if (type > 7)
    {
        show_message(automap, _("Cannot break links to a node going up or down"), 750);
        return;
    }

    /* FIXME
     * Just redraw the two nodes, minus the removed link
     * However if there is a node line that passes through this broken link,
     * then we are in 'deep shit', and that link will appear broken. The image
     * would have to be redrawn
     *
     * So ... store all lines drawn in a hash somewhere to test if making an
     * image modification will require more redraws to maintain image
     * correctness ? hmm ...
	 *
	 * That normally work now.
     */

	maplink_delete(this, type);
    blit_nodes(automap, ws, nodelist);
}

static void move_player_real(AutoMap *automap, guint type)
{
    MapNode *this = automap->player;
    MapNode *next = automap->player->connections[type].node;
    struct win_scale *ws = map_coords(automap);
    guint opposite = OPPOSITE(type);
    gboolean redraw = FALSE;
    GList *puck;

    /* Check if this is following a path */
    if (next)
    {

        /* FIXME indicate which areas have been 'tainted', so have to
         * be redrawn
         */

        /* If the node is on a different map, recenter the map */
        automap->map    = next->map;
        automap->player = next;

        if (automap->map != this->map)
        {
            automap->x = next->x;
            automap->y = next->y;
			maplink_change_map(automap->map);

            redraw = TRUE;
        }
    }
	else
	{
        /* See if there are any nodes on this path */
        if (type == UP || type == DOWN)
        {
            /* Create a new map */
            automap->map = map_new();
            next = g_malloc0(sizeof(MapNode));
			
            if (next == NULL)
            {
                g_error("main: g_malloc0 error: %s\n", strerror(errno));
                gtk_exit(1);
            }

            /* Add this unreferenced initial node to the map and hash */
            automap->map->nodelist = g_list_append(automap->map->nodelist, next);
            node_hash_prepend(automap->map->nodes, next);

            /* Link the two nodes up */
            next->map = automap->map;
			maplink_change_map(next->map);
            next->connections[opposite].node = automap->player;
            automap->player->connections[type].node = next;
            automap->player = next;

            /* Reset the scrollbar stuff, fixed up selected data
             * redraw_map takes care of freeing automap->visible
             */
            automap->last_hvalue = 0;
            automap->last_vvalue = 0;
            remove_selected(automap);

            /* Recalculate the common window info calculated */
            ws = map_coords(automap);

            redraw = TRUE;
        } 
		else
		{
            GList *list;
            MapNode node;

            node.x = automap->player->x; node.y = automap->player->y;
            node_reposition(type, &node.x, &node.y);
            list = g_hash_table_lookup(automap->map->nodes, &node);

            if (list)
            {
                /* Use the top (first) node in this list, to join this node to */
                next = (MapNode *)list->data;

                if (next->connections[opposite].node)
                {
                    /* It would seem this node already has a connection to here, That's 
					 * possible so we need to inform the user that his move is illegal.
                     */

                    show_message(automap, _("Destination node has already a connection here"), 1500);
                    return;

                }
				else
				{
                    /* If the node we're connecting to is part of a different trail
                     * (ie automap->map->nodelist start point is different to the start
                     * point of this node), then delete the start point of the other node
                     * (since we are connecting start points)
                     */

                    MapNode *my_start_node, *next_start_node;
                    GHashTable *hash;

                    hash = g_hash_table_new((GHashFunc)node_hash, (GCompareFunc)node_comp);
                    my_start_node = connected_node_in_list(automap->map, this, hash, NULL);
                    g_hash_table_destroy(hash);

                    hash = g_hash_table_new((GHashFunc)node_hash, (GCompareFunc)node_comp);
                    next_start_node = connected_node_in_list(automap->map, next, hash, NULL);
                    g_hash_table_destroy(hash);

                    if (my_start_node != next_start_node)
                        automap->map->nodelist =
                            g_list_remove(automap->map->nodelist, next_start_node);

                }
            }
			else
			{
			  Pos pos;
                next = g_malloc0(sizeof(MapNode));

                next->x = node.x;
                next->y = node.y;

 				node_hash_prepend(automap->map->nodes, next);
				
				/* Test if there is links passing by this position
				 * If true, we need to redraw the previous and the next part
				 * of this link.
				 */
				pos.x = next->x * 2;
				pos.y = next->y * 2;
				puck = g_hash_table_lookup(actual->link_table, &pos);
				while (puck)
				{
					Link* link = puck->data;
					
					if (link->type == LK_PART)
					{
						gint x, y;
						
						/* We need to delete manually the point between the node and 
						 * the adjacent links because maplink_draw doesn't
						 */
			    		x = ( (gint) ((link->x + link->next->x - 4 * automap->x) * ws->mapped_unit / 2) + ws->width ) / 2;
   						y = ( ws->height - (gint)((link->y + link->next->y - 4 * automap->y) * ws->mapped_unit / 2) ) / 2;
						gdk_draw_point(automap->pixmap,
									   automap->draw_area->style->white_gc,
					   				   x, y);
			    		x = ( (gint) ((link->x + link->prev->x - 4 * automap->x) * ws->mapped_unit / 2) + ws->width ) / 2;
   						y = ( ws->height - (gint)((link->y + link->prev->y - 4 * automap->y) * ws->mapped_unit / 2) ) / 2;
						gdk_draw_point(automap->pixmap,
									   automap->draw_area->style->white_gc,
					   				   x, y);
									   
						maplink_draw(link->prev->x, link->prev->y, ws);
						maplink_draw(link->next->x, link->next->y, ws);
					}
					
					puck = puck->next;
				}
				
			}

			maplink_create(automap->player, next);
			
            next->map = automap->map;
            automap->player = next;
        }
    }

    /* Check the boundaries to see if they have been extended */
    if (next->x <= automap->map->min_x)
        automap->map->min_x = next->x;
    else if (next->x >= automap->map->max_x)
        automap->map->max_x = next->x;
    if (next->y <= automap->map->min_y)
        automap->map->min_y = next->y;
    else if (next->y >= automap->map->max_y)
        automap->map->max_y = next->y;

    /* If the node is not on the screen, then centre the view, redraw the screen
     * and fix up the scrollbars
     */
    if ((next->x <= ws->mapped_x) || (next->x >= ws->mapped_x + ws->mapped_width) ||
        (next->y <= ws->mapped_y) || (next->y >= ws->mapped_y + ws->mapped_height) ||
        redraw)
    {
        automap->x = next->x;
        automap->y = next->y;

        scrollbar_adjust(automap);
    }
	else
	{

        /* Pass the old co-ordinates along with the automap details to the drawing
         * routine and add the new node to the automap->visible list
         */
		MapNode *thislist[] = { this, NULL };
        MapNode *nodelist[] = { this, next, NULL };

		blank_nodes(automap, ws, thislist);
		draw_dot(automap, ws, this);
		draw_dot(automap, ws, next);
        draw_player(automap, ws, next);
        blit_nodes(automap, ws, nodelist);
        draw_selected(automap, ws);
    }
}

static void move_player (AutoMap *automap, guint type)
{
  struct win_scale *ws;
  gchar *text;
  gchar command[15];

    if (automap->node_break)
    {

        /* Doesn't belong here ? Lazyness ... */
        automap->node_break = FALSE;
        node_break(automap, type);

        return;
    }
	if (automap->create_link_data)
	{
		Link *link;
		int x, y;
		for (link = automap->create_link_data->link; link->next != NULL; link = link->next) { }
		x = link->x;
		y = link->y;
		
		if (type == UP || type == DOWN)
		{
			show_message(automap, _("Can't create a link to another floor!"), 2000);
			return;
		}
		
		if (automap->create_link_data->first_move == -1 && automap->player->connections[type].node != NULL)
		{
			show_message(automap, _("There is already a link here!"), 1000);
			return;
		}
		
		/* Add a part */
		node_reposition(type, &x, &y);
		automap->create_link_data->link = link_add(automap->create_link_data->link, LK_PART, x, y);
		link = link->next;
		
		/* Update the data */
		if (automap->create_link_data->first_move == -1)
			automap->create_link_data->first_move = type;
		automap->create_link_data->last_move = type;

	    /* If the node is not on the screen, then centre the view, redraw the screen
   		 * and fix up the scrollbars
     	 */
		ws = map_coords(automap);

    	if ((link->x <= 2 * ws->mapped_x) || (link->x >= 2 * (ws->mapped_x + ws->mapped_width)) ||
        	(link->y <= 2 * ws->mapped_y) || (link->y >= 2 * (ws->mapped_y + ws->mapped_height)))
    	{
        	automap->x = (gint) link->x / 2;
        	automap->y = (gint) link->y / 2;

        	scrollbar_adjust(automap);
    	}
		else
		{		
			link_draw(link, ws); // Draw the new part (the old one has been drawed by link_add)
			link_blit(link, ws);	
		}

		return;
	}

	/* Call the real function */
	move_player_real(automap, type);
	
	/* And to finish, send the command to the server */
	text = get_direction_text(type);
	g_snprintf(command, 15, "%s\r\n", text);
	
	if(cd)
		plugin_connection_send(command, cd);
	//connection_send(main_connection, command);
	g_free(text);
}

static void scrollbar_value_changed (GtkAdjustment *adjustment,
				     AutoMap *automap)
{
    GtkAdjustment *vsb, *hsb;
    gint redraw = 0;

    if (automap->modifying_coords)
        return;

    hsb = GTK_ADJUSTMENT(automap->hsbdata);
    vsb = GTK_ADJUSTMENT(automap->vsbdata);

    /* Check horizontal scrollbar */
    if ((gint)hsb->value != automap->last_hvalue)
    {
        redraw++;

        automap->last_hvalue = hsb->value;
        automap->x = hsb->value;
    }

    /* Horizontal scrollbar */
    if ((gint)vsb->value != automap->last_vvalue) {
        redraw++;

        automap->last_vvalue = vsb->value;
        automap->y = -(vsb->value - automap->map->max_y - automap->map->min_y);
    }

    if (redraw)
        redraw_map(automap);
}

static gint enter_notify_event (GtkWidget *widget, GdkEventCrossing *event,
				AutoMap *automap)
{
    gtk_window_set_focus (GTK_WINDOW(automap->window), automap->draw_area);

    /* If the program exit while key repeating is disabled, then on my
     * X server all repeated key events are disabled too, and I can't enable
     * them by calling gdk_key_repeat_restore in a different application.
     */
    /*gdk_key_repeat_disable();*/

    return TRUE;
}

static gint leave_notify_event (GtkWidget *widget, GdkEventCrossing *event,
                         	AutoMap *automap)
{
    /*gdk_key_repeat_restore();*/
    return TRUE;
}

static void automap_quit(GtkWidget *widget, AutoMap *automap)
{
	/* Remove the automap from the AutoMapList */
	free_maps(automap);
	g_free(automap);
	AutoMapList = g_list_remove(AutoMapList, automap);
}

AutoMap *auto_map_new (void)
{
    AutoMap *automap = g_malloc0(sizeof(AutoMap));
    GtkWidget *hbox, *updownvbox, *loadsavevbox, *vbox, *sep;
    GtkWidget *n, *ne, *e, *se, *s, *sw, *w, *nw, *up, *down;
    GtkWidget *load, *save, *remove;
    GtkWidget *table, *table_draw;
	GtkWidget *hint;

	if (g_list_length(AutoMapList) != 0)
	{
		g_free(automap);
		return NULL;
	}
	
    if (automap == NULL)
    {
        g_error("auto_map_new: g_malloc0 error: %s\n", strerror(errno));
        gtk_exit(1);
    }

    /* Create main window */
    automap->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

    /* Set the title */
    /*g_snprintf(name, 100, "window%d", g_list_length(AutoMapList));*/
    /*gtk_window_set_title(GTK_WINDOW(automap->window), name);*/
    gtk_window_set_title (GTK_WINDOW(automap->window), _("GNOME-Mud AutoMapper"));

    /* Create the drawing window and allocate its colours */
    automap->draw_area = gtk_drawing_area_new();
    gtk_drawing_area_size(GTK_DRAWING_AREA(automap->draw_area),
                          X_INIT_SIZE, Y_INIT_SIZE);
    gdk_color_alloc(gtk_widget_get_colormap(automap->draw_area), &red);

    /* Let the window focus (so it can capture key events */
    GTK_WIDGET_SET_FLAGS(automap->draw_area, GTK_CAN_FOCUS);

    /* Specify which events must be generated on the drawing area */
    gtk_widget_set_events(automap->draw_area,
                            GDK_EXPOSURE_MASK
                          | GDK_BUTTON_PRESS_MASK
                          | GDK_BUTTON_RELEASE_MASK
                          | GDK_BUTTON_MOTION_MASK
                          | GDK_KEY_PRESS_MASK
                          | GDK_KEY_RELEASE_MASK
                          | GDK_ENTER_NOTIFY_MASK
                          | GDK_LEAVE_NOTIFY_MASK
                          | GDK_POINTER_MOTION_MASK
                          | GDK_POINTER_MOTION_HINT_MASK);

    /* Now connect some signals for drawing/redrawing the backing pixmap */
    gtk_signal_connect(GTK_OBJECT(automap->draw_area), "expose_event",
                       GTK_SIGNAL_FUNC(expose_event), automap);
    
    gtk_signal_connect(GTK_OBJECT(automap->draw_area), "configure_event",
                       GTK_SIGNAL_FUNC(configure_event), automap);

    /* Set up the event handler signals for movement and mouse clicking  */
    gtk_signal_connect(GTK_OBJECT(automap->draw_area), "button_press_event",
                       GTK_SIGNAL_FUNC(button_press_event), automap);

    gtk_signal_connect(GTK_OBJECT(automap->draw_area), "button_release_event",
                       GTK_SIGNAL_FUNC(button_release_event), automap);

    gtk_signal_connect(GTK_OBJECT(automap->draw_area), "motion_notify_event",
                       GTK_SIGNAL_FUNC(motion_notify_event), automap);

    /* And key events */
    gtk_signal_connect(GTK_OBJECT(automap->draw_area), "key_press_event",
                       GTK_SIGNAL_FUNC(key_press_event), automap);

    gtk_signal_connect(GTK_OBJECT(automap->draw_area), "key_release_event",
                       GTK_SIGNAL_FUNC(key_release_event), automap);

    /* And the enter/leave notification events, to set window focus/key repetition
     */
    gtk_signal_connect(GTK_OBJECT(automap->draw_area), "enter_notify_event",
                       GTK_SIGNAL_FUNC(enter_notify_event), automap);

    gtk_signal_connect(GTK_OBJECT(automap->draw_area), "leave_notify_event",
                       GTK_SIGNAL_FUNC(leave_notify_event), automap);

	/* When the automapper ends */
	gtk_signal_connect(GTK_OBJECT(automap->window), "destroy",
					   GTK_SIGNAL_FUNC(automap_quit), automap);

    /* Set up the scrollbars/scrollbar data */
    {
        gint ps = X_INIT_SIZE / PIX_ZOOM, upper;

        if (ps & 1) ps++;
        upper = ps / 2;

        automap->last_hvalue = 0;
        automap->hsbdata = gtk_adjustment_new(0, 0, upper, 1, ps, ps);

        ps = Y_INIT_SIZE / PIX_ZOOM;

        if (ps & 1) ps++;
        upper = ps/2;

        automap->last_vvalue = 0;
        automap->vsbdata = gtk_adjustment_new(0, 0, upper, 1, ps, ps);
    }

    gtk_signal_connect(GTK_OBJECT(automap->hsbdata), "value_changed",
                       GTK_SIGNAL_FUNC(scrollbar_value_changed), automap);

    gtk_signal_connect(GTK_OBJECT(automap->vsbdata), "value_changed",
                       GTK_SIGNAL_FUNC(scrollbar_value_changed), automap);

    automap->hsb = gtk_hscrollbar_new(GTK_ADJUSTMENT(automap->hsbdata));
    automap->vsb = gtk_vscrollbar_new(GTK_ADJUSTMENT(automap->vsbdata));

    gtk_range_set_update_policy(GTK_RANGE(automap->hsb), GTK_UPDATE_CONTINUOUS);
    gtk_range_set_update_policy(GTK_RANGE(automap->vsb), GTK_UPDATE_CONTINUOUS);

	/* Create the hint label */
	hint = gtk_label_new(_("Ready"));
	gtk_misc_set_alignment(GTK_MISC(hint), 0, 0);
	automap->hint = hint;
	automap->last_unlimited_message = g_strdup(_("Ready"));
	
    /* Pack the range/drawing widgets into the draw table */
    table_draw = gtk_table_new(3, 2, FALSE);
    gtk_table_attach(GTK_TABLE(table_draw), automap->draw_area,
                     0, 1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
    gtk_table_attach(GTK_TABLE(table_draw), automap->hsb,
                     0, 1, 1, 2, GTK_EXPAND | GTK_FILL,                     0, 0, 0);
    gtk_table_attach(GTK_TABLE(table_draw), automap->vsb,
                     1, 2, 0, 1, 0                    , GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(table_draw), automap->hint,
					 0, 2, 2, 3, GTK_EXPAND | GTK_FILL,                     0, 0, 0); 

    gtk_widget_show(automap->hsb);
    gtk_widget_show(automap->vsb);
    gtk_widget_show(automap->draw_area);
	gtk_widget_show(automap->hint);
    gtk_widget_show(table_draw);

    /* Some buttons */
    load = gtk_button_new_with_label(_("Load"));
    save = gtk_button_new_with_label(_("Save"));
    remove = gtk_button_new_with_label(_("Remove"));

    /* Create button directions */
    n  = gtk_button_new_with_label(_("N" ));
    ne = gtk_button_new_with_label(_("NE"));
    e  = gtk_button_new_with_label(_("E" ));
    se = gtk_button_new_with_label(_("SE"));
    s  = gtk_button_new_with_label(_("S" ));
    sw = gtk_button_new_with_label(_("SW"));
    w  = gtk_button_new_with_label(_("W" ));
    nw = gtk_button_new_with_label(_("NW"));
    up = gtk_button_new_with_label(_("Up"));
    down = gtk_button_new_with_label(_("Down"));

    table = gtk_table_new(3, 3, TRUE);
    gtk_table_attach_defaults(GTK_TABLE(table), n,  1, 2, 0, 1);
    gtk_table_attach_defaults(GTK_TABLE(table), ne, 2, 3, 0, 1);
    gtk_table_attach_defaults(GTK_TABLE(table), e,  2, 3, 1, 2);
    gtk_table_attach_defaults(GTK_TABLE(table), se, 2, 3, 2, 3);
    gtk_table_attach_defaults(GTK_TABLE(table), s,  1, 2, 2, 3);
    gtk_table_attach_defaults(GTK_TABLE(table), sw, 0, 1, 2, 3);
    gtk_table_attach_defaults(GTK_TABLE(table), w,  0, 1, 1, 2);
    gtk_table_attach_defaults(GTK_TABLE(table), nw, 0, 1, 0, 1);

    /* Make a seperator, and continue packing the buttons/attaching signals */
    sep = gtk_hseparator_new();

    loadsavevbox = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(loadsavevbox), load, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(loadsavevbox), save, FALSE, FALSE, 0);

    updownvbox = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(updownvbox), up, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(updownvbox), down, FALSE, FALSE, 0);

    vbox = gtk_vbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(vbox), loadsavevbox, TRUE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), remove, TRUE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), sep, TRUE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), updownvbox, TRUE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), table, TRUE, FALSE, 0);

    gtk_signal_connect(GTK_OBJECT(load), "clicked", GTK_SIGNAL_FUNC(button_cb), automap);
    gtk_signal_connect(GTK_OBJECT(save), "clicked", GTK_SIGNAL_FUNC(button_cb), automap);
    gtk_signal_connect(GTK_OBJECT(remove), "clicked", GTK_SIGNAL_FUNC(button_cb), automap);

    gtk_signal_connect(GTK_OBJECT(n)   , "clicked", GTK_SIGNAL_FUNC(button_cb), automap);
    gtk_signal_connect(GTK_OBJECT(ne)  , "clicked", GTK_SIGNAL_FUNC(button_cb), automap);
    gtk_signal_connect(GTK_OBJECT(e)   , "clicked", GTK_SIGNAL_FUNC(button_cb), automap);
    gtk_signal_connect(GTK_OBJECT(se)  , "clicked", GTK_SIGNAL_FUNC(button_cb), automap);
    gtk_signal_connect(GTK_OBJECT(s)   , "clicked", GTK_SIGNAL_FUNC(button_cb), automap);
    gtk_signal_connect(GTK_OBJECT(sw)  , "clicked", GTK_SIGNAL_FUNC(button_cb), automap);
    gtk_signal_connect(GTK_OBJECT(w)   , "clicked", GTK_SIGNAL_FUNC(button_cb), automap);
    gtk_signal_connect(GTK_OBJECT(nw)  , "clicked", GTK_SIGNAL_FUNC(button_cb), automap);
    gtk_signal_connect(GTK_OBJECT(up)  , "clicked", GTK_SIGNAL_FUNC(button_cb), automap);
    gtk_signal_connect(GTK_OBJECT(down), "clicked", GTK_SIGNAL_FUNC(button_cb), automap);

    gtk_widget_show(load);
    gtk_widget_show(save);
    gtk_widget_show(remove);

    gtk_widget_show(n);
    gtk_widget_show(ne);
    gtk_widget_show(e);
    gtk_widget_show(se);
    gtk_widget_show(s);
    gtk_widget_show(sw);
    gtk_widget_show(w);
    gtk_widget_show(nw);
    gtk_widget_show(up);
    gtk_widget_show(down);

    gtk_widget_show(sep);
    gtk_widget_show(table);
    gtk_widget_show(loadsavevbox);
    gtk_widget_show(updownvbox);
    gtk_widget_show(vbox);

    /* Add widgets to the main horizontal box, and show them */
    hbox = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox), table_draw, TRUE, TRUE, 0);

    /* Add the horizontal box to the main window, and show them */
    gtk_container_add(GTK_CONTAINER(automap->window), hbox);
    gtk_widget_show(hbox);

    return automap;
}

Map *map_new (void)
{
    Map *map;
    char name[28];

    /* Create our map */
    map = g_malloc0(sizeof(MapNode));

    if (map == NULL)
    {
        g_error("map_new: g_malloc0 error: %s\n", strerror(errno));
        gtk_exit(1);
    }

    g_snprintf(name, 27, "GNOME-Mud Mapper - map%d", g_list_length(MapList));
    map->name = g_strdup(name);

    if (map->name == NULL)
    {
        g_error("map_new: g_malloc0 error: %s\n", strerror(errno));
        gtk_exit(1);
    }

    MapList = g_list_prepend(MapList, map);

    /* Create the global hash table */
    map->nodes = g_hash_table_new((GHashFunc)node_hash, (GCompareFunc)node_comp);

    return map;
}

static void remove_map (Map *map)
{
    if (map->nodelist) g_list_free(map->nodelist);

    g_hash_table_destroy(map->nodes);
    MapList = g_list_remove(MapList, map);
    g_free(map);
}

void window_automap (GtkWidget *button, gpointer data)
{
    new_automap_with_node ();
}

static void new_automap_with_node (void)
{
    AutoMap *automap;
    Map *map;
    MapNode *node;

    /* Create our automaplist ... */
    if ((automap = auto_map_new()) == NULL)
		return;

    AutoMapList = g_list_append(AutoMapList, automap);
    automap->zoom = 1;

    /* Create our map */
    map = map_new();

    /* Create our first node and draw it */
    node = g_malloc0(sizeof(MapNode));

    if (node == NULL)
    {
        g_error("main: g_malloc0 error: %s\n", strerror(errno));
        gtk_exit(1);
    }

    /* Add this unreferenced initial node to the map and hash */
    map->nodelist = g_list_append(map->nodelist, node);
    node_hash_prepend(map->nodes, node);

    /* Finalise the automap details before configure events occur */
    automap->map = node->map = map;
    automap->player = node;

	/* Initialise the structures of map-link */
	maplink_init_new_map(map);
	
    /* Display the main window in the main() routine (ensures the
     * first node is drawn
     */
    gtk_widget_show(automap->window);
}

static gchar *get_token (gchar *text, gchar *token)
{
    while (isspace(*text) || *text == ',') text++;
    if (*text == 0)
        return NULL;

    for (; !isspace(*text) && *text != ',' && *text != 0; text++) {
        if (*text == '(' || *text == ')')
            continue;
        *token++ = *text;
    }

    *token = 0;

    return text;
}

static gboolean is_numeric (gchar *token)
{
    for (; *token != 0; token++) {
        gchar c = *token;

        if (!isdigit(c) && c != '.' && c != '-')

            return FALSE;
    }

    return TRUE;
}

static void translate_path(gchar* path, MapNode* target_index, void* data[2])
{
	GPtrArray* arr = data[0];
	MapNode* node = data[1];

	MapNode* target = g_ptr_array_index(arr, GPOINTER_TO_INT(target_index));
	
	g_hash_table_replace(node->gates, path, target);

	if (!target->in_gates)
		target->in_gates = g_hash_table_new(g_str_hash, g_str_equal);
	g_hash_table_insert(target->in_gates, path, node);
}

static void load_automap_from_file (const gchar *filename, AutoMap *automap)
{
    FILE *file;
    gchar buf[256], token[256], *bptr, *mapname, *tmp;
    GPtrArray *arr = g_ptr_array_new();
	GPtrArray *links = g_ptr_array_new();
    GList *maps = NULL, *puck;
    gint i, o, explicit_redraw = (automap != NULL);
	void* data[2];
    Map *map = NULL;
	
	mapname = g_strdup("");

    file = fopen(filename, "r");

    if (file == NULL)
    {
        g_warning("load_automap_from_file: Could not open %s for reading: %s\n",
                  filename, strerror(errno));
        return;
    }

    /* Create our automaplist ... */
    if (!automap)
	{
		automap = auto_map_new();
	    AutoMapList = g_list_append(AutoMapList, automap);
	}
    free_maps(automap);
 
    /* Get details from the file to fill in to our automap */
    bptr = fgets(buf, BUFSIZ, file);
    bptr = get_token(bptr, token);
    bptr = get_token(bptr, token);
    bptr = get_token(bptr, token);
	while (strcmp(token, "~map"))
	{
		tmp = g_strdup(mapname);
		g_free(mapname);
		mapname = g_malloc0(sizeof(mapname) + sizeof(token));
 		sprintf(mapname, "%s %s", tmp, token);
		g_free(tmp);
		
		bptr = get_token(bptr, token);
    }
	mapname = g_strstrip(mapname);
	bptr = get_token(bptr, token);
    bptr = get_token(bptr, token);
    automap->player = (MapNode *)atol(token);
    bptr = get_token(bptr, token);
    bptr = get_token(bptr, token);
    automap->zoom = atof(token);
    bptr = get_token(bptr, token);
    bptr = get_token(bptr, token);
    automap->x = atol(token);
    bptr = get_token(bptr, token);
    automap->y = atol(token);

    /* Pass 1, part 1:
     *
     * Get all map names and details, insert them into MapList. Store then node
     * numbers and in a later pass replace the node numbers with the nodes they
     * reference
     */
    for (;;) {
        bptr = fgets(buf, BUFSIZ, file);
        bptr = get_token(bptr, token);

        if (is_numeric(token))
            break;
		
		map = g_malloc0(sizeof(Map));

        map->nodes = g_hash_table_new((GHashFunc)node_hash, (GCompareFunc)node_comp);

		bptr = get_token(bptr, token);
		map->name = g_strdup("");
		while (strcmp(token, "~map"))
		{
			tmp = g_strdup(map->name);
			g_free(map->name);
			map->name = g_malloc0(sizeof(map->name) + sizeof(token));
 			sprintf(map->name, "%s %s", tmp, token);
			g_free(tmp);
		
			bptr = get_token(bptr, token);
    	}
				
		map->name =	g_strstrip(map->name);
		if (mapname && !strcmp(mapname, map->name))
        {
            g_free(mapname);
            mapname = NULL;
            automap->map = map;
        }

        bptr = get_token(bptr, token);
        bptr = get_token(bptr, token);
        map->min_x = atol(token);
        bptr = get_token(bptr, token);
        bptr = get_token(bptr, token);
        map->max_x = atol(token);
        bptr = get_token(bptr, token);
        bptr = get_token(bptr, token);
        map->min_y = atol(token);
        bptr = get_token(bptr, token);
        bptr = get_token(bptr, token);
        map->max_y = atol(token);
        bptr = get_token(bptr, token);

        while ((bptr = get_token(bptr, token)) != NULL)
            map->nodelist = g_list_prepend(map->nodelist, (gpointer)atol(token));
        
        MapList = g_list_prepend(MapList, map);
        maps = g_list_append(maps, map);
		
		maplink_init_new_map(map);
    }

    bptr = buf;

    /* Pass 1, part 2:
     *
     * Get all node details, insert them into their respective MapLists
     */
    do {
        MapNode *node;
        gint num;

        node = g_malloc0(sizeof(MapNode));
        bptr = get_token(bptr, token);
        num = atol(token);
        bptr = get_token(bptr, token);
        node->x = atol(token);

        bptr = get_token(bptr, token);
        node->y = atol(token);

        bptr = get_token(bptr, token);

        node->map = g_list_nth_data(maps, atol(token));
        node_hash_prepend(node->map->nodes, node);

        if (num >= arr->len)
            g_ptr_array_set_size(arr, num + 1);

        g_ptr_array_index(arr, num) = node;

        for (num = 0; num < 10; num++)
        {
            bptr = get_token(bptr, token);
            bptr = get_token(bptr, token);
			
			/* This means that the link is strange (can't be created with maplink_create */
			if (token[0] == '*')
			{
				gchar* toktmp = token + 1;
				node->connections[num].node = (MapNode *) atol(toktmp);
				node->connections[num].link = (gpointer) TRUE;
			}
			else
			{
            	node->connections[num].node = (MapNode *) (atol(token) + 1);
        		node->connections[num].link = NULL;
			}
		}
		
		g_strstrip(bptr);
		while (bptr[0] != '\0')
		{
			gchar* path;
			bptr = get_token(bptr, token);
			path = g_strdup(token);
			bptr = get_token(bptr, token);
			if (!node->gates)
				node->gates = g_hash_table_new(g_str_hash, g_str_equal);
			g_hash_table_insert(node->gates, path, (MapNode*) atol(token));
		}

		bptr = fgets(buf, BUFSIZ, file);
	} while (strcmp(bptr, "--Links--\n") != 0);

	/* Pass 1, part 3:
	 *
	 * Parse the file to create the strange links_arr
	 */
	while ((bptr = fgets(buf, BUFSIZ, file)) != NULL)
	{
		Link *link = NULL, *prev = NULL;
		Map *map;
		guint index;
		
		bptr = get_token(bptr, token);
		index = atol(token);
		
		bptr = get_token(bptr, token);
		bptr = get_token(bptr, token);
		map = g_list_nth_data(maps, atol(token));
		
		maplink_change_map(map);
				
		g_strstrip(bptr);
		while (bptr[0] != '\0')
		{
			GList *list;
			Pos* pos;	
			guint x, y;
			
			/* Get the coords */
			bptr = get_token(bptr, token);
			x = atol(token);
			
			bptr = get_token(bptr, token);
			y = atol(token);
			
			/* Create the link structure */
			link = g_malloc0(sizeof(Link));
			link->type = LK_PART;
			link->x = x;
			link->y = y;

			/* Create a Pos structure for the hash */
			pos = g_malloc0(sizeof(Pos));
			pos->x = x;
			pos->y = y;

			/* Append to the built-in list */
			if (prev)
			{
				prev->next = link;
				link->prev = prev;
				link->next = NULL;
			}
			else
			{
				link->prev = NULL;
				link->next = NULL;
			}

			/* Append to the list gived by global hash */
			list = g_hash_table_lookup(actual->link_table, pos);
			list = g_list_append(list, link);
			g_hash_table_steal(actual->link_table, pos); // Steal to avoid freeing the list with link_free_list
			g_hash_table_insert(actual->link_table, pos, list);

			/* Keep a pointer to the previous part */
			prev = link;
		}
		
		/* Add the link to the hash */
		if (links->len < index + 1)
			g_ptr_array_set_size(links, index + 1);
		g_ptr_array_index(links, index) = link;
	}
	
    fclose(file);

    /* Pass 2, part 1:
     *
     * Go through all maps, substituting the node numbers for the actual nodes
     */
    for (puck = maps; puck != NULL; puck = puck->next)
    {
        GList *inner;
        map = puck->data;

        for (inner = map->nodelist; inner != NULL; inner = inner->next)
            inner->data = g_ptr_array_index(arr, GPOINTER_TO_INT(inner->data));
    }

    /* Pass 2, part 2:
     *
     * Go through all map nodes, substituting the node numbers for the
     * actual nodes
     */
	data[0] = arr;
    for (i = 0; i < arr->len; i++)
    {
        MapNode *node = g_ptr_array_index(arr, i);

        for (o = 0; o < 8; o++)
		{
			if (node->connections[o].node != NULL && node->connections[o].link == NULL && GPOINTER_TO_INT(node->connections[o].node) < arr->len + 2)
			{
				MapNode* target = g_ptr_array_index(arr, GPOINTER_TO_INT(node->connections[o].node) - 1);
				node->connections[o].node = NULL;
				target->connections[OPPOSITE(o)].node = NULL;
				maplink_create(node, target);
			}
			else if ((gboolean) node->connections[o].link == TRUE )
			{
				Link* opposite;
				Link* link = g_ptr_array_index(links, (guint) node->connections[o].node);
				link = link_reorder(link);
				if (link->x == 2 * node->x && link->y == 2 * node->y && get_direction(link->x, link->y, link->next->x, link->next->y) == o)
				{
					link->node = node;
					link->type = LK_NODE;
					node->connections[o].link = link;
					
					opposite = link;
					while (opposite->next)
						opposite = opposite->next;
					
					if (opposite->node)
					{
						node->connections[o].node = opposite->node;
						opposite->node->connections[get_direction(opposite->x, opposite->y, opposite->prev->x, opposite->prev->y)].node = node;
					}
				}
				else
				{
					/* We assume that the good node is the other */
					while (link->next)
						link = link->next;
					link = link_reorder(link);
					
					link->node = node;
					link->type = LK_NODE;
					node->connections[o].link = link;
					
					opposite = link;
					while (opposite->next)
						opposite = opposite->next;
					
					if (opposite->node)
					{
						node->connections[o].node = opposite->node;
						opposite->node->connections[get_direction(opposite->x, opposite->y, opposite->prev->x, opposite->prev->y)].node = node;
					}
				}
				
			}
		}
		for (o = 8; o < 10; o++)
		{
			if (node->connections[o].node != NULL)
				node->connections[o].node = g_ptr_array_index(arr, GPOINTER_TO_INT(node->connections[o].node) - 1);
		}
		data[1] = node;
		if (node->gates)
			g_hash_table_foreach(node->gates, (GHFunc) translate_path, data);			
	}


    automap->player = g_ptr_array_index(arr, GPOINTER_TO_INT(automap->player));
	maplink_change_map(automap->player->map);
    g_ptr_array_free(arr, TRUE);
    g_list_free(maps);

    if (explicit_redraw)
    {
        scrollbar_adjust(automap);
        redraw_map(automap);
    } else {
        gtk_widget_show(automap->window);

    }
}
