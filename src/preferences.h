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

#ifndef PREFERENCES_H
#define PREFERENCES_H

G_BEGIN_DECLS

#define INIT_HISTORY_KEY      NULL
#define INIT_ACTIONS_KEY      NULL
#define INIT_MENU_KEY         NULL

#define DEF_USE_COPY          TRUE
#define DEF_USE_PRIMARY       FALSE
#define DEF_SYNCHRONIZE       FALSE
#define DEF_SAVE_HISTORY      TRUE
#define DEF_HISTORY_LIMIT     25
#define DEF_HYPERLINKS_ONLY   FALSE
#define DEF_CONFIRM_CLEAR     FALSE
#define DEF_SINGLE_LINE       TRUE
#define DEF_REVERSE_HISTORY   FALSE
#define DEF_ITEM_LENGTH       50
#define DEF_ELLIPSIZE         2
#define DEF_PHISTORY_KEY      "<Ctrl><Alt>X"
#define DEF_HISTORY_KEY       "<Ctrl><Alt>H"
#define DEF_ACTIONS_KEY       "<Ctrl><Alt>A"
#define DEF_MENU_KEY          "<Ctrl><Alt>P"
#define DEF_NO_ICON           FALSE

#define ACTIONS_FILE          "parcellite/actions"
#define FIFO_FILE_C          "parcellite/fifo_c"
#define FIFO_FILE_P          "parcellite/fifo_p"
#define PREFERENCES_FILE      "parcellite/parcelliterc"

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
  
	gchar*    phistory_key;      /* Persistent History menu hotkey */
  gchar*    history_key;      /* History menu hotkey */
  gchar*    actions_key;      /* Actions menu hotkey */
  gchar*    menu_key;         /* Parcellite menu hotkey */
  
  gboolean  no_icon;          /* No icon */
	gboolean  history_pos;			/* set postion (or not)  */
	gint      history_x;        /* location of x location to display history  */	
	gint      history_y;        /* location of y location to display history  */	
	gboolean  case_search;      /* turn on case sensitive search */
	gboolean  type_search;      /* turn on search-as-you-type */
  gint32      data_size;      /**size, in megabytes to limit text copied.  */
	gboolean  ignore_whiteonly; /** will not add entries that are only whitespace */
	gboolean  trim_wspace_begend; /** Trims whitespace from beginning and end of line*/
	gboolean  trim_newline;      /** Trims newlines from lines */
}
prefs_t;

extern prefs_t prefs;

void read_preferences();

void show_preferences(gint tab);

G_END_DECLS

#endif
