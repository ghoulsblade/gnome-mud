#define __MODULE__
#include <gtk/gtk.h>
#include <glib.h>
#include <string.h>

#include "../../src/modules_api.h"

static void init_plugin   (PLUGIN_OBJECT *, gint);

PLUGIN_INFO gnomemud_plugin_info =
{
    "Test Plugin",
    "Robin Ericsson",
    "1.1",
    "Small plugin just to show how the plugins were supposed to work.",
    init_plugin,
};

void init_plugin(PLUGIN_OBJECT *plugin, gint context)
{
  plugin_popup_message ("Test Plugin Registered");
  plugin_register_menu(context, "Test Plugin", "menu_function");
  plugin_register_data_incoming(context, "data_in_function");
}

void data_in_function(PLUGIN_OBJECT *plugin, gchar *data, MudConnectionView *view)
{
  g_message("Recieved (%d) bytes.", strlen(data));
  plugin_add_connection_text("Plugin Called!", 0, view);
}

void menu_function(GtkWidget *widget, gint data)
{
    GtkWidget *label;
    GtkWidget *button;
    GtkWidget *main_box;
    GtkWidget *box2;
    GtkWidget *a_window;
    GtkWidget *separator;

    a_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (a_window),"About, Olle Olle!!!");

    main_box = gtk_vbox_new (FALSE, 0);
    gtk_container_border_width (GTK_CONTAINER (main_box), 5);
    gtk_container_add (GTK_CONTAINER (a_window), main_box);

    label = gtk_label_new ("AMCL version 0.7.0");
    gtk_box_pack_start (GTK_BOX (main_box), label, FALSE, FALSE, 5);
    gtk_widget_show (label);

    label = gtk_label_new ("Copyright © 1998-1999 Robin Ericsson <lobbin@localhost.nu>");
    gtk_box_pack_start (GTK_BOX (main_box), label, FALSE, FALSE, 0);
    gtk_widget_show (label);

    label = gtk_label_new ("   Licensed under GNU GENERAL PUBLIC LICENSE (GPL)    ");
    gtk_box_pack_start (GTK_BOX (main_box), label, FALSE, FALSE, 5);
    gtk_widget_show (label);

    label = gtk_label_new ("Homepage: http://www.localhost.nu/apps/amcl/");
    gtk_box_pack_start (GTK_BOX (main_box), label, FALSE, FALSE, 5);
    gtk_widget_show (label);

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (main_box), separator, FALSE, TRUE, 5);
    gtk_widget_show (separator);

    box2 = gtk_hbox_new (FALSE, 5);
    gtk_box_pack_start (GTK_BOX (main_box), box2, FALSE, FALSE, 0);

    button = gtk_button_new_with_label ( " close ");
    gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
                        GTK_SIGNAL_FUNC (gtk_widget_destroy), (gpointer) a_window);
    gtk_box_pack_start (GTK_BOX (box2), button, TRUE, TRUE, 5);

    gtk_widget_show (button  );
    gtk_widget_show (box2    );
    gtk_widget_show (main_box);
    gtk_widget_show (a_window);
};
