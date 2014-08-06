/* Copyright (C) 2011-2012 by rickyrockrat <gpib at rickyrockrat dot net>
 *
 * This file is part of Parcellite.
 *
 * Parcellite is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Parcellite is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef _PARCELLITE_H_
#define _PARCELLITE_H_ 1
#include <glib.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>
#include <string.h>
#include <unistd.h>
#ifdef HAVE_APPINDICATOR
#include <libappindicator/app-indicator.h>
#endif
#include <stdlib.h>
#define PARCELLITE_PROG_NAME "parcellite"
#define PARCELLITE_ICON PARCELLITE_PROG_NAME
/*uncomment the next line to debug the clipboard updates
 This provides a debug_update preference. 
must have debug_update enabled in prefs when started. 
*/
/*#define DEBUG_UPDATE   */
/* Uncomment the next line to print a debug trace. */
/*#define DEBUG      */

#ifdef DEBUG
#  define TRACE(x) x
#else
#  define TRACE(x) do {} while (FALSE);
#endif

#ifdef DEBUG_UPDATE
static int debug_update=0; /**disable/enable DTRACE  */
#  define DTRACE(x) if (debug_update) x
#else
#  define DTRACE(x) do {} while (FALSE);
#endif
#include "daemon.h"
#include "utils.h"
#include "preferences.h"
#include "history.h"
#include "main.h"
#include "keybinder.h"
#include "parcellite-i18n.h"
#include "attr_list.h"
#endif

