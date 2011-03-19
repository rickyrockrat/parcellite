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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <stdlib.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <pthread.h>
#include <string.h>
#include "main.h"
#include "utils.h"
#include "history.h"
#include "keybinder.h"
#include "preferences.h"
#include "parcellite-i18n.h"

#define PARCELLITE_ICON "parcellite"

struct history_info{
  GtkWidget *menu;
  GtkWidget *clip_item;
  GtkWidget *title_item;
};
/* Uncomment the next line to print a debug trace. */
/*#define DEBUG   */

#ifdef DEBUG
#  define TRACE(x) x
#else
#  define TRACE(x) do {} while (FALSE);
#endif

static gchar* primary_text;
static gchar* clipboard_text;
static gchar* synchronized_text;
static GtkClipboard* primary;
static GtkClipboard* clipboard;
static GtkStatusIcon* status_icon;

static gboolean actions_lock = FALSE;

/* Init preferences structure */
prefs_t prefs = {DEF_USE_COPY,        DEF_USE_PRIMARY,      DEF_SYNCHRONIZE,
                 DEF_SAVE_HISTORY,    DEF_HISTORY_LIMIT,
                 DEF_HYPERLINKS_ONLY, DEF_CONFIRM_CLEAR,
                 DEF_SINGLE_LINE,     DEF_REVERSE_HISTORY,  DEF_ITEM_LENGTH,
                 DEF_ELLIPSIZE,
                 INIT_HISTORY_KEY,    INIT_ACTIONS_KEY,     INIT_MENU_KEY,
                 DEF_NO_ICON};


/**Turns up in 2.16  */
int p_strcmp (const char *str1, const char *str2)
{
#if (GTK_MAJOR_VERSION > 2 || ( GTK_MAJOR_VERSION == 2 && GTK_MAJOR_VERSION >= 16))
  return g_strcmp0(str1,str2);
#else
  if(NULL ==str1 && NULL == str2)
    return 0;
  if(NULL ==str1 && str2)
    return -1;
  if(NULL ==str2 && str1)
    return 1;
  return strcmp(str1,str2);
#endif
}
/* Called every CHECK_INTERVAL seconds to check for new items */
static gboolean item_check(gpointer data)
{
  /* Grab the current primary and clipboard text */
  gchar* primary_temp = gtk_clipboard_wait_for_text(primary);
  gchar* clipboard_temp = gtk_clipboard_wait_for_text(clipboard);
  
  /* What follows is an extremely confusing system of tests and crap... */
  
  /* Check if primary contents were lost */
  if ((primary_temp == NULL) && (primary_text != NULL))
  {
    /* Check contents */
    gint count;
    GdkAtom *targets;
    gboolean contents = gtk_clipboard_wait_for_targets(primary, &targets, &count);
    g_free(targets);
    /* Only recover lost contents if there isn't any other type of content in the clipboard */
    if (!contents)
    {
      gtk_clipboard_set_text(primary, primary_text, -1);
    }
  }
  else
  {
    GdkModifierType button_state;
    gdk_window_get_pointer(NULL, NULL, NULL, &button_state);
    /* Proceed if mouse button not being held */
    if ((primary_temp != NULL) && !(button_state & GDK_BUTTON1_MASK))
    {
      /* Check if primary is the same as the last entry */
      if (p_strcmp(primary_temp, primary_text) != 0)
      {
        /* New primary entry */
        g_free(primary_text);
        primary_text = p_strdup(primary_temp);
        /* Check if primary option is enabled and if there's text to add */
        if (prefs.use_primary && primary_text)
        {
          /* Check contents before adding */
          if (prefs.hyperlinks_only && is_hyperlink(primary_text))
          {
            delete_duplicate(primary_text);
            append_item(primary_text);
          }
          else
          {
            delete_duplicate(primary_text);
            append_item(primary_text);
          }
        }
      }
    }
  }
  
  /* Check if clipboard contents were lost */
  if ((clipboard_temp == NULL) && (clipboard_text != NULL))
  {
    /* Check contents */
    gint count;
    GdkAtom *targets;
    gboolean contents = gtk_clipboard_wait_for_targets(clipboard, &targets, &count);
    g_free(targets);
		/* Only recover lost contents if there isn't any other type of content in the clipboard */
		if (!contents)
		{
      g_print("Clipboard is null, recovering ...\n");
      gtk_clipboard_set_text(clipboard, clipboard_text, -1);
    }
  }
  else
  {
    /* Check if clipboard is the same as the last entry */
    if (p_strcmp(clipboard_temp, clipboard_text) != 0)
    {
      /* New clipboard entry */
      g_free(clipboard_text);
      clipboard_text = p_strdup(clipboard_temp);
      /* Check if clipboard option is enabled and if there's text to add */
      if (prefs.use_copy && clipboard_text)
      {
        /* Check contents before adding */
        if (prefs.hyperlinks_only && is_hyperlink(clipboard_text))
        {
          delete_duplicate(clipboard_text);
          append_item(clipboard_text);
        }
        else
        {
          delete_duplicate(clipboard_text);
          append_item(clipboard_text);
        }
      }
    }
  }
  
  /* Synchronization */
  if (prefs.synchronize)
  {
    if (p_strcmp(synchronized_text, primary_text) != 0)
    {
      g_free(synchronized_text);
      synchronized_text = p_strdup(primary_text);
      gtk_clipboard_set_text(clipboard, primary_text, -1);
    }
    else if (p_strcmp(synchronized_text, clipboard_text) != 0)
    {
      g_free(synchronized_text);
      synchronized_text = p_strdup(clipboard_text);
      gtk_clipboard_set_text(primary, clipboard_text, -1);
    }
  }
  
  /* Cleanup */
  g_free(primary_temp);
  g_free(clipboard_temp);
  
  return TRUE;
}

/* Thread function called for each action performed */
static void *execute_action(void *command)
{
  /* Execute action */
  actions_lock = TRUE;
  if (!prefs.no_icon)
  {
  gtk_status_icon_set_from_stock((GtkStatusIcon*)status_icon, GTK_STOCK_EXECUTE);
  gtk_status_icon_set_tooltip((GtkStatusIcon*)status_icon, _("Executing action..."));
  }
  if(system((gchar*)command))
  	g_print("sytem command '%s' failed\n",(gchar *)command);
  if (!prefs.no_icon)
  {
	gtk_status_icon_set_from_stock((GtkStatusIcon*)status_icon, PARCELLITE_ICON);
  gtk_status_icon_set_tooltip((GtkStatusIcon*)status_icon, _("Clipboard Manager"));
  }
  actions_lock = FALSE;
  g_free((gchar*)command);
  /* Exit this thread */
  pthread_exit(NULL);
}

/* Called when execution action exits */
static void action_exit(GPid pid, gint status, gpointer data)
{
  g_spawn_close_pid(pid);
  if (!prefs.no_icon)
  {
		gtk_status_icon_set_from_stock((GtkStatusIcon*)status_icon, PARCELLITE_ICON);
    gtk_status_icon_set_tooltip((GtkStatusIcon*)status_icon, _("Clipboard Manager"));
  }
  actions_lock = FALSE;
}

/* Called when an action is selected from actions menu */
static void action_selected(GtkButton *button, gpointer user_data)
{
  /* Change icon and enable lock */
  actions_lock = TRUE;
  if (!prefs.no_icon)
  {
    gtk_status_icon_set_from_stock((GtkStatusIcon*)status_icon, GTK_STOCK_EXECUTE);
    gtk_status_icon_set_tooltip((GtkStatusIcon*)status_icon, _("Executing action..."));
  }
  
  /* Insert clipboard into command (user_data), and prepare it for execution */
  gchar* clipboard_text = gtk_clipboard_wait_for_text(clipboard);
  gchar* command = g_markup_printf_escaped((gchar*)user_data, clipboard_text);
  g_free(clipboard_text);
  g_free(user_data);
  gchar* shell_command = g_shell_quote(command);
  g_free(command);
  gchar* cmd = g_strconcat("/bin/sh -c ", shell_command, NULL);
  g_free(shell_command);
  
  /* Execute action */
  GPid pid;
  gchar **argv;
  g_shell_parse_argv(cmd, NULL, &argv, NULL);
  g_free(cmd);
  g_spawn_async(NULL, argv, NULL, G_SPAWN_DO_NOT_REAP_CHILD, NULL, NULL, &pid, NULL);
  g_child_watch_add(pid, (GChildWatchFunc)action_exit, NULL);
  g_strfreev(argv);
}

/* Called when Edit Actions is selected from actions menu */
static void edit_actions_selected(GtkButton *button, gpointer user_data)
{
  /* This helps prevent multiple instances */
  if (!gtk_grab_get_current())
    /* Show the preferences dialog on the actions tab */
    show_preferences(ACTIONS_TAB);
}

/* Called when Edit is selected from history menu */
static void edit_selected(GtkMenuItem *menu_item, gpointer user_data)
{
  /* This helps prevent multiple instances */
  if (!gtk_grab_get_current())
  {
	  gchar* current_clipboard_text;
    /* Create clipboard buffer and set its text */
    GtkTextBuffer* clipboard_buffer = gtk_text_buffer_new(NULL);
		if( NULL != menu_item){
			current_clipboard_text=p_strdup(gtk_label_get_text((GtkLabel *)gtk_bin_get_child((GtkBin*)menu_item)));
			
		}else{
			current_clipboard_text = gtk_clipboard_wait_for_text(clipboard);
		}
    
    if (current_clipboard_text != NULL)
    {
			TRACE(g_print("Got '%s'\n",current_clipboard_text));
      gtk_text_buffer_set_text(clipboard_buffer, current_clipboard_text, -1);
    }
    
    /* Create the dialog */
    GtkWidget* dialog = gtk_dialog_new_with_buttons(_("Editing Clipboard"), NULL,
                                                   (GTK_DIALOG_MODAL   +    GTK_DIALOG_NO_SEPARATOR),
                                                    GTK_STOCK_CANCEL,       GTK_RESPONSE_REJECT,
                                                    GTK_STOCK_OK,           GTK_RESPONSE_ACCEPT, NULL);
    
    gtk_window_set_default_size((GtkWindow*)dialog, 450, 300);
    gtk_window_set_icon((GtkWindow*)dialog, gtk_widget_render_icon(dialog, GTK_STOCK_EDIT, GTK_ICON_SIZE_MENU, NULL));
    
    /* Build the scrolled window with the text view */
    GtkWidget* scrolled_window = gtk_scrolled_window_new((GtkAdjustment*) gtk_adjustment_new(0, 0, 0, 0, 0, 0),
                                                         (GtkAdjustment*) gtk_adjustment_new(0, 0, 0, 0, 0, 0));
    
    gtk_scrolled_window_set_policy((GtkScrolledWindow*)scrolled_window,
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), scrolled_window, TRUE, TRUE, 2);
    GtkWidget* text_view = gtk_text_view_new_with_buffer(clipboard_buffer);
    gtk_text_view_set_left_margin((GtkTextView*)text_view, 2);
    gtk_text_view_set_right_margin((GtkTextView*)text_view, 2);
    gtk_container_add((GtkContainer*)scrolled_window, text_view);
    
    /* Run the dialog */
    gtk_widget_show_all(dialog);
    if (gtk_dialog_run((GtkDialog*)dialog) == GTK_RESPONSE_ACCEPT)
    {
      /* Save changes done to the clipboard */
      GtkTextIter start, end;
      gtk_text_buffer_get_start_iter(clipboard_buffer, &start);
      gtk_text_buffer_get_end_iter(clipboard_buffer, &end);
      gchar* new_clipboard_text = gtk_text_buffer_get_text(clipboard_buffer, &start, &end, TRUE);
      gtk_clipboard_set_text(clipboard, new_clipboard_text, -1);
      g_free(new_clipboard_text);
    }
    gtk_widget_destroy(dialog);
    g_free(current_clipboard_text);
  }
	else TRACE(g_print("gtk_grab_get_current returned !0\n"));
}

/* Called when an item is selected from history menu */
static void item_selected(GtkMenuItem *menu_item, gpointer user_data)
{
	GdkEventButton *b;
	
	GdkEvent *event=gtk_get_current_event();
	GSList* element = g_slist_nth(history, GPOINTER_TO_INT(user_data));
	/*GdkEventKey *k; 
	k=(GdkEventKey *)event; 
	g_print ("item_selected '%s' type %x val %x\n",(gchar *)element->data,event->type, k->keyval); */
	b=(GdkEventButton *)event;
	
	/*does GDK_KEY_PRESS for space, and enter gives 0xff0d*/
	
	if(GDK_BUTTON_RELEASE==event->type && 3 == b->button){ 
		TRACE(g_print("Right-click\n!"));	
		g_free(element->data);
     history = g_slist_delete_link(history, element);
		 /* Save changes */
   	if (prefs.save_history)
     	save_history();
	}else {			
	  /* Get the text from the right element and set as clipboard */
	  gtk_clipboard_set_text(clipboard, (gchar*)element->data, -1);
	  gtk_clipboard_set_text(primary, (gchar*)element->data, -1);
	}	
	
  gdk_event_free(event);		
}

/* Called when Clear is selected from history menu */
static void clear_selected(GtkMenuItem *menu_item, gpointer user_data)
{
  /* Check for confirm clear option */
  if (prefs.confirm_clear)
  {
    GtkWidget* confirm_dialog = gtk_message_dialog_new(NULL,
                                                       GTK_DIALOG_MODAL,
                                                       GTK_MESSAGE_OTHER,
                                                       GTK_BUTTONS_OK_CANCEL,
                                                       _("Clear the history?"));
    
    if (gtk_dialog_run((GtkDialog*)confirm_dialog) == GTK_RESPONSE_OK)
    {
      /* Clear history and free history-related variables */
      g_slist_free(history);
      history = NULL;
      save_history();
      g_free(primary_text);
      g_free(clipboard_text);
      g_free(synchronized_text);
      primary_text = p_strdup("");
      clipboard_text = p_strdup("");
      synchronized_text = p_strdup("");
      gtk_clipboard_set_text(primary, "", -1);
      gtk_clipboard_set_text(clipboard, "", -1);
    }
    gtk_widget_destroy(confirm_dialog);
  }
  else
  {
    /* Clear history and free history-related variables */
    g_slist_free(history);
    history = NULL;
    save_history();
    g_free(primary_text);
    g_free(clipboard_text);
    g_free(synchronized_text);
    primary_text = p_strdup("");
    clipboard_text = p_strdup("");
    synchronized_text = p_strdup("");
    gtk_clipboard_set_text(primary, "", -1);
    gtk_clipboard_set_text(clipboard, "", -1);
  }
}

/* Called when About is selected from right-click menu */
static void show_about_dialog(GtkMenuItem *menu_item, gpointer user_data)
{
  /* This helps prevent multiple instances */
  if (!gtk_grab_get_current())
  {
    const gchar* authors[] = {"Gilberto \"Xyhthyx\" Miralla <xyhthyx@gmail.com>\nDoug Springer <gpib@rickyrockrat.com>", NULL};
    const gchar* license =
      "This program is free software; you can redistribute it and/or modify\n"
      "it under the terms of the GNU General Public License as published by\n"
      "the Free Software Foundation; either version 3 of the License, or\n"
      "(at your option) any later version.\n"
      "\n"
      "This program is distributed in the hope that it will be useful,\n"
      "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
      "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
      "GNU General Public License for more details.\n"
      "\n"
      "You should have received a copy of the GNU General Public License\n"
      "along with this program.  If not, see <http://www.gnu.org/licenses/>.";
    
    /* Create the about dialog */
    GtkWidget* about_dialog = gtk_about_dialog_new();
    gtk_window_set_icon((GtkWindow*)about_dialog,
                        gtk_widget_render_icon(about_dialog, GTK_STOCK_ABOUT, GTK_ICON_SIZE_MENU, NULL));
    
    gtk_about_dialog_set_name((GtkAboutDialog*)about_dialog, "Parcellite");
    #ifdef HAVE_CONFIG_H
    gtk_about_dialog_set_version((GtkAboutDialog*)about_dialog, VERSION);
    #endif
    gtk_about_dialog_set_comments((GtkAboutDialog*)about_dialog,
                                _("Lightweight GTK+ clipboard manager."));
    
    gtk_about_dialog_set_website((GtkAboutDialog*)about_dialog,
                                 "http://parcellite.sourceforge.net");
    
    gtk_about_dialog_set_copyright((GtkAboutDialog*)about_dialog, "Copyright (C) 2007, 2008 Gilberto \"Xyhthyx\" Miralla\n"
	   "Copyright (C) 2010-2011 Doug Springer");
    gtk_about_dialog_set_authors((GtkAboutDialog*)about_dialog, authors);
    gtk_about_dialog_set_translator_credits ((GtkAboutDialog*)about_dialog,
                                             "Miloš Koutný <milos.koutny@gmail.com>\n"
                                             "Kim Jensen <reklamepost@energimail.dk>\n"
                                             "Eckhard M. Jäger <bart@neeneenee.de>\n"
                                             "Michael Stempin <mstempin@web.de>\n"
                                             "Benjamin 'sphax3d' Danon <sphax3d@gmail.com>\n"
                                             "Németh Tamás <ntomasz@vipmail.hu>\n"
                                             "Davide Truffa <davide@catoblepa.org>\n"
                                             "Jiro Kawada <jiro.kawada@gmail.com>\n"
                                             "Øyvind Sæther <oyvinds@everdot.org>\n"
                                             "pankamyk <pankamyk@o2.pl>\n"
                                             "Tomasz Rusek <tomek.rusek@gmail.com>\n"
                                             "Phantom X <megaphantomx@bol.com.br>\n"
                                             "Ovidiu D. Niţan <ov1d1u@sblug.ro>\n"
                                             "Alexander Kazancev <kazancas@mandriva.ru>\n"
                                             "Daniel Nylander <po@danielnylander.se>\n"
                                             "Hedef Türkçe <iletisim@hedefturkce.com>\n"
                                             "Lyman Li <lymanrb@gmail.com>\n"
                                             "Gilberto \"Xyhthyx\" Miralla <xyhthyx@gmail.com>");
    
    gtk_about_dialog_set_license((GtkAboutDialog*)about_dialog, license);
	  gtk_about_dialog_set_logo_icon_name((GtkAboutDialog*)about_dialog, PARCELLITE_ICON);
    /* Run the about dialog */
    gtk_dialog_run((GtkDialog*)about_dialog);
    gtk_widget_destroy(about_dialog);
  }
}

/* Called when Preferences is selected from right-click menu */
static void preferences_selected(GtkMenuItem *menu_item, gpointer user_data)
{
  /* This helps prevent multiple instances */
  if (!gtk_grab_get_current())
    /* Show the preferences dialog */
    show_preferences(0);
}

/* Called when Quit is selected from right-click menu */
static void quit_selected(GtkMenuItem *menu_item, gpointer user_data)
{
  /* Prevent quit with dialogs open */
  if (!gtk_grab_get_current())
    /* Quit the program */
    gtk_main_quit();
}

/* Called when status icon is control-clicked */
static gboolean show_actions_menu(gpointer data)
{
  /* Declare some variables */
  GtkWidget *menu,       *menu_item,
            *menu_image, *item_label;
  
  /* Create menu */
  menu = gtk_menu_new();
  g_signal_connect((GObject*)menu, "selection-done", (GCallback)gtk_widget_destroy, NULL);
  /* Actions using: */
  menu_item = gtk_image_menu_item_new_with_label("Actions using:");
  menu_image = gtk_image_new_from_stock(GTK_STOCK_EXECUTE, GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image((GtkImageMenuItem*)menu_item, menu_image);
  g_signal_connect((GObject*)menu_item, "select", (GCallback)gtk_menu_item_deselect, NULL);
  gtk_menu_shell_append((GtkMenuShell*)menu, menu_item);
  /* Clipboard contents */
  gchar* text = gtk_clipboard_wait_for_text(clipboard);
  if (text != NULL)
  {
    menu_item = gtk_menu_item_new_with_label("None");
    /* Modify menu item label properties */
    item_label = gtk_bin_get_child((GtkBin*)menu_item);
    gtk_label_set_single_line_mode((GtkLabel*)item_label, TRUE);
    gtk_label_set_ellipsize((GtkLabel*)item_label, prefs.ellipsize);
    gtk_label_set_width_chars((GtkLabel*)item_label, 30);
    /* Making bold... */
    gchar* bold_text = g_markup_printf_escaped("<b>%s</b>", text);
    gtk_label_set_markup((GtkLabel*)item_label, bold_text);
    g_free(bold_text);
    /* Append menu item */
    g_signal_connect((GObject*)menu_item, "select", (GCallback)gtk_menu_item_deselect, NULL);
    gtk_menu_shell_append((GtkMenuShell*)menu, menu_item);
  }
  else
  {
    /* Create menu item for empty clipboard contents */
    menu_item = gtk_menu_item_new_with_label("None");
    /* Modify menu item label properties */
    item_label = gtk_bin_get_child((GtkBin*)menu_item);
    gtk_label_set_markup((GtkLabel*)item_label, _("<b>None</b>"));
    /* Append menu item */
    g_signal_connect((GObject*)menu_item, "select", (GCallback)gtk_menu_item_deselect, NULL);
    
    gtk_menu_shell_append((GtkMenuShell*)menu, menu_item);
  }
  /* -------------------- */
  gtk_menu_shell_append((GtkMenuShell*)menu, gtk_separator_menu_item_new());
  /* Actions */
  gchar* path = g_build_filename(g_get_home_dir(), ACTIONS_FILE, NULL);
  FILE* actions_file = fopen(path, "rb");
  g_free(path);
  /* Check that it opened and begin read */
  if (actions_file)
  {
    gint size;
    if(0==fread(&size, 4, 1, actions_file))
    	g_print("1:got 0 items from fread\n");
    /* Check if actions file is empty */
    if (!size)
    {
      /* File contained no actions so adding empty */
      menu_item = gtk_menu_item_new_with_label(_("Empty"));
      gtk_widget_set_sensitive(menu_item, FALSE);
      gtk_menu_shell_append((GtkMenuShell*)menu, menu_item);
    }
    /* Continue reading items until size is 0 */
    while (size)
    {
      /* Read name */
      gchar* name = (gchar*)g_malloc(size + 1);
      if( 0 ==fread(name, size, 1, actions_file))
      	g_print("2:got 0 items from fread\n");
      name[size] = '\0';
      menu_item = gtk_menu_item_new_with_label(name);
      g_free(name);
      if(0 ==fread(&size, 4, 1, actions_file))
      	g_print("3:got 0 items from fread\n");
      /* Read command */
      gchar* command = (gchar*)g_malloc(size + 1);
      if(0 ==fread(command, size, 1, actions_file))
      	g_print("4:got 0 items from fread\n");
      command[size] = '\0';
      if(0 ==fread(&size, 4, 1, actions_file))
      	g_print("5:got 0 items from fread\n");
      /* Append the action */
      gtk_menu_shell_append((GtkMenuShell*)menu, menu_item);
      g_signal_connect((GObject*)menu_item,        "activate",
                       (GCallback)action_selected, (gpointer)command);      
    }
    fclose(actions_file);
  }
  else
  {
    /* File did not open so adding empty */
    menu_item = gtk_menu_item_new_with_label(_("Empty"));
    gtk_widget_set_sensitive(menu_item, FALSE);
    gtk_menu_shell_append((GtkMenuShell*)menu, menu_item);
  }
  /* -------------------- */
  gtk_menu_shell_append((GtkMenuShell*)menu, gtk_separator_menu_item_new());
  /* Edit actions */
  menu_item = gtk_image_menu_item_new_with_mnemonic(_("_Edit actions"));
  menu_image = gtk_image_new_from_stock(GTK_STOCK_EDIT, GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image((GtkImageMenuItem*)menu_item, menu_image);
  g_signal_connect((GObject*)menu_item, "activate", (GCallback)edit_actions_selected, NULL);
  gtk_menu_shell_append((GtkMenuShell*)menu, menu_item);
  /* Popup the menu... */
  gtk_widget_show_all(menu);
  gtk_menu_popup((GtkMenu*)menu, NULL, NULL, NULL, NULL, 1, gtk_get_current_event_time());
  /* Return false so the g_timeout_add() function is called only once */
  return FALSE;
}
#define KBUF_SIZE 20
static gboolean selection_done(GtkMenuShell *menushell, gpointer user_data) 
{
	/*g_print("Got selection_done\n"); */
	gtk_widget_destroy((GtkWidget *)menushell);
}

static void activate_current (GtkMenuShell *menushell, gboolean force_hide, gpointer user) 
{
  GdkEventButton *b;
	GdkEvent *event=gtk_get_current_event();
	GdkEventKey *k; 
	k=(GdkEventKey *)event; 
/*	g_print ("activate_current: type %x val %x\n",event->type, k->keyval); */
/**    if(GDK_KEY_PRESS == k->type && ' ' ==k->keyval)
    return TRUE;*/
/*  g_print("Got activate_current\n"); */


	/*gtk_menu_shell_select_item((GtkMenuShell *)user,(GtkWidget *)menushell); */
	/*key_release_cb((GtkWidget *)menushell,2,user_data); */
}
void set_widget_bg(gchar *color, GtkWidget *w)
{
  GdkColor c, *cp;
  GtkRcStyle *st;
  /** c.red = 65535;
  c.green = 0;
  c.blue = 0;*/
  /*g_print("set_widget_bg\n"); */
  if(NULL != color){
    gdk_color_parse (color, &c); 
    cp=&c;
  }else
    cp=NULL;
    
  gtk_widget_modify_bg(w, GTK_STATE_NORMAL, cp);
  return;
#if 0 /**none of this works  */  
  gtk_widget_modify_bg(w, GTK_STATE_ACTIVE, cp);

  /*gdk_color_parse (color, &c); */
  st=gtk_widget_get_modifier_style(w);
  /*st=gtk_rc_style_new (); */
  st->bg[GTK_STATE_NORMAL] = st->bg[GTK_STATE_ACTIVE] = c;
  gtk_widget_modify_style (w, st);
  gtk_widget_show(w);
  /*gtk_widget_modify_bg (w, GTK_STATE_NORMAL, &c); */
#endif
}
static gboolean key_release_cb (GtkWidget *w,GdkEventKey *e, gpointer user)
{
	static gchar *kstr=NULL;
	static gint idx;
  /*static GdkEvent *last_event=NULL; */
	gint first, current,off;
	static GtkWidget *item=NULL;
	GList *children;
  struct history_info *h;

  h=(struct history_info *)user;
	
/**  	if( NULL != e ){
    printf("krc %c (%x) S%x T%x C%x,SE%x, G%x, W%p\n",e->keyval,e->keyval,e->state,e->type,e->hardware_keycode,e->send_event,e->group,e->window);
    fflush(NULL);
  }*/
		
	/**serves as init for keysearch  */
	if(NULL ==w && NULL==e && NULL == user){
		if(NULL != kstr)
			g_free(kstr);
		kstr=g_strnfill(KBUF_SIZE+8,0);
		idx=0;
		return 0;
	}else if(NULL == kstr){
		g_print("kstr null. Not init\n");
		return FALSE;
	}
  if(NULL == e){
    g_print("No Event!\n");
    return FALSE;
  }
  if(0 == prefs.type_search)/**searching is turned off  */
    return FALSE;
  if(GDK_KEY_PRESS == e->type && ' ' == e->keyval) /**ignore space presses  */
    return TRUE;
    /**pass all other non-release events on  */
  if(GDK_KEY_RELEASE != e->type && GDK_BUTTON_RELEASE != e->type) 
    return FALSE;
  /** if(GDK_SELECTION_NOTIFY == e->type){
    g_print("last %x\n",last_event->type);
    last_event=(GdkEvent *)e;
    return TRUE;
  }
  last_event=(GdkEvent *)e;*/
	/** if(user)
	if(e->state & (GDK_SHIFT_MASK|GDK_LOCK_MASK) != e->state){
		g_print("rfs to use mods\n");
		TRACE(g_print("state is %X. Refusing to use mods\n",e->state));
		return TRUE;
	} have to use for _ and others*/
	if( e->state & GDK_MOD1_MASK){/**alt key pressed  */
		if(e->keyval == 'e'){
			TRACE(g_print("Alt-E\n"));
			gtk_grab_remove(w);
		  edit_selected((GtkMenuItem *)item, NULL);
		}
			
		else if(e->keyval == 'c'){
			TRACE(g_print("Alt-C\n"));
			clear_selected(NULL, NULL); 
		}	else{
			TRACE(g_print("Ignoring Alt-%c (0x%02x) state 0x%x",e->keyval,e->keyval,e->state));
		}
		return FALSE;
	}
	if(e->keyval == 0xff08){/**backspace  */
		if(idx)
			--idx;
    else if( NULL != h->clip_item){
      gtk_menu_shell_select_item((GtkMenuShell *)h->menu,(GtkWidget *)h->clip_item);
    }
    set_widget_bg(NULL,h->menu);
		kstr[idx]=0;
		return 0;
	}
	if(e->keyval >'~'){
		TRACE(g_print("Ignoring key '%c' 0x%02x\n",e->keyval,e->keyval));	
		return FALSE;
	}
	if(idx>=KBUF_SIZE){
		TRACE(g_print("keys full\n"));
		return TRUE;
	}
	kstr[idx++]=e->keyval;
	kstr[idx]=0;
	for ( off=0; off<50;++off){ /** this loop does a char search based on offset  */
		children=gtk_container_get_children((GtkContainer *)h->menu);
		item=NULL;
		current=first=0; /**first is edit,   */
		while(NULL != children->next){
			gchar *l;
			gint slen;
			GtkWidget *child=gtk_bin_get_child((GtkBin*)children->data);
			if(GTK_IS_LABEL(child)){
				l=(gchar *)gtk_label_get_text((GtkLabel *)child);
				slen=strlen(l);
				if(slen>off){
					gint c;
					if(prefs.case_search)
						c=strncmp(kstr,&l[off],idx);
					else
						c=g_ascii_strncasecmp(kstr,&l[off],idx);
					if(!c){
						if(0 ==current){
							first=1;
						}	else{
							first=0;
						}
						
						if(!first ){
							TRACE(g_print("Got cmp'%s'='%s'\n",kstr,l));
							item=(GtkWidget *)children->data;
							goto foundit;
						}
						
					}		
				}
			}
			children=children->next;
			++current;
		}	
  }
  /**didn't find it. Set our title and return */
  set_widget_bg("red",h->menu);
  return TRUE;
foundit:
  set_widget_bg(NULL,h->menu);
	/**user->children...
	GList *children;  
	gpointer data,next,prev
	data should be a GtkMenuItem list, whose children are labels...
	*/	 
	/*str=(gchar *)gtk_label_get_text((GtkLabel *)gtk_bin_get_child((GtkBin*)children->data)); */
	TRACE(g_print("Got '%c' 0x%02x, state 0x%02X",e->keyval,e->keyval,e->state));
	if(NULL !=item){
		if(first){TRACE(g_print("First:"));}
		/*if(last)TRACE(g_print("Last:")); */
		TRACE(g_print("At Item '%s'",gtk_label_get_text((GtkLabel *)gtk_bin_get_child((GtkBin*)item))));
		gtk_menu_shell_select_item((GtkMenuShell *)h->menu,(GtkWidget *)item);
	}
		
	TRACE(g_print("\n"));
	return TRUE;
}
/**postition the history dialog  - should only be called if prefs.history_pos is set */
void postition_history(GtkMenu *menu,gint *x,gint *y,gboolean *push_in, gpointer user_data)
{
	GdkScreen *s;
	gint sx,sy;
	s=gdk_screen_get_default();
	sx= gdk_screen_get_width(s);
	sy= gdk_screen_get_height(s);
	if(NULL !=push_in)
		*push_in=FALSE;
	if(1 == GPOINTER_TO_INT(user_data)){
		if(NULL !=x) *x=sx;
		if(NULL !=y) *y=sy;	
	}else{
		if(prefs.history_pos){
			int xx,yy;
			if(prefs.history_x > prefs.item_length )
				xx=prefs.history_x-prefs.item_length;
			else
				xx=1;
			if(prefs.history_y > prefs.history_limit )
				yy=prefs.history_y-prefs.history_limit;
			else
				yy=1;
			if(NULL !=x) *x=xx;
			if(NULL !=y) *y=yy;	
			TRACE(g_print("x=%d, y=%d\n",xx,yy));
		}
		
	}
	
}

static GtkWidget *create_history_title(struct history_info *h, gchar *text)
{
  GtkWidget *menu_image;
  /* Create box for xpm and label */
  /*h->title_box = gtk_hbox_new (FALSE, 0); */
  /*gtk_container_set_border_width (GTK_CONTAINER (h->title_box), 2); */
  /* Get the style of the button to get the
   * background color. */
  /*h->tstyle= gtk_widget_get_style(h->menu); */
  if(0 ==prefs.type_search){
    h->title_item = gtk_image_menu_item_new_with_mnemonic(_("_Edit Clipboard"));
    menu_image = gtk_image_new_from_stock(GTK_STOCK_EDIT, GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image((GtkImageMenuItem*)h->title_item, menu_image);
    g_signal_connect((GObject*)h->title_item, "activate", (GCallback)edit_selected, NULL);
  }else{
    h->title_item = gtk_image_menu_item_new_with_label(text);
    menu_image = gtk_image_new_from_stock(GTK_STOCK_EDIT, GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image((GtkImageMenuItem*)h->title_item, menu_image);
  #if 0 /**doesn't work. Gives a GTK_MENU_ITEM fail  */ 
     /* Pack the item into the box */
    gtk_container_add ((GtkContainer *)(h->title_box),h->title_item);
    gtk_widget_show(h->title_box);
    gtk_widget_show(h->title_item);
    return(h->title_box);
  #endif    
  }
  

  return(h->title_item);
}
/* Called when status icon is left-clicked */
static gboolean show_history_menu(gpointer data)
{
  /* Declare some variables */
  GtkWidget *menu,       *menu_item,
            *menu_image, *item_label;
  static struct history_info h;
  /**init our keystroke function  */
	key_release_cb(NULL,NULL,NULL);
  /* Create the menu */
  menu = gtk_menu_new();
  h.menu=menu;
  h.clip_item=NULL;
	gtk_menu_shell_set_take_focus((GtkMenuShell *)menu,TRUE); /**grab keyboard focus  */
	g_signal_connect((GObject*)menu, "selection-done", (GCallback)selection_done, NULL);
  /*g_signal_connect((GObject*)menu, "selection-done", (GCallback)gtk_widget_destroy, NULL); */
	/**Trap key events  */
	g_signal_connect((GObject*)menu, "key-release-event", (GCallback)key_release_cb, (gpointer)&h);
  g_signal_connect((GObject*)menu, "event", (GCallback)key_release_cb, (gpointer)&h);
	g_signal_connect((GObject*)menu, "activate-current", (GCallback)activate_current, (gpointer)menu);
	/**trap mnemonic events  */
	/*g_signal_connect((GObject*)menu, "mnemonic-activate", (GCallback)key_release_cb, (gpointer)menu);  */
  if(prefs.type_search){
    /* Edit clipboard */
    gtk_menu_shell_append((GtkMenuShell*)menu, create_history_title(&h,_("Use Alt-E to edit, Alt-C to clear")));    
  }else{
    menu_item = gtk_image_menu_item_new_with_mnemonic(_("_Edit Clipboard"));
    menu_image = gtk_image_new_from_stock(GTK_STOCK_EDIT, GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image((GtkImageMenuItem*)menu_item, menu_image);
    g_signal_connect((GObject*)menu_item, "activate", (GCallback)edit_selected, NULL);
    gtk_menu_shell_append((GtkMenuShell*)menu, menu_item);
  }

  /* -------------------- */
  gtk_menu_shell_append((GtkMenuShell*)menu, gtk_separator_menu_item_new());
  /* Items */
  if ((history != NULL) && (history->data != NULL))
  {
    /* Declare some variables */
    GSList* element;
    gint element_number = 0;
    gchar* primary_temp = gtk_clipboard_wait_for_text(primary);
    gchar* clipboard_temp = gtk_clipboard_wait_for_text(clipboard);
    /* Reverse history if enabled */
    if (prefs.reverse_history)
    {
      history = g_slist_reverse(history);
      element_number = g_slist_length(history) - 1;
    }
    /* Go through each element and adding each */
    for (element = history; element != NULL; element = element->next)
    {
      GString* string = g_string_new((gchar*)element->data);
      /* Ellipsize text */
      if (string->len > prefs.item_length)
      {
        switch (prefs.ellipsize)
        {
          case PANGO_ELLIPSIZE_START:
            string = g_string_erase(string, 0, string->len-(prefs.item_length));
            string = g_string_prepend(string, "...");
            break;
          case PANGO_ELLIPSIZE_MIDDLE:
            string = g_string_erase(string, (prefs.item_length/2), string->len-(prefs.item_length));
            string = g_string_insert(string, (string->len/2), "...");
            break;
          case PANGO_ELLIPSIZE_END:
            string = g_string_truncate(string, prefs.item_length);
            string = g_string_append(string, "...");
            break;
        }
      }
		  /* Remove control characters */
      gsize i = 0;
      while (i < string->len)
      {	 /**fix 100% CPU utilization for odd data. - bug 2976890   */
				gsize nline=0;
				while(string->str[i+nline] == '\n' && nline+i<string->len)
					nline++;
				if(nline){
					g_string_erase(string, i, nline);
					/* RMME printf("e %ld",nline);fflush(NULL); */
				}
				else
          i++;

      }
      /* Make new item with ellipsized text */
      menu_item = gtk_menu_item_new_with_label(string->str);
      g_signal_connect((GObject*)menu_item,      "activate",
                       (GCallback)item_selected, GINT_TO_POINTER(element_number));
      
      /* Modify menu item label properties */
      item_label = gtk_bin_get_child((GtkBin*)menu_item);
      gtk_label_set_single_line_mode((GtkLabel*)item_label, prefs.single_line);
      
      /* Check if item is also clipboard text and make bold */
      if ((clipboard_temp) && (p_strcmp((gchar*)element->data, clipboard_temp) == 0))
      {
        gchar* bold_text = g_markup_printf_escaped("<b>%s</b>", string->str);
        gtk_label_set_markup((GtkLabel*)item_label, bold_text);
        g_free(bold_text);
        h.clip_item=menu_item;
      }
      else if ((primary_temp) && (p_strcmp((gchar*)element->data, primary_temp) == 0))
      {
        gchar* italic_text = g_markup_printf_escaped("<i>%s</i>", string->str);
        gtk_label_set_markup((GtkLabel*)item_label, italic_text);
        g_free(italic_text);
        h.clip_item=menu_item;
      }
      /* Append item */
      gtk_menu_shell_append((GtkMenuShell*)menu, menu_item);
      /* Prepare for next item */
      g_string_free(string, TRUE);
      if (prefs.reverse_history)
        element_number--;
      else
        element_number++;
    }
    /* Cleanup */
    g_free(primary_temp);
    g_free(clipboard_temp);
    /* Return history to normal if reversed */
    if (prefs.reverse_history)
      history = g_slist_reverse(history);
  }
  else
  {
    /* Nothing in history so adding empty */
    menu_item = gtk_menu_item_new_with_label(_("Empty"));
    gtk_widget_set_sensitive(menu_item, FALSE);
    gtk_menu_shell_append((GtkMenuShell*)menu, menu_item);
  }
  /* -------------------- */
  gtk_menu_shell_append((GtkMenuShell*)menu, gtk_separator_menu_item_new());
  /* Clear */
	if(0 ==prefs.type_search){
  menu_item = gtk_image_menu_item_new_with_mnemonic(_("_Clear"));
  menu_image = gtk_image_new_from_stock(GTK_STOCK_CLEAR, GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image((GtkImageMenuItem*)menu_item, menu_image);
	g_signal_connect((GObject*)menu_item, "activate", (GCallback)clear_selected, NULL);
  gtk_menu_shell_append((GtkMenuShell*)menu, menu_item);
  }  
  /* Popup the menu... */
  gtk_widget_show_all(menu);
  gtk_menu_popup((GtkMenu*)menu, NULL, NULL, prefs.history_pos?postition_history:NULL, NULL, 1, gtk_get_current_event_time());
	/**set last entry at first -fixes bug 2974614 */
	gtk_menu_shell_select_first((GtkMenuShell*)menu, TRUE);
  /* Return FALSE so the g_timeout_add() function is called only once */
  return FALSE;
}

/* Called when status icon is right-clicked */
static void
show_parcellite_menu(GtkStatusIcon *status_icon, guint button,
                     guint activate_time,        gpointer user_data)
{
  /* Declare some variables */
  GtkWidget *menu, *menu_item;
  
  /* Create the menu */
  menu = gtk_menu_new();
  g_signal_connect((GObject*)menu, "selection-done", (GCallback)gtk_widget_destroy, NULL);
  /* About */
  menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ABOUT, NULL);
  g_signal_connect((GObject*)menu_item, "activate", (GCallback)show_about_dialog, NULL);
  gtk_menu_shell_append((GtkMenuShell*)menu, menu_item);
  /* Preferences */
  menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_PREFERENCES, NULL);
  g_signal_connect((GObject*)menu_item, "activate", (GCallback)preferences_selected, NULL);
  gtk_menu_shell_append((GtkMenuShell*)menu, menu_item);
  /* -------------------- */
  gtk_menu_shell_append((GtkMenuShell*)menu, gtk_separator_menu_item_new());
  /* Quit */
  menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, NULL);
  g_signal_connect((GObject*)menu_item, "activate", (GCallback)quit_selected, NULL);
  gtk_menu_shell_append((GtkMenuShell*)menu, menu_item);
  /* Popup the menu... */
  gtk_widget_show_all(menu);
  gtk_menu_popup((GtkMenu*)menu, NULL, NULL, NULL, user_data, button, activate_time);
}

/* Called when status icon is left-clicked */
static void
status_icon_clicked(GtkStatusIcon *status_icon, gpointer user_data)
{
  /* Check what type of click was recieved */
  GdkModifierType state;
  gtk_get_current_event_state(&state);
  /* Control click */
  if (state == GDK_MOD2_MASK+GDK_CONTROL_MASK || state == GDK_CONTROL_MASK)
  {
    if (actions_lock == FALSE)
    {
      g_timeout_add(POPUP_DELAY, show_actions_menu, NULL);
    }
  }
  /* Normal click */
  else
  {
    g_timeout_add(POPUP_DELAY, show_history_menu, NULL);
  }
}

/* Called when history global hotkey is pressed */
void
history_hotkey(char *keystring, gpointer user_data)
{
  g_timeout_add(POPUP_DELAY, show_history_menu, NULL);
}

/* Called when actions global hotkey is pressed */
void
actions_hotkey(char *keystring, gpointer user_data)
{
  g_timeout_add(POPUP_DELAY, show_actions_menu, NULL);
}

/* Called when actions global hotkey is pressed */
void
menu_hotkey(char *keystring, gpointer user_data)
{
  show_parcellite_menu(status_icon, 0, 0, NULL);
}

/* Startup calls and initializations */
static void parcellite_init()
{
  /* Create clipboard */
  primary = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
  clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
  g_timeout_add(CHECK_INTERVAL, item_check, NULL);
  
  /* Read preferences */
  read_preferences();
  
  /* Read history */
  if (prefs.save_history)
    read_history();
  
  /* Bind global keys */
  keybinder_init();
  keybinder_bind(prefs.history_key, history_hotkey, NULL);
  keybinder_bind(prefs.actions_key, actions_hotkey, NULL);
  keybinder_bind(prefs.menu_key, menu_hotkey, NULL);
  
  /* Create status icon */
  if (!prefs.no_icon)
  {
	  status_icon = gtk_status_icon_new_from_icon_name(PARCELLITE_ICON); 
    gtk_status_icon_set_tooltip((GtkStatusIcon*)status_icon, _("Clipboard Manager"));
    g_signal_connect((GObject*)status_icon, "activate", (GCallback)status_icon_clicked, NULL);
    g_signal_connect((GObject*)status_icon, "popup-menu", (GCallback)show_parcellite_menu, NULL);
  }
}

/* This is Sparta! */
int
main(int argc, char *argv[])
{
  bindtextdomain(GETTEXT_PACKAGE, PARCELLITELOCALEDIR);
  bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
  textdomain(GETTEXT_PACKAGE);
  
  /* Initiate GTK+ */
  gtk_init(&argc, &argv);
  
  /* Parse options and exit if returns TRUE */
  if (argc > 1)
  {
    if (parse_options(argc, argv))
      return 0;
  }
  /* Read input from stdin (if any) */
  else
  {
    /* Check if stdin is piped */
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
        GtkClipboard* clip = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
        gtk_clipboard_set_text(clip, piped_string->str, -1);
        gtk_clipboard_store(clip);
        /* Exit */
        return 0;
      }
      g_string_free(piped_string, TRUE);
    }
  }
  
  /* Init Parcellite */
  parcellite_init();
  
  /* Run GTK+ loop */
  gtk_main();
  
  /* Unbind keys */
  keybinder_unbind(prefs.history_key, history_hotkey);
  keybinder_unbind(prefs.actions_key, actions_hotkey);
  keybinder_unbind(prefs.menu_key, menu_hotkey);
  /* Cleanup */
  g_free(prefs.history_key);
  g_free(prefs.actions_key);
  g_free(prefs.menu_key);
  g_slist_free(history);
  g_free(primary_text);
  g_free(clipboard_text);
  g_free(synchronized_text);
  
  /* Exit */
  return 0;
}
