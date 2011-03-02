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
GtkWidget *copy_check,
          *primary_check,
          *synchronize_check,
          *history_spin,
          *charlength_spin,
          *ellipsize_combo,
          *history_key_entry,
          *actions_key_entry,
          *menu_key_entry,
          *save_check,
          *confirm_check,
          *reverse_check,
          *linemode_check,
          *hyperlinks_check,
          *history_pos,
          *history_x,
          *history_y;

GtkListStore* actions_list;
GtkTreeSelection* actions_selection;

/* Apply the new preferences */
static void
apply_preferences()
{
  /* Unbind the keys before binding new ones */
  keybinder_unbind(prefs.history_key, history_hotkey);
  g_free(prefs.history_key);
  prefs.history_key = NULL;
  keybinder_unbind(prefs.actions_key, actions_hotkey);
  g_free(prefs.actions_key);
  prefs.actions_key = NULL;
  keybinder_unbind(prefs.menu_key, menu_hotkey);
  g_free(prefs.menu_key);
  prefs.menu_key = NULL;
  
  /* Get the new preferences */
  prefs.use_copy = gtk_toggle_button_get_active((GtkToggleButton*)copy_check);
  prefs.use_primary = gtk_toggle_button_get_active((GtkToggleButton*)primary_check);
  prefs.synchronize = gtk_toggle_button_get_active((GtkToggleButton*)synchronize_check);
  prefs.save_history = gtk_toggle_button_get_active((GtkToggleButton*)save_check);
  prefs.history_limit = gtk_spin_button_get_value_as_int((GtkSpinButton*)history_spin);
	prefs.history_pos = gtk_toggle_button_get_active((GtkToggleButton*)history_pos);
	prefs.history_x=gtk_spin_button_get_value_as_int((GtkSpinButton*)history_x);
	prefs.history_y=gtk_spin_button_get_value_as_int((GtkSpinButton*)history_y);
  prefs.hyperlinks_only = gtk_toggle_button_get_active((GtkToggleButton*)hyperlinks_check);
  prefs.confirm_clear = gtk_toggle_button_get_active((GtkToggleButton*)confirm_check);
  prefs.single_line = gtk_toggle_button_get_active((GtkToggleButton*)linemode_check);
  prefs.reverse_history = gtk_toggle_button_get_active((GtkToggleButton*)reverse_check);
  prefs.item_length = gtk_spin_button_get_value_as_int((GtkSpinButton*)charlength_spin);
  prefs.ellipsize = gtk_combo_box_get_active((GtkComboBox*)ellipsize_combo) + 1;
  prefs.history_key = g_strdup(gtk_entry_get_text((GtkEntry*)history_key_entry));
  prefs.actions_key = g_strdup(gtk_entry_get_text((GtkEntry*)actions_key_entry));
  prefs.menu_key = g_strdup(gtk_entry_get_text((GtkEntry*)menu_key_entry));
  
  /* Bind keys and apply the new history limit */
  keybinder_bind(prefs.history_key, history_hotkey, NULL);
  keybinder_bind(prefs.actions_key, actions_hotkey, NULL);
  keybinder_bind(prefs.menu_key, menu_hotkey, NULL);
  truncate_history();
}

/* Save preferences to ~/.config/parcellite/parcelliterc */
static void
save_preferences()
{
  /* Create key */
  GKeyFile* rc_key = g_key_file_new();
  
  /* Add values */
  g_key_file_set_boolean(rc_key, "rc", "use_copy", prefs.use_copy);
  g_key_file_set_boolean(rc_key, "rc", "use_primary", prefs.use_primary);
  g_key_file_set_boolean(rc_key, "rc", "synchronize", prefs.synchronize);
  g_key_file_set_boolean(rc_key, "rc", "save_history", prefs.save_history);
  g_key_file_set_integer(rc_key, "rc", "history_limit", prefs.history_limit);
  g_key_file_set_boolean(rc_key, "rc", "hyperlinks_only", prefs.hyperlinks_only);
  g_key_file_set_boolean(rc_key, "rc", "confirm_clear", prefs.confirm_clear);
  g_key_file_set_boolean(rc_key, "rc", "single_line", prefs.single_line);
  g_key_file_set_boolean(rc_key, "rc", "reverse_history", prefs.reverse_history);
  g_key_file_set_integer(rc_key, "rc", "item_length", prefs.item_length);
  g_key_file_set_integer(rc_key, "rc", "ellipsize", prefs.ellipsize);
  g_key_file_set_string(rc_key, "rc", "history_key", prefs.history_key);
  g_key_file_set_string(rc_key, "rc", "actions_key", prefs.actions_key);
  g_key_file_set_string(rc_key, "rc", "menu_key", prefs.menu_key);
	g_key_file_set_boolean(rc_key, "rc", "history_pos", prefs.history_pos);
	g_key_file_set_integer(rc_key, "rc", "history_x", prefs.history_x);
	g_key_file_set_integer(rc_key, "rc", "history_y", prefs.history_y);
  
  /* Check config and data directories */
  check_dirs();
  /* Save key to file */
  gchar* rc_file = g_build_filename(g_get_home_dir(), PREFERENCES_FILE, NULL);
  g_file_set_contents(rc_file, g_key_file_to_data(rc_key, NULL, NULL), -1, NULL);
  g_key_file_free(rc_key);
  g_free(rc_file);
}

/* Read ~/.config/parcellite/parcelliterc */
void read_preferences()
{
  gchar* rc_file = g_build_filename(g_get_home_dir(), PREFERENCES_FILE, NULL);
	gint x,y;
  /* Create key */
  GKeyFile* rc_key = g_key_file_new();
  if (g_key_file_load_from_file(rc_key, rc_file, G_KEY_FILE_NONE, NULL)) {
    /* Load values */
    prefs.use_copy = g_key_file_get_boolean(rc_key, "rc", "use_copy", NULL);
    prefs.use_primary = g_key_file_get_boolean(rc_key, "rc", "use_primary", NULL);
    prefs.synchronize = g_key_file_get_boolean(rc_key, "rc", "synchronize", NULL);
    prefs.save_history = g_key_file_get_boolean(rc_key, "rc", "save_history", NULL);
    prefs.history_limit = g_key_file_get_integer(rc_key, "rc", "history_limit", NULL);
    prefs.hyperlinks_only = g_key_file_get_boolean(rc_key, "rc", "hyperlinks_only", NULL);
    prefs.confirm_clear = g_key_file_get_boolean(rc_key, "rc", "confirm_clear", NULL);
    prefs.single_line = g_key_file_get_boolean(rc_key, "rc", "single_line", NULL);
    prefs.reverse_history = g_key_file_get_boolean(rc_key, "rc", "reverse_history", NULL);
    prefs.item_length = g_key_file_get_integer(rc_key, "rc", "item_length", NULL);
    prefs.ellipsize = g_key_file_get_integer(rc_key, "rc", "ellipsize", NULL);
    prefs.history_key = g_key_file_get_string(rc_key, "rc", "history_key", NULL);
    prefs.actions_key = g_key_file_get_string(rc_key, "rc", "actions_key", NULL);
    prefs.menu_key = g_key_file_get_string(rc_key, "rc", "menu_key", NULL);
		prefs.history_pos = g_key_file_get_boolean(rc_key, "rc", "history_pos", NULL);
	  prefs.history_x = g_key_file_get_integer(rc_key, "rc", "history_x", NULL);
	  prefs.history_y = g_key_file_get_integer(rc_key, "rc", "history_y", NULL);
    
    /* Check for errors and set default values if any */
    postition_history(NULL,&x,&y,NULL, (gpointer)1);
		if(prefs.history_x>x) prefs.history_x=x;
			if(prefs.history_y>y) prefs.history_y=y;
	
    if ((!prefs.history_limit) || (prefs.history_limit > 100) || (prefs.history_limit < 0))
      prefs.history_limit = DEF_HISTORY_LIMIT;
    if ((!prefs.item_length) || (prefs.item_length > 75) || (prefs.item_length < 0))
      prefs.item_length = DEF_ITEM_LENGTH;
    if ((!prefs.ellipsize) || (prefs.ellipsize > 3) || (prefs.ellipsize < 0))
      prefs.ellipsize = DEF_ELLIPSIZE;
    if (!prefs.history_key)
      prefs.history_key = g_strdup(DEF_HISTORY_KEY);
    if (!prefs.actions_key)
      prefs.actions_key = g_strdup(DEF_ACTIONS_KEY);
    if (!prefs.menu_key)
      prefs.menu_key = g_strdup(DEF_MENU_KEY);
  }
  else
  {
    /* Init default keys on error */
    prefs.history_key = g_strdup(DEF_HISTORY_KEY);
    prefs.actions_key = g_strdup(DEF_ACTIONS_KEY);
    prefs.menu_key = g_strdup(DEF_MENU_KEY);
  }
  g_key_file_free(rc_key);
  g_free(rc_file);
}

/* Read ~/.parcellite/actions into the treeview */
static void
read_actions()
{
  /* Open the file for reading */
  gchar* path = g_build_filename(g_get_home_dir(), ACTIONS_FILE, NULL);
  FILE* actions_file = fopen(path, "rb");
  g_free(path);
  /* Check that it opened and begin read */
  if (actions_file)
  {
    /* Keep a row reference */
    GtkTreeIter row_iter;
    /* Read the size of the first item */
    gint size;
    if(0 ==fread(&size, 4, 1, actions_file)) g_print("P1:0 Items read\n");
    /* Continue reading until size is 0 */
    while (size != 0)
    {
      /* Read name */
      gchar* name = (gchar*)g_malloc(size + 1);
      if(0 ==fread(name, size, 1, actions_file))  g_print("P1:0 Items read\n");
      name[size] = '\0';
      if(0 ==fread(&size, 4, 1, actions_file))  g_print("P1:0 Items read\n");
      /* Read command */
      gchar* command = (gchar*)g_malloc(size + 1);
      if(0 ==fread(command, size, 1, actions_file))  g_print("P1:0 Items read\n");
      command[size] = '\0';
      if(0 ==fread(&size, 4, 1, actions_file))  g_print("P1:0 Items read\n");
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
  gchar* path = g_build_filename(g_get_home_dir(), ACTIONS_FILE, NULL);
  FILE* actions_file = fopen(path, "wb");
  g_free(path);
  /* Check that it opened and begin write */
  if (actions_file)
  {
    GtkTreeIter action_iter;
    /* Get and check if there's a first iter */
    if (gtk_tree_model_get_iter_first((GtkTreeModel*)actions_list, &action_iter))
    {
      do
      {
        /* Get name and command */
        gchar *name, *command;
        gtk_tree_model_get((GtkTreeModel*)actions_list, &action_iter, 0, &name, 1, &command, -1);
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
      while(gtk_tree_model_iter_next((GtkTreeModel*)actions_list, &action_iter));
    }
    /* End of file write */
    gint end = 0;
    fwrite(&end, 4, 1, actions_file);
    fclose(actions_file);
  }
}

/* Called when clipboard checks are pressed */
static void
check_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
  if (gtk_toggle_button_get_active((GtkToggleButton*)copy_check) &&
      gtk_toggle_button_get_active((GtkToggleButton*)primary_check))
  {
    /* Only allow synchronize option if both primary and clipboard are enabled */
    gtk_widget_set_sensitive((GtkWidget*)synchronize_check, TRUE);
  }
  else
  {
    /* Disable synchronize option */
    gtk_toggle_button_set_active((GtkToggleButton*)synchronize_check, FALSE);
    gtk_widget_set_sensitive((GtkWidget*)synchronize_check, FALSE);

  }
}

/* Called when Add... button is clicked */
static void
add_action(GtkButton *button, gpointer user_data)
{
  /* Append new item */
  GtkTreeIter row_iter;
  gtk_list_store_append(actions_list, &row_iter);
  /* Add a %s to the command */
  gtk_list_store_set(actions_list, &row_iter, 1, "%s", -1);
  /* Set the first column to editing */
  GtkTreePath* row_path = gtk_tree_model_get_path((GtkTreeModel*)actions_list, &row_iter);
  GtkTreeView* treeview = gtk_tree_selection_get_tree_view(actions_selection);
  GtkTreeViewColumn* column = gtk_tree_view_get_column(treeview, 0);
  gtk_tree_view_set_cursor(treeview, row_path, column, TRUE);
  gtk_tree_path_free(row_path);
}

/* Called when Remove button is clicked */
static void
remove_action(GtkButton *button, gpointer user_data)
{
  GtkTreeIter sel_iter;
  /* Check if selected */
  if (gtk_tree_selection_get_selected(actions_selection, NULL, &sel_iter))
  {
    /* Delete selected and select next */
    GtkTreePath* tree_path = gtk_tree_model_get_path((GtkTreeModel*)actions_list, &sel_iter);
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
move_action_up(GtkButton *button, gpointer user_data)
{
  GtkTreeIter sel_iter;
  /* Check if selected */
  if (gtk_tree_selection_get_selected(actions_selection, NULL, &sel_iter))
  {
    /* Create path to previous row */
    GtkTreePath* tree_path = gtk_tree_model_get_path((GtkTreeModel*)actions_list, &sel_iter);
    /* Check if previous row exists */
    if (gtk_tree_path_prev(tree_path))
    {
      /* Swap rows */
      GtkTreeIter prev_iter;
      gtk_tree_model_get_iter((GtkTreeModel*)actions_list, &prev_iter, tree_path);
      gtk_list_store_swap(actions_list, &sel_iter, &prev_iter);
    }
    gtk_tree_path_free(tree_path);
  }
}

/* Called when Down button is clicked */
static void
move_action_down(GtkButton *button, gpointer user_data)
{
  GtkTreeIter sel_iter;
  /* Check if selected */
  if (gtk_tree_selection_get_selected(actions_selection, NULL, &sel_iter))
  {
    /* Create iter to next row */
    GtkTreeIter next_iter = sel_iter;
    /* Check if next row exists */
    if (gtk_tree_model_iter_next((GtkTreeModel*)actions_list, &next_iter))
      /* Swap rows */
      gtk_list_store_swap(actions_list, &sel_iter, &next_iter);
  }
}

/* Called when delete key is pressed */
static void
delete_key_pressed(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
  /* Check if DEL key was pressed (keyval: 65535) */
  if (event->keyval == 65535)
    remove_action(NULL, NULL);
}

/* Called when a cell is edited */
static void
edit_action(GtkCellRendererText *renderer, gchar *path,
            gchar *new_text,               gpointer cell)
{
  GtkTreeIter sel_iter;
  /* Check if selected */
  if (gtk_tree_selection_get_selected(actions_selection, NULL, &sel_iter))
  {
    /* Apply changes */
    gtk_list_store_set(actions_list, &sel_iter, (gint)cell, new_text, -1);
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
	gint x,y;
  GtkTreeViewColumn *tree_column;
  
  /* Create the dialog */
  GtkWidget* dialog = gtk_dialog_new_with_buttons(_("Preferences"),     NULL,
                                                   (GTK_DIALOG_MODAL  + GTK_DIALOG_NO_SEPARATOR),
                                                    GTK_STOCK_CANCEL,   GTK_RESPONSE_REJECT,
                                                    GTK_STOCK_OK,       GTK_RESPONSE_ACCEPT, NULL);
  
  gtk_window_set_icon((GtkWindow*)dialog, gtk_widget_render_icon(dialog, GTK_STOCK_PREFERENCES, GTK_ICON_SIZE_MENU, NULL));
  gtk_window_set_resizable((GtkWindow*)dialog, FALSE);
  
  /* Create notebook */
  GtkWidget* notebook = gtk_notebook_new();
#if GTK_CHECK_VERSION (2,14,0)
  gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area (GTK_DIALOG(dialog))), notebook, TRUE, TRUE, 2);
#else
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), notebook, TRUE, TRUE, 2);
#endif
  
  /* Build the behavior page */  
  GtkWidget* page_behavior = gtk_alignment_new(0.50, 0.50, 1.0, 1.0);
  gtk_alignment_set_padding((GtkAlignment*)page_behavior, 12, 6, 12, 6);
  gtk_notebook_append_page((GtkNotebook*)notebook, page_behavior, gtk_label_new(_("Behavior")));
  GtkWidget* vbox_behavior = gtk_vbox_new(FALSE, 12);
  gtk_container_add((GtkContainer*)page_behavior, vbox_behavior);
  
  /* Build the clipboards frame */
  frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type((GtkFrame*)frame, GTK_SHADOW_NONE);
  label = gtk_label_new(NULL);
  gtk_label_set_markup((GtkLabel*)label, _("<b>Clipboards</b>"));
  gtk_frame_set_label_widget((GtkFrame*)frame, label);
  alignment = gtk_alignment_new(0.50, 0.50, 1.0, 1.0);
  gtk_alignment_set_padding((GtkAlignment*)alignment, 12, 0, 12, 0);
  gtk_container_add((GtkContainer*)frame, alignment);
  vbox = gtk_vbox_new(FALSE, 2);
  gtk_container_add((GtkContainer*)alignment, vbox);
  copy_check = gtk_check_button_new_with_mnemonic(_("Use _Copy (Ctrl-C)"));
  g_signal_connect((GObject*)copy_check, "toggled", (GCallback)check_toggled, NULL);
  gtk_box_pack_start((GtkBox*)vbox, copy_check, FALSE, FALSE, 0);
  primary_check = gtk_check_button_new_with_mnemonic(_("Use _Primary (Selection)"));
  g_signal_connect((GObject*)primary_check, "toggled", (GCallback)check_toggled, NULL);
  gtk_box_pack_start((GtkBox*)vbox, primary_check, FALSE, FALSE, 0);
  synchronize_check = gtk_check_button_new_with_mnemonic(_("S_ynchronize clipboards"));
  gtk_box_pack_start((GtkBox*)vbox, synchronize_check, FALSE, FALSE, 0);
  gtk_box_pack_start((GtkBox*)vbox_behavior, frame, FALSE, FALSE, 0);
  
  /* Build the history frame */
  frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type((GtkFrame*)frame, GTK_SHADOW_NONE);
  label = gtk_label_new(NULL);
  gtk_label_set_markup((GtkLabel*)label, _("<b>History</b>"));
  gtk_frame_set_label_widget((GtkFrame*)frame, label);
  alignment = gtk_alignment_new(0.50, 0.50, 1.0, 1.0);
  gtk_alignment_set_padding((GtkAlignment*)alignment, 12, 0, 12, 0);
  gtk_container_add((GtkContainer*)frame, alignment);
  vbox = gtk_vbox_new(FALSE, 2);
  gtk_container_add((GtkContainer*)alignment, vbox);
  save_check = gtk_check_button_new_with_mnemonic(_("_Save history"));
  gtk_widget_set_tooltip_text(save_check, _("Save and restore history between sessions"));
  gtk_box_pack_start((GtkBox*)vbox, save_check, FALSE, FALSE, 0);
	/**set the history position  */
	hbox = gtk_hbox_new(FALSE, 4);
  gtk_box_pack_start((GtkBox*)vbox, hbox, FALSE, FALSE, 0);
	history_pos = gtk_check_button_new_with_mnemonic(_("_Postion history"));
  gtk_widget_set_tooltip_text(history_pos, _("Set the location where history appears"));
  gtk_box_pack_start((GtkBox*)hbox, history_pos, FALSE, FALSE, 0);
	postition_history(NULL,&x,&y,NULL, (gpointer)1);
	label = gtk_label_new("X");
	gtk_box_pack_start((GtkBox*)hbox, label, FALSE, FALSE, 0);
	history_x=gtk_spin_button_new((GtkAdjustment*)gtk_adjustment_new (prefs.history_x,1,x,10,100,0 ),10,0); 
	gtk_box_pack_start((GtkBox*)hbox, history_x, FALSE, FALSE, 0);
	label = gtk_label_new("Y");
	gtk_box_pack_start((GtkBox*)hbox, label, FALSE, FALSE, 0);
	history_y=gtk_spin_button_new((GtkAdjustment*)gtk_adjustment_new (prefs.history_y,1,y,10,100,0 ),10,0); 
	gtk_box_pack_start((GtkBox*)hbox, history_y, FALSE, FALSE, 0);
	gtk_spin_button_set_update_policy((GtkSpinButton*)history_x, GTK_UPDATE_IF_VALID);
	gtk_spin_button_set_update_policy((GtkSpinButton*)history_y, GTK_UPDATE_IF_VALID);
	/* Set items in history  */
  hbox = gtk_hbox_new(FALSE, 4);
  gtk_box_pack_start((GtkBox*)vbox, hbox, FALSE, FALSE, 0);
  label = gtk_label_new(_("Items in history:"));
  gtk_misc_set_alignment((GtkMisc*)label, 0.0, 0.50);
  gtk_box_pack_start((GtkBox*)hbox, label, FALSE, FALSE, 0);
  adjustment = gtk_adjustment_new(25, 5, 100, 1, 10, 0);
  history_spin = gtk_spin_button_new((GtkAdjustment*)adjustment, 0.0, 0);
  gtk_spin_button_set_update_policy((GtkSpinButton*)history_spin, GTK_UPDATE_IF_VALID);
  gtk_box_pack_start((GtkBox*)hbox, history_spin, FALSE, FALSE, 0);
  gtk_box_pack_start((GtkBox*)vbox_behavior, frame, FALSE, FALSE, 0);
  
  /* Build the miscellaneous frame */
  frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type((GtkFrame*)frame, GTK_SHADOW_NONE);
  label = gtk_label_new(NULL);
  gtk_label_set_markup((GtkLabel*)label, _("<b>Miscellaneous</b>"));
  gtk_frame_set_label_widget((GtkFrame*)frame, label);
  alignment = gtk_alignment_new(0.50, 0.50, 1.0, 1.0);
  gtk_alignment_set_padding((GtkAlignment*)alignment, 12, 0, 12, 0);
  gtk_container_add((GtkContainer*)frame, alignment);
  vbox = gtk_vbox_new(FALSE, 2);
  gtk_container_add((GtkContainer*)alignment, vbox);
  hyperlinks_check = gtk_check_button_new_with_mnemonic(_("Capture _hyperlinks only"));
  gtk_box_pack_start((GtkBox*)vbox, hyperlinks_check, FALSE, FALSE, 0);
  confirm_check = gtk_check_button_new_with_mnemonic(_("C_onfirm before clearing history"));
  gtk_box_pack_start((GtkBox*)vbox, confirm_check, FALSE, FALSE, 0);
  gtk_box_pack_start((GtkBox*)vbox_behavior, frame, FALSE, FALSE, 0);
  
  /* Build the display page */
  GtkWidget* page_display = gtk_alignment_new(0.50, 0.50, 1.0, 1.0);
  gtk_alignment_set_padding((GtkAlignment*)page_display, 12, 6, 12, 6);
  gtk_notebook_append_page((GtkNotebook*)notebook, page_display, gtk_label_new(_("Display")));
  GtkWidget* vbox_display = gtk_vbox_new(FALSE, 12);
  gtk_container_add((GtkContainer*)page_display, vbox_display);
  
  /* Build the items frame */
  frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type((GtkFrame*)frame, GTK_SHADOW_NONE);
  label = gtk_label_new(NULL);
  gtk_label_set_markup((GtkLabel*)label, _("<b>Items</b>"));
  gtk_frame_set_label_widget((GtkFrame*)frame, label);
  alignment = gtk_alignment_new(0.50, 0.50, 1.0, 1.0);
  gtk_alignment_set_padding((GtkAlignment*)alignment, 12, 0, 12, 0);
  gtk_container_add((GtkContainer*)frame, alignment);
  vbox = gtk_vbox_new(FALSE, 2);
  gtk_container_add((GtkContainer*)alignment, vbox);
  linemode_check = gtk_check_button_new_with_mnemonic(_("Show in a _single line"));
  gtk_box_pack_start((GtkBox*)vbox, linemode_check, FALSE, FALSE, 0);
  reverse_check = gtk_check_button_new_with_mnemonic(_("Show in _reverse order"));
  gtk_box_pack_start((GtkBox*)vbox, reverse_check, FALSE, FALSE, 0);
  hbox = gtk_hbox_new(FALSE, 4);
  gtk_box_pack_start((GtkBox*)vbox, hbox, FALSE, FALSE, 0);
  label = gtk_label_new(_("Character length of items:"));
  gtk_misc_set_alignment((GtkMisc*)label, 0.0, 0.50);
  gtk_box_pack_start((GtkBox*)hbox, label, FALSE, FALSE, 0);
  adjustment = gtk_adjustment_new(50, 25, 75, 1, 5, 0);
  charlength_spin = gtk_spin_button_new((GtkAdjustment*)adjustment, 0.0, 0);
  gtk_spin_button_set_update_policy((GtkSpinButton*)charlength_spin, GTK_UPDATE_IF_VALID);
  gtk_box_pack_start((GtkBox*)hbox, charlength_spin, FALSE, FALSE, 0);
  gtk_box_pack_start((GtkBox*)vbox_display, frame, FALSE, FALSE, 0);
  
  /* Build the omitting frame */
  frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type((GtkFrame*)frame, GTK_SHADOW_NONE);
  label = gtk_label_new(NULL);
  gtk_label_set_markup((GtkLabel*)label, _("<b>Omitting</b>"));
  gtk_frame_set_label_widget((GtkFrame*)frame, label);
  alignment = gtk_alignment_new(0.50, 0.50, 1.0, 1.0);
  gtk_alignment_set_padding((GtkAlignment*)alignment, 12, 0, 12, 0);
  gtk_container_add((GtkContainer*)frame, alignment);
  vbox = gtk_vbox_new(FALSE, 2);
  gtk_container_add((GtkContainer*)alignment, vbox);
  hbox = gtk_hbox_new(FALSE, 4);
  gtk_box_pack_start((GtkBox*)vbox, hbox, FALSE, FALSE, 0);
  label = gtk_label_new(_("Omit items in the:"));
  gtk_misc_set_alignment((GtkMisc*)label, 0.0, 0.50);
  gtk_box_pack_start((GtkBox*)hbox, label, FALSE, FALSE, 0);
  ellipsize_combo = gtk_combo_box_new_text();
  gtk_combo_box_append_text((GtkComboBox*)ellipsize_combo, _("Beginning"));
  gtk_combo_box_append_text((GtkComboBox*)ellipsize_combo, _("Middle"));
  gtk_combo_box_append_text((GtkComboBox*)ellipsize_combo, _("End"));
  gtk_box_pack_start((GtkBox*)hbox, ellipsize_combo, FALSE, FALSE, 0);
  gtk_box_pack_start((GtkBox*)vbox_display, frame, FALSE, FALSE, 0);
  
  /* Build the actions page */
  GtkWidget* page_actions = gtk_alignment_new(0.50, 0.50, 1.0, 1.0);
  gtk_alignment_set_padding((GtkAlignment*)page_actions, 6, 6, 6, 6);
  gtk_notebook_append_page((GtkNotebook*)notebook, page_actions, gtk_label_new(_("Actions")));
  GtkWidget* vbox_actions = gtk_vbox_new(FALSE, 6);
  gtk_container_add((GtkContainer*)page_actions, vbox_actions);
  
  /* Build the actions label */
  label = gtk_label_new(_("Control-click Parcellite\'s tray icon to use actions"));
  gtk_label_set_line_wrap((GtkLabel*)label, TRUE);
  gtk_misc_set_alignment((GtkMisc*)label, 0.0, 0.50);
  gtk_box_pack_start((GtkBox*)vbox_actions, label, FALSE, FALSE, 0);
  
  /* Build the actions treeview */
  GtkWidget* scrolled_window = gtk_scrolled_window_new(
                               (GtkAdjustment*)gtk_adjustment_new(0, 0, 0, 0, 0, 0),
                               (GtkAdjustment*)gtk_adjustment_new(0, 0, 0, 0, 0, 0));
  
  gtk_scrolled_window_set_policy((GtkScrolledWindow*)scrolled_window, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type((GtkScrolledWindow*)scrolled_window, GTK_SHADOW_ETCHED_OUT);
  GtkWidget* treeview = gtk_tree_view_new();
  gtk_tree_view_set_reorderable((GtkTreeView*)treeview, TRUE);
  gtk_tree_view_set_rules_hint((GtkTreeView*)treeview, TRUE);
  actions_list = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
  gtk_tree_view_set_model((GtkTreeView*)treeview, (GtkTreeModel*)actions_list);
  GtkCellRenderer* name_renderer = gtk_cell_renderer_text_new();
  g_object_set(name_renderer, "editable", TRUE, NULL);
  g_signal_connect((GObject*)name_renderer, "edited", (GCallback)edit_action, (gpointer)0);
  tree_column = gtk_tree_view_column_new_with_attributes(_("Action"), name_renderer, "text", 0, NULL);
  gtk_tree_view_column_set_resizable(tree_column, TRUE);
  gtk_tree_view_append_column((GtkTreeView*)treeview, tree_column);
  GtkCellRenderer* command_renderer = gtk_cell_renderer_text_new();
  g_object_set(command_renderer, "editable", TRUE, NULL);
  g_object_set(command_renderer, "ellipsize-set", TRUE, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  g_signal_connect((GObject*)command_renderer, "edited", (GCallback)edit_action, (gpointer)1);
  tree_column = gtk_tree_view_column_new_with_attributes(_("Command"), command_renderer, "text", 1, NULL);
  gtk_tree_view_column_set_expand(tree_column, TRUE);
  gtk_tree_view_append_column((GtkTreeView*)treeview, tree_column);
  gtk_container_add((GtkContainer*)scrolled_window, treeview);
  gtk_box_pack_start((GtkBox*)vbox_actions, scrolled_window, TRUE, TRUE, 0);
  
  /* Edit selection and connect treeview related signals */
  actions_selection = gtk_tree_view_get_selection((GtkTreeView*)treeview);
  gtk_tree_selection_set_mode(actions_selection, GTK_SELECTION_BROWSE);
  g_signal_connect((GObject*)treeview, "key-press-event", (GCallback)delete_key_pressed, NULL);
  
  /* Build the buttons */
  GtkWidget* hbbox = gtk_hbutton_box_new();
  gtk_box_set_spacing((GtkBox*)hbbox, 6);
  gtk_button_box_set_layout((GtkButtonBox*)hbbox, GTK_BUTTONBOX_START);
  GtkWidget* add_button = gtk_button_new_with_label(_("Add..."));
  gtk_button_set_image((GtkButton*)add_button, gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_MENU));
  g_signal_connect((GObject*)add_button, "clicked", (GCallback)add_action, NULL);
  gtk_box_pack_start((GtkBox*)hbbox, add_button, FALSE, TRUE, 0);
  GtkWidget* remove_button = gtk_button_new_with_label(_("Remove"));
  gtk_button_set_image((GtkButton*)remove_button, gtk_image_new_from_stock(GTK_STOCK_REMOVE, GTK_ICON_SIZE_MENU));
  g_signal_connect((GObject*)remove_button, "clicked", (GCallback)remove_action, NULL);
  gtk_box_pack_start((GtkBox*)hbbox, remove_button, FALSE, TRUE, 0);
  GtkWidget* up_button = gtk_button_new();
  gtk_button_set_image((GtkButton*)up_button, gtk_image_new_from_stock(GTK_STOCK_GO_UP, GTK_ICON_SIZE_MENU));
  g_signal_connect((GObject*)up_button, "clicked", (GCallback)move_action_up, NULL);
  gtk_box_pack_start((GtkBox*)hbbox, up_button, FALSE, TRUE, 0);
  GtkWidget* down_button = gtk_button_new();
  gtk_button_set_image((GtkButton*)down_button, gtk_image_new_from_stock(GTK_STOCK_GO_DOWN, GTK_ICON_SIZE_MENU));
  g_signal_connect((GObject*)down_button, "clicked", (GCallback)move_action_down, NULL);
  gtk_box_pack_start((GtkBox*)hbbox, down_button, FALSE, TRUE, 0);
  gtk_box_pack_start((GtkBox*)vbox_actions, hbbox, FALSE, FALSE, 0);
  
  /* Build the hotkeys page */
  GtkWidget* page_extras = gtk_alignment_new(0.50, 0.50, 1.0, 1.0);
  gtk_alignment_set_padding((GtkAlignment*)page_extras, 12, 6, 12, 6);
  gtk_notebook_append_page((GtkNotebook*)notebook, page_extras, gtk_label_new(_("Hotkeys")));
  GtkWidget* vbox_extras = gtk_vbox_new(FALSE, 12);
  gtk_container_add((GtkContainer*)page_extras, vbox_extras);
  
  /* Build the hotkeys frame */
  frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type((GtkFrame*)frame, GTK_SHADOW_NONE);
  label = gtk_label_new(NULL);
  gtk_label_set_markup((GtkLabel*)label, _("<b>Hotkeys</b>"));
  gtk_frame_set_label_widget((GtkFrame*)frame, label);
  alignment = gtk_alignment_new(0.50, 0.50, 1.0, 1.0);
  gtk_alignment_set_padding((GtkAlignment*)alignment, 12, 0, 12, 0);
  gtk_container_add((GtkContainer*)frame, alignment);
  vbox = gtk_vbox_new(FALSE, 2);
  gtk_container_add((GtkContainer*)alignment, vbox);
  hbox = gtk_hbox_new(TRUE, 4);
  gtk_box_pack_start((GtkBox*)vbox, hbox, FALSE, FALSE, 0);
  label = gtk_label_new(_("History key combination:"));
  gtk_misc_set_alignment((GtkMisc*)label, 0.0, 0.50);
  gtk_box_pack_start((GtkBox*)hbox, label, TRUE, TRUE, 0);
  history_key_entry = gtk_entry_new();
  gtk_entry_set_width_chars((GtkEntry*)history_key_entry, 10);
  gtk_box_pack_end((GtkBox*)hbox, history_key_entry, TRUE, TRUE, 0);
  hbox = gtk_hbox_new(TRUE, 4);
  gtk_box_pack_start((GtkBox*)vbox, hbox, FALSE, FALSE, 0);
  label = gtk_label_new(_("Actions key combination:"));
  gtk_misc_set_alignment((GtkMisc*)label, 0.0, 0.50);
  gtk_box_pack_start((GtkBox*)hbox, label, TRUE, TRUE, 0);
  actions_key_entry = gtk_entry_new();
  gtk_entry_set_width_chars((GtkEntry*)actions_key_entry, 10);
  gtk_box_pack_end((GtkBox*)hbox, actions_key_entry, TRUE, TRUE, 0);
  hbox = gtk_hbox_new(TRUE, 4);
  gtk_box_pack_start((GtkBox*)vbox, hbox, FALSE, FALSE, 0);
  label = gtk_label_new(_("Menu key combination:"));
  gtk_misc_set_alignment((GtkMisc*)label, 0.0, 0.50);
  gtk_box_pack_start((GtkBox*)hbox, label, TRUE, TRUE, 0);
  menu_key_entry = gtk_entry_new();
  gtk_entry_set_width_chars((GtkEntry*)menu_key_entry, 10);
  gtk_box_pack_end((GtkBox*)hbox, menu_key_entry, TRUE, TRUE, 0);
  gtk_box_pack_start((GtkBox*)vbox_extras, frame, FALSE, FALSE, 0);
  
  /* Make widgets reflect current preferences */
  gtk_toggle_button_set_active((GtkToggleButton*)copy_check, prefs.use_copy);
  gtk_toggle_button_set_active((GtkToggleButton*)primary_check, prefs.use_primary);
  gtk_toggle_button_set_active((GtkToggleButton*)synchronize_check, prefs.synchronize);
  gtk_toggle_button_set_active((GtkToggleButton*)save_check, prefs.save_history);
  gtk_spin_button_set_value((GtkSpinButton*)history_spin, (gdouble)prefs.history_limit);
  gtk_toggle_button_set_active((GtkToggleButton*)hyperlinks_check, prefs.hyperlinks_only);
  gtk_toggle_button_set_active((GtkToggleButton*)confirm_check, prefs.confirm_clear);
  gtk_toggle_button_set_active((GtkToggleButton*)linemode_check, prefs.single_line);
  gtk_toggle_button_set_active((GtkToggleButton*)reverse_check, prefs.reverse_history);
  gtk_spin_button_set_value((GtkSpinButton*)charlength_spin, (gdouble)prefs.item_length);
	gtk_toggle_button_set_active((GtkToggleButton*)history_pos, prefs.history_pos);
	gtk_spin_button_set_value((GtkSpinButton*)history_x, (gdouble)prefs.history_x);
	gtk_spin_button_set_value((GtkSpinButton*)history_y, (gdouble)prefs.history_y);
  gtk_combo_box_set_active((GtkComboBox*)ellipsize_combo, prefs.ellipsize - 1);
  gtk_entry_set_text((GtkEntry*)history_key_entry, prefs.history_key);
  gtk_entry_set_text((GtkEntry*)actions_key_entry, prefs.actions_key);
  gtk_entry_set_text((GtkEntry*)menu_key_entry, prefs.menu_key);
  
  /* Read actions */
  read_actions();
  
  /* Run the dialog */
  gtk_widget_show_all(dialog);
  gtk_notebook_set_current_page((GtkNotebook*)notebook, tab);
  if (gtk_dialog_run((GtkDialog*)dialog) == GTK_RESPONSE_ACCEPT)
  {
    /* Apply and save preferences */
    apply_preferences();
    save_preferences();
    save_actions();
  }
  gtk_widget_destroy(dialog);
}
