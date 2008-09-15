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
#include "history.h"
#include "keybinder.h"
#include "preferences.h"
#include "parcellite-i18n.h"

/* Declare some widgets */
GtkWidget *preferences_dialog, *history_spin,
          *charlength_spin,    *ellipsize_combo,
          *history_key_entry,  *actions_key_entry,
          *menu_key_entry,     *save_check,
          *confirm_check,      *reverse_check,
          *linemode_check,     *hyperlinks_check;
          

GtkListStore* actions_list;
GtkTreeSelection* actions_selection;

/* Apply the new preferences */
static void
apply_preferences()
{
  /* Unbind the keys before binding new ones */
  keybinder_unbind(prefs.histkey, history_hotkey);
  g_free(prefs.histkey);
  prefs.histkey = NULL;
  keybinder_unbind(prefs.actionkey, actions_hotkey);
  g_free(prefs.actionkey);
  prefs.actionkey = NULL;
  keybinder_unbind(prefs.menukey, menu_hotkey);
  g_free(prefs.menukey);
  prefs.menukey = NULL;
  
  /* Get the new preferences */
  prefs.histlim = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(history_spin));
  prefs.charlength = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(charlength_spin));
  prefs.ellipsize = gtk_combo_box_get_active(GTK_COMBO_BOX(ellipsize_combo)) + 1;
  prefs.histkey = g_strdup(gtk_entry_get_text(GTK_ENTRY(history_key_entry)));
  prefs.actionkey = g_strdup(gtk_entry_get_text(GTK_ENTRY(actions_key_entry)));
  prefs.menukey = g_strdup(gtk_entry_get_text(GTK_ENTRY(menu_key_entry)));
  prefs.savehist = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(save_check));
  prefs.confclear = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(confirm_check));
  prefs.revhist = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(reverse_check));
  prefs.singleline = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(linemode_check));
  prefs.hyperlinks = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(hyperlinks_check));
  
  /* Bind keys and apply the new history limit */
  keybinder_bind(prefs.histkey, history_hotkey, NULL);
  keybinder_bind(prefs.actionkey, actions_hotkey, NULL);
  keybinder_bind(prefs.menukey, menu_hotkey, NULL);
  truncate_history();
}

/* Save preferences to ~/.config/parcellite/parcelliterc */
static void
save_preferences()
{
  /* Create key */
  GKeyFile* rc_key = g_key_file_new();
  
  /* Add values */
  g_key_file_set_integer(rc_key, "rc", "history_limit", prefs.histlim);
  g_key_file_set_integer(rc_key, "rc", "character_length", prefs.charlength);
  g_key_file_set_integer(rc_key, "rc", "ellipsize", prefs.ellipsize);
  g_key_file_set_string(rc_key, "rc", "history_key", prefs.histkey);
  g_key_file_set_string(rc_key, "rc", "actions_key", prefs.actionkey);
  g_key_file_set_string(rc_key, "rc", "menu_key", prefs.menukey);
  g_key_file_set_boolean(rc_key, "rc", "save_history", prefs.savehist);
  g_key_file_set_boolean(rc_key, "rc", "confirm_clear", prefs.confclear);
  g_key_file_set_boolean(rc_key, "rc", "reverse_history", prefs.revhist);
  g_key_file_set_boolean(rc_key, "rc", "single_line_mode", prefs.singleline);
  g_key_file_set_boolean(rc_key, "rc", "hyperlinks_mode", prefs.hyperlinks);
  
  /* Check config and data directories */
  check_dirs();
  /* Save key to file */
  gchar* rc_file = g_build_filename(g_get_home_dir(), ".config/parcellite/parcelliterc", NULL);
  g_file_set_contents(rc_file, g_key_file_to_data(rc_key, NULL, NULL), -1, NULL);
  g_key_file_free(rc_key);
  g_free(rc_file);
}

/* Read ~/.config/parcellite/parcelliterc */
void
read_preferences()
{
  gchar* rc_file = g_build_filename(g_get_home_dir(), ".config/parcellite/parcelliterc", NULL);
  /* Create key */
  GKeyFile* rc_key = g_key_file_new();
  if (g_key_file_load_from_file(rc_key, rc_file, G_KEY_FILE_NONE, NULL))
  {
    /* Load values */
    prefs.histlim = g_key_file_get_integer(rc_key, "rc", "history_limit", NULL);
    prefs.charlength = g_key_file_get_integer(rc_key, "rc", "character_length", NULL);
    prefs.ellipsize = g_key_file_get_integer(rc_key, "rc", "ellipsize", NULL);
    prefs.histkey = g_key_file_get_string(rc_key, "rc", "history_key", NULL);
    prefs.actionkey = g_key_file_get_string(rc_key, "rc", "actions_key", NULL);
    prefs.menukey = g_key_file_get_string(rc_key, "rc", "menu_key", NULL);
    prefs.savehist = g_key_file_get_boolean(rc_key, "rc", "save_history", NULL);
    prefs.confclear = g_key_file_get_boolean(rc_key, "rc", "confirm_clear", NULL);
    prefs.revhist = g_key_file_get_boolean(rc_key, "rc", "reverse_history", NULL);
    prefs.singleline = g_key_file_get_boolean(rc_key, "rc", "single_line_mode", NULL);
    prefs.hyperlinks = g_key_file_get_boolean(rc_key, "rc", "hyperlinks_mode", NULL);
    
    /* Check for errors and set default values if any */
    if ((!prefs.histlim) || (prefs.histlim > 100) || (prefs.histlim < 0))
      prefs.histlim = DEFHISTORYLIM;
    if ((!prefs.charlength) || (prefs.charlength > 75) || (prefs.charlength < 0))
      prefs.charlength = DEFCHARLENGTH;
    if ((!prefs.ellipsize) || (prefs.ellipsize > 3) || (prefs.ellipsize < 0))
      prefs.ellipsize = DEFELLIPSIZE;
    if (!prefs.histkey)
      prefs.histkey = g_strdup(DEFHISTORYKEY);
    if (!prefs.actionkey)
      prefs.actionkey = g_strdup(DEFACTIONSKEY);
    if (!prefs.menukey)
      prefs.menukey = g_strdup(DEFMENUKEY);
  }
  else
  {
    /* Init default keys on error */
    prefs.histkey = g_strdup(DEFHISTORYKEY);
    prefs.actionkey = g_strdup(DEFACTIONSKEY);
    prefs.menukey = g_strdup(DEFMENUKEY);
  }
  g_key_file_free(rc_key);
  g_free(rc_file);
}

/* Read ~/.parcellite/actions into the treeview */
static void
read_actions()
{
  /* Open the file for reading */
  gchar* path = g_build_filename(g_get_home_dir(), ACTIONSFILE, NULL);
  FILE* actions_file = fopen(path, "rb");
  g_free(path);
  /* Check that it opened and begin read */
  if (actions_file)
  {
    /* Keep a row reference */
    GtkTreeIter row_iter;
    /* Read the size of the first item */
    gint size;
    fread(&size, 4, 1, actions_file);
    /* Continue reading until size is 0 */
    while (size != 0)
    {
      /* Read name */
      gchar* name = (gchar*)g_malloc(size + 1);
      fread(name, size, 1, actions_file);
      name[size] = '\0';
      fread(&size, 4, 1, actions_file);
      /* Read command */
      gchar* command = (gchar*)g_malloc(size + 1);
      fread(command, size, 1, actions_file);
      command[size] = '\0';
      fread(&size, 4, 1, actions_file);
      /* Append the read action */
      gtk_list_store_append(actions_list, &row_iter);
      gtk_list_store_set(actions_list, &row_iter, 0, name, 1, command, -1);
      g_free(name);
      g_free(command);
    }
    fclose(actions_file);
  }
}

/* Save the actions treeview to ~/.local/share/parcellite/actions */
static void
save_actions()
{
  /* Check config and data directories */
  check_dirs();
  /* Open the file for writing */
  gchar* path = g_build_filename(g_get_home_dir(), ACTIONSFILE, NULL);
  FILE* actions_file = fopen(path, "wb");
  g_free(path);
  /* Check that it opened and begin write */
  if (actions_file)
  {
    GtkTreeIter action_iter;
    /* Get and check if there's a first iter */
    if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(actions_list), &action_iter))
    {
      do
      {
        /* Get name and command */
        gchar *name, *command;
        gtk_tree_model_get(GTK_TREE_MODEL(actions_list), &action_iter, 0, &name, 1, &command, -1);
        GString* s_name = g_string_new(name);
        GString* s_command = g_string_new(command);
        g_free(name);
        g_free(command);
        /* Check that there's text to save */
        if ((s_name->len == 0) || (s_command->len == 0))
        {
          /* Free strings and skip iteration */
          g_string_free(s_name, TRUE);
          g_string_free(s_command, TRUE);
          continue;
        }
        else
        {
          /* Save action */
          fwrite(&(s_name->len), 4, 1, actions_file);
          fputs(s_name->str, actions_file);
          fwrite(&(s_command->len), 4, 1, actions_file);
          fputs(s_command->str, actions_file);
          /* Free strings */
          g_string_free(s_name, TRUE);
          g_string_free(s_command, TRUE);
        }
      }
      while(gtk_tree_model_iter_next(GTK_TREE_MODEL(actions_list), &action_iter));
    }
    /* End of file write */
    gint end = 0;
    fwrite(&end, 4, 1, actions_file);
    fclose(actions_file);
  }
}

/* Called when New button is clicked */
static void
on_new_clicked(GtkButton *button, gpointer user_data)
{
  /* Create the dialog */
  GtkWidget* new_dialog = gtk_dialog_new_with_buttons(_("New Action"),
             GTK_WINDOW(preferences_dialog),
             (GTK_DIALOG_MODAL + GTK_DIALOG_NO_SEPARATOR + GTK_DIALOG_DESTROY_WITH_PARENT),
             GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);
  
  gtk_window_set_icon(GTK_WINDOW(new_dialog),
                      gtk_widget_render_icon(new_dialog, GTK_STOCK_NEW,
                                             GTK_ICON_SIZE_MENU, NULL));
  
  gtk_box_set_spacing(GTK_BOX(GTK_DIALOG(new_dialog)->vbox), 6);
  gtk_window_set_resizable(GTK_WINDOW(new_dialog), FALSE);
  
  /* Build the name entry */
  GtkWidget* name_hbox = gtk_hbox_new(FALSE, 6);
  GtkWidget* name_label = gtk_label_new(NULL);
  gtk_label_set_markup(GTK_LABEL(name_label), _("<b>Name:</b>"));
  gtk_misc_set_alignment(GTK_MISC(name_label), 1.0, 0.50);
  gtk_label_set_width_chars(GTK_LABEL(name_label), 10);
  gtk_box_pack_start(GTK_BOX(name_hbox), name_label, TRUE, TRUE, 0);
  GtkWidget* name_entry = gtk_entry_new();
  gtk_widget_set_tooltip_text(name_entry, _("Name your action"));
  gtk_box_pack_end(GTK_BOX(name_hbox), name_entry, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(new_dialog)->vbox), name_hbox, FALSE, FALSE, 0);
  
  /* Build the command entry */
  GtkWidget* command_hbox = gtk_hbox_new(FALSE, 6);
  GtkWidget* command_label = gtk_label_new(NULL);
  gtk_label_set_markup(GTK_LABEL(command_label), _("<b>Command:</b>"));
  gtk_misc_set_alignment(GTK_MISC(command_label), 1.0, 0.50);
  gtk_label_set_width_chars(GTK_LABEL(command_label), 10);
  gtk_box_pack_start(GTK_BOX(command_hbox), command_label, TRUE, TRUE, 0);
  GtkWidget* command_entry = gtk_entry_new();
  gtk_widget_set_tooltip_text(command_entry,
                              _("\"%s\" will be replaced with the clipboard contents"));
  
  gtk_box_pack_end(GTK_BOX(command_hbox), command_entry, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(new_dialog)->vbox), command_hbox, FALSE, FALSE, 0);
  
  /* Run the dialog */
  gtk_widget_show_all(new_dialog);
  if (gtk_dialog_run(GTK_DIALOG(new_dialog)) == GTK_RESPONSE_ACCEPT)
  {
    /* Append new item */
    GtkTreeIter row_iter;
    gtk_list_store_append(actions_list, &row_iter);
    gtk_list_store_set(actions_list, &row_iter, 0,
                       gtk_entry_get_text(GTK_ENTRY(name_entry)), 1,
                       gtk_entry_get_text(GTK_ENTRY(command_entry)), -1);
  }
  gtk_widget_destroy(new_dialog);
}

/* Called when Edit button is clicked */
static void
on_edit_clicked(GtkButton *button, gpointer user_data)
{
  GtkTreeIter sel_iter;
  /* Check if selected */
  if (gtk_tree_selection_get_selected(actions_selection, NULL, &sel_iter))
  {
    /* Create the dialog */
    GtkWidget* edit_dialog = gtk_dialog_new_with_buttons(_("Edit Action"),
               GTK_WINDOW(preferences_dialog),
               (GTK_DIALOG_MODAL + GTK_DIALOG_NO_SEPARATOR + GTK_DIALOG_DESTROY_WITH_PARENT),
               GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
               GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);
    
    gtk_window_set_icon(GTK_WINDOW(edit_dialog),
                        gtk_widget_render_icon(edit_dialog,
                                               GTK_STOCK_NEW,
                                               GTK_ICON_SIZE_MENU, NULL));
    
    gtk_box_set_spacing(GTK_BOX(GTK_DIALOG(edit_dialog)->vbox), 6);
    gtk_window_set_resizable(GTK_WINDOW(edit_dialog), FALSE);
    
    /* Build the name entry */
    GtkWidget* name_hbox = gtk_hbox_new(FALSE, 6);
    GtkWidget* name_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(name_label), _("<b>Name:</b>"));
    gtk_misc_set_alignment(GTK_MISC(name_label), 1.0, 0.50);
    gtk_label_set_width_chars(GTK_LABEL(name_label), 10);
    gtk_box_pack_start(GTK_BOX(name_hbox), name_label, TRUE, TRUE, 0);
    GtkWidget* name_entry = gtk_entry_new();
    gtk_widget_set_tooltip_text(name_entry, _("Name your action"));
    gtk_box_pack_end(GTK_BOX(name_hbox), name_entry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(edit_dialog)->vbox), name_hbox, FALSE, FALSE, 0);
    
    /* Build the command entry */
    GtkWidget* command_hbox = gtk_hbox_new(FALSE, 6);
    GtkWidget* command_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(command_label), _("<b>Command:</b>"));
    gtk_misc_set_alignment(GTK_MISC(command_label), 1.0, 0.50);
    gtk_label_set_width_chars(GTK_LABEL(command_label), 10);
    gtk_box_pack_start(GTK_BOX(command_hbox), command_label, TRUE, TRUE, 0);
    GtkWidget* command_entry = gtk_entry_new();
    gtk_widget_set_tooltip_text(command_entry,
                                _("\"%s\" will be replaced with the clipboard contents"));
    
    gtk_box_pack_end(GTK_BOX(command_hbox), command_entry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(edit_dialog)->vbox), command_hbox, FALSE, FALSE, 0);
    
    /* Set the values */
    gchar* name;
    gchar* command;
    gtk_tree_model_get(GTK_TREE_MODEL(actions_list), &sel_iter, 0, &name, 1, &command, -1);
    gtk_entry_set_text(GTK_ENTRY(name_entry), name);
    gtk_entry_set_text(GTK_ENTRY(command_entry), command);
    g_free(name);
    g_free(command);
    
    /* Run the dialog */
    gtk_widget_show_all(edit_dialog);
    if (gtk_dialog_run(GTK_DIALOG(edit_dialog)) == GTK_RESPONSE_ACCEPT)
    {
      /* Apply changes */
      gtk_list_store_set(actions_list, &sel_iter, 0,
                         gtk_entry_get_text(GTK_ENTRY(name_entry)), 1,
                         gtk_entry_get_text(GTK_ENTRY(command_entry)), -1);
    }
    gtk_widget_destroy(edit_dialog);
  }
}

/* Called when Delete button is clicked */
static void
on_delete_clicked(GtkButton *button, gpointer user_data)
{
  GtkTreeIter sel_iter;
  /* Check if selected */
  if (gtk_tree_selection_get_selected(actions_selection, NULL, &sel_iter))
  {
    /* Delete selected and select next */
    GtkTreePath* tree_path = gtk_tree_model_get_path(GTK_TREE_MODEL(actions_list), &sel_iter);
    gtk_list_store_remove(actions_list, &sel_iter);
    gtk_tree_selection_select_path(actions_selection, tree_path);
    /* Select previous if the last row was deleted */
    if (!gtk_tree_selection_path_is_selected(actions_selection, tree_path))
    {
      if (gtk_tree_path_prev(tree_path))
        gtk_tree_selection_select_path(actions_selection, tree_path);
    }
    gtk_tree_path_free(tree_path);
  }
}

/* Called when Up button is clicked */
static void
on_up_clicked(GtkButton *button, gpointer user_data)
{
  GtkTreeIter sel_iter;
  /* Check if selected */
  if (gtk_tree_selection_get_selected(actions_selection, NULL, &sel_iter))
  {
    /* Create path to previous row */
    GtkTreePath* tree_path = gtk_tree_model_get_path(GTK_TREE_MODEL(actions_list), &sel_iter);
    /* Check if previous row exists */
    if (gtk_tree_path_prev(tree_path))
    {
      /* Swap rows */
      GtkTreeIter prev_iter;
      gtk_tree_model_get_iter(GTK_TREE_MODEL(actions_list), &prev_iter, tree_path);
      gtk_list_store_swap(actions_list, &sel_iter, &prev_iter);
    }
    gtk_tree_path_free(tree_path);
  }
}

/* Called when Down button is clicked */
static void
on_down_clicked(GtkButton *button, gpointer user_data)
{
  GtkTreeIter sel_iter;
  /* Check if selected */
  if (gtk_tree_selection_get_selected(actions_selection, NULL, &sel_iter))
  {
    /* Create iter to next row */
    GtkTreeIter next_iter = sel_iter;
    /* Check if next row exists */
    if (gtk_tree_model_iter_next(GTK_TREE_MODEL(actions_list), &next_iter))
      /* Swap rows */
      gtk_list_store_swap(actions_list, &sel_iter, &next_iter);
  }
}

/* Shows the preferences dialog on the given tab */
void
show_preferences(gint tab)
{
  /* Declare some variables */
  GtkWidget *frame,     *label,
            *alignment, *hbox,
            *vbox;
  
  GtkObject *adjustment;
  GtkTreeViewColumn *tree_column;
  
  /* Create the dialog */
  preferences_dialog = gtk_dialog_new_with_buttons(_("Preferences"),    NULL,
                                                    (GTK_DIALOG_MODAL + GTK_DIALOG_NO_SEPARATOR),
                                                    GTK_STOCK_CANCEL,   GTK_RESPONSE_REJECT,
                                                    GTK_STOCK_OK,       GTK_RESPONSE_ACCEPT, NULL);
  
  gtk_window_set_icon(GTK_WINDOW(preferences_dialog),
                      gtk_widget_render_icon(preferences_dialog, GTK_STOCK_PREFERENCES,
                                             GTK_ICON_SIZE_MENU, NULL));
  
  gtk_window_set_resizable(GTK_WINDOW(preferences_dialog), FALSE);
  
  /* Create notebook */
  GtkWidget* notebook = gtk_notebook_new();
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(preferences_dialog)->vbox), notebook, TRUE, TRUE, 2);
  
  /* Build the general page */  
  GtkWidget* page_general = gtk_alignment_new(0.50, 0.50, 1.0, 1.0);
  gtk_alignment_set_padding(GTK_ALIGNMENT(page_general), 6, 6, 6, 6);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page_general, gtk_label_new(_("General")));
  GtkWidget* vbox_general = gtk_vbox_new(FALSE, 6);
  gtk_container_add(GTK_CONTAINER(page_general), vbox_general);
  
  /* Build the history frame */
  frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
  label = gtk_label_new(NULL);
  gtk_label_set_markup(GTK_LABEL(label), _("<b>History</b>"));
  gtk_frame_set_label_widget(GTK_FRAME(frame), label);
  alignment = gtk_alignment_new(0.50, 0.50, 1.0, 1.0);
  gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 0, 12, 0);
  gtk_container_add(GTK_CONTAINER(frame), alignment);
  hbox = gtk_hbox_new(FALSE, 6);
  gtk_container_add(GTK_CONTAINER(alignment), hbox);
  label = gtk_label_new(_("Amount of items to keep in history:"));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.50);
  gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);
  adjustment = gtk_adjustment_new(25, 5, 100, 1, 10, 0);
  history_spin = gtk_spin_button_new(GTK_ADJUSTMENT(adjustment), 0.0, 0);
  gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(history_spin), GTK_UPDATE_IF_VALID);
  gtk_box_pack_end(GTK_BOX(hbox), history_spin, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox_general), frame, FALSE, FALSE, 0);
  
  /* Build the display frame */
  frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
  label = gtk_label_new(NULL);
  gtk_label_set_markup(GTK_LABEL(label), _("<b>Display</b>"));
  gtk_frame_set_label_widget(GTK_FRAME(frame), label);
  alignment = gtk_alignment_new(0.50, 0.50, 1.0, 1.0);
  gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 0, 12, 0);
  gtk_container_add(GTK_CONTAINER(frame), alignment);
  vbox = gtk_vbox_new(FALSE, 2);
  gtk_container_add(GTK_CONTAINER(alignment), vbox);
  hbox = gtk_hbox_new(FALSE, 6);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  label = gtk_label_new(_("Length of each history item:"));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.50);
  gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);
  adjustment = gtk_adjustment_new(50, 25, 75, 1, 5, 0);
  charlength_spin = gtk_spin_button_new(GTK_ADJUSTMENT(adjustment), 0.0, 0);
  gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(charlength_spin), GTK_UPDATE_IF_VALID);
  gtk_box_pack_end(GTK_BOX(hbox), charlength_spin, FALSE, FALSE, 0);
  hbox = gtk_hbox_new(FALSE, 20);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  label = gtk_label_new(_("Long items omitted in the:"));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.50);
  gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);
  ellipsize_combo = gtk_combo_box_new_text();
  gtk_combo_box_append_text(GTK_COMBO_BOX(ellipsize_combo), _("Beginning"));
  gtk_combo_box_append_text(GTK_COMBO_BOX(ellipsize_combo), _("Middle"));
  gtk_combo_box_append_text(GTK_COMBO_BOX(ellipsize_combo), _("End"));
  gtk_box_pack_end(GTK_BOX(hbox), ellipsize_combo, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox_general), frame, FALSE, FALSE, 0);
  
  /* Build the hotkeys frame */
  frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
  label = gtk_label_new(NULL);
  gtk_label_set_markup(GTK_LABEL(label), _("<b>Hotkeys</b>"));
  gtk_frame_set_label_widget(GTK_FRAME(frame), label);
  alignment = gtk_alignment_new(0.50, 0.50, 1.0, 1.0);
  gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 0, 12, 0);
  gtk_container_add(GTK_CONTAINER(frame), alignment);
  vbox = gtk_vbox_new(FALSE, 2);
  gtk_container_add(GTK_CONTAINER(alignment), vbox);
  hbox = gtk_hbox_new(TRUE, 2);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  label = gtk_label_new(_("History menu hotkey:"));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.50);
  gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);
  history_key_entry = gtk_entry_new();
  gtk_entry_set_width_chars(GTK_ENTRY(history_key_entry), 10);
  gtk_box_pack_end(GTK_BOX(hbox), history_key_entry, TRUE, TRUE, 0);
  hbox = gtk_hbox_new(TRUE, 2);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  label = gtk_label_new(_("Actions menu hotkey:"));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.50);
  gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);
  actions_key_entry = gtk_entry_new();
  gtk_entry_set_width_chars(GTK_ENTRY(actions_key_entry), 10);
  gtk_box_pack_end(GTK_BOX(hbox), actions_key_entry, TRUE, TRUE, 0);
  hbox = gtk_hbox_new(TRUE, 2);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  label = gtk_label_new(_("Parcellite menu hotkey:"));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.50);
  gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);
  menu_key_entry = gtk_entry_new();
  gtk_entry_set_width_chars(GTK_ENTRY(menu_key_entry), 10);
  gtk_box_pack_end(GTK_BOX(hbox), menu_key_entry, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(vbox_general), frame, FALSE, FALSE, 0);
  
  /* Build the behaviour frame */
  frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
  label = gtk_label_new(NULL);
  gtk_label_set_markup(GTK_LABEL(label), _("<b>Behaviour</b>"));
  gtk_frame_set_label_widget(GTK_FRAME(frame), label);
  alignment = gtk_alignment_new(0.50, 0.50, 1.0, 1.0);
  gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 0, 12, 0);
  gtk_container_add(GTK_CONTAINER(frame), alignment);
  vbox = gtk_vbox_new(FALSE, 2);
  gtk_container_add(GTK_CONTAINER(alignment), vbox);
  save_check = gtk_check_button_new_with_mnemonic(_("_Save history"));
  gtk_widget_set_tooltip_text(save_check, _("Keep and restore history in between sessions"));
  gtk_box_pack_start(GTK_BOX(vbox), save_check, FALSE, FALSE, 0);
  confirm_check = gtk_check_button_new_with_mnemonic(_("C_onfirm clear"));
  gtk_widget_set_tooltip_text(confirm_check, _("Confirm before clearing history"));
  gtk_box_pack_start(GTK_BOX(vbox), confirm_check, FALSE, FALSE, 0);
  reverse_check = gtk_check_button_new_with_mnemonic(_("_Reverse history"));
  gtk_widget_set_tooltip_text(reverse_check, _("Show history items in reverse order"));
  gtk_box_pack_start(GTK_BOX(vbox), reverse_check, FALSE, FALSE, 0);
  linemode_check = gtk_check_button_new_with_mnemonic(_("Single _line mode"));
  gtk_widget_set_tooltip_text(linemode_check, _("Show items in a single line"));
  gtk_box_pack_start(GTK_BOX(vbox), linemode_check, FALSE, FALSE, 0);
  hyperlinks_check = gtk_check_button_new_with_mnemonic(_("_Capture hyperlinks only"));
  gtk_widget_set_tooltip_text(hyperlinks_check, _("Ignore all non-hyperlink text"));
  gtk_box_pack_start(GTK_BOX(vbox), hyperlinks_check, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox_general), frame, FALSE, FALSE, 0);
  
  /* Build the actions page */
  GtkWidget* page_actions = gtk_alignment_new(0.50, 0.50, 1.0, 1.0);
  gtk_alignment_set_padding(GTK_ALIGNMENT(page_actions), 6, 6, 6, 6);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page_actions, gtk_label_new(_("Actions")));
  GtkWidget* vbox_actions = gtk_vbox_new(FALSE, 6);
  gtk_container_add(GTK_CONTAINER(page_actions), vbox_actions);
  
  /* Build the actions label */
  label = gtk_label_new(_("Control-click Parcellite\'s tray icon to use actions"));
  gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.50);
  gtk_box_pack_start(GTK_BOX(vbox_actions), label, FALSE, FALSE, 0);
  
  /* Build the actions treeview */
  GtkWidget* scrolled_window = gtk_scrolled_window_new(
                               GTK_ADJUSTMENT(gtk_adjustment_new(0, 0, 0, 0, 0, 0)),
                               GTK_ADJUSTMENT(gtk_adjustment_new(0, 0, 0, 0, 0, 0)));
  
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window),
                                      GTK_SHADOW_ETCHED_OUT);
  
  GtkWidget* treeview = gtk_tree_view_new();
  gtk_tree_view_set_reorderable(GTK_TREE_VIEW(treeview), TRUE);
  gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(treeview), TRUE);
  actions_list = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
  gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), GTK_TREE_MODEL(actions_list));
  tree_column = gtk_tree_view_column_new_with_attributes(_("Action"),
                                                         gtk_cell_renderer_text_new(),
                                                         "text", 0, NULL);
  
  gtk_tree_view_column_set_resizable(tree_column, TRUE);
  gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), tree_column);
  GtkCellRenderer* cell_renderer = gtk_cell_renderer_text_new();
  g_object_set(cell_renderer, "ellipsize-set", TRUE, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  tree_column = gtk_tree_view_column_new_with_attributes(_("Command"), cell_renderer,
                                                         "text", 1, NULL);
  
  gtk_tree_view_column_set_expand(tree_column, TRUE);
  gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), tree_column);
  gtk_container_add(GTK_CONTAINER(scrolled_window), treeview);
  actions_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
  gtk_tree_selection_set_mode(actions_selection, GTK_SELECTION_BROWSE);
  gtk_box_pack_start(GTK_BOX(vbox_actions), scrolled_window, TRUE, TRUE, 0);
  
  /* Build the buttons */
  GtkWidget* hbutton_box = gtk_hbutton_box_new();
  gtk_button_box_set_layout(GTK_BUTTON_BOX(hbutton_box), GTK_BUTTONBOX_SPREAD);
  hbox = gtk_hbox_new(TRUE, 6);
  gtk_box_pack_start(GTK_BOX(hbutton_box), hbox, TRUE, TRUE, 0);
  GtkWidget* button_new = gtk_button_new_with_label(_("New"));
  gtk_button_set_image(GTK_BUTTON(button_new),
                       gtk_image_new_from_stock(GTK_STOCK_NEW, GTK_ICON_SIZE_MENU));
  
  gtk_widget_set_tooltip_text(button_new, _("New action"));
  g_signal_connect(G_OBJECT(button_new), "clicked", G_CALLBACK(on_new_clicked), NULL);
  gtk_box_pack_start(GTK_BOX(hbox), button_new, FALSE, TRUE, 0);
  GtkWidget* button_edit = gtk_button_new_with_label(_("Edit"));
  gtk_button_set_image(GTK_BUTTON(button_edit),
                       gtk_image_new_from_stock(GTK_STOCK_EDIT, GTK_ICON_SIZE_MENU));
  
  gtk_widget_set_tooltip_text(button_edit, _("Edit selected"));
  g_signal_connect(G_OBJECT(button_edit), "clicked", G_CALLBACK(on_edit_clicked), NULL);
  gtk_box_pack_start(GTK_BOX(hbox), button_edit, FALSE, TRUE, 0);
  GtkWidget* button_delete = gtk_button_new_with_label(_("Delete"));
  gtk_button_set_image(GTK_BUTTON(button_delete),
                       gtk_image_new_from_stock(GTK_STOCK_DELETE, GTK_ICON_SIZE_MENU));
  
  gtk_widget_set_tooltip_text(button_delete, _("Delete selected"));
  g_signal_connect(G_OBJECT(button_delete), "clicked", G_CALLBACK(on_delete_clicked), NULL);
  gtk_box_pack_start(GTK_BOX(hbox), button_delete, FALSE, TRUE, 0);
  GtkWidget* button_up = gtk_button_new();
  gtk_button_set_image(GTK_BUTTON(button_up),
                       gtk_image_new_from_stock(GTK_STOCK_GO_UP, GTK_ICON_SIZE_MENU));
  
  gtk_widget_set_tooltip_text(button_up, _("Move selected up"));
  g_signal_connect(G_OBJECT(button_up), "clicked", G_CALLBACK(on_up_clicked), NULL);
  gtk_box_pack_start(GTK_BOX(hbox), button_up, FALSE, TRUE, 0);
  GtkWidget* button_down = gtk_button_new();
  gtk_button_set_image(GTK_BUTTON(button_down),
                       gtk_image_new_from_stock(GTK_STOCK_GO_DOWN, GTK_ICON_SIZE_MENU));
  
  gtk_widget_set_tooltip_text(button_down, _("Move selected down"));
  g_signal_connect(G_OBJECT(button_down), "clicked", G_CALLBACK(on_down_clicked), NULL);
  gtk_box_pack_start(GTK_BOX(hbox), button_down, FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(vbox_actions), hbutton_box, FALSE, FALSE, 0);
  
  /* Make widgets reflect current preferences */
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(history_spin), (gdouble)prefs.histlim);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(charlength_spin), (gdouble)prefs.charlength);
  gtk_combo_box_set_active(GTK_COMBO_BOX(ellipsize_combo), prefs.ellipsize - 1);
  gtk_entry_set_text(GTK_ENTRY(history_key_entry), prefs.histkey);
  gtk_entry_set_text(GTK_ENTRY(actions_key_entry), prefs.actionkey);
  gtk_entry_set_text(GTK_ENTRY(menu_key_entry), prefs.menukey);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(save_check), prefs.savehist);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(confirm_check), prefs.confclear);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(reverse_check), prefs.revhist);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(linemode_check), prefs.singleline);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hyperlinks_check), prefs.hyperlinks);
  
  /* Read actions */
  read_actions();
  
  /* Run the dialog */
  gtk_widget_show_all(preferences_dialog);
  gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), tab);
  if (gtk_dialog_run(GTK_DIALOG(preferences_dialog)) == GTK_RESPONSE_ACCEPT)
  {
    /* Apply and save preferences */
    apply_preferences();
    save_preferences();
    save_actions();
  }
  gtk_widget_destroy(preferences_dialog);
}
