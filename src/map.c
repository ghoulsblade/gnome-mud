/* GNOME-Mud - A simple Mud CLient
 * Copyright (C) 1998-2002 Robin Ericsson <lobbin@localhost.nu>
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

#include "config.h"
#ifndef WITHOUT_MAPPER

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <sys/stat.h>

#include <ctype.h>
#include <errno.h>
#include <libintl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if 0
#define USE_DMALLOC
#include <glib.h>
#include <dmalloc.h>
#endif

#define _(string) gettext(string)

static char const rcsid[] =
    "$Id$";


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

#define RGB(r, g, b) { ((r)<<24) | ((g)<<16) | ((b)<<8), (r)<<8, (g)<<8, (b)<<8 }

GdkColor red = RGB(255, 0, 0);

typedef struct _AutoMap     AutoMap;
typedef struct _Map         Map;
typedef struct _MapNode     MapNode;

typedef GdkPoint            Point;
typedef struct _Rectangle   Rectangle;
typedef struct _RectNode    RectNode;
typedef struct _SPVertex    SPVertex; /* Shortest path vertex */

struct _Map {

    gchar *name;

    /* The extents of the map */
    gint min_x, min_y, max_x, max_y;

    /* The map contains a linked list of all nodes which aren't interlinked */
    GList *nodelist;

    /* Hash table of linked lists of all MapNodes in this map */
    GHashTable *nodes;
};

char *direction[] = { "N", "NE", "E", "SE", "S", "SW", "W", "NW", "U", "D" };
char *direction_long[] = { "North", "Northeast", "East", "Southeast", "South", "Southwest", "West", "Northwest", "Up", "Down" };

/* Direction values and button direction codes */
#define NORTH     0
#define NORTHEAST 1
#define EAST      2
#define SOUTHEAST 3
#define SOUTH     4
#define SOUTHWEST 5
#define WEST      6
#define NORTHWEST 7
#define UP        8
#define DOWN      9
#define OPPOSITE(x) ( ((x) & ~7) ? (!(x - 8) + 8) : (((x) + 4) & 7) )
/* Other button codes */
#define REMOVE   10
#define LOAD     11
#define SAVE     12

#define PIX_ZOOM    40
#define X_INIT_SIZE 200
#define Y_INIT_SIZE 200
#define X_PADDING   (X_INIT_SIZE / PIX_ZOOM / 2)
#define Y_PADDING   (Y_INIT_SIZE / PIX_ZOOM / 2)

/* The join types.
 * LINE has no additional parameters, so needs no additional struct
 * ARC requires that the arc thingy be known. The rest is calculated
 */
struct _MapNode {

    gint32 x, y;
    Map *map;
    int conn; /* The number of node connections (discounting up/down) */

    /* There is a one to one mapping between node connections */
    struct {
        MapNode *node; /* The map this node is located on */
    } connections[10];
};

struct _Rectangle {

    gint16 x, y;
    gint16 width, height;
};

struct _RectNode {

    Rectangle  rectangle;
    MapNode   *node;
};

/* This struct contains the window the drawable is being drawn in, the
 * drawable, the backing pixmap, and anything else
 */
struct _AutoMap {

    GtkWidget *window;
    GtkWidget *draw_area;
    GdkPixmap *pixmap;

    /* Horizontal and vertical scrollbars, plus adjustment data */
    GtkWidget *hsb, *vsb;
    GtkObject *hsbdata, *vsbdata;
    gint last_hvalue, last_vvalue;
    
    Map *map;         /* The map which is currently being displayed    */
    gint16 x, y;      /* The center of the map currently displayed     */
    gfloat zoom;      /* Zoom factor, must be > 0, > 1 means zoom out  */
    MapNode *player;  /* The map node the player is currently on       */
    GList *visible;   /* A linked list of RectNode elements            */

    /* Use this to determine what state the program is in when the mouse
     * cursor is moving
     */
    enum { NONE, SELECTMOVE, BOXSELECT } state;

    GList *selected;  /* A list of all selected nodes */

    /* The X and Y positions where the last node was selected */
    gint16 x_orig, y_orig;

    /* The X and Y offsets of the selected nodes */
    gint16 x_offset, y_offset;

    /* A box being drawn to handle grouping selected nodes */
    Rectangle selection_box;

    /* And all nodes which fall within this box */
    GList *in_selection_box;

    /* Program states */
    guint shift : 1;
    guint node_break : 1;
    guint node_goto : 1;
    guint print_coord : 1;
    guint modifying_coords : 1;
    guint redraw_map : 1;
};

struct _SPVertex {

    MapNode *node;
    int label;
    int working;
};

struct win_scale {

    gint16 width;         /* Width of the pixmap */
    gint16 height;        /* Height of the pixmap in question */
    gint16 mapped_unit;   /* The number of pixels in a unit of the graph */
    gint16 mapped_x;      /* The leftmost x coord in the graph visible in the pixmap */
    gint16 mapped_y;      /* The bottom y coord in the graph visible in the pixmap */
    gint16 mapped_width;  /* The width of the pixmap in units of the graph */
    gint16 mapped_height; /* The height of the pixmap in units of the graph */
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
static void		 draw_dot (AutoMap *, struct win_scale *, MapNode *);
static void		 draw_nodes (AutoMap *, struct win_scale *, MapNode *,
				     MapNode *, GHashTable *);
static void		 draw_player (AutoMap *, struct win_scale *, MapNode *);
static void		 draw_selected (AutoMap *, struct win_scale *);
static void		 file_sel_cancel_cb (GtkWidget *, void *[]);
static void		 file_sel_delete_event (GtkWidget *, GdkEventAny *,
					        void *[]);
static void		 file_sel_ok_cb (GtkWidget *, void *[]);
static void		 fill_node_path (SPVertex *, MapNode *, GHashTable *,
					 GList **);
static void		 free_maps (AutoMap *);
static gboolean		 free_node_count (MapNode *, guint8 *, gpointer);
static gboolean		 free_nodes (MapNode *, GList *, GList **);
static gboolean		 free_vertexes (MapNode *, SPVertex *, gpointer);
static void		 get_nodes (GHashTable *, Map *, int *);
static gboolean		 is_numeric (gchar *);
static void		 load_automap_from_file (gchar *, AutoMap *);
static struct win_scale	*map_coords (AutoMap *);
static void		 move_player (AutoMap *, guint);
static void		 node_break (AutoMap *, guint);
static void		 new_automap_with_node (void);
static gint		 node_comp (MapNode *, MapNode *);
static void		 node_goto (AutoMap *, struct win_scale *, MapNode *);
static guint		 node_hash (MapNode *);
static void		 remove_map (Map *);
static gboolean		 remove_node (MapNode *, guint *, void *);
static void		 remove_player_node (AutoMap *);
static void		 remove_selected (AutoMap *);
static void		 scrollbar_adjust(AutoMap *);
static void		 scrollbar_value_changed (GtkAdjustment *, AutoMap *);
static gint		 sp_compare (SPVertex *, SPVertex *);
static void		 undraw_selected (AutoMap *, struct win_scale *);
       void		 window_automap (GtkWidget *, gpointer);
static void		 write_node (MapNode *, guint *, void *[]);



static struct win_scale *map_coords (AutoMap *automap)
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

static guint node_hash (MapNode *a)
{
    return (guint)(a->x << 16 | a->y);
}

static gint node_comp (MapNode *a, MapNode *b)
{
    return a->x == b->x && a->y == b->y;
}

static inline
void node_hash_prepend (GHashTable *hash, MapNode *node)
{
    GList *list = g_hash_table_lookup(hash, node);
    list = g_list_prepend(list, node);
    g_hash_table_insert(hash, node, list);
}

static inline
void node_hash_remove (GHashTable *hash, MapNode *node)
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

static void remove_selected (AutoMap *automap)
{
    undraw_selected(automap, map_coords(automap));
    g_list_free(automap->selected);
    automap->selected = NULL;
}

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
static MapNode *connected_node_in_list (Map *map, MapNode *node,
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
            if (i < 8) this->conn--;

            memset(&this->connections[OPPOSITE(i)], 0, sizeof(*this->connections));

            if (automap->player == NULL)
                automap->player = this;
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
        for (i = 0; i < 8; i++) {
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
    } else {

        /* This node had no connections, yet there were other nodes on the map */
        automap->player = automap->map->nodelist->data;
    }

    g_hash_table_destroy(hash);
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

static gint
key_press_event (GtkWidget *widget, GdkEventKey *event, AutoMap *automap)
{

    switch (event->keyval)
    {
    case GDK_q:
        gtk_exit(0); break;
        
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

static gint
button_press_event (GtkWidget *widget, GdkEventButton *event, AutoMap *automap)
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
                    MapNode *nodelist[] = { automap->player, NULL };
                    automap->state = NONE;

                    draw_dot(automap, ws, automap->player);
                    blit_nodes(automap, ws, nodelist);

                    nodelist[0] = automap->player = rn->node;
                    draw_player(automap, ws, automap->player);
                    blit_nodes(automap, ws, nodelist);

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
        hsb->lower = automap->map->min_x;
        hsb->upper = automap->map->min_x + ws->mapped_width / 2;
        hsb->value = automap->map->min_x;
    } else {
        hsb->lower = automap->map->min_x;
        hsb->upper = automap->map->max_x + ws->mapped_width / 2;
        hsb->value = automap->x;
    }

    vsb->page_size = ws->mapped_height / 2;

    if (automap->map->max_y - automap->map->min_y < ws->mapped_height / 2)
    {
        vsb->lower = automap->map->min_y;
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

         if (!strcasecmp(text, _("N")     )) return NORTH;
    else if (!strcasecmp(text, _("NE")    )) return NORTHEAST;
    else if (!strcasecmp(text, _("E")     )) return EAST;
    else if (!strcasecmp(text, _("SE")    )) return SOUTHEAST;
    else if (!strcasecmp(text, _("S")     )) return SOUTH;
    else if (!strcasecmp(text, _("SW")    )) return SOUTHWEST;
    else if (!strcasecmp(text, _("W")     )) return WEST;
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
 * Iterate over the nodes stored in Map
 * For each node, pass it to the drawing routine
 *
 * The drawing routine requires that the node that called it be
 * pass to it in the arguments list. It needs this so it can draw
 * paths from the calling node to itself.
 *
 * The draw_node routine has more than one mode of control
 *   1) It can iterate through all nodes and draw the links to them
 *   2) It only draw one node
 *
 * When iterating through all nodes and drawing them, it is possible
 * that it may run into recursion errors, so a hash table is needed to
 * store all nodes that have been drawn. This hash table should be passed
 * to it, the act of creating and destroying it is not this routine's
 * responsibility
 *
 * If passed a hash table, assume that mode 1 is the mode of control
 * (draw all nodes). If there was no hash table passed, assume mode 2.
 *
 * If in mode of control 1, then this is the process of drawing:
 *   1) If I had a parent, draw the line from the parent to me
 *   2) Check if I have any nodes that branch off me
 *   3) Foreach node that branches off me, if it is not in the hash
 *      table, then add it. Call myself with the current node as the parent,
 *      and the node that we just checked if in the hash as the start node.
 *   4) When done drawing all nodes, draw the actual 'dot' that signifies that
 *      this is a node
 *
 * If in mode of control 2:
 *   1) If I had a parent, draw the line from the parent to me
 *   2) Draw the dot that signifies this is a node
 */

static inline
void translate (AutoMap *automap, struct win_scale *ws, Point *p)
{

    p->x = (gint)((p->x - automap->x) * ws->mapped_unit) + ws->width / 2;
    p->y = ws->height / 2 - (gint)((p->y - automap->y) * ws->mapped_unit);
}

static inline
gboolean adjust (struct win_scale *ws, Rectangle *rect)
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

static void draw_line (AutoMap *automap, struct win_scale *ws,
		       MapNode *node1, MapNode *node2)
{

    /* nodewidth is the distance from the nodes centre, to the nodes
     * edge (not its diagonal edge)
     */
    gint nodewidth = ws->mapped_unit / 4;

    Point p1 = { node1->x, node1->y }, p2 = { node2->x, node2->y };
    translate(automap, ws, &p1);
    translate(automap, ws, &p2);

    /* Draw line from the edge of one node, to the edge of the other
     * node
     */

    if (p1.x < p2.x)
    {
        p1.x += nodewidth;
        p2.x -= nodewidth;
    } else if (p1.x > p2.x) {
        p1.x -= nodewidth;
        p2.x += nodewidth;
    }

    if (p1.y < p2.y)
    {
        p1.y += nodewidth;
        p2.y -= nodewidth;
    } else if (p1.y > p2.y) {
        p1.y -= nodewidth;
        p2.y += nodewidth;
    }

    gdk_draw_line(automap->pixmap,
                  automap->draw_area->style->black_gc, p1.x, p1.y, p2.x, p2.y);
}

static void draw_dot (AutoMap *automap, struct win_scale *ws, MapNode *node)
{

    /* nodewidth is the distance from the nodes centre, to the nodes
     * edge (not its diagonal edge)
     */
    gint nodewidth = ws->mapped_unit / 4;

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
                      p.x + nodewidth/2, p.y - nodewidth/5*4,
                      p.x + nodewidth/2, p.y + nodewidth/5*4);

        gdk_draw_line(automap->pixmap,
                      automap->draw_area->style->black_gc,
                      p.x + nodewidth/2, p.y - nodewidth/5*4,
                      p.x + nodewidth/5, p.y - nodewidth/5*2);

        gdk_draw_line(automap->pixmap,
                      automap->draw_area->style->black_gc,
                      p.x + nodewidth/2, p.y - nodewidth/5*4,
                      p.x + nodewidth/5*4, p.y - nodewidth/5*2);
    }

    /* Going down ? Draw down arrow */
    if (node->connections[DOWN].node)
    {
        gdk_draw_line(automap->pixmap,
                      automap->draw_area->style->black_gc,
                      p.x - nodewidth/2, p.y + nodewidth/5*4,
                      p.x - nodewidth/2, p.y - nodewidth/5*4);

        gdk_draw_line(automap->pixmap,
                      automap->draw_area->style->black_gc,
                      p.x - nodewidth/2, p.y + nodewidth/5*4,
                      p.x - nodewidth/5, p.y + nodewidth/5*2);

        gdk_draw_line(automap->pixmap,
                      automap->draw_area->style->black_gc,
                      p.x - nodewidth/2, p.y + nodewidth/5*4,
                      p.x - nodewidth/5*4, p.y + nodewidth/5*2);
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
        gdk_draw_pixmap(automap->draw_area->window,
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

    if (adjust(ws, &rect)) {
        gdk_draw_rectangle(automap->pixmap,
                           automap->draw_area->style->white_gc, TRUE,
                           rect.x, rect.y, rect.width, rect.height);
    }
}

static void draw_selected (AutoMap *automap, struct win_scale *ws)
{
    gint16 radius = 5; /* ceil (sqrt (3*3 + 3*3)) */
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
                               x - 3, y - 3, 6, 6);
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
    gint16 radius = 5; /* ceil (sqrt (3*3 + 3*3)) */
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
            rect.x = p.x - 3; rect.y = p.y - 3;
            rect.width = rect.height = 6;
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

static void draw_player (AutoMap *automap, struct win_scale *ws, MapNode *node)
{
    gint16 radius = 5; /* ceil (sqrt (3*3 + 3*3)) */
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
            draw_line(automap, ws, next, start);
        }

        for (puck = unvisited; puck != NULL; puck = puck->next)
            draw_nodes(automap, ws, (MapNode *)puck->data, start, hash);
        
        g_slist_free(unvisited);
    } else {
        if (parent)
        {
            for (i = 0; i < 8; i++)
            {
                next = start->connections[i].node;

            if (next && next != parent)
                draw_line(automap, ws, next, start);
            }
        } else {

            /* Clear this entire pixel block */
            clear_block(automap, ws, start->x, start->y);

            for (i = 0; i < 8; i++)
            {
                next = start->connections[i].node;

                if (next)
                    draw_line(automap, ws, next, start);
            }
        }
    }

    /* Now insert the node into the visible nodes list (for selecting items)
     */
    if (node_visible (start, ws))
    {
        GList *puck = automap->visible;

        for (; puck != NULL; puck = puck->next)
        {
            if ( ((RectNode *)puck->data)->node == start )
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

            rn->rectangle.x = (start->x - automap->x) * ws->mapped_unit +
                ws->width / 2 - 3;

            rn->rectangle.y = ws->height / 2 - (gint)((start->y - automap->y) *
                                                      ws->mapped_unit) - 3;

            rn->rectangle.width = rn->rectangle.height = 6;
            rn->node = start;

            automap->visible = g_list_prepend(automap->visible, rn);
        }
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

    /* Iterate over all the nodes in automap->map, and recursively draw
     * all those nodes which are on the screen
      */

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

static void get_node (GHashTable *hash, MapNode *node, gint *n)
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
}

static void get_nodes (GHashTable *hash, Map *map, int *n)
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

static void write_node (MapNode *node, guint *num, void *data[2])
{
    FILE *file = data[0];
    GHashTable *hash = data[1];
    int i;

    fprintf(file, "%d (%d, %d) %s ", *num, node->x, node->y, node->map->name);

    for (i = 0; i < 10; i++)
    {
        MapNode *next = node->connections[i].node;

        fprintf(file, "%s %d ", direction[i],
                next ? *(gint *)g_hash_table_lookup(hash, next) : -1 );
    }

    fputs("\n", file);
}

static gboolean remove_node (MapNode *node, guint *num, void *data)
{
    g_free(num);
    return TRUE;
}

static void save_maps (gchar *filename, AutoMap *automap)
{
    GHashTable *hash;
    GList *puck;
    Map *map;
    FILE *file;
    int n = 0;
    void *data[2];

    file = fopen(filename, "w");

    if (file == NULL)
    {
        g_warning("save_maps: Can't open %s for writing: %s\n",
                  filename, strerror(errno));
        return;
    }

    hash = g_hash_table_new(g_direct_hash, g_direct_equal);
    get_nodes(hash, automap->map, &n);

    fprintf(file, "automap map %s player %d zoom %.2f, center (%d, %d)\n",
            automap->map->name, *(gint *)g_hash_table_lookup(hash, automap->player),
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

        if (!g_hash_table_lookup(hash, map->nodelist->data))
            continue;

        fprintf(file, "map %s min_x %d min_y %d max_x %d max_y %d nodelist ", map->name,
                map->min_x, map->min_y, map->max_x, map->max_y);

        for (inner = map->nodelist; inner != NULL; inner = inner->next)
            fprintf(file, "%d ", *(gint *)g_hash_table_lookup(hash, inner->data));

        fputs("\n", file);
    }

    data[0] = file;
    data[1] = hash;

    g_hash_table_foreach(hash, (GHFunc)write_node, (gpointer)data);
    g_hash_table_foreach_remove(hash, (GHRFunc)remove_node, NULL);
    g_hash_table_destroy(hash);

    fclose(file);
}

static gboolean free_nodes (MapNode *key, GList *value, GList **maps)
{
    MapNode *node, *next;
    GList *puck, *map_link;
    gint i;

    for (puck = value; puck != NULL; puck = puck->next)
    {
        node = puck->data;

        for (i = 0; i < 10; i++)
        {
            next = node->connections[i].node;

            if (next)
            {
                if ((map_link = g_list_find(MapList, next->map)) != NULL)
                {
                    *maps = g_list_prepend(*maps, next->map);
                    MapList = g_list_remove_link(MapList, map_link);
                }

                next->connections[OPPOSITE(i)].node = NULL;
            }
        }

        g_free(node);
    }

    g_list_free(value);

    return TRUE;
}

static void free_maps (AutoMap *automap)
{
    GList *list = g_list_prepend(NULL, automap->map);
    Map *map;
    MapList = g_list_remove(MapList, automap->map);

    while (list)
    {
        map = (Map *)list->data;
        list = g_list_remove_link(list, list);

        g_hash_table_foreach_remove(map->nodes, (GHRFunc)free_nodes, &list);
        g_hash_table_destroy(map->nodes);
        g_list_free(map->nodelist);
        g_free(map->name);
        g_free(map);
    }

    automap->player = NULL;
    automap->map = NULL;
    automap->x = automap->y = 0;
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
    gchar *filename = gtk_file_selection_get_filename(find);

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
        free_maps(automap);
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
    gchar *text = GTK_LABEL( GTK_BIN(widget)->child )->label, dir[256], *home;
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

        find = gtk_file_selection_new(type == LOAD ? "Load map" : "Save map");

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
    GHashTable *hash, *thishash;

    if (!this)
    {
        g_warning("node_break: no node existed in that direction\n");
        return;
    }

    if (type > 7)
    {
        g_warning("node_break: will not break links to a node going up or down\n");
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
     */

    automap->player = next;

    blank_nodes(automap, ws, nodelist);
    memset(&this->connections[type], 0, sizeof(*this->connections));
    memset(&next->connections[OPPOSITE(type)], 0, sizeof(*next->connections));
    draw_nodes(automap, ws, this, NULL, NULL);
    draw_nodes(automap, ws, next, NULL, NULL);
    draw_player(automap, ws, next);
    blit_nodes(automap, ws, nodelist);

    /* If removing this link means one section of the map will be unreachable
     * from the other section of the map, then add the unreachable
     * section to the nodelist
     */

    hash = g_hash_table_new( (GHashFunc)node_hash, (GCompareFunc)node_comp );
    thishash = g_hash_table_new( (GHashFunc)node_hash, (GCompareFunc)node_comp );

    if (!connected_node_in_list(automap->map, this, thishash, hash))
    {
        automap->map->nodelist = g_list_prepend(automap->map->nodelist, this);
    } else {
        g_hash_table_destroy(thishash);
        thishash = g_hash_table_new( (GHashFunc)node_hash, (GCompareFunc)node_comp );

        if (!connected_node_in_list(automap->map, this, thishash, hash))
            automap->map->nodelist = g_list_prepend(automap->map->nodelist, this);
    }

    g_hash_table_destroy(thishash);
    g_hash_table_destroy(hash);
}

static void move_player (AutoMap *automap, guint type)
{
    MapNode *this = automap->player;
    MapNode *next = automap->player->connections[type].node;
    struct win_scale *ws = map_coords(automap);
    guint opposite = OPPOSITE(type);
    gboolean redraw = FALSE;

    if (automap->node_break)
    {

        /* Doesn't belong here ? Lazyness ... */
        automap->node_break = FALSE;
        node_break(automap, type);

        return;
    }

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

            redraw = TRUE;
        }
    } else {
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
        } else {
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
                    /* It would seem this node already has a connection to here, and
                     * it's not from us. Probably is possible, will see. Deal with it
                     * later
                     */

                    g_warning("move_player: map does not support this move situation yet\n");
                    return;

                } else {
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
            } else {
                next = g_malloc0(sizeof(MapNode));

                next->x = node.x;
                next->y = node.y;

                node_hash_prepend(automap->map->nodes, next);
            }

            next->conn++;
            automap->player->conn++;

            next->connections[opposite].node = automap->player;
            automap->player->connections[type].node = next;

            next->map = automap->map;
            automap->player = next;
        }
    }

    /* FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME
     * Here you would insert the command to send the direction of travel to the
     * mud
     */

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
    } else {

        /* Pass the old co-ordinates along with the automap details to the drawing
         * routine
         */
        MapNode *nodelist[] = { this, next, NULL };

        draw_nodes(automap, ws, this, NULL, NULL);
        draw_nodes(automap, ws, next, this, NULL);
        draw_player(automap, ws, next);
        blit_nodes(automap, ws, nodelist);
        draw_selected(automap, ws);
    }
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

AutoMap *auto_map_new (void)
{
    AutoMap *automap = g_malloc0(sizeof(AutoMap));
    GtkWidget *hbox, *updownvbox, *loadsavevbox, *vbox, *sep;
    GtkWidget *n, *ne, *e, *se, *s, *sw, *w, *nw, *up, *down;
    GtkWidget *load, *save, *remove;
    GtkWidget *table, *table_draw;

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

    /* Pack the range/drawing widgets into the draw table */
    table_draw = gtk_table_new(2, 2, FALSE);
    gtk_table_attach(GTK_TABLE(table_draw), automap->draw_area,
                     0, 1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
    gtk_table_attach(GTK_TABLE(table_draw), automap->hsb,
                     0, 1, 1, 2, GTK_EXPAND | GTK_FILL,                     0, 0, 0);
    gtk_table_attach(GTK_TABLE(table_draw), automap->vsb,
                     1, 2, 0, 1, 0                    , GTK_EXPAND | GTK_FILL, 0, 0);

    gtk_widget_show(automap->hsb);
    gtk_widget_show(automap->vsb);
    gtk_widget_show(automap->draw_area);
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
    char name[10];

    /* Create our map */
    map = g_malloc0(sizeof(MapNode));

    if (map == NULL)
    {
        g_error("map_new: g_malloc0 error: %s\n", strerror(errno));
        gtk_exit(1);
    }

    g_snprintf(name, 10, "GNOME-Mud Mapper - map%d", g_list_length(MapList));
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
    automap = auto_map_new();

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

static void load_automap_from_file (gchar *filename, AutoMap *automap)
{
    FILE *file;
    gchar buf[256], token[256], *bptr, *mapname;
    GPtrArray *arr = g_ptr_array_new();
    GList *maps = NULL, *puck;
    gint i, o, explicit_redraw = (automap != NULL);
    Map *map = NULL;

    file = fopen(filename, "r");

    if (file == NULL)
    {
        g_warning("load_automap_from_file: Could not open %s for reading: %s\n",
                  filename, strerror(errno));
        return;
    }

    /* Create our automaplist ... */
    if (!automap) automap = auto_map_new();
    AutoMapList = g_list_append(AutoMapList, automap);

    /* Get details from the file to fill in to our automap */
    bptr = fgets(buf, BUFSIZ, file);
    bptr = get_token(bptr, token);
    bptr = get_token(bptr, token);
    bptr = get_token(bptr, token);
    mapname = g_strdup(token);
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

        bptr = get_token(bptr, token);
        map = g_malloc0(sizeof(Map));

        if (mapname && !strcmp(mapname, token))
        {
            g_free(mapname);
            mapname = NULL;
            automap->map = map;
        }

        map->nodes = g_hash_table_new((GHashFunc)node_hash, (GCompareFunc)node_comp);
        map->name = g_strdup(token);

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
        maps = g_list_prepend(maps, map);
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

        for (puck = MapList; puck != NULL; puck = puck->next)
        {
            map = puck->data;

            if (!strcmp(token, map->name))
                break;
        }

        node_hash_prepend(map->nodes, node);
        node->map = map;

        if (num >= arr->len)
            g_ptr_array_set_size(arr, num + 1);

        g_ptr_array_index(arr, num) = node;

        for (num = 0; num < 10; num++)
        {
            bptr = get_token(bptr, token);
            bptr = get_token(bptr, token);
            node->connections[num].node = (MapNode *)(atol(token) + 1);
        }
    } while ((bptr = fgets(buf, BUFSIZ, file)) != NULL);

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
    for (i = 0; i < arr->len; i++)
    {
        MapNode *node = g_ptr_array_index(arr, i);

        for (o = 0; o < 10; o++)
            if (node->connections[o].node != NULL)
                node->connections[o].node =
                    g_ptr_array_index(arr, GPOINTER_TO_INT(node->connections[o].node) - 1);
    }

    automap->player = g_ptr_array_index(arr, GPOINTER_TO_INT(automap->player));
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

#endif /* WITHOUT_MAPPER */
