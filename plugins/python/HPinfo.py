#!/usr/bin/python

# This plugin is copyright 2003 Adam Luchjenbroers.
# Use and distribution of this plugin is permitted under the terms of the GNU General Public License.
# http://www.gnu.org/licenses/gpl.html

import gtk
import re
import GnomeMud

#The regex used for parsing prompt strings and status lines is determined using this list
#default is used if no entry corresponding to the host:port combination that has been connected to.
MUD_REGEX = {"default"                     : "hp: ([0-9]*)\|? *sp: ([0-9]*)\|? *mp: ([0-9]*)\|? *", 
             "mud.primaldarkness.com:5000" : "hp: ([0-9]*)\|? *sp: ([0-9]*)\|? *mp: ([0-9]*)\|? *"}

def getRegex(host,port):
  addr = host + ":" + port
  if MUD_REGEX.has_key(addr):
    return MUD_REGEX[addr]
  else:
    return MUD_REGEX["default"]             
             
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
    
    self.hpbar.modify_base(gtk.STATE_NORMAL,      gtk.gdk.color_parse("#880000"))
    self.hpbar.modify_base(gtk.STATE_ACTIVE,      gtk.gdk.color_parse("#AA0000"))
    self.hpbar.modify_base(gtk.STATE_PRELIGHT,    gtk.gdk.color_parse("#AA0000"))
    self.hpbar.modify_base(gtk.STATE_SELECTED,    gtk.gdk.color_parse("#AA0000"))
    self.hpbar.modify_base(gtk.STATE_INSENSITIVE, gtk.gdk.color_parse("#660000"))
    
    self.spbar.modify_base(gtk.STATE_NORMAL,      gtk.gdk.color_parse("#008800"))
    self.spbar.modify_base(gtk.STATE_ACTIVE,      gtk.gdk.color_parse("#00AA00"))
    self.spbar.modify_base(gtk.STATE_PRELIGHT,    gtk.gdk.color_parse("#00AA00"))
    self.spbar.modify_base(gtk.STATE_SELECTED,    gtk.gdk.color_parse("#00AA00"))
    self.spbar.modify_base(gtk.STATE_INSENSITIVE, gtk.gdk.color_parse("#006600"))
    
    self.mpbar.modify_base(gtk.STATE_NORMAL,      gtk.gdk.color_parse("#000088"))
    self.mpbar.modify_base(gtk.STATE_ACTIVE,      gtk.gdk.color_parse("#0000AA"))
    self.mpbar.modify_base(gtk.STATE_PRELIGHT,    gtk.gdk.color_parse("#0000AA"))
    self.mpbar.modify_base(gtk.STATE_SELECTED,    gtk.gdk.color_parse("#0000AA"))
    self.mpbar.modify_base(gtk.STATE_INSENSITIVE, gtk.gdk.color_parse("#000066"))
    
    self.hpbar.show()
    self.spbar.show()
    self.mpbar.show()   
    
    self.frame.add(self.hpbar)
    self.frame.add(self.spbar)
    self.frame.add(self.mpbar)
    
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
      m = re.search(getRegex(c.host,c.port),noansi)
      if m:
        self.updateHPInfo(m)
   
if __name__ == "__main__":
  hpinfo = HPInfo()
  player = PlayerState()
     
  GnomeMud.add_user_widget(hpinfo.frame,1,1,5)
  GnomeMud.register_input_handler(player.inputText)
  
  c = GnomeMud.connection()
  c.write("\n[1;35m[*][22m-----------------------------------------------------[1m[*]\n")
  c.write("[1;31mHP-Info plugin 1.0 loaded[1;39m\n")
  c.write("Distribution and use of this plugin is covered under the \nterms of the GPL\n")
  c.write("[1;35m[*][22m-----------------------------------------------------[1m[*][0m\n")

