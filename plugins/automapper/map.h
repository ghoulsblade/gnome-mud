#ifndef __MAP__H__
#define __MAP__H__

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

typedef struct _Map            Map;
typedef struct _MapNode        MapNode;
typedef struct _Rectangle      Rectangle;
typedef struct _Pos			   Pos;
typedef struct _Link		   Link;
typedef struct _LinkMemory	   LinkMemory; 	
typedef struct _AutoMap        AutoMap;
typedef struct _AutoMapConfig  AutoMapConfig;

struct _AutoMapConfig
{
	GList* unusual_exits;
};


struct _Rectangle {

    gint16 x, y;
    gint16 width, height;
};

struct _MapNode {

    gint32 x, y;
    Map *map;
    int conn; /* The number of node connections (discounting up/down) */

    GHashTable* gates; /* Gates are link with unusal name like farm, in, out, gate, enter... */
	GHashTable* in_gates; /* Unusuals gates which leads to this node */
	
    /* There is a one to one mapping between node connections */
    struct {
        MapNode *node; /* The node pointed by this node */
		Link* link;    
    } connections[10];
};

/* This struct contains the window the drawable is being drawn in, the
 * drawable, the backing pixmap, and anything else
 */
struct _AutoMap {

    GtkWidget *window;
    GtkWidget *draw_area;
	GtkWidget *hint;      // A label used to display hints or automap status
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

	gchar *last_unlimited_message; /* The last message displayed in the hint bar without time limit */
	
    /* The X and Y positions where the last node was selected */
    gint16 x_orig, y_orig;

    /* The X and Y offsets of the selected nodes */
    gint16 x_offset, y_offset;

    /* A box being drawn to handle grouping selected nodes */
    Rectangle selection_box;

    /* And all nodes which fall within this box */
    GList *in_selection_box;

	/* A structure to contain data for the create_link.
     * If it is non-NULL, we assume that the AutoMapper
     * is in a create_link state
     */
	struct _create_link_data {
		MapNode* from;
		Link* link;
		gint first_move, last_move;
	} *create_link_data;
	
    /* Program states */
    guint shift : 1;
    guint node_break : 1;
    guint node_goto : 1;
    guint print_coord : 1;
    guint modifying_coords : 1;
    guint redraw_map : 1;
};

struct _Map {

    gchar *name;

    /* The extents of the map */
    gint min_x, min_y, max_x, max_y;

    /* The map contains a linked list of all nodes which aren't interlinked */
    GList *nodelist;

    /* Hash table of linked lists of all MapNodes in this map */
    GHashTable *nodes;
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


struct _Pos {
	gint x;
	gint y;
};

struct _Link {
	enum { LK_PART, LK_NODE } type; // LK_NODE means that the struct don't represent a link but a node. It is the begin and the end of every GList made with LK_PART.
	guint level; // If there is more than one link with the same direction in this place, this field give an unique identifiant for the link (used for drawing)
	gint x;     // link between node1 and node2 has x = node1->x + node2->x
	gint y;     //     --id--
	Link* prev;
	Link* next;
	MapNode* node; // If type is LK_NODE, pointer to the good structure
};

struct _LinkMemory {
	Map* map;
	GHashTable *link_table; // Hash from struct Pos to struct GList (list of struct Link)	
};

/* Fonctions: map-link.c */
gboolean node_exists(gint x, gint y); // x and y are the link coordinates (node coord x2)
gint get_direction(gint x1, gint y1, gint x2, gint y2);
Link* link_add(Link* link_list, gint type, gint x, gint y); // Add a part of a link to link_list with pos (x, y)
void link_draw(Link* link, struct win_scale *ws); // Draw a part of a link
void link_delete(Link* link);  // Delete a link
Link *link_reorder(Link* in);
gboolean link_is_standard(MapNode *node, guint dir);
inline void link_blit(Link* link, struct win_scale *ws);
void maplink_init_new_map(Map* map);
void maplink_change_map(Map* map);
gint maplink_check(MapNode* from, MapNode* to); // Return a positive number if the link exist and a negative number if it doesn't. The absolute value is the code of the direction + 1
Link* maplink_create(MapNode* from, MapNode* to); // Return list of Link 
void maplink_delete(MapNode* from, guint dir); // Delete a link
void maplink_draw(gint x, gint y, struct win_scale* ws); // Draw the links in one place
void maplink_draw_link(MapNode* from, MapNode* to, struct win_scale *ws); // Draw a link from a node to an another
void maplink_draw_all(struct win_scale *ws);     // Draw all nodes on the screen using the hash
void maplink_free(); // Draw all links undrawed 

/* Fonctions: map.c */
struct win_scale* map_coords (AutoMap* automap);
inline gboolean adjust(struct win_scale *ws, Rectangle *rect);
void draw_dot(AutoMap *automap, struct win_scale *ws, MapNode *node);
void draw_player(AutoMap *automap, struct win_scale *ws, MapNode *node);
guint node_hash(MapNode *a);
gboolean node_comp(MapNode *a, MapNode *b);
MapNode* connected_node_in_list(Map *map, MapNode *node, GHashTable *hash, GHashTable *global);

#endif
