#!/usr/bin/python

import gtk
import re
import GnomeMud

c = GnomeMud.connection()
c.write("\nPD-Info plugin 0.2 loaded\n")

# ===========================================================
# Useful shiznat
# ---------
# Not a class that handles a UI component, just a bunch of 
# useful methods
# ===========================================================

#Strip ANSI colour codes from string s
def stripAnsi(s):
  return re.sub("\[[\d;]*m","",s)
  
def atoi(s):
   num = 0
   for i in range(len(s)):
    c = s[i]
    if c.isdigit():
      num *= 10
      num += (ord(c) - ord('0'))
    
   return num

# ===========================================================
# HPInfo
# ---------
# Displays HP/MP/SP information as a row of vertical bars
# 
# Constructor values, h,s,m correspond to the maximums for
# hp, sp and mp respectively
# ===========================================================        
class HPInfo:
  
  def __init__(self):
    
    self.frame = gtk.VBox()
     
    self.hpbar = gtk.ProgressBar()
    self.spbar = gtk.ProgressBar()
    self.mpbar = gtk.ProgressBar() 
    
    #self.hpbar.set_orientation(gtk.PROGRESS_BOTTOM_TO_TOP)
    #self.spbar.set_orientation(gtk.PROGRESS_BOTTOM_TO_TOP)
    #self.mpbar.set_orientation(gtk.PROGRESS_BOTTOM_TO_TOP)
    
    self.hpbar.modify_base(gtk.STATE_NORMAL, gtk.gdk.color_parse("#880000"))
    self.hpbar.modify_base(gtk.STATE_ACTIVE, gtk.gdk.color_parse("#AA0000"))
    self.hpbar.modify_base(gtk.STATE_PRELIGHT, gtk.gdk.color_parse("#AA0000"))
    self.hpbar.modify_base(gtk.STATE_SELECTED, gtk.gdk.color_parse("#AA0000"))
    self.hpbar.modify_base(gtk.STATE_INSENSITIVE, gtk.gdk.color_parse("#660000"))
    
    self.spbar.modify_base(gtk.STATE_NORMAL, gtk.gdk.color_parse("#008800"))
    self.spbar.modify_base(gtk.STATE_ACTIVE, gtk.gdk.color_parse("#00AA00"))
    self.spbar.modify_base(gtk.STATE_PRELIGHT, gtk.gdk.color_parse("#00AA00"))
    self.spbar.modify_base(gtk.STATE_SELECTED, gtk.gdk.color_parse("#00AA00"))
    self.spbar.modify_base(gtk.STATE_INSENSITIVE, gtk.gdk.color_parse("#006600"))
    
    self.mpbar.modify_base(gtk.STATE_NORMAL, gtk.gdk.color_parse("#000088"))
    self.mpbar.modify_base(gtk.STATE_ACTIVE, gtk.gdk.color_parse("#0000AA"))
    self.mpbar.modify_base(gtk.STATE_PRELIGHT, gtk.gdk.color_parse("#0000AA"))
    self.mpbar.modify_base(gtk.STATE_SELECTED, gtk.gdk.color_parse("#0000AA"))
    self.mpbar.modify_base(gtk.STATE_INSENSITIVE, gtk.gdk.color_parse("#000066"))
    
    self.hpbar.show()
    self.spbar.show()
    self.mpbar.show()   
    
    self.frame.add(self.hpbar)
    self.frame.add(self.spbar)
    self.frame.add(self.mpbar)
    
    #self.frame.set_size_request(120,100)
    
    self.frame.show()
    
  def render(self,hp=1.0, sp=1.0, mp=1.0, hpmax=2.0, spmax=2.0, mpmax=2.0):
    
    if hp > 0:
      self.hpbar.set_fraction(hp/hpmax)
    else:
      self.hpbar.set_fraction(0)
      
    if sp > 0:
      self.spbar.set_fraction(sp/spmax)
    else:
      self.spbar.set_fraction(0)
      
    if mp > 0:
      self.mpbar.set_fraction(mp/mpmax)
    else:
      self.mpbar.set_fraction(0)

# ===========================================================
# PlayerState
# ---------
# Tracks player information, vitals, and whatever else we 
# have code to track.
# ===========================================================        
class PlayerState:
    
  def __init__(self):
    #Initial values for playerstate
    self.hp = self.hpmax = 1.0
    self.sp = self.spmax = 1.0
    self.mp = self.mpmax = 1.0
    
  def updateHPInfo(self,m):
    self.hp = atoi(m.group(1)) * 1.0
    self.sp = atoi(m.group(2)) * 1.0
    self.mp = atoi(m.group(3)) * 1.0
    if self.hp > self.hpmax:
      self.hpmax = self.hp
    if self.sp > self.spmax:
      self.spmax = self.sp
    if self.mp > self.mpmax:
      self.mpmax = self.mp
    hpinfo.render(self.hp,self.sp,self.mp,self.hpmax,self.spmax,self.mpmax)
  
  def inputText(self,c,s):
    
    lines = s.splitlines()
    
    for line in lines:
  
      noansi = stripAnsi(line)
      m = re.search("hp: ([0-9]*)\|? *sp: ([0-9]*)\|? *mp: ([0-9]*)\|? *",noansi)
      if m:
        self.updateHPInfo(m)
   
   
if __name__ == "__main__":
  hpinfo = HPInfo()
  player = PlayerState()
  
  GnomeMud.add_user_widget(hpinfo.frame,1,1,5)
  GnomeMud.register_input_handler(player.inputText)

