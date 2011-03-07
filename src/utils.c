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
#include "main.h"
#include "utils.h"
#include "daemon.h"
#include "history.h"
#include "parcellite-i18n.h"
#include <string.h>

/** Wrapper to replace g_strdup to limit size of text copied from clipboard. 
g_strndup will dup to the size of the limit, which will waste resources, so
try to allocate using other methods.
*/
gchar *p_strdup( const gchar *str )
{
  gchar *n=NULL;
  gint u8;
  size_t l,x;
  if(NULL == str)
    return NULL;
  if(0==prefs.data_size)
    return g_strdup(str);
  x=prefs.data_size*1000000; 
  /**use the following to test truncation  */
  /*x=prefs.data_size*10; */
  if(TRUE ==g_utf8_validate (str,-1,NULL)){
/*    g_printf("UTF8 "); */
    l=g_utf8_strlen(str,-1);
    u8=1;
  } else{
    l=strlen(str);
    u8=0;
  }
    
  
  if(l>x){
    l=x;
  }
  
  if(NULL !=(n=g_malloc(l+8))){
    n[l+7]=0;
    g_strlcpy(n,str,l+1);
  }
    
  return n;
}
/* Creates program related directories if needed */
void check_dirs()
{
  gchar* data_dir = g_build_path("/", g_get_home_dir(), DATA_DIR,  NULL);
  gchar* config_dir = g_build_path("/", g_get_home_dir(), CONFIG_DIR,  NULL);
  /* Check if data directory exists */
  if (!g_file_test(data_dir, G_FILE_TEST_EXISTS))
  {
    /* Try to make data directory */
    if (g_mkdir_with_parents(data_dir, 0755) != 0)
      g_warning(_("Couldn't create directory: %s\n"), data_dir);
  }
  /* Check if config directory exists */
  if (!g_file_test(config_dir, G_FILE_TEST_EXISTS))
  {
    /* Try to make config directory */
    if (g_mkdir_with_parents(config_dir, 0755) != 0)
      g_warning(_("Couldn't create directory: %s\n"), config_dir);
  }
  /* Cleanup */
  g_free(data_dir);
  g_free(config_dir);
}

/* Returns TRUE if text is a hyperlink */
gboolean is_hyperlink(gchar* text)
{
  /* TODO: I need a better regex, this one is poor */
  GRegex* regex = g_regex_new("([A-Za-z][A-Za-z0-9+.-]{1,120}:[A-Za-z0-9/]" \
                              "(([A-Za-z0-9$_.+!*,;/?:@&~=-])|%[A-Fa-f0-9]{2}){1,333}" \
                              "(#([a-zA-Z0-9][a-zA-Z0-9$_.+!*,;/?:@&~=%-]{0,1000}))?)",
                              G_REGEX_CASELESS, 0, NULL);
  
  gboolean result = g_regex_match(regex, text, 0, NULL);
  g_regex_unref(regex);
  return result;
}

/* Parses the program arguments. Returns TRUE if program needs
 * to exit after parsing is complete
 */
gboolean parse_options(int argc, char* argv[])
{
  /* Declare argument options and argument variables */
  gboolean icon   = FALSE,    daemon = FALSE,
           clipboard = FALSE, primary = FALSE,
           exit = FALSE;
  
  GOptionEntry main_entries[] = 
  {
    {
      "daemon", 'd',
      G_OPTION_FLAG_NO_ARG,
      G_OPTION_ARG_NONE,
      &daemon, _("Run as daemon"),
      NULL
    },
    {
      "no-icon", 'n',
      G_OPTION_FLAG_NO_ARG,
      G_OPTION_ARG_NONE,
      &icon, _("Do not use status icon (Ctrl-Alt-P for menu)"),
      NULL
    },
    {
      "clipboard", 'c',
      G_OPTION_FLAG_NO_ARG,
      G_OPTION_ARG_NONE,
      &clipboard, _("Print clipboard contents"),
      NULL
    },
    {
      "primary", 'p',
      G_OPTION_FLAG_NO_ARG,
      G_OPTION_ARG_NONE,
      &primary, _("Print primary contents"),
      NULL
    },
    {
      NULL
    }
  };
  
  /* Option parsing */
  GOptionContext* context = g_option_context_new(NULL);
  /* Set summary */
  g_option_context_set_summary(context,
                             _("Clipboard CLI usage examples:\n\n"
                               "  echo \"copied to clipboard\" | parcellite\n"
                               "  parcellite \"copied to clipboard\"\n"
                               "  echo \"copied to clipboard\" | parcellite -c"));
  /* Set description */
  g_option_context_set_description(context,
                                 _("Written by Gilberto \"Xyhthyx\" Miralla.\n"
                                   "Report bugs to <gpib@rickyrockrat.com>."));
  /* Add entries and parse options */
  g_option_context_add_main_entries(context, main_entries, NULL);
  g_option_context_parse(context, &argc, &argv, NULL);
  g_option_context_free(context);
  
  /* Check which options were parseed */
  
  /* Do not display icon option */
  if (icon)
  {
    prefs.no_icon = TRUE;
  }
  /* Run as daemon option */
  else if (daemon)
  {
    init_daemon_mode();
    exit = TRUE;
  }
  /* Print clipboard option */
  else if (clipboard)
  {
    /* Grab clipboard */
    GtkClipboard* clip = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    
    /* Check if stdin has text to copy */
    if (!isatty(fileno(stdin)))
    {
      GString* piped_string = g_string_new(NULL);
      /* Append stdin to string */
      while (1)
      {
        gchar* buffer = (gchar*)g_malloc(256);
        if (fgets(buffer, 256, stdin) == NULL)
        {
          g_free(buffer);
          break;
        }
        g_string_append(piped_string, buffer);
        g_free(buffer);
      }
      /* Check if anything was piped in */
      if (piped_string->len > 0)
      {
        /* Truncate new line character */
        /* g_string_truncate(piped_string, (piped_string->len - 1)); */
        /* Copy to clipboard */
        gtk_clipboard_set_text(clip, piped_string->str, -1);
        gtk_clipboard_store(clip);
      }
      g_string_free(piped_string, TRUE);
    }
    /* Print clipboard text (if any) */
    gchar* clip_text = gtk_clipboard_wait_for_text(clip);
    if (clip_text)
      g_print("%s", clip_text);
    g_free(clip_text);
    
    /* Return true so program exits when finished parsing */
    exit = TRUE;
  }
  else if (primary)
  {
    /* Grab primary */
    GtkClipboard* prim = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
    /* Print primary text (if any) */
    gchar* prim_text = gtk_clipboard_wait_for_text(prim);
    if (prim_text)
      g_print("%s", prim_text);
    g_free(prim_text);
    
    /* Return true so program exits when finished parsing */
    exit = TRUE;
  }
  else
  {
    /* Copy from unrecognized options */
    gchar* argv_string = g_strjoinv(" ", argv + 1);
    GtkClipboard* clip = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    gtk_clipboard_set_text(clip, argv_string, -1);
    gtk_clipboard_store(clip);
    g_free(argv_string);
    /* Return true so program exits when finished parsing */
    exit = TRUE;
  }
  return exit;
}

