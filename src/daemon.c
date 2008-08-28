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

/* Called during daemon mode to safeguard clipboard contents */
static void
clipboard_daemon(GtkClipboard *clipboard,
                 GdkEvent     *event,
                 gpointer      user_data)
{
  static gchar* clipboard_contents = NULL;
  gchar* text = gtk_clipboard_wait_for_text(clipboard);
  /* Check if clipboard contents were lost */
  if (!text)
  {
    if (clipboard_contents)
      gtk_clipboard_set_text(clipboard, clipboard_contents, -1);
  }
  else
  {
    g_free(clipboard_contents);
    clipboard_contents = g_strdup(text);
  }
  g_free(text);
}

/* Called during daemon mode to safeguard primary contents */
static gboolean
primary_daemon(gpointer primary)
{
  static gchar* primary_contents = NULL;
  gchar* text = gtk_clipboard_wait_for_text((GtkClipboard*)primary);
  /* Check if primary contents were lost */
  if (!text)
  {
    if (primary_contents)
      gtk_clipboard_set_text((GtkClipboard*)primary, primary_contents, -1);
  }
  else
  {
    g_free(primary_contents);
    primary_contents = g_strdup(text);
  }
  g_free(text);
}

/* Initializes daemon mode */
void
init_daemon_mode()
{
  /* Create clipboard and primary and connect signals */
  GtkClipboard* clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
  g_signal_connect(G_OBJECT(clipboard), "owner-change", G_CALLBACK(clipboard_daemon), NULL);
  GtkClipboard* primary = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
  g_timeout_add(PRIMCHECKDELAY, primary_daemon, (gpointer)primary);
  
  /* Start daemon loop */
  gtk_main();
  gtk_main_quit();
}
