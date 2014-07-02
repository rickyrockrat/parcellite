/* Copyright (C) 2011-2013 by rickyrockrat <gpib at rickyrockrat dot net>
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

/** \file ******************************************************************
\n\b File:        attr_list.c
\n\b Author:      Doug Springer
\n\b Company:     DNK Designs Inc.
\n\b Date:        01/09/2013  9:22 pm
\n\b Description: General attribute changing and list manipulation routines.
*/ /************************************************************************
*/

#include "parcellite.h"
/***************************************************************************/
/** Get the underline on the given lable, or allocate new one.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
PangoAttrInt *get_underline_attr(GtkLabel *label)
{
	PangoAttrList *attrs=gtk_label_get_attributes (label);
	PangoAttribute *attr;
	
  if (NULL == attrs){/**no existing list. Add one  */
		/*attrs = pango_attr_list_new (); 
		attr=NULL;*/
		return(NULL);
	}else{ /**grab attribte, if there is one.  */
		PangoAttrIterator* iter;
		iter=pango_attr_list_get_iterator(attrs);
		attr=pango_attr_iterator_get (iter,PANGO_ATTR_UNDERLINE);
		pango_attr_iterator_destroy(iter);	
	}
	return (PangoAttrInt *)attr;
}
/***************************************************************************/
/** Returns 0 if not strikethrough, 1 if it is.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
gboolean is_underline(GtkLabel *label)
{
	PangoAttrInt *strike=get_underline_attr(label);
	if(NULL == strike || strike->value == FALSE)
		return FALSE;
	return TRUE;
}

/***************************************************************************/
/** Set the strike through on/off. Create a new strike through and/or attr
list.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void set_underline(GtkLabel *label, gboolean mode)
{
	PangoAttrInt *strike;
	PangoAttrList *attrs;
	
	strike=get_underline_attr(label);
	if(NULL ==(attrs=gtk_label_get_attributes (label)) )
		attrs = pango_attr_list_new ();
	
	/** printf("strike=%p attrs=%p\n",strike,attrs);	fflush(NULL);*/
	if(NULL ==strike || strike->value != mode){
		PangoAttribute *attr=pango_attr_underline_new(mode);
		pango_attr_list_change (attrs, attr);
		gtk_label_set_attributes (label, attrs);
	}
}
/***************************************************************************/
/** Get the strikethrough on the given lable, or allocate new one.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
PangoAttrInt *get_strikethrough_attr(GtkLabel *label)
{
	PangoAttrList *attrs=gtk_label_get_attributes (label);
	PangoAttribute *attr;
	
  if (NULL == attrs){/**no existing list. Add one  */
		/*attrs = pango_attr_list_new (); 
		attr=NULL;*/
		return(NULL);
	}else{ /**grab attribte, if there is one.  */
		PangoAttrIterator* iter;
		iter=pango_attr_list_get_iterator(attrs);
		attr=pango_attr_iterator_get (iter,PANGO_ATTR_STRIKETHROUGH);
		pango_attr_iterator_destroy(iter);	
	}
	if(0 && NULL == attr){ /**No existing strikethrough. Add the attribute.  */
		attr=pango_attr_strikethrough_new(FALSE);
		pango_attr_list_change (attrs, attr);
		gtk_label_set_attributes (label, attrs);
	}
	return (PangoAttrInt *)attr;
}

/***************************************************************************/
/** Returns 0 if not strikethrough, 1 if it is.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
gboolean is_strikethrough(GtkLabel *label)
{
	PangoAttrInt *strike=get_strikethrough_attr(label);
	if(NULL == strike || strike->value == FALSE)
		return FALSE;
	return TRUE;
}

/***************************************************************************/
/** Set the strike through on/off. Create a new strike through and/or attr
list.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void set_strikethrough(GtkLabel *label, gboolean mode)
{
	PangoAttrInt *strike;
	PangoAttrList *attrs;
	
	strike=get_strikethrough_attr(label);
	if(NULL ==(attrs=gtk_label_get_attributes (label)) )
		attrs = pango_attr_list_new ();
	
	/** printf("strike=%p attrs=%p\n",strike,attrs);	fflush(NULL);*/
	if(NULL ==strike || strike->value != mode){
		PangoAttribute *attr=pango_attr_strikethrough_new(mode);
		pango_attr_list_change (attrs, attr);
		gtk_label_set_attributes (label, attrs);
	}
}
/***************************************************************************/
/** Find the item in the delete list.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
GList *find_h_item(GList *list,GtkWidget *w, GList *e)
{
	GList *i;
	for ( i=list; NULL != i; i=i->next){
		struct s_item_info *it=(struct s_item_info *)i->data;
		if( (NULL == w || it->item ==w) && it->element==e){/**found it  */
			  /*printf("Found %p\n",i);	fflush(NULL); */
			return i;
		}
	}
	return NULL;
}

/***************************************************************************/
/** Add an item to the history delete list.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void add_h_item(struct history_info *h, GtkWidget *w, GList* element, gint which)
{
	GList *ele;
	GList *op=NULL;
	switch(which){
		case OPERATE_DELETE:
			op=h->delete_list;
			break;
		case OPERATE_PERSIST:
			op=h->persist_list;
			break;
		default:
			g_fprintf(stderr,"Invalid list '%d'\n",which);
			return;
	}
	struct s_item_info *i;
	if(NULL == (ele=find_h_item(op,w,element) ) ){
		if(NULL != (i=g_malloc(sizeof(struct s_item_info)) ) ){
			i->item=w;
			i->element=element;
			switch(which){
				case OPERATE_DELETE:
					h->delete_list=g_list_prepend(op,(gpointer)i);
					break;
				case OPERATE_PERSIST:
					h->persist_list=g_list_prepend(op,(gpointer)i);
					break;
			}
			
			/*g_printf("Added w %p e %p %p\n",w,element,h->delete_list);	fflush(NULL); */
		}/*else g_fprintf(stderr,"%s: NULL allocating element for delete list!\n",__func__);	 */
	}/*else g_printf("found element??\n"); */
}

/***************************************************************************/
/** Delete an item from the history delete list.
h->delete_list is a GList. The data points to a struct s_item_info, which
contains a widget and an element - these latter two are part of the history 
menu, so we just need to de-allocate the delete_list structure, and delete it 
from the list.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void rm_h_item(struct history_info *h, GtkWidget *w, GList* element, gint which)
{
	GList *i;
	GList *op;
	switch(which){
		case OPERATE_DELETE:
			op=h->delete_list;
			break;
		case OPERATE_PERSIST:
			op=h->persist_list;
			break;
	}
	if(NULL != (i=find_h_item(op,w,element) ) ){
		/*printf("Freeing %p\n",i->data); */
		g_free(i->data);
		i->data=NULL;
		/*fflush(NULL); */
	  switch(which){
			case OPERATE_DELETE:
				h->delete_list=g_list_delete_link(op,i);
				break;
			case OPERATE_PERSIST:
				h->persist_list=g_list_delete_link(op,i);
				break;
		}
		/** printf("Removed %p\n",i);	fflush(NULL);*/
	}
		
}


/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void remove_deleted_items(struct history_info *h)
{
	if(NULL != h && NULL != h->delete_list){/**have a list of items to delete.  */
		GList *i;
		g_mutex_lock(hist_lock);
		/*g_print("Deleting items\n"); */
		for (i=h->delete_list; NULL != i; i=i->next){
			struct s_item_info *it=(struct s_item_info *)i->data;
			/*printf("Free %p.. '%s' ",it->element->data,(char *)(it->((struct history_item *(element->data))->text))); */
			g_free(it->element->data);
			it->element->data=NULL;
			history_list = g_list_delete_link(history_list, it->element);
			/** printf("Free %p\n",it);
			fflush(NULL);*/
			g_free(it);
		}
		h->delete_list=NULL;
		g_mutex_unlock(hist_lock);
		if (get_pref_int32("save_history"))
		  save_history();
	}	
}
/***************************************************************************/
/** Handle marking for history window. Also manages adding/removing from 
the delete list, which gets called when the history window closes.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void handle_marking(struct history_info *h, GtkWidget *w, gint index, gint which)
{
  GtkLabel *l=(GtkLabel *)(gtk_bin_get_child((GtkBin*)w)) ;
	GList* element = g_list_nth(history_list, index);
	if(OPERATE_DELETE == which){
		if(is_strikethrough(l)){ /**un-highlight  */
			set_strikethrough(l,FALSE);
			rm_h_item(h,w,element,OPERATE_DELETE);
		}
		else {
			/*g_printf("marking %p ",element); */
			set_strikethrough(l,TRUE);
			add_h_item(h,w,element,OPERATE_DELETE);
		}
	}else if(OPERATE_PERSIST == which){
		struct history_item *c=(struct history_item *)(element->data);
		if(NULL !=c){
			if(c->flags & CLIP_TYPE_PERSISTENT)
				c->flags&=~(CLIP_TYPE_PERSISTENT);
			else
				c->flags|=CLIP_TYPE_PERSISTENT;
			h->change_flag|=CLIP_TYPE_PERSISTENT;
			}
		if(is_underline(l)){ /**un-highlight  */
			set_underline(l,FALSE);
			rm_h_item(h,w,element,OPERATE_PERSIST);
		}
		else {
			set_underline(l,TRUE);
			add_h_item(h,w,element,OPERATE_PERSIST);
		}
	}
	
}

