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

#ifndef HISTORY_H
#define HISTORY_H

G_BEGIN_DECLS

#define HISTORY_FILE "parcellite/history"

#define CLIP_TYPE_TEXT       0x1
#define CLIP_TYPE_IMG        0x2
#define CLIP_TYPE_PERSISTENT 0x4
/**give us a genric struct for future expansion  */
struct history_gen{ /**16 bytes, for overlay over res, below.  */
	gint16 a;
	gint16 b;
	gint16 c;
	gint16 d;
}__attribute__((__packed__));
struct history_item {
	guint32 len; /**length of data item, MUST be first in structure  */
	gint16 type; /**currently, text or image  */
	gint16 flags;	/**persistence, or??  */
	guint32 res[4];
	gchar text[8]; /**reserve 64 bits (8 bytes) for pointer to data.  */
}__attribute__((__packed__));


extern GSList* history_list;

glong validate_utf8_text(gchar *text, glong len);

void read_history();

void save_history();

void append_item(gchar* item);

void truncate_history();

gpointer get_last_item();

void delete_duplicate(gchar* item);

G_END_DECLS

#endif
