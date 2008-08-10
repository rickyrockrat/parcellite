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

/* Defines: default preferences */
#define DEFELLIPSIZE       2
#define DEFHISTORYLIM      25
#define DEFCHARLENGTH      50
#define DEFACTIONSKEY      "<Ctrl><Alt>A"
#define DEFHISTORYKEY      "<Ctrl><Alt>H"
#define DEFSAVELIST        TRUE
#define DEFREVHISTORY      FALSE
#define DEFSINGLELINEMODE  TRUE
#define DEFHYPERLINKSMODE  FALSE
#define DEFNOICON          FALSE

/* Defines: others */
#define ACTIONSFILE  ".local/share/parcellite/actions"
#define PREFFILE     ".config/parcellite/parcelliterc"

/* Functions */
void
read_preferences();

void
show_preferences(gint tab);

G_END_DECLS

#endif
