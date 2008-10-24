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

#include <glib.h>
#include <gtk/gtk.h>
#include "daemon.h"

/* Declare some variables */
gint timeout_id;
GtkClipboard* clipboard;
GtkClipboard* primary;

static void
daemon_check()
{
  static gchar* primary_last = NULL;
  static gchar* clipboard_last = NULL;
  /* Get current primary/clipboard contents */
  gchar* p_text = gtk_clipboard_wait_for_text(primary);
  gchar* c_text = gtk_clipboard_wait_for_text(clipboard);
  /* Check if primary contents were lost */
  if (!p_text)
  {
    if (primary_last)
      gtk_clipboard_set_text(primary, primary_last, -1);
  }
  else
  {
    g_free(primary_last);
    primary_last = g_strdup(p_text);
  }
  /* Check if clipboard contents were lost */
  if (!c_text)
  {
    if (clipboard_last)
      gtk_clipboard_set_text(clipboard, clipboard_last, -1);
  }
  else
  {
    g_free(clipboard_last);
    clipboard_last = g_strdup(c_text);
  }
  /* Cleanup */
  g_free(p_text);
  g_free(c_text);
}

/* Called if timeout was destroyed */
static void
reset_daemon(gpointer data)
{
  if (timeout_id != 0)
    g_source_remove(timeout_id);
  
  timeout_id = g_timeout_add_full(G_PRIORITY_LOW,
                                  CHECKINTERVAL,
                                  (GSourceFunc)daemon_check,
                                  NULL,
                                  (GDestroyNotify)reset_daemon);
}

/* Initializes daemon mode */
void
init_daemon_mode()
{
  /* Create clipboard and primary and connect signals */
  primary = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
  clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
  /* Add the daemon loop */
  timeout_id = g_timeout_add_full(G_PRIORITY_LOW,
                                  CHECKINTERVAL,
                                  (GSourceFunc)daemon_check,
                                  NULL,
                                  (GDestroyNotify)reset_daemon);
  
  /* Start daemon loop */
  gtk_main();
  gtk_main_quit();
}
