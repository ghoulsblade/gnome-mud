#!/usr/bin/env python

import GnomeMud,re,time
from _gtk import *
from GTK import *

class glob:
    phys = "empty"
    ment = "empty"
    stat_time = 0
    stat_gag = 1

def input(c,s):
    it = 1
    while it:
        it = 0
        m = re.search("You are physically (.+?) and mentally (.+?)\.\n",s)
        if m:
            glob.phys = m.group(1)
            glob.ment = m.group(2)
            gtk_label_set_text(glob.d_phys,glob.phys)
            gtk_label_set_text(glob.d_ment,glob.ment)
            if glob.stat_gag:
                s = s[:m.start(0)]+s[m.end(0):]
                it = 1
        m = re.search("(?m)^> ",s)
        if m:
            s = s[:m.start(0)]+s[m.end(0):]
            it = 1
    glob.stat_gag = 1
    if time.time() > (glob.stat_time + 30):
        c.send("health\n",0)
        glob.stat_time = time.time()
    return s

def output(c,s):
    if re.match("(?m)^(v|health)( |$)",s):
        print "health!"
        glob.stat_gag = 0
    return s

c = GnomeMud.connection()
c.write("Registering Python callbacks... ")
GnomeMud.register_input_handler(input)
GnomeMud.register_output_handler(output)
c.write("done\n")

c.write("Running GNOME-Mud version %s\n\n" % GnomeMud.version())

def hello(*args):
    c = GnomeMud.connection()
    c.write("Connected to host %s:%s and using profile %s.\n" % (c.host,c.port,c.profile))

def status(title,dest):
    box = gtk_hbox_new(FALSE,0)
    frame = gtk_frame_new(None)
    disp = gtk_label_new("")
    gtk_container_add(frame,disp)
    label = gtk_label_new(title)
    gtk_box_pack_start(box,label,FALSE,FALSE,5)
    gtk_box_pack_start(box,frame,TRUE,TRUE,5)
    gtk_box_pack_start(dest,box,TRUE,TRUE,0)
    return disp

frame = gtk_frame_new("Wombat")
box = gtk_hbox_new(FALSE,0)
gtk_container_set_border_width(box, 5)
gtk_container_add(frame, box)

button = gtk_object_new(gtk_button_get_type(), {
	'label': 'Hello World',
	'visible': 1
})
gtk_signal_connect(button, "clicked", hello);
gtk_box_pack_start(box, button, FALSE, FALSE, 5)

glob.d_phys = status("Physical",box)
glob.d_ment = status("Mental",box)

GnomeMud.add_user_widget(frame,TRUE,TRUE,5)
gtk_widget_show_all(frame)
