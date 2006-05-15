/* GNOME-Mud - A simple Mud CLient
 * Copyright (C) 1998-2006 Robin Ericsson <lobbin@localhost.nu>
 *
 * map-links.c is written by Paul Cameron <thrase@progsoc.uts.edu.au> and 
 * Remi Bonnet <remi.bonnet@laposte.net> with modifications by Robin
 * Ericsson to make it work with AMCL.
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

#include "../../config.h"

#ifndef WITHOUT_MAPPER

#include <gtk/gtk.h>

#include "map.h"

extern GList* AutoMapList;

/* Globals */
GHashTable *map_to_link_memory = NULL;
LinkMemory *actual = NULL;  // The aim of this file is not to lose much time to call g_hash_table_lookup at every action
AutoMap *automap = NULL;    // I suppose that there is only one automapper. I *really* don't see any use for more than one.

/*
 * Functions
 */
inline guint pos_hash(Pos* key)
{
	return (key->x << 16) + key->y;
}

inline gboolean pos_equal(Pos* a, Pos* b)
{
	if (a->x == b->x && a->y == b->y)
		return TRUE;
	else
		return FALSE;
}

gboolean node_exists(gint x, gint y) // x et y are not the node coordinates but the link coordinates ( x2 )
{
	if (((gint) (x / 2)) * 2 == x && ((gint) (y / 2)) * 2 == y)
	{
		MapNode test;
		test.x = x / 2; test.y = y / 2;
		
		if (g_hash_table_lookup(automap->map->nodes, &test))
			return TRUE;
	}		
	return FALSE;  
}
gint get_direction(gint x1, gint y1, gint x2, gint y2)
{
	if (x1 > x2)
	{
		if (y1 > y2)
			return SOUTHWEST;
		else if (y1 < y2)
			return NORTHWEST;
		else
			return WEST;
	}
	else if (x1 < x2)
	{
		if (y1 > y2)
			return SOUTHEAST;
		else if (y1 < y2)
			return NORTHEAST;
		else
			return EAST;
	}
	else
	{
		if (y1 > y2)
			return SOUTH;
		else if (y1 < y2)
			return NORTH;
		else
		{
			g_warning("get_direction: same node");
			return -1;
		}
	}
}

void link_blit_xy(int x, int y, struct win_scale *ws)
{
	gint width = ws->mapped_unit / 4;
	gint x1, y1;
	Rectangle rect;

	/* Translate in pixels: (thanks to the code of the translate function) */
	x1 = ( (gint) ((x - 2 * automap->x) * ws->mapped_unit) + ws->width ) / 2;
 	y1 = ( ws->height - (gint)((y - 2 * automap->y) * ws->mapped_unit) ) / 2;

	rect.x = x1 - width;
	rect.y = y1 - width;
	rect.width = 2 * width + 1;
	rect.height = 2 * width + 1;	
	
    if (adjust(ws, &rect))
    {
        gdk_draw_drawable(automap->draw_area->window,
                          automap->draw_area->style->fg_gc[GTK_WIDGET_STATE (automap->draw_area)],
                          automap->pixmap, rect.x, rect.y, rect.x, rect.y, rect.width, rect.height);
    }
}

inline void link_blit(Link *link, struct win_scale *ws)
{
	link_blit_xy(link->x, link->y, ws);
}

void link_draw(Link* link, struct win_scale *ws)
{
  	gint tmp_width, width;
	gint x1, y1, x2, y2;

	if (link->type == LK_NODE)
		return;

	width = ws->mapped_unit / 4;

	/* Translate in pixels: (thanks to the code of the translate function) */
	x1 = ( (gint) ((link->x - 2 * automap->x) * ws->mapped_unit) + ws->width ) / 2;
 	y1 = ( ws->height - (gint)((link->y - 2 * automap->y) * ws->mapped_unit) ) / 2;
	
	if (!node_exists(link->x, link->y)) 
	{
		/* We leave a blank space to mean that the link doesn't lead to the next node */
		if (node_exists(link->prev->x, link->prev->y) && link->prev->type != LK_NODE)
			tmp_width = ws->mapped_unit / 6;
		else
			tmp_width = width;
	
		/* Draw a line from the entrance of the rectangle to its center */
		x2 = x1 + tmp_width * (link->prev->x - link->x);
		y2 = y1 - tmp_width * (link->prev->y - link->y);	
		gdk_draw_line(automap->pixmap,
					  automap->draw_area->style->black_gc,
					  x1, y1, x2, y2);
	}

	if (link->next) // Link->next == NULL is possible if the link is in building
	{
		/* We leave a blank space to mean that the link doesn't lead to the next node */
		if (node_exists(link->next->x, link->next->y) && link->next->type != LK_NODE)
			tmp_width = ws->mapped_unit / 6;
		else
			tmp_width = width;

		/* Draw a line from the entrance of the rectangle to its center */
		x2 = x1 + tmp_width * (link->next->x - link->x);
		y2 = y1 - tmp_width * (link->next->y - link->y);
		gdk_draw_line(automap->pixmap,
					  automap->draw_area->style->black_gc,
					  x1, y1, x2, y2);
	}
	else
	{
		/* Create a small circle */
		gdk_draw_arc(automap->pixmap,
			   		 automap->draw_area->style->black_gc, TRUE,
					 x1 - width / 3, y1 - width / 3, width * 2 / 3, width * 2 / 3,
				     0, 64 * 360);
	}
}

void link_draw_all(Link* link, struct win_scale *ws)
{
	Link* it = link;
	while (it->prev)
	 it = it->prev;
	while (it)
	{
		maplink_draw(it->x, it->y, ws);
		it = it->next;
	}
}

void link_draw_glist(Pos* position, GList* links, struct win_scale *ws)
{
	if (position->x >= 2 * ws->mapped_x && position->x <= 2 * (ws->mapped_x + ws->mapped_width) &&
		position->y >= 2 * ws->mapped_y && position->y <= 2 * (ws->mapped_y + ws->mapped_height) )
	{
		maplink_draw(position->x, position->y, ws);
	}
}
Link* link_reorder(Link* in) // Reorder the list to make in the first part
{
	if (in->next && in->prev)
	{
		g_critical("link_reorder: in is not one of the link bounds");
		return NULL;
	}
	
	if (in->next) // The link is ok
		return in;
	
	if (in->prev)
	{
		/* We need to reverse the link */
		Link *tmp, *puck;
		for (puck = in; puck != NULL; puck = puck->next)
		{
			tmp = puck->prev;
			puck->prev = puck->next;
			puck->next = tmp;
		}
		return in;
	}
	
	g_warning("link_reorder: in was composed of only one part");
	return in;
}

Link* link_add(Link* link_list, gint type, gint x, gint y)
{
	Link *link, *previous;
	GList *list;
	Pos* pos;	

	/* Create the link structure */
	link = g_malloc0(sizeof(Link));
	link->type = type;
	link->x = x;
	link->y = y;

	/* Create a Pos structure for the hash */
	pos = g_malloc0(sizeof(Pos));
	pos->x = x;
	pos->y = y;

	/* Append to the built-in list */
	if (link_list)
	{
		previous = link_list;
		while(previous->next != NULL)
			previous = previous->next;
		
		previous->next = link;
		link->prev = previous;
		link->next = NULL;
	}
	else
	{
		link_list = link;
		link->prev = NULL;
		link->next = NULL;
	}

	/* Draw the previous node
	 * We can't draw the actual node because we need a pointer
	 * to the next node but we can also do this for the previous
	 */
	if (link->prev && link->prev->type != LK_NODE)
		maplink_draw(link->prev->x, link->prev->y, map_coords(automap));

	
	/* Append to the list gived by global hash */
	list = g_hash_table_lookup(actual->link_table, pos);
	list = g_list_append(list, link);
	g_hash_table_steal(actual->link_table, pos); // Steal to avoid freeing the list with link_free_list
	g_hash_table_insert(actual->link_table, pos, list);
	
	return link_list;
}

MapNode* link_get_opposite(Link* in)
{
	/* Ensure that we will go in the good direction */
	in = link_reorder(in);
	
	/* Get the ends node */
	while (in->next)
		in = in->next;
	
	return in->node;
}
gint link_get_opposite_dir(MapNode* node, gint dir)
{
	int i;
	
	/* Get the ends node */
	Link* end = link_reorder(node->connections[dir].link);
	while (end->next)
		end = end->next;

	for (i = 0; i < 8; i++)
	{
		if (end->node->connections[i].link == end)
		{
			return i;
		}
	}
	
	g_warning("link_get_opposite_dir: Can't find opposite direction.");
	return -1;

}
	
void link_delete(Link* link)
{
	Pos pos;
	gpointer orig_key;
	GList *list;
	
	struct win_scale *ws = map_coords(automap);
	gint x, y;
	
	link = link_reorder(link);
	
	while (link)
	{
		/* Remove the link from the list given by the global hash */
		pos.x = link->x;
		pos.y = link->y;
		g_hash_table_lookup_extended(actual->link_table, &pos, &orig_key, (gpointer*) &list);
		list = g_list_remove(list, link);
		if (!list)
		{
			g_hash_table_steal(actual->link_table, &pos);
			g_free(orig_key);
		}
		else
		{
			g_hash_table_steal(actual->link_table, &pos);
			g_hash_table_replace(actual->link_table, orig_key, list);
		}
		/* Delete the point between this node and the next one
		 * Indeed, maplink_draw will not clear it because it can't
		 * know if another link pass near the link rectangle
		 */
		if (link->next)
		{
	    	x = ( (gint) ((link->x + link->next->x - 4 * automap->x) * ws->mapped_unit / 2) + ws->width ) / 2;
   			y = ( ws->height - (gint)((link->y + link->next->y - 4 * automap->y) * ws->mapped_unit / 2) ) / 2;
			gdk_draw_point(automap->pixmap,
					   automap->draw_area->style->white_gc,
					   x, y);
		}
		
		/* Redraw the place */
		maplink_draw(link->x, link->y, ws);
		
		/* Free the structure and go to next part */
		if (link->next)
		{
			link = link->next;
			g_free(link->prev);
		}
		else
		{
			g_free(link);
			link = NULL;
		}
	}
}

void link_free_list(GList* list)
{
	GList* it;
	for (it = g_list_first(list); it != NULL; it = it->next)
		g_free(it->data);
	g_list_free(list);
}

gboolean link_is_standard(MapNode* node, guint dir)
{
	Link *link; guint count = 0;

	if (link_get_opposite_dir(node, dir) != OPPOSITE(dir))
		return FALSE;
	
	for (link = node->connections[dir].link; link != NULL; link = link->next)
		count++;
	
	/* A normal link must have three parts */
	if (count == 3)
		return TRUE;
	
	return FALSE;
}
void linkmemory_free(LinkMemory* data)
{	
	g_hash_table_destroy(data->link_table);
	g_free(data);
}

void maplink_init_new_map(Map* map)
{
	LinkMemory* memory;
	automap = AutoMapList->data;
	
	if (!map_to_link_memory)
		map_to_link_memory = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, (GDestroyNotify) linkmemory_free);
	
	memory = g_malloc0(sizeof(LinkMemory));
	memory->map = map;
	memory->link_table = g_hash_table_new_full((GHashFunc) pos_hash, (GCompareFunc) pos_equal, (GDestroyNotify) g_free, (GDestroyNotify) link_free_list);

	g_hash_table_insert(map_to_link_memory, map, memory);
	
	actual = memory;
}

void maplink_change_map(Map* map)
{
	LinkMemory *memory;
	
	if ((memory = g_hash_table_lookup(map_to_link_memory, map)) != NULL)
		actual = memory;
	else
		maplink_init_new_map(map);
}

gint maplink_check(MapNode* from, MapNode* to)
{
	/* First check the direction of the link */
	gint type = get_direction(from->x, from->y, to->x, to->y);
	if (type == -1)
	{
		g_warning("maplink_check: from and to are the same nodes");
		return 0; 
	}
	
	if (from->connections[type].node == to && to->connections[OPPOSITE(type)].node == from)
		return (type + 1);
	else if (from->connections[type].node == NULL && to->connections[OPPOSITE(type)].node == NULL)
		return (-1) * (type + 1);
	else 
		return 0;
}

Link* maplink_create(MapNode* from, MapNode* to)
{
  	gint type;
	Link *link_list;
	int x, y, end_x, end_y, dx, dy;

	if (actual == NULL)
		maplink_init_new_map(from->map);
	
	if (from->map != actual->map)
		maplink_change_map(from->map);
	
	type = maplink_check(from, to);

	if (type > 0)
	{
		g_warning("maplink_create: link already exist");
		return NULL;
	}
	if (type == 0)
	{
		g_warning("maplink_create: can't create link");
		return NULL;
	}
	type = (type * (-1)) - 1;
	
	x = 2 * from->x; y = 2 * from->y;
	end_x = 2 * to->x; end_y = 2 * to->y;
	
	/* Create the starting point of the link */
	link_list = link_add(NULL, LK_NODE, x, y);

	/* Gets the move needed to go to to */ // Whah 3 * to !!!
	switch (type)
	{
		case NORTH: 	dx =  0; dy =  1; break;
		case NORTHWEST: dx = -1; dy =  1; break;
		case WEST: 		dx = -1; dy =  0; break;
		case SOUTHWEST: dx = -1; dy = -1; break;
		case SOUTH: 	dx =  0; dy = -1; break;
		case SOUTHEAST: dx =  1; dy = -1; break;
		case EAST: 		dx =  1; dy =  0; break;
		case NORTHEAST: dx =  1; dy =  1; break;
		default:
			g_warning("maplink_create: type is not valid");
			return NULL;
	}
		
	x += dx; y += dy; // One move in the correct direction.
	
	/* First go horizontally */
	while (x != end_x - dx)
	{
		link_list = link_add(link_list, LK_PART, x, y);
		
		x += dx;
	}

	/* Then vertically */
	while (y != end_y - dy)
	{
		link_list = link_add(link_list, LK_PART, x, y);
				
		y += dy;
	}

	/* Add the last node before the last */
	link_list = link_add(link_list, LK_PART, x, y);
	
	/* Finally, add the ending node */
	link_list = link_add(link_list, LK_NODE, end_x, end_y);
		
	/* To finish, update the connection table */
	from->conn++;
	to->conn++;
	from->connections[type].node = to;
	from->connections[type].link = link_list;
	link_list->node = from;
	while (link_list->next)              // Get the end link
		link_list = link_list->next;
	to->connections[OPPOSITE(type)].node = from;
	to->connections[OPPOSITE(type)].link = link_list;
	link_list->node = to;

	return link_list;
}

void maplink_delete(MapNode* from, guint dir)
{
	MapNode *end;
	guint i;
	GHashTable *thishash, *hash;
	
	/* Get opposite stats */
	i = link_get_opposite_dir(from, dir);
	end = link_get_opposite(from->connections[dir].link);
	
	/* Delete the link */
	link_delete(from->connections[dir].link);

	/* Update the connections table */	
	end->connections[i].node = NULL;
	end->connections[i].link = NULL;
	end->conn--;
	from->connections[dir].node = NULL;
	from->connections[dir].link = NULL;
	from->conn--;

    /* If removing this link means one section of the map will be unreachable
     * from the other section of the map, then add the unreachable
     * section to the nodelist
     */

    hash = g_hash_table_new( (GHashFunc)node_hash, (GCompareFunc)node_comp );
    thishash = g_hash_table_new( (GHashFunc)node_hash, (GCompareFunc)node_comp );

    if (!connected_node_in_list(automap->map, from, thishash, hash))
    {
        automap->map->nodelist = g_list_prepend(automap->map->nodelist, from);
    } else {
        g_hash_table_destroy(thishash);
        thishash = g_hash_table_new( (GHashFunc)node_hash, (GCompareFunc)node_comp );

        if (!connected_node_in_list(automap->map, end, thishash, hash))
            automap->map->nodelist = g_list_prepend(automap->map->nodelist, end);
    }

    g_hash_table_destroy(thishash);
    g_hash_table_destroy(hash);
}

void maplink_draw(gint x, gint y, struct win_scale* ws)
{
  gint width;
  
	/* Create a Pos structure for later */
	Pos pos;
	pos.x = x;
	pos.y = y;
	
	/* Translate in pixels: (thanks to the code of the translate function) */
    x = ( (gint) ((x - 2 * automap->x) * ws->mapped_unit) + ws->width ) / 2;
    y = ( ws->height - (gint)((y - 2 * automap->y) * ws->mapped_unit) ) / 2;

	width = ws->mapped_unit / 4 - 1;

	/* Clear the place */
	gdk_draw_rectangle(automap->pixmap,
					   automap->draw_area->style->white_gc,
					   TRUE,
					   x - width, y - width, 2 * width + 1, 2 * width + 1);

	
	/* Test if there is a node at this place */
	if (node_exists(pos.x, pos.y))
	{
		MapNode *node = g_malloc0(sizeof(MapNode));
		GList *list;
		node->x = pos.x / 2;
		node->y = pos.y / 2;
		list = g_hash_table_lookup(automap->map->nodes, node);
		g_free(node);
		node = list->data;
		draw_dot(automap, ws, node);
		if (node == automap->player)
			draw_player(automap, ws, node);
	}
	else
	{
		/* Call link_draw for each link in the GList */
		GList* list = g_hash_table_lookup(actual->link_table, &pos);
		g_list_foreach(list, (GFunc) link_draw, ws);
	}	

	/* Blit the pixmap */
	link_blit_xy(pos.x, pos.y, ws);
}

void maplink_draw_link(MapNode* from, MapNode* to, struct win_scale *ws)
{
	int i;
	
	/* For each entry in the connection table, check if it leads to the node to
	 * If true, draw the link
	 */
	for (i=0; i<8; i++)
	{
		if (from->connections[i].node == to)
			link_draw_all(from->connections[i].link, ws);
	}
}

void maplink_draw_all(struct win_scale *ws)
{
	g_hash_table_foreach(actual->link_table, (GHFunc) link_draw_glist, ws);
}
void maplink_free()
{
	if (map_to_link_memory)
		g_hash_table_destroy(map_to_link_memory);
	actual = NULL;
	map_to_link_memory = NULL;
}
#endif
