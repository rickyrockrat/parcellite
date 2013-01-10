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
 
build with:
gcc -Wall `pkg-config --cflags gtk+-2.0` utils.c history.clipitem.c `pkg-config --libs gtk+-2.0` -o history.clipitem
 */

#include <glib.h>
#include <gtk/gtk.h>
#include "main.h"
#include "utils.h"
#include "history.h"
#include <string.h>
prefs_t prefs = {DEF_USE_COPY,        DEF_USE_PRIMARY,      DEF_SYNCHRONIZE,
                 DEF_SAVE_HISTORY,    DEF_HISTORY_LIMIT,
                 DEF_HYPERLINKS_ONLY, DEF_CONFIRM_CLEAR,
                 DEF_SINGLE_LINE,     DEF_REVERSE_HISTORY,  DEF_ITEM_LENGTH,
                 DEF_ELLIPSIZE,
                 INIT_HISTORY_KEY,    INIT_ACTIONS_KEY,     INIT_MENU_KEY,
                 DEF_NO_ICON};
GSList* history_list=NULL;

#define HISTORY_MAGIC_SIZE 32
#define HISTORY_VERSION     1 /**index (-1) into array below  */
static gchar* history_magics[]={  
																"1.0ParcelliteHistoryFile",
																NULL,
};


/* Reads history from ~/.local/share/parcellite/history */
void read_history_old ()
{
  /* Build file path */
  gchar* history_path = g_build_filename(g_get_user_data_dir(), HISTORY_FILE,   NULL);
  
  /* Open the file for reading */
  FILE* history_file = fopen(history_path, "rb");
  g_free(history_path);
  /* Check that it opened and begin read */
  if (history_file)   {
    /* Read the size of the first item */
    gint size;
    if (fread(&size, 4, 1, history_file) != 1)
      size = 0;
    /* Continue reading until size is 0 */
    while (size)   {
      /* Malloc according to the size of the item */
      gchar* item = (gchar*)g_malloc(size + 1);
      /* Read item and add ending character */
      fread(item, size, 1, history_file);
      item[size] = '\0';
      /* Prepend item and read next size */
      history_list = g_slist_prepend(history_list, item);
      if (fread(&size, 4, 1, history_file) != 1)
        size = 0;
    }
    /* Close file and reverse the history to normal */
    fclose(history_file);
    history_list = g_slist_reverse(history_list);
  }
}

/* Saves history to ~/.local/share/parcellite/history */
void save_history_old()
{
  /* Check that the directory is available */
  check_dirs();
  /* Build file path */
  gchar* history_path = g_build_filename(g_get_user_data_dir(),HISTORY_FILE, NULL);
  /* Open the file for writing */
  FILE* history_file = fopen(history_path, "wb");
  g_free(history_path);
  /* Check that it opened and begin write */
  if (history_file)  {
    GSList* element;
    /* Write each element to a binary file */
    for (element = history_list; element != NULL; element = element->next) {
      /* Create new GString from element data, write its length (size)
       * to file followed by the element data itself
       */
      GString* item = g_string_new((gchar*)element->data);
      fwrite(&(item->len), 4, 1, history_file);
      fputs(item->str, history_file);
      g_string_free(item, TRUE);
    }
    /* Write 0 to indicate end of file */
    gint end = 0;
    fwrite(&end, 4, 1, history_file);
    fclose(history_file);
  }
}


/***************************************************************************/
/** .
\n\b Arguments:
magic is what we are looking for, fmagic iw what we read from the file.
\n\b Returns: history matched on match, -1 on erro, 0 if not found 
****************************************************************************/
int check_magic(gchar *fmagic)
{
	gint i, rtn;
	gchar *magic=g_malloc0(2+HISTORY_MAGIC_SIZE);
	if( NULL == magic) return -1;
	for (i=0;NULL !=history_magics[i];++i){
		memset(magic,0,HISTORY_MAGIC_SIZE);
		memcpy(magic,history_magics[i],strlen(history_magics[i]));	
		if(!memcmp(magic,fmagic,HISTORY_MAGIC_SIZE)){
			rtn= i+1;	
			goto done;
		}
			
	}
	rtn=0;
done:
	g_free(magic);
	return rtn;
}
/***************************************************************************/
/** Reads history from ~/.local/share/parcellite/history .
Current scheme is to have the total zize of element followed by the type, then the data
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void read_history ()
{
  /* Build file path */
  /*gchar* history_path = g_build_filename(g_get_user_data_dir(),HISTORY_FILE,NULL); */
	gchar* history_path = g_build_filename(g_get_user_data_dir(),"parcellite/hist.test", NULL);
	gchar *magic=g_malloc0(2+HISTORY_MAGIC_SIZE);
  /*g_printf("History file '%s'",history_path); */
  /* Open the file for reading */
  FILE* history_file = fopen(history_path, "rb");
  g_free(history_path);
  /* Check that it opened and begin read */
  if (history_file)  {
    /* Read the magic*/
    guint32 size=1, end;
		if (fread(magic,HISTORY_MAGIC_SIZE , 1, history_file) != 1){
			g_printf("No magic! Assume no history.\n");
			goto done;
		}
    if(HISTORY_VERSION !=check_magic(magic)){
			g_printf("TODO! History version not matching!!Discarding history.\n");
			goto done;
		}
		g_printf("History Magic OK. Reading\n");
    /* Continue reading until size is 0 */
    while (size)   {
			struct history_item *c;
			if (fread(&size, 4, 1, history_file) != 1)
       size = 0;
			if(0 == size)
				break;
      /* Malloc according to the size of the item */
      c = (struct history_item *)g_malloc(size+ 1);
			end=size-(sizeof(struct history_item)+4);
      
      if (fread(c, sizeof(struct history_item), 1, history_file) !=1)
      	g_printf("history_read: Invalid type!");
			if(c->len != end)
				g_printf("len check: invalid: ex %d got %d\n",end,c->len);
			/* Read item and add ending character */
			if (fread(&c->data,end,1,history_file) != 1)
				g_printf("history_read: Invalid text!");
      c->data[end] = '\0';
			g_printf("len %d type %d '%s'\n",c->len,c->type,c->data); 
      /* Prepend item and read next size */
      history_list = g_slist_prepend(history_list, c);
    }
done:
		g_free(magic);
    /* Close file and reverse the history to normal */
    fclose(history_file);
    history_list = g_slist_reverse(history_list);
  }
}
/**  NOTES:
	gint width, height, rowstride, n_channels,bits_per_sample ;
  guchar *pixels;

  n_channels = gdk_pixbuf_get_n_channels (pixbuf);

  g_assert (gdk_pixbuf_get_colorspace (pixbuf) == GDK_COLORSPACE_RGB);
  bits_per_sample=gdk_pixbuf_get_bits_per_sample (pixbuf);

  width = gdk_pixbuf_get_width (pixbuf);
  height = gdk_pixbuf_get_height (pixbuf);

  rowstride = gdk_pixbuf_get_rowstride (pixbuf);
  pixels = gdk_pixbuf_get_pixels (pixbuf);

len of pixbuf=rowstride*(height-1)+width * ((n_channels * bits_per_sample + 7) / 8)

last row of pixbuf=width * ((n_channels * bits_per_sample + 7) / 8)
*/
/* Saves history to ~/.local/share/parcellite/history */

/***************************************************************************/
/** write total len, then write type, then write data.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void save_history()
{
  /* Check that the directory is available */
  check_dirs();
  /* Build file path */
/*  gchar* history_path = g_build_filename(g_get_user_data_dir(), HISTORY_FILE, NULL); */
	gchar* history_path = g_build_filename(g_get_user_data_dir(),"parcellite/hist.test", NULL);
  /* Open the file for writing */
  FILE* history_file = fopen(history_path, "wb");
  g_free(history_path);
  /* Check that it opened and begin write */
  if (history_file)  {
    GSList* element;
		gchar *magic=g_malloc0(2+HISTORY_MAGIC_SIZE);
	  if( NULL == magic) return;
		memcpy(magic,history_magics[HISTORY_VERSION-1],strlen(history_magics[HISTORY_VERSION-1]));	
		fwrite(magic,HISTORY_MAGIC_SIZE,1,history_file);
    /* Write each element to a binary file */
    for (element = history_list; element != NULL; element = element->next) {
		  struct history_item *c;
			gint32 len;
      /* Create new GString from element data, write its length (size)
       * to file followed by the element data itself
       */
			c=(struct history_item *)element->data;
			/**write total len  */
			/**write total len  */
			len=c->len+sizeof(struct history_item)+4;
			fwrite(&len,4,1,history_file);
			fwrite(c,sizeof(struct history_item),1,history_file);
			fwrite(c->data,c->len,1,history_file);
    }
    /* Write 0 to indicate end of file */
    gint end = 0;
    fwrite(&end, 4, 1, history_file);
    fclose(history_file);
  }
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
struct history_item *new_clip_item(gint type, guint32 len, void *data)
{
	struct history_item *c;
	if(NULL == (c=g_malloc(sizeof(struct history_item)+len))){
		printf("Hit NULL for malloc of history_item!\n");
		return NULL;
	}
		
	c->type=type;
	memcpy(c->data,data,len);
	c->len=len;
	return c;
}

/***************************************************************************/
/**  Adds item to the end of history .
\n\b Arguments:
\n\b Returns:
****************************************************************************/

void append_item(gchar* item)
{
  if (item)  {
		struct history_item *c;
		if(NULL == (c=new_clip_item(CLIP_TYPE_TEXT,strlen(item),item)) )
			return;
    /* Prepend new item */
    history_list = g_slist_prepend(history_list, c);
    /* Shorten history if necessary */
    GSList* last_possible_element = g_slist_nth(history_list, prefs.history_limit - 1);
    if (last_possible_element)     {
      /* Free last posible element and subsequent elements */
      g_slist_free(last_possible_element->next);
      last_possible_element->next = NULL;
    }
    /* Save changes */
    if (prefs.save_history)
      save_history();
  }
}

/***************************************************************************/
/**  Deletes duplicate item in history .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void delete_duplicate(gchar* item)
{
  GSList* element;
  /* Go through each element compare each */
  for (element = history_list; element != NULL; element = element->next) {
	  struct history_item *c;
		c=(struct history_item *)element->data;
		if(CLIP_TYPE_TEXT == c->type){
	    if (g_strcmp0((gchar*)c->data, item) == 0) {
	      g_free(element->data);
	      history_list = g_slist_delete_link(history_list, element);
	      break;
	    }
		}
  }
}

/***************************************************************************/
/** Truncates history to history_limit items .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void truncate_history()
{
  if (history_list)  {
    /* Shorten history if necessary */
    GSList* last_possible_element = g_slist_nth(history_list, prefs.history_limit - 1);
    if (last_possible_element)    {
      /* Free last posible element and subsequent elements */
      g_slist_free(last_possible_element->next);
      last_possible_element->next = NULL;
    }
    /* Save changes */
    if (prefs.save_history)
      save_history();
  }
}

/***************************************************************************/
/** Returns pointer to last item in history .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
gpointer get_last_item()
{
  if (history_list)
  {
    if (history_list->data)
    {
      /* Return the last element */
      gpointer last_item = history_list->data;
      return last_item;
    }
    else
      return NULL;
  }
  else
    return NULL;
}


int main (int argc, char *argv[])
{
	read_history();
	append_item("This is first");
	append_item("This is next");
	save_history();
}
