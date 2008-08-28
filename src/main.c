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
#include <gtk/gtk.h>
#include <pthread.h>
#include "main.h"
#include "utils.h"
#include "history.h"
#include "keybinder.h"
#include "preferences.h"
#include "parcellite-i18n.h"


/* Declare clipboard, menu and status icon */
static GtkClipboard *clipboard, *primary;
static gchar* clipboard_text;
static gchar* clipboard_last_item;
static GtkWidget* history_menu;
static GtkStatusIcon* status_icon;

/* This variable locks actions when one is being executed */
static gboolean executing_action = FALSE;

/* Create preferences */
prefs_t prefs = {DEFHISTORYLIM,      DEFCHARLENGTH,
                 DEFELLIPSIZE,       NULL,
                 NULL,               NULL,
                 DEFSAVEHIST,        DEFCONFIRMCLEAR,
                 DEFREVHISTORY,      DEFSINGLELINEMODE,
                 DEFHYPERLINKSMODE,  DEFNOICON};


/* Called every time user copies something to the clipboard (Ctrl-V) */
static void
clipboard_new_item(GtkClipboard *clipboard,
                   GdkEvent     *event,
                   gpointer      user_data)
{
  gchar* text = gtk_clipboard_wait_for_text(clipboard);
  /* Empty text check */
  if (!text)
  {
    g_free(clipboard_text);
    clipboard_text = NULL;
  }
  /* Duplicate text check */
  else if ((clipboard_text) && (g_ascii_strcasecmp(text, clipboard_text) == 0))
  {
    g_free(clipboard_text);
    clipboard_text = g_strdup(text);
  }
  /* Duplicate text check */
  else if ((clipboard_last_item) && (g_ascii_strcasecmp(text, clipboard_last_item) == 0))
  {
    g_free(clipboard_text);
    clipboard_text = g_strdup(text);
  }
  /* Hyperlink check */
  else if ((prefs.hyperlinks) && (!is_hyperlink(text)))
  {
    g_free(clipboard_text);
    clipboard_text = g_strdup(text);
  }
  /* New item */
  else
  {
    g_free(clipboard_text);
    clipboard_text = g_strdup(text);
    g_free(clipboard_last_item);
    clipboard_last_item = g_strdup(text);
    append_item(text);
  }
  g_free(text);
}

/* Thread function called for each action performed */
void *
execute_action(void *command)
{
  executing_action = TRUE;
  gtk_status_icon_set_from_stock(GTK_STATUS_ICON(status_icon), GTK_STOCK_EXECUTE);
  gtk_status_icon_set_tooltip(GTK_STATUS_ICON(status_icon), _("Executing action..."));
  system((gchar*)command);
  gtk_status_icon_set_from_stock(GTK_STATUS_ICON(status_icon), GTK_STOCK_PASTE);
  gtk_status_icon_set_tooltip(GTK_STATUS_ICON(status_icon), _("Clipboard Manager"));
  executing_action = FALSE;
  g_free((gchar*)command);
  /* Exit this thread */
  pthread_exit(NULL);
}

/* Called when an action is selected from actions menu */
static void
action_selected(GtkButton *button, gpointer user_data)
{
  /* Insert clipboard into command (user_data), execute and free */
  gchar* command = g_markup_printf_escaped((gchar*)user_data, clipboard_text);
  g_free(user_data);
  
  /* Create thread */
  pthread_t action_thread;
  if (pthread_create(&action_thread, NULL, execute_action, (void *)command))
  {
    g_free(command);
    g_warning(_("Could not create thread for executed action\n"));
  }
}

/* Called when Edit Actions is selected from actions menu */
static void
edit_actions_selected(GtkButton *button, gpointer user_data)
{
  /* This helps prevent multiple instances */
  if (!gtk_grab_get_current())
    /* Show the preferences dialog on the actions tab */
    show_preferences(1);
}

/* Called when the save button is clicked from the edit window */
static void
save_clicked(GtkButton *button, gpointer user_data)
{
  /* Get the text buffer, its text and set as clipboard */
  GtkTextBuffer* text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(user_data));
  GtkTextIter start, end;
  gtk_text_buffer_get_start_iter(text_buffer, &start);
  gtk_text_buffer_get_end_iter(text_buffer, &end);
  gchar* new_clipboard_text = gtk_text_buffer_get_text(text_buffer, &start, &end, TRUE);
  gtk_clipboard_set_text(clipboard, new_clipboard_text, -1);
  /* Get top-most window widget and destroy it */
  GtkWidget* window = gtk_widget_get_ancestor(GTK_WIDGET(user_data), GTK_TYPE_WINDOW);
  g_free(new_clipboard_text);
  gtk_widget_destroy(window);
}

/* Called when Edit is selected from history menu */
static void
edit_selected(GtkMenuItem *menu_item, gpointer user_data)
{
  /* This helps prevent multiple instances */
  if (!gtk_grab_get_current())
  {
    /* Create clipboard buffer and set its text */
    GtkTextBuffer* clipboard_buffer = gtk_text_buffer_new(NULL);
    gchar* cb_text = gtk_clipboard_wait_for_text(clipboard);
    if (cb_text)
      gtk_text_buffer_set_text(clipboard_buffer, cb_text, -1);
    g_free(cb_text);
    
    /* Create window */
    GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_grab_add(window);
    gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
    gtk_window_set_default_size(GTK_WINDOW(window), 450, 300);
    gtk_window_set_keep_above(GTK_WINDOW(window), TRUE);
    gtk_window_set_title(GTK_WINDOW(window), _("Editing Clipboard"));
    gtk_window_set_icon(GTK_WINDOW(window), gtk_widget_render_icon(window,
                                                                   GTK_STOCK_EDIT,
                                                                   GTK_ICON_SIZE_MENU,
                                                                   NULL));
    
    /* Add viewport, frame and alignment */
    GtkWidget* viewport = gtk_viewport_new(GTK_ADJUSTMENT(gtk_adjustment_new(0, 0, 0, 0, 0, 0)), 
                                           GTK_ADJUSTMENT(gtk_adjustment_new(0, 0, 0, 0, 0, 0)));
    
    /* gtk_viewport_set_shadow_type(GTK_VIEWPORT(viewport), GTK_SHADOW_ETCHED_OUT); */
    gtk_viewport_set_shadow_type(GTK_VIEWPORT(viewport), GTK_SHADOW_NONE);
    gtk_container_add(GTK_CONTAINER(window), viewport);
    GtkWidget* frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
    GtkWidget* label = gtk_label_new("Clipboard");
    gtk_misc_set_padding(GTK_MISC(label), 0, 2);
    gtk_label_set_markup(GTK_LABEL(label), _("<b>Clipboard</b>"));
    gtk_frame_set_label_widget(GTK_FRAME(frame), label);
    gtk_container_add(GTK_CONTAINER(viewport), frame);
    GtkWidget* alignment = gtk_alignment_new(0.50, 0.50, 1.0, 1.0);
    gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 2, 0, 2, 2);
    gtk_container_add(GTK_CONTAINER(frame), alignment);
    /* Add vbox for two rows of widgets */
    GtkWidget* vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(alignment), vbox);
    /* Add inner viewport and textview */
    GtkWidget* inner_viewport = gtk_viewport_new(GTK_ADJUSTMENT(
                                                 gtk_adjustment_new(0, 0, 0, 0, 0, 0)),
                                                 GTK_ADJUSTMENT(
                                                 gtk_adjustment_new(0, 0, 0, 0, 0, 0)));
    
    gtk_viewport_set_shadow_type(GTK_VIEWPORT(inner_viewport), GTK_SHADOW_ETCHED_OUT);
    gtk_box_pack_start(GTK_BOX(vbox), inner_viewport, TRUE, TRUE, 0);
    GtkWidget* scrolled_window = gtk_scrolled_window_new(GTK_ADJUSTMENT(
                                                         gtk_adjustment_new(0, 0, 0, 0, 0, 0)),
                                                         GTK_ADJUSTMENT(
                                                         gtk_adjustment_new(0, 0, 0, 0, 0, 0)));
    
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    
    gtk_container_add(GTK_CONTAINER(inner_viewport), scrolled_window);
    GtkWidget* text_view = gtk_text_view_new_with_buffer(clipboard_buffer);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(text_view), 2);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(text_view), 2);
    gtk_container_add(GTK_CONTAINER(scrolled_window), text_view);
    /* Button box underneath textview */
    GtkWidget* hbutton_box = gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(hbutton_box), GTK_BUTTONBOX_END);
    gtk_box_pack_start(GTK_BOX(vbox), hbutton_box, FALSE, FALSE, 2);
    GtkWidget* hbox = gtk_hbox_new(FALSE, 2);
    gtk_box_pack_start(GTK_BOX(hbutton_box), hbox, FALSE, FALSE, 0);
    /* Save button */
    GtkWidget* button_save = gtk_button_new();
    
    gtk_button_set_relief(GTK_BUTTON(button_save), GTK_RELIEF_NONE);
    gtk_button_set_image(GTK_BUTTON(button_save),
                         gtk_image_new_from_stock(GTK_STOCK_SAVE,
                                                  GTK_ICON_SIZE_MENU));
    
    gtk_widget_set_tooltip_text(button_save, _("Save changes"));
    g_signal_connect(G_OBJECT(button_save), "clicked", G_CALLBACK(save_clicked), text_view);
    gtk_box_pack_end(GTK_BOX(hbox), button_save, FALSE, FALSE, 0);
    /* Close button */
    GtkWidget* button_close = gtk_button_new();
    gtk_button_set_relief(GTK_BUTTON(button_close), GTK_RELIEF_NONE);
    gtk_button_set_image(GTK_BUTTON(button_close),
                         gtk_image_new_from_stock(GTK_STOCK_CLOSE,
                                                  GTK_ICON_SIZE_MENU));
    
    gtk_widget_set_tooltip_text(button_close, _("Close"));
    g_signal_connect_swapped(G_OBJECT(button_close), "clicked",
                             G_CALLBACK(gtk_widget_destroy), window);
    
    gtk_box_pack_end(GTK_BOX(hbox), button_close, FALSE, FALSE, 0);
    
    /* Position window near status icon */
    if (!prefs.noicon)
    {
      GdkRectangle area;
      gtk_status_icon_get_geometry(status_icon, NULL, &area, NULL);
      gtk_window_move(GTK_WINDOW(window), area.x, area.y);
    }
    gtk_widget_show_all(window);
  }
}

/* Called when an item is selected from history menu */
static void
item_selected(GtkMenuItem *menu_item, gpointer user_data)
{
  /* Get the text from the right element and set as clipboard */
  GSList* element = g_slist_nth(history, (guint)user_data);
  g_free(clipboard_text);
  clipboard_text = g_strdup((gchar*)element->data);
  gtk_clipboard_set_text(clipboard, (gchar*)element->data, -1);
  gtk_clipboard_set_text(primary, (gchar*)element->data, -1);
}

/* Called when Clear is selected from history menu */
static void
clear_selected(GtkMenuItem *menu_item, gpointer user_data)
{
  /* Check for confirm clear option */
  if (prefs.confclear)
  {
    GtkWidget* confirm_dialog = gtk_message_dialog_new(NULL,
                                                       GTK_DIALOG_MODAL,
                                                       GTK_MESSAGE_OTHER,
                                                       GTK_BUTTONS_OK_CANCEL,
                                                       _("Clear the history?"));
    
    if (gtk_dialog_run(GTK_DIALOG(confirm_dialog)) == GTK_RESPONSE_OK)
    {
      /* Clear history and free history-related variables */
      g_free(clipboard_text);
      clipboard_text = NULL;
      g_slist_free(history);
      history = NULL;
      save_history();
    }
    gtk_widget_destroy(confirm_dialog);
  }
  else
  {
    /* Clear history and free history-related variables */
    g_free(clipboard_text);
    clipboard_text = NULL;
    g_slist_free(history);
    history = NULL;
    save_history();
  }
}

/* Called when About is selected from right-click menu */
static void
show_about_dialog(GtkMenuItem *menu_item, gpointer user_data)
{
  /* This helps prevent multiple instances */
  if (!gtk_grab_get_current())
  {
    const gchar* authors[] = {"Gilberto \"Xyhthyx\" Miralla <xyhthyx@gmail.com>", NULL};
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
    gtk_window_set_icon(GTK_WINDOW(about_dialog),
                        gtk_widget_render_icon(about_dialog,
                                               GTK_STOCK_ABOUT,
                                               GTK_ICON_SIZE_MENU,
                                               NULL));
    
    gtk_about_dialog_set_name(GTK_ABOUT_DIALOG(about_dialog), "Parcellite");
    gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(about_dialog), VERSION);
    gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(about_dialog),
                                  _("Lightweight GTK+ clipboard manager."));
    
    gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(about_dialog),
                                 "http://parcellite.sourceforge.net");
    
    gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(about_dialog), "Copyright (C) 2007, 2008 Xyhthyx");
    gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(about_dialog), authors);
    gtk_about_dialog_set_translator_credits (GTK_ABOUT_DIALOG(about_dialog),
                                             "Davide Truffa <davide@catoblepa.org>\n"
                                             "Eckhard M. Jäger <bart@neeneenee.de>\n"
                                             "Gultyaev Alexey <hokum83@gmail.com>\n"
                                             "Michael Stempin <mstempin@web.de>\n"
                                             "Németh Tamás <ntomasz@vipmail.hu>\n"
                                             "Gilberto \"Xyhthyx\" Miralla <xyhthyx@gmail.com>");
    
    gtk_about_dialog_set_license(GTK_ABOUT_DIALOG(about_dialog), license);
    gtk_about_dialog_set_logo_icon_name(GTK_ABOUT_DIALOG(about_dialog), GTK_STOCK_PASTE);
    /* Run the about dialog */
    gtk_dialog_run(GTK_DIALOG(about_dialog));
    gtk_widget_destroy(about_dialog);
  }
}

/* Called when Preferences is selected from right-click menu */
static void
preferences_selected(GtkMenuItem *menu_item, gpointer user_data)
{
  /* This helps prevent multiple instances */
  if (!gtk_grab_get_current())
    /* Show the preferences dialog */
    show_preferences(0);
}

/* Called when Quit is selected from right-click menu */
static void
quit_selected(GtkMenuItem *menu_item, gpointer user_data)
{
  /* Prevent quit with dialogs open */
  if (!gtk_grab_get_current())
    /* Quit the program */
    gtk_main_quit();
}

/* Called when status icon is control-clicked */
static gboolean
show_actions_menu(gpointer data)
{
  /* Declare some variables */
  GtkWidget *menu,       *menu_item,
            *menu_image, *item_label;
  
  /* Create menu */
  menu = gtk_menu_new();
  g_signal_connect(G_OBJECT(menu), "selection-done", G_CALLBACK(gtk_widget_destroy), NULL);
  /* Actions using: */
  menu_item = gtk_image_menu_item_new_with_label("Actions using:");
  menu_image = gtk_image_new_from_stock(GTK_STOCK_EXECUTE, GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item), menu_image);
  g_signal_connect(G_OBJECT(menu_item), "select", G_CALLBACK(gtk_menu_item_deselect), NULL);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
  /* Clipboard contents */
  if (clipboard_text)
  {
    menu_item = gtk_menu_item_new_with_label("None");
    /* Modify menu item label properties */
    item_label = gtk_bin_get_child(GTK_BIN(menu_item));
    gtk_label_set_single_line_mode(GTK_LABEL(item_label), TRUE);
    gtk_label_set_ellipsize(GTK_LABEL(item_label), prefs.ellipsize);
    gtk_label_set_width_chars(GTK_LABEL(item_label), 30);
    /* Making bold... */
    gchar* bold_text = g_markup_printf_escaped("<b>%s</b>", clipboard_text);
    gtk_label_set_markup(GTK_LABEL(item_label), bold_text);
    g_free(bold_text);
    /* Append menu item */
    g_signal_connect(G_OBJECT(menu_item), "select", G_CALLBACK(gtk_menu_item_deselect), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
  }
  else
  {
    /* Create menu item for empty clipboard contents */
    menu_item = gtk_menu_item_new_with_label("None");
    /* Modify menu item label properties */
    item_label = gtk_bin_get_child(GTK_BIN(menu_item));
    gtk_label_set_markup(GTK_LABEL(item_label), _("<b>None</b>"));
    /* Append menu item */
    g_signal_connect(G_OBJECT(menu_item), "select", G_CALLBACK(gtk_menu_item_deselect), NULL);
    
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
  }
  /* -------------------- */
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
  /* Actions */
  gchar* path = g_build_filename(g_get_home_dir(), ACTIONSFILE, NULL);
  FILE* actions_file = fopen(path, "rb");
  g_free(path);
  /* Check that it opened and begin read */
  if (actions_file)
  {
    gint size;
    fread(&size, 4, 1, actions_file);
    /* Check if actions file is empty */
    if (!size)
    {
      /* File contained no actions so adding empty */
      menu_item = gtk_menu_item_new_with_label(_("Empty"));
      gtk_widget_set_sensitive(menu_item, FALSE);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    }
    /* Continue reading items until size is 0 */
    while (size)
    {
      /* Read name */
      gchar* name = (gchar*)g_malloc(size + 1);
      fread(name, size, 1, actions_file);
      name[size] = '\0';
      menu_item = gtk_menu_item_new_with_label(name);
      g_free(name);
      fread(&size, 4, 1, actions_file);
      /* Read command */
      gchar* command = (gchar*)g_malloc(size + 1);
      fread(command, size, 1, actions_file);
      command[size] = '\0';
      fread(&size, 4, 1, actions_file);
      /* Append the action */
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
      g_signal_connect(G_OBJECT(menu_item),         "activate",
                       G_CALLBACK(action_selected), (gpointer)command);      
    }
    fclose(actions_file);
  }
  else
  {
    /* File did not open so adding empty */
    menu_item = gtk_menu_item_new_with_label(_("Empty"));
    gtk_widget_set_sensitive(menu_item, FALSE);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
  }
  /* -------------------- */
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
  /* Edit actions */
  menu_item = gtk_image_menu_item_new_with_mnemonic(_("_Edit actions"));
  menu_image = gtk_image_new_from_stock(GTK_STOCK_EDIT, GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item), menu_image);
  g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(edit_actions_selected), NULL);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
  /* Popup the menu... */
  gtk_widget_show_all(menu);
  gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 1, gtk_get_current_event_time());
  /* Return false so the g_timeout_add() function is called only once */
  return FALSE;
}

/* Called when status icon is left-clicked */
static gboolean
show_history_menu(gpointer data)
{
  /* Declare some variables */
  GtkWidget *menu,       *menu_item,
            *menu_image, *item_label;
  
  /* Create the menu */
  menu = gtk_menu_new();
  g_signal_connect(G_OBJECT(menu), "selection-done", G_CALLBACK(gtk_widget_destroy), NULL);
  /* Edit clipboard */
  menu_item = gtk_image_menu_item_new_with_mnemonic(_("_Edit Clipboard"));
  menu_image = gtk_image_new_from_stock(GTK_STOCK_EDIT, GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item), menu_image);
  g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(edit_selected), NULL);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
  /* -------------------- */
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
  /* Items */
  if ((history) && (history->data))
  {
    /* Keeps track of item number */
    gint element_number = 0;
    
    /* Reverse history */
    if (prefs.revhist)
    {
      history = g_slist_reverse(history);
      element_number = g_slist_length(history) - 1;
    }
    
    GSList* element = history;
    /* Go through each element and add each */
    for (element = history; element != NULL; element = element->next)
    {
      GString* string = g_string_new((gchar*)element->data);
      /* Ellipsize text */
      if (string->len > prefs.charlength)
      {
        switch (prefs.ellipsize)
        {
          case PANGO_ELLIPSIZE_START:
            string = g_string_erase(string, 0, string->len-(prefs.charlength));
            string = g_string_prepend(string, "...");
            break;
          case PANGO_ELLIPSIZE_MIDDLE:
            string = g_string_erase(string, (prefs.charlength/2), string->len-(prefs.charlength));
            string = g_string_insert(string, (string->len/2), "...");
            break;
          case PANGO_ELLIPSIZE_END:
            string = g_string_truncate(string, prefs.charlength);
            string = g_string_append(string, "...");
            break;
        }
      }
      menu_item = gtk_menu_item_new_with_label(string->str);
      g_signal_connect(G_OBJECT(menu_item),          "activate",
                       G_CALLBACK(item_selected), (gpointer)element_number);
      
      /* Modify menu item label properties */
      item_label = gtk_bin_get_child(GTK_BIN(menu_item));
      gtk_label_set_single_line_mode(GTK_LABEL(item_label), prefs.singleline);
      
      /* Check if item is also clipboard text and make bold */
      if ((clipboard_text) && (g_ascii_strcasecmp((gchar*)element->data, clipboard_text) == 0))
      {
        gchar* bold_text = g_markup_printf_escaped("<b>%s</b>", string->str);
        gtk_label_set_markup(GTK_LABEL(item_label), bold_text);
        g_free(bold_text);
      }
      /* Append item */
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
      
      /* Prepare for next item */
      g_string_free(string, TRUE);
      if (prefs.revhist)
        element_number--;
      else
        element_number++;
    }
    if (prefs.revhist)
      history = g_slist_reverse(history);
  }
  else
  {
    /* Nothing in history so adding empty */
    menu_item = gtk_menu_item_new_with_label(_("Empty"));
    gtk_widget_set_sensitive(menu_item, FALSE);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
  }
  /* -------------------- */
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
  /* Clear */
  menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_CLEAR, NULL);
  g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(clear_selected), NULL);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
  /* Popup the menu... */
  gtk_widget_show_all(menu);
  gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 1, gtk_get_current_event_time());
  /* Return FALSE so the g_timeout_add() function is called only once */
  return FALSE;
}

/* Called when status icon is right-clicked */
static void
show_parcellite_menu(GtkStatusIcon *status_icon, guint button,
                     guint activate_time,  gpointer user_data)
{
  /* Declare some variables */
  GtkWidget *menu, *menu_item;
  
  /* Create the menu */
  menu = gtk_menu_new();
  g_signal_connect(G_OBJECT(menu), "selection-done", G_CALLBACK(gtk_widget_destroy), NULL);
  /* About */
  menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ABOUT, NULL);
  g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(show_about_dialog), NULL);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
  /* Preferences */
  menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_PREFERENCES, NULL);
  g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(preferences_selected), NULL);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
  /* -------------------- */
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
  /* Quit */
  menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, NULL);
  g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(quit_selected), NULL);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
  /* Popup the menu... */
  gtk_widget_show_all(menu);
  gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, user_data, button, activate_time);
}

/* Called when status icon is left-clicked */
static void
check_click(GtkStatusIcon *status_icon, gpointer user_data)
{
  /* Check what type of click was recieved */
  GdkEvent* current_event = gtk_get_current_event();
  /* Control click */
  if (current_event->button.state == (GDK_MOD2_MASK | GDK_CONTROL_MASK))
  {
    if (!executing_action)
      g_timeout_add(POPUPDELAY, show_actions_menu, NULL);
  }
  /* Normal click */
  else
    g_timeout_add(POPUPDELAY, show_history_menu, NULL);
  /* Free the event */
  gdk_event_free(current_event);
}

/* Called when history global hotkey is pressed */
void
history_hotkey(char *keystring, gpointer user_data)
{
  g_timeout_add(POPUPDELAY, show_history_menu, NULL);
}

/* Called when actions global hotkey is pressed */
void
actions_hotkey(char *keystring, gpointer user_data)
{
  g_timeout_add(POPUPDELAY, show_actions_menu, NULL);
}

/* Called when actions global hotkey is pressed */
void
menu_hotkey(char *keystring, gpointer user_data)
{
  show_parcellite_menu(status_icon, 0, 0, NULL);
}

/* Initialize some variables among other things */
static void
parcellite_init()
{
  
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
        g_string_truncate(piped_string, (piped_string->len - 1));
        /* Copy to clipboard */
        GtkClipboard* cb = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
        gtk_clipboard_set_text(cb, piped_string->str, -1);
        gtk_clipboard_store(cb);
        /* Exit */
        return 0;
      }
      g_string_free(piped_string, TRUE);
    }
  }
  
  /* Create clipboard */
  clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
  primary = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
  g_signal_connect(G_OBJECT(clipboard), "owner-change", G_CALLBACK(clipboard_new_item), NULL);
  
  /* Read preferences */
  read_preferences();
  
  /* Read history and get last item */
  if (prefs.savehist)
    read_history();
  
  clipboard_last_item = g_strdup((gchar*)get_last_item());
  
  /* Add current clipboard contents */
  GdkEvent* owner_change_event = gdk_event_new(GDK_OWNER_CHANGE);
  g_signal_emit_by_name(clipboard, "owner-change", owner_change_event);
  gdk_event_free(owner_change_event);
  
  /* Bind global keys */
  keybinder_init();
  keybinder_bind(prefs.histkey, history_hotkey, NULL);
  keybinder_bind(prefs.actionkey, actions_hotkey, NULL);
  keybinder_bind(prefs.menukey, menu_hotkey, NULL);
  
  /* Create status icon */
  if (!prefs.noicon)
  {
    status_icon = gtk_status_icon_new_from_stock(GTK_STOCK_PASTE);
    gtk_status_icon_set_tooltip(GTK_STATUS_ICON(status_icon), _("Clipboard Manager"));
    g_signal_connect(G_OBJECT(status_icon), "activate", G_CALLBACK(check_click), NULL);
    g_signal_connect(G_OBJECT(status_icon), "popup-menu", G_CALLBACK(show_parcellite_menu), NULL);
  }
  
  /* Run GTK+ loop */
  gtk_main();
  
  /* Unbind keys */
  keybinder_unbind(prefs.histkey, history_hotkey);
  keybinder_unbind(prefs.actionkey, actions_hotkey);
  keybinder_unbind(prefs.menukey, menu_hotkey);
  /* Free memory */
  g_free(prefs.histkey);
  g_free(prefs.actionkey);
  g_free(prefs.menukey);
  g_slist_free(history);
  g_free(clipboard_text);
  g_free(clipboard_last_item);
  
  /* Exit */
  return 0;
}
