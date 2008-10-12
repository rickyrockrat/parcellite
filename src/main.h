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

/* Defines */
#define ACTIONSTAB    3
#define POPUPDELAY    100
#define CHECKINTERVAL 500

/* Typedefs */
typedef struct
{
  gboolean  usecopy;     /* Use copy */
  gboolean  useprim;     /* Use primary */
  
  gboolean  savehist;    /* Save history */
  gint      histlim;     /* Items in history */
  
  gboolean  hyperlinks;  /* Hyperlinks only */
  gboolean  confclear;   /* Confirm clear */
  
  gboolean  singleline;  /* Show in a single line */
  gboolean  revhist;     /* Show in reverse order */
  gint      charlength;  /* Character length of items */
  
  gint      ellipsize;   /* Omitting */
  
  gchar*    histkey;     /* History menu hotkey */
  gchar*    actionkey;   /* Actions menu hotkey */
  gchar*    menukey;     /* Parcellite menu hotkey */
  
  gboolean  noicon;      /* No icon */
}
prefs_t;

/* Declare options structure global */
extern prefs_t prefs;

/* Functions */
void
history_hotkey(char *keystring, gpointer user_data);

void
actions_hotkey(char *keystring, gpointer user_data);

void
menu_hotkey(char *keystring, gpointer user_data);

G_END_DECLS

#endif
