/* GNOME-Mud - A simple Mud Client
 * mud-telnet-handlers.h
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

#ifndef MUD_TELNET_HANDLERS_H
#define MUD_TELNET_HANDLERS_H

G_BEGIN_DECLS

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mud-telnet-handler-interface.h"
#include "mud-telnet-charset.h"
#include "mud-telnet-echo.h"
#include "mud-telnet-eor.h"

#ifdef ENABLE_MCCP
#include "mud-telnet-mccp.h"
#endif

#ifdef ENABLE_GST
#include "mud-telnet-msp.h"
#endif

#include "mud-telnet-mssp.h"
#include "mud-telnet-naws.h"
#include "mud-telnet-new-environ.h"
#include "mud-telnet-ttype.h"
#include "mud-telnet-zmp.h"
#include "mud-telnet-aardwolf.h"

G_END_DECLS

#endif // MUD_TELNET_HANDLERS_H

