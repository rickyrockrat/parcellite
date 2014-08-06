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

#include "parcellite.h"


/**This is now a gslist of   */
GList* history_list=NULL;
static gint dbg=0;
/**todo:
handle parcellite history magic:
if it does not exist, then we assume original format, and convert. 
	Need to provide a menu item that allows user to convert back to old?
	or just provide manual instructions to convert back (i.e. dd skip)?
	
  */
#define HISTORY_MAGIC_SIZE 32
#define HISTORY_VERSION     1 /**index (-1) into array below  */
static gchar* history_magics[]={  
																"1.0ParcelliteHistoryFile",
																NULL,
};

#define HISTORY_FILE0 HISTORY_FILE 
/*#define HISTORY_FILE0 "parcellite/hist.test" */

/***************************************************************************/
/** Pass in the text via the struct. We assume len is correct, and BYTE based,
not character.
\n\b Arguments:
\n\b Returns:	length of resulting string.
****************************************************************************/
glong validate_utf8_text(gchar *text, glong len)
{
	const gchar *valid;
	if(NULL == text || len <= 0)
		return 0;
	text[len]=0;
	if(FALSE == g_utf8_validate(text,-1,&valid)) {
		len=valid-text;
		text[len]=0;
		g_fprintf(stderr,"Truncating invalid utf8 text entry: '%s'\n",text);
	}
	return len;
}
/***************************************************************************/
/** Read the old history file and covert to the new format.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void read_history_old ()
{
  /* Build file path */
  gchar* history_path = g_build_filename(g_get_user_data_dir(), HISTORY_FILE0,   NULL);
  
  /* Open the file for reading */
  FILE* history_file = fopen(history_path, "rb");
  g_free(history_path);
  /* Check that it opened and begin read */
  if (history_file)   {
    /* Read the size of the first item */
    gint size=1;
    g_mutex_lock(hist_lock);
    /* Continue reading until size is 0 */
    while (size)   {
			struct history_item *c;
			if (fread(&size, 4, 1, history_file) != 1){
				size = 0;
				break;
			} else if( 0 == size )
				break;
      /* Malloc according to the size of the item */  
			c = (struct history_item *)g_malloc0(size+ 2+sizeof(struct history_item));
      c->type=CLIP_TYPE_TEXT;
			c->len=size;
      /* Read item and add ending character */
      if(fread(c->text, size, 1, history_file) !=1){
				g_printf("Error reading history file entry \n");
			}else{
				c->text[size] = 0;
				c->len=validate_utf8_text(c->text,c->len);
				if(0 != c->len) /* Prepend item and read next size */
	      	history_list = g_list_prepend(history_list, c);
				else g_free(c);
			}
    }
    /* Close file and reverse the history to normal */
    fclose(history_file);
    history_list = g_list_reverse(history_list);
		g_mutex_unlock(hist_lock);
  }
}

/* Saves history to ~/.local/share/parcellite/history */
void save_history_old()
{
  /* Check that the directory is available */
  check_dirs();
  /* Build file path */
  gchar* history_path = g_build_filename(g_get_user_data_dir(),HISTORY_FILE0, NULL);
  /* Open the file for writing */
  FILE* history_file = fopen(history_path, "wb");
  g_free(history_path);
  /* Check that it opened and begin write */
  if (history_file)  {
    GList* element;
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
	size_t x;
  /* Build file path */
  gchar* history_path = g_build_filename(g_get_user_data_dir(),HISTORY_FILE0,NULL); 
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
			g_fprintf(stderr,"No magic! Assume no history.\n");
			goto done;
		}
    if(HISTORY_VERSION !=check_magic(magic)){
			g_fprintf(stderr,"Assuming old history style. Read and convert.\n");
			/*g_printf("TODO! History version not matching!!Discarding history.\n"); */
			g_free(magic);
			fclose(history_file);
			read_history_old();
			return;
		}
		if(dbg) g_printf("History Magic OK. Reading\n");
    /* Continue reading until size is 0 */
		g_mutex_lock(hist_lock);
    while (size)   {
			struct history_item *c;
			if (fread(&size, 4, 1, history_file) != 1)
       size = 0;
			if(0 == size)
				break;
      /* Malloc according to the size of the item */
      c = (struct history_item *)g_malloc0(size+ 1);
			end=size-(sizeof(struct history_item)+4);
      
      if (fread(c, sizeof(struct history_item), 1, history_file) !=1)
      	g_fprintf(stderr,"history_read: Invalid type!");
			if(c->len != end)
				g_fprintf(stderr,"len check: invalid: ex %d got %d\n",end,c->len);
			/* Read item and add ending character */
			if ((x =fread(&c->text,end,1,history_file)) != 1){
				c->text[end] = 0;
				g_fprintf(stderr,"history_read: Invalid text, code %ld!\n'%s'\n",(unsigned long)x,c->text);
			}	else {
				c->text[end] = 0;
				c->len=validate_utf8_text(c->text,c->len);
				if(dbg) g_fprintf(stderr,"len %d type %d '%s'\n",c->len,c->type,c->text); 
				if(0 != c->len) /* Prepend item and read next size */
	      	history_list = g_list_prepend(history_list, c);
				else g_free(c);
			}
      
    }
done:
		g_free(magic);
    /* Close file and reverse the history to normal */
    fclose(history_file);
    history_list = g_list_reverse(history_list);
		g_mutex_unlock(hist_lock);
  }
	if(dbg) g_printf("History read done\n");
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
  gchar* history_path = g_build_filename(g_get_user_data_dir(), HISTORY_FILE0, NULL); 
  /* Open the file for writing */
  FILE* history_file = fopen(history_path, "wb");
  
  /* Check that it opened and begin write */
  if (history_file)  {
    GList* element;
		gchar *magic=g_malloc0(2+HISTORY_MAGIC_SIZE);
	  if( NULL == magic) return;
		memcpy(magic,history_magics[HISTORY_VERSION-1],strlen(history_magics[HISTORY_VERSION-1]));	
		fwrite(magic,HISTORY_MAGIC_SIZE,1,history_file);
		g_mutex_lock(hist_lock);
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
			if(c->len >0){
				len=c->len+sizeof(struct history_item)+4;
				fwrite(&len,4,1,history_file);
				fwrite(c,sizeof(struct history_item),1,history_file);
				fwrite(c->text,c->len,1,history_file);	
			}
			
    }
		g_mutex_unlock(hist_lock);
    /* Write 0 to indicate end of file */
    gint end = 0;
    fwrite(&end, 4, 1, history_file);
    fclose(history_file);
  }else{
		g_fprintf(stderr,"Unable to open history file for save '%s'\n",history_path);
	}
	g_free(history_path);
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
struct history_item *new_clip_item(gint type, guint32 len, void *data)
{
	struct history_item *c;
	if(NULL == (c=g_malloc0(sizeof(struct history_item)+len))){
		g_fprintf(stderr,"Hit NULL for malloc of history_item!\n");
		return NULL;
	}
		
	c->type=type;
	memcpy(c->text,data,len);
	c->len=len;
	return c;
}
/***************************************************************************/
/**  checks to see if text is already in history. Also is a find text
\n\b Arguments: if mode is 1, delete it too.
\n\b Returns: -1 if not found, or nth element.
****************************************************************************/
gint is_duplicate(gchar* item, int mode, gint *flags)
{
  GList* element;
	gint i;
	if(NULL ==item)
		return -1;
  /* Go through each element compare each */
  for (i=0,element = history_list; element != NULL; element = element->next,++i) {
	  struct history_item *c;
		c=(struct history_item *)element->data;
		if(CLIP_TYPE_TEXT == c->type){
	    if (g_strcmp0((gchar*)c->text, item) == 0) {
				if(mode){
					if(NULL != flags && (CLIP_TYPE_PERSISTENT&c->flags)){
						*flags=c->flags;
					}
					/*g_printf("Freeing 0x%02X '%s'\n",c->flags,c->text);  */
					/** g_free(element->data);
		      history_list = g_list_delete_link(history_list, element);*/
				}
				return i;
	      break;
	    }
		}
  }
	return -1;
}
/***************************************************************************/
/**  Adds item to the end of history .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void append_item(gchar* item, int checkdup, gint iflags, gint itype)
{
	gint flags=0,node=-1;
	GList *element=NULL;
	struct history_item *c=NULL;
	if(NULL == item)
		return;
	g_mutex_lock(hist_lock);
/**delete if HIST_DEL flag is set.  */
	if( checkdup & HIST_CHECKDUP){
		node=is_duplicate(item, checkdup & HIST_DEL, &flags);
		/*g_printf("isd done "); */
		if(node > -1){ /**found it  */
			/*g_printf(" found\n"); */
			if(!(checkdup & HIST_DEL))
				return;	
		}
	}
	if (-1 != node ){/**we found a duplicate in the history, remove, then re-add existing  */
		element=g_list_nth(history_list,node);
		history_list=g_list_remove_link(history_list,element);
	}else{ /**not found   */
		if(NULL == (c=new_clip_item(CLIP_TYPE_TEXT,strlen(item),item)) )
			return;
		if(node > -1 && (checkdup & HIST_KEEP_FLAGS) ){
			c->flags=flags;
			/*g_printf("Restoring 0x%02X '%s'\n",c->flags,c->text);  */
		}	else
			c->flags=iflags;
		c->type=itype;
	}
	

		
	/*g_printf("Append '%s'\n",item); */
   /* Prepend new item */
  history_list = g_list_prepend(history_list, NULL ==element?c:element->data);
	g_mutex_unlock(hist_lock);
  /* Shorten history if necessary */
  truncate_history();
}

/***************************************************************************/
/**  Deletes duplicate item in history . Orphaned function.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void delete_duplicate(gchar* item)
{
  GList* element;
	int p=get_pref_int32("persistent_history");
  /* Go through each element compare each */
	g_mutex_lock(hist_lock);
  for (element = history_list; element != NULL; element = element->next) {
	  struct history_item *c;
		c=(struct history_item *)element->data;
		if( (!p || !(CLIP_TYPE_PERSISTENT&c->flags)) && CLIP_TYPE_TEXT == c->type){
	    if (g_strcmp0((gchar*)c->text, item) == 0) {
				g_printf("del dup '%s'\n",c->text);
	      g_free(element->data);
	      history_list = g_list_delete_link(history_list, element);
	      break;
	    }
		}
  }
	g_mutex_unlock(hist_lock);
}

/***************************************************************************/
/**  Truncates history to history_limit items, while preserving persistent
    data, if specified by the user. FIXME: This may not shorten the history 
    
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void truncate_history()
{
  int p=get_pref_int32("persistent_history");
  if (history_list)  {
		g_mutex_lock(hist_lock);
		guint ll=g_list_length(history_list);
		guint lim=get_pref_int32("history_limit");
		if(ll > lim){ /* Shorten history if necessary */
			GList* last = g_list_last(history_list);	
			while (last->prev && ll>lim)   {
	      struct history_item *c=(struct history_item *)last->data;
				last=last->prev;
	      if(!p || !(c->flags&CLIP_TYPE_PERSISTENT)){
					history_list=g_list_remove(history_list,c);
					--ll;
				}
	    }	
		}
		g_mutex_unlock(hist_lock);
    /* Save changes */
    if (get_pref_int32("save_history"))
      save_history();
  }
}

/* Returns pointer to last item in history */
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


/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void clear_history( void )
{
	g_mutex_lock(hist_lock);
	if( !get_pref_int32("persistent_history")){
		g_list_free(history_list);
		history_list = NULL;
	}	else{ /**save any persistent items  */
		GList* element;
		for (element = history_list; element != NULL; element = element->next) {
		  struct history_item *c;
			c=(struct history_item *)element->data;
			if(!(c->flags & CLIP_TYPE_PERSISTENT))
				history_list=g_list_remove(history_list,c);
		}		
	}
	g_mutex_unlock(hist_lock);
  if (get_pref_int32("save_history"))
  	save_history();
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int save_history_as_text(gchar *path)
{
	FILE* fp = fopen(path, "w");
  /* Check that it opened and begin write */
  if (fp)  {
		gint i;
    GList* element;
		g_mutex_lock(hist_lock);
    /* Write each element to  file */
    for (i=0,element = history_list; element != NULL; element = element->next) {
		  struct history_item *c;
			c=(struct history_item *)element->data;
			if(c->flags & CLIP_TYPE_PERSISTENT)
				fprintf(fp,"PHIST_%04d %s\n",i,c->text);
			else
				fprintf(fp,"NHIST_%04d %s\n",i,c->text);
			++i;
    }
		g_mutex_unlock(hist_lock);
    fclose(fp);
  }
	
	g_printf("histpath='%s'\n",path);
	return 0;
}
/***************************************************************************/
/** Dialog to save the history file.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void history_save_as(GtkMenuItem *menu_item, gpointer user_data)
{
	GtkWidget *dialog;
  dialog = gtk_file_chooser_dialog_new ("Save History File",
				      NULL,/**parent  */
				      GTK_FILE_CHOOSER_ACTION_SAVE,
				      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				      GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
				      NULL);
	gtk_window_set_icon((GtkWindow*)dialog, gtk_widget_render_icon(dialog, GTK_STOCK_SAVE_AS, -1, NULL));
	gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);
/*	if (user_edited_a_new_document)  { */
	    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), g_get_home_dir());
	    gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), "ParcelliteHistory.txt");
/**    }
	else
	  gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (dialog), filename_for_existing_document);*/	
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
	    gchar *filename;
	    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
	    save_history_as_text (filename);
	    g_free (filename);
	  }
	gtk_widget_destroy (dialog);
}

