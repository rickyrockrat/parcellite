/* Copyright (C) 2007-2008 by Xyhthyx <xyhthyx@gmail.com>
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

#include "preferences.h"

#ifndef MAIN_H
#define MAIN_H

G_BEGIN_DECLS

#define ACTIONS_TAB    2
#define POPUP_DELAY    100
#define CHECK_INTERVAL 500

typedef struct
{
  gboolean  use_copy;         /* Use copy */
  gboolean  use_primary;      /* Use primary */
  gboolean  synchronize;      /* Synchronize copy and primary */
  
  gboolean  save_history;     /* Save history */
  gint      history_limit;    /* Items in history */
  
  gboolean  hyperlinks_only;  /* Hyperlinks only */
  gboolean  confirm_clear;    /* Confirm clear */
  
  gboolean  single_line;      /* Show in a single line */
  gboolean  reverse_history;  /* Show in reverse order */
  gint      item_length;      /* Length of items */
  
  gint      ellipsize;        /* Omitting */
  
  gchar*    history_key;      /* History menu hotkey */
  gchar*    actions_key;      /* Actions menu hotkey */
  gchar*    menu_key;         /* Parcellite menu hotkey */
  
  gboolean  no_icon;          /* No icon */
	gboolean  history_pos;			/* set postion (or not)  */
	gint      history_x;        /* location of x location to display history  */	
	gint      history_y;        /* location of y location to display history  */	
}
prefs_t;

extern prefs_t prefs;

void
history_hotkey(char *keystring, gpointer user_data);

void
actions_hotkey(char *keystring, gpointer user_data);

void
menu_hotkey(char *keystring, gpointer user_data);

void postition_history(GtkMenu *menu,gint *x,gint *y,gboolean *push_in, gpointer user_data);

G_END_DECLS

#endif
