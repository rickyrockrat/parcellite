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

extern GMutex *hist_lock;

#define ACTIONS_TAB    2
#define POPUP_DELAY    100
#define CHECK_INTERVAL 500
#define CHECK_APPINDICATOR_INTERVAL 30000 /**check for existence of indicator-appmenu  */
#define ID_PRIMARY   0
#define ID_CLIPBOARD 1
#define ID_CMD       2

#define HIST_DISPLAY_NORMAL     1
#define HIST_DISPLAY_PERSISTENT	2

#define KBUF_SIZE 200
struct widget_info{
	GtkWidget *menu; /**top level history list window  */
	GtkWidget *item; /**item we are looking at  */
	 GdkEventKey *event; /**event info where we filled this struct  */
	gint index;      /**index into the array  */
	gint tmp1;			 /**gp vars  */
	gint tmp2;
};
/**keeps track of each menu item and the element it's created from.  */
struct s_item_info {
	GtkWidget *item;
	GList *element;
};

struct history_info{
  GtkWidget *menu;			/**top level history menu  */
  GtkWidget *clip_item; /**currently selected history item (represents clipboard)  */
	gchar *element_text;	/**texts of selected history clipboard item  */
  GtkWidget *title_item;
	GList *delete_list; /**struct s_item_info - for the delete list  */
	GList *persist_list; /**struct s_item_info - for the persistent list  */
	struct widget_info wi;  /**temp  for usage in popups  */
	gint histno;           /**which history?  HIST_DISPLAY_NORMAL/HIST_DISPLAY_PERSISTENT*/
	gint change_flag;	/**bit wise flags for history state  */
};

int p_strcmp (const char *str1, const char *str2);
void phistory_hotkey(char *keystring, gpointer user_data);
void history_hotkey(char *keystring, gpointer user_data);
void actions_hotkey(char *keystring, gpointer user_data);
void menu_hotkey(char *keystring, gpointer user_data);
void postition_history(GtkMenu *menu,gint *x,gint *y,gboolean *push_in, gpointer user_data);

G_END_DECLS

#endif
