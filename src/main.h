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

#ifndef MAIN_H
#define MAIN_H
#include "preferences.h"
G_BEGIN_DECLS

#define ACTIONS_TAB    2
#define POPUP_DELAY    100
#define CHECK_INTERVAL 500
#define ID_PRIMARY   0
#define ID_CLIPBOARD 1

int p_strcmp (const char *str1, const char *str2);
void history_hotkey(char *keystring, gpointer user_data);
void actions_hotkey(char *keystring, gpointer user_data);
void menu_hotkey(char *keystring, gpointer user_data);
void postition_history(GtkMenu *menu,gint *x,gint *y,gboolean *push_in, gpointer user_data);

G_END_DECLS

#endif
