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
 
NOTES:
We keep track of a delete list while the history menu is up. We add/remove items from that 
list until we get a selection done event, then we delete those items from the real history

From http://standards.freedesktop.org/clipboards-spec/clipboards-latest.txt

Application authors should follow the following guidelines to get
correct behavior:

 - selecting but with no explicit copy should only set PRIMARY,
   never CLIPBOARD

 - middle mouse button should paste PRIMARY, never CLIPBOARD

 - explicit cut/copy commands (i.e. menu items, toolbar buttons)
   should always set CLIPBOARD to the currently-selected data (i.e.
   conceptually copy PRIMARY to CLIPBOARD)

 - explicit cut/copy commands should always set both CLIPBOARD and
   PRIMARY, even when copying doesn't involve a selection (e.g. a
   "copy url" -option which explicitly copies an url without the
   url being selected first)

 - explicit paste commands should paste CLIPBOARD, not PRIMARY

 - a selection becoming unselected should never unset PRIMARY

 - possibly contradicting the ICCCM, clients don't need to support
   SECONDARY, though if anyone can figure out what it's good
   for they should feel free to use it for that

 - cut buffers are evil; they only support ASCII, they don't 
   work with many clients, and they require data to be 
   copied to the X server. Therefore clients should avoid 
   using cut buffers and use only selections.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "parcellite.h"

/**ACT are actions, and MODE is the mode of the action  */
/** #define ACT_STOP  0
#define ACT_RUN   1 */

/** defines the mode of the above actions. These are bit-wise */
#define CMODE_PRI 1
#define	CMODE_CLI 2
#define CMODE_ALL 3 /**needs to be or of the above  */

#define FIFCMD_STOP_PRI "stop_pri"
#define FIFCMD_STOP_CLI "stop_cli"
#define FIFCMD_STOP_ALL "stop_all"
#define FIFCMD_RUN_PRI "run_pri"
#define FIFCMD_RUN_CLI "run_cli"
#define FIFCMD_RUN_ALL "run_all"

GtkWidget *hmenu;
/* Uncomment the next line to print a debug trace. */
/*#define DEBUG   */

#ifdef DEBUG
#  define TRACE(x) x
#else
#  define TRACE(x) do {} while (FALSE);
#endif
static GtkClipboard* primary;
static GtkClipboard* clipboard;
struct p_fifo *fifo;
#ifdef HAVE_APPINDICATOR
static AppIndicator *indicator=NULL;
static GtkWidget *indicator_menu = NULL;
#endif
static GtkStatusIcon *status_icon; 
static GMutex *clip_lock=NULL;
GMutex *hist_lock=NULL;
static gboolean actions_lock = FALSE;
static int show_icon=0;
static int have_appindicator=0; /**if set, we have a running indicator-appmenu  */
static gchar *appindicator_process="indicator-appmenu"; /**process name  */

static int cmd_mode=CMODE_ALL; /**both clipboards  */
/** static int cmd_state=ACT_RUN; running  */
/**defines for moving between clipboard histories  */
#define HIST_MOVE_TO_CANCEL     0
#define HIST_MOVE_TO_OK         1
/**clipboard handling modes  */
#define H_MODE_INIT  0	/**clear out clipboards  */
#define H_MODE_NEW  1	/**new text, process it  */
#define H_MODE_LIST 2	/**from list, just put it on the clip  */
#define H_MODE_CHECK 3 /**see if there is new/lost contents.   */
#define H_MODE_LAST  4 /**just return the last updated value.  */
#define H_MODE_IGNORE 5 /**just put it on the clipboard, do not process 
                       and do not add to hist list  */

#define EDIT_MODE_USE_RIGHT_CLICK 1 /**used in edit dialog creation to determine behaviour. 
                                    If this is set, it will edit the entry, and replace it in the history.  */

/**protos in this file  */
void create_app_indicator(void);

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

/***************************************************************************/
/** Process the text based on our preferences.
\n\b Arguments:
\n\b Returns: processed text, or NULL if it is invalid.
****************************************************************************/
gchar *process_new_item(GtkClipboard *clip,gchar *ntext)
{
	glong len;
	gchar *rtn=NULL;
	if(NULL == ntext)
		return NULL;
	
	
/**we now check our options...  */		
	/*printf("opt\n"); fflush(NULL); */
	if (get_pref_int32("hyperlinks_only")){
		 if(is_hyperlink(ntext))
			 	goto process;
	}	else {
		/*printf("wo\n"); fflush(NULL); */
		if(get_pref_int32("ignore_whiteonly")){
			gchar *s;
			for (s=ntext; NULL !=s && *s; ++s){
				if(!isspace(*s)){
		/*			printf("Saw 0x%x\n",*s); */
					goto process;
					break;
				}
			}
		}else
			goto process;
	}
	/**set the clipboard to the last entry - effectively deleting this entry */
	goto done;
	
 process:  /**now process the text.  */
	/*printf("proc\n"); fflush(NULL); */
  len=strlen(ntext);/*g_utf8_strlen(ntext,-1); */
  len= validate_utf8_text(ntext, len);
	if(len){
		rtn=ntext;
		gint i;
		if(get_pref_int32("trim_newline")){
			gchar *c;
			for (c=ntext;*c;){
				if(iscntrl(*c))
					*c=' ';
				c=g_utf8_next_char(c);
			}	
		}
			
		if( get_pref_int32("trim_wspace_begend") )
			ntext=g_strstrip(ntext);
	}	
done:
	return rtn;	
}


/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:	text that was updated or NULL if not.
****************************************************************************/
gchar *_update_clipboard (GtkClipboard *clip, gchar *n, gchar **old, int set)
{
	
	if(NULL != n)	{
	/**  	if(clip==primary)
			g_printf("set PRI to %s\n",n);
		else
			g_printf("set CLI to %s\n",n);*/
		if( set)
			gtk_clipboard_set_text(clip, n, -1);
		if(NULL != *old)
		  g_free(*old);
		*old=g_strdup(n);
		return *old;
	}else{
		if(NULL != *old)
		  g_free(*old);
		*old=NULL;
	}
		
	return NULL;
}


/***************************************************************************/
/** This DOES NOT WORK!! WTH??.
Fix is a bit hacky.
\n\b Arguments:
\n\b Returns:	 clipboard contents, to be freed with g_free, or NULL if empty.
****************************************************************************/
gchar *is_clipboard_empty(GtkClipboard *clip)
{
  /** int count;
  GdkAtom *targets;
	if(clipboard == clip) g_printf("-%s-",gtk_clipboard_wait_for_text(clip));
  gboolean contents = gtk_clipboard_wait_for_targets(clip, &targets, &count);
	if(clipboard == clip) g_printf("-%s-2nd-",gtk_clipboard_wait_for_text(clip));
  g_free(targets);
	if(TRUE == contents || count >0)
		return 0;*/
	return(gtk_clipboard_wait_for_text(clip));
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
gchar *update_clipboard(GtkClipboard *clip,gchar *intext,  gint mode)
{
	/**current/last item in clipboard  */
	static gchar *ptext=NULL;
	static gchar *ctext=NULL;
	static gchar *last=NULL; /**last text change, for either clipboard  */
	gchar **existing, *changed=NULL;
	gchar *processed;
	GdkModifierType button_state;
/*	gchar *clipname; */
	int set=1;
	if( H_MODE_LAST == mode)
		return last;
	if(clip==primary){
/*		clipname="PRI "; */
		existing=&ptext;
	}	else{
/*		clipname="CLI "; */
		existing=&ctext;
	}
	
	/** if(H_MODE_CHECK!=mode )
	g_printf("HC%d-%c: in %s,ex %s\n",mode,clip==primary?'p':'c',intext,*existing);*/
	if( H_MODE_INIT == mode){
		if(NULL != *existing)
			g_free(*existing);
		*existing=NULL;
		if(NULL != intext)
		gtk_clipboard_set_text(clip, intext, -1);
		return NULL;
	}
	/**check that our clipboards are valid and user wants to use them  */
	if((clip != primary && clip != clipboard) ||
		(clip == primary && !get_pref_int32("use_primary")) ||
		(clip == clipboard && !get_pref_int32("use_copy")))
			return NULL;
	
	if(H_MODE_CHECK==mode &&clip == primary){/*fix auto-deselect of text in applications like DevHelp and LyX*/
   	gdk_window_get_pointer(NULL, NULL, NULL, &button_state);
		if ( button_state & (GDK_BUTTON1_MASK|GDK_SHIFT_MASK) ) /**button down, done.  */
			goto done;
	}
	/*g_printf("BS=0x%02X ",button_state); */
	if( H_MODE_IGNORE == mode){	/**ignore processing and just put it on the clip.  */
/*		g_printf("%sJustSet '%s'\n",clipname,intext); */
		gtk_clipboard_set_text(clip, intext, -1);
		return intext;
	}
	/**check for lost contents and restore if lost */
	/* Only recover lost contents if there isn't any other type of content in the clipboard */
	if(NULL ==(changed=is_clipboard_empty(clip)) && NULL != *existing ) {
		g_printf("%sclp empty, set to '%s'\n",clip==clipboard?"CLI":"PRI",*existing); 
    gtk_clipboard_set_text(clip, *existing, -1);
		last=*existing;
  }
	/**check for changed clipboard content - in all modes */
	/*changed=gtk_clipboard_wait_for_text(clip); */
	if(0 == p_strcmp(*existing, changed) ){
		g_free(changed);                    /**no change, do nothing  */
		changed=NULL;
	}	else {
/*		g_printf("%sclp changed: ex '%s' is '%s'\n",clipname,*existing,changed); */
		if(NULL != (processed=process_new_item(clip,changed)) ){
			if(0 == p_strcmp(processed,changed)) set=0;
			else set=1;
				 
			last=_update_clipboard(clip,processed,existing,set);
		}else {/**restore clipboard  */
			gchar *d;
			
			if(NULL ==*existing && NULL != history_list){
				struct history_item *c;
				c=(struct history_item *)(history_list->data);	
				d=c->text;
			}else 
				d=*existing;
			if(NULL != d){
/*				g_printf("%srestore clp '%s'\n",clipname,d); */
				last=_update_clipboard(clip,d,existing,1);
			}
				
		}
		if(NULL != last)
			append_item(last,get_pref_int32("current_on_top")?HIST_DEL|HIST_CHECKDUP|HIST_KEEP_FLAGS:0);
		g_free(changed);
		changed=NULL;
	}
	if( H_MODE_CHECK==mode ){
		goto done;
	}
/**FIXME: Do we use the changed clip item or the one passed to us?  
	hmmm Use the changed one.
	
	*/		
	
	if(H_MODE_LIST == mode && 0 != p_strcmp(intext,*existing)){ /**just set clipboard contents. Already in list  */
/*		g_printf("%sInList '%s' ex '%s'\n",clipname,intext,*existing); */
		last=_update_clipboard(clip,intext,existing,1);
		if(NULL != last){/**maintain persistence, if set  */
			append_item(last,get_pref_int32("current_on_top")?HIST_DEL|HIST_CHECKDUP|HIST_KEEP_FLAGS:0);
		}
		goto done;
	}else if(H_MODE_NEW==mode){
		if(NULL != (processed=process_new_item(clip,intext)) ){
/*			g_printf("%sNEW '%s'\n",clipname,processed); */
			if(0 == p_strcmp(processed,*existing))set=0;
			else set=1;
			last=_update_clipboard(clip,processed,existing,set);
			if(NULL != last)
				append_item(last,get_pref_int32("current_on_top")?HIST_DEL|HIST_CHECKDUP|HIST_KEEP_FLAGS:0);
		}else 
			return NULL;	
	}
		
done:
	return *existing;
}

/***************************************************************************/
/** Convience function to update both clipboards at the same time
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void update_clipboards(gchar *intext, gint mode)
{
	/*g_printf("upclips\n"); */
	update_clipboard(primary, intext, mode);
	update_clipboard(clipboard, intext, mode);
}

/***************************************************************************/
/** Run a command. For now, just start and stop
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void do_command(gchar *buf, gint len)
{
  g_printf("Got '%s' cmd\n",buf);
	if(!p_strcmp(buf,FIFCMD_RUN_ALL)) {
		cmd_mode|=CMODE_ALL;
		return;
	}	
	if(!p_strcmp(buf,FIFCMD_RUN_CLI)) {
		cmd_mode|=CMODE_CLI;
		return;
	}	
	if(!p_strcmp(buf,FIFCMD_RUN_PRI)) {
		cmd_mode|=CMODE_PRI;
		return;
	}	
	if(!p_strcmp(buf,FIFCMD_STOP_ALL)) {
		cmd_mode&=~(CMODE_ALL);
		return;
	}
		if(!p_strcmp(buf,FIFCMD_STOP_CLI)) {
		cmd_mode&=~(CMODE_CLI);
		return;
	}
		if(!p_strcmp(buf,FIFCMD_STOP_PRI)) {
		cmd_mode&=~(CMODE_PRI);
		return;
	}
	
}

/***************************************************************************/
/** Checks the clipboards and fifos for changes.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void check_clipboards(gint mode)
{
	gchar *ptext, *ctext, *last;
	int n=0;
	
	/*g_printf("check_clipboards\n"); */
	/*g_mutex_lock(clip_lock); */
	if(fifo->clen){/**we have a command to execute  */
			/*fifo->which should be ID_CMD: */
		if(fifo->dbg) g_printf("Running CMD '%s'\n",fifo->cbuf);
		do_command(fifo->cbuf, fifo->clen);
		if(fifo->dbg) g_printf("mode is 0x%X\n",cmd_mode);
		fifo->clen=0;
		return;
	}	
	if(!(CMODE_ALL & cmd_mode))
		return;
	if(fifo->rlen >0){
		switch(fifo->which){
			case ID_PRIMARY:
				fifo->rlen=validate_utf8_text(fifo->buf, fifo->rlen);
				if(fifo->dbg) g_printf("Setting PRI '%s'\n",fifo->buf);
				update_clipboard(primary, fifo->buf, H_MODE_NEW);
				fifo->rlen=0;
				n=1;
				break;
			case ID_CLIPBOARD:
				fifo->rlen=validate_utf8_text(fifo->buf, fifo->rlen);
				if(fifo->dbg) g_printf("Setting CLI '%s'\n",fifo->buf);
				update_clipboard(clipboard, fifo->buf, H_MODE_NEW);
				n=2;
				fifo->rlen=0;
				break;
			
			default:
				fifo->rlen=validate_utf8_text(fifo->buf, fifo->rlen);
				g_printf("CLIP not set, discarding '%s'\n",fifo->buf);
				fifo->rlen=0;
				break;
		}
	}
	ptext=update_clipboard(primary, NULL, H_MODE_CHECK);
	ctext=update_clipboard(clipboard, NULL, H_MODE_CHECK);
	
	/*g_printf("pt=%s,ct=%s\n",ptext,ctext); */
  /* Synchronization */
  if (get_pref_int32("synchronize"))  {
  
		if(NULL==ptext && NULL ==ctext)
			goto done;
			last=update_clipboard(NULL, NULL, H_MODE_LAST);
		if( NULL != last && 0 != p_strcmp(ptext,ctext)){
			/**last is a copy, of things that may be deallocated  */
			last=strdup(last);
			/*g_printf("Update clipb '%s' '%s' to '%s'\n",ptext,ctext,last);  */
			update_clipboards(last, H_MODE_LIST);
			g_free(last);
		}
		  
	}	
done:	
	return;
	/*g_mutex_unlock(clip_lock);	 */
}
#ifdef HAVE_APPINDICATOR
/***************************************************************************/
/** Check for appindicator.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
gboolean check_for_appindictor( gpointer data)
{ 
	int mode=PROC_MODE_STRSTR;
	if(NULL != appindicator_process && !have_appindicator ){
		g_printf("Looking for '%s'\n",appindicator_process);
		if(get_pref_int32("multi_user"))
			mode|=PROC_MODE_USER_QUALIFY;
		if(proc_find(appindicator_process,mode,NULL) >0){
			have_appindicator=1;
			if(NULL == indicator && show_icon)
				create_app_indicator();	
			return FALSE;
		}
	}
	return TRUE;
}
#endif
/***************************************************************************/
/** Called every CHECK_INTERVAL seconds to check for new items 
\n\b Arguments:
\n\b Returns:
****************************************************************************/
gboolean check_clipboards_tic(gpointer data)
{
	/** gchar *txt=gtk_clipboard_wait_for_text(clipboard);
	g_printf("%s\n",txt);*/
	check_clipboards(H_MODE_CHECK);
#ifdef HAVE_APPINDICATOR
	if(have_appindicator){
		if(NULL == indicator && show_icon)
			create_app_indicator();	
	}
	
#endif
	return TRUE;
}

/* Thread function called for each action performed */
static void *execute_action(void *command)
{
  /* Execute action */
  actions_lock = TRUE;
  if (!have_appindicator && show_icon) {
  gtk_status_icon_set_from_stock((GtkStatusIcon*)status_icon, GTK_STOCK_EXECUTE);
  gtk_status_icon_set_tooltip((GtkStatusIcon*)status_icon, _("Executing action..."));
  }
  if(system((gchar*)command))
  	g_print("sytem command '%s' failed\n",(gchar *)command);
  if (!have_appindicator &&show_icon) {
	gtk_status_icon_set_from_icon_name((GtkStatusIcon*)status_icon, PARCELLITE_ICON);
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
  if (!have_appindicator && show_icon) {
		gtk_status_icon_set_from_icon_name((GtkStatusIcon*)status_icon, PARCELLITE_ICON);
    gtk_status_icon_set_tooltip((GtkStatusIcon*)status_icon, _("Clipboard Manager"));
  }
  actions_lock = FALSE;
}

/* Called when an action is selected from actions menu */
static void action_selected(GtkButton *button, gpointer user_data)
{
  /* Change icon and enable lock */
  actions_lock = TRUE;
  if (!have_appindicator && show_icon) {
    gtk_status_icon_set_from_stock((GtkStatusIcon*)status_icon, GTK_STOCK_EXECUTE);
    gtk_status_icon_set_tooltip((GtkStatusIcon*)status_icon, _("Executing action..."));
  }
  /* Insert clipboard into command (user_data), and prepare it for execution */
  gchar* clipboard_text = gtk_clipboard_wait_for_text(clipboard);
	/*g_print("Got cmd '%s', text '%s'->",(gchar *)user_data,clipboard_text);fflush(NULL);  */
	gchar* command=g_strdup_printf((gchar *)user_data,clipboard_text);
	/*g_print(" '%s'\n",command);fflush(NULL);  */
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
	/*g_print("cmd '%s' argv '%s' '%s' '%s'\n",cmd,argv[1],argv[2],argv[3]);  */
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

/***************************************************************************/
/**  Called when Edit is selected from history menu 
\n\b Arguments:
\n\b Returns:
****************************************************************************/
static void edit_selected(GtkMenuItem *menu_item, gpointer user_data)
{
	struct history_info *h=(struct history_info*)user_data;
	GList* element=NULL;
	struct history_item *c=NULL;
  /* This helps prevent multiple instances */
  if (!gtk_grab_get_current())  {
	  gchar* current_clipboard_text;
    /* Create clipboard buffer and set its text */
    GtkTextBuffer* clipboard_buffer = gtk_text_buffer_new(NULL);
		if(h->wi.tmp1&EDIT_MODE_USE_RIGHT_CLICK){/**use index as pointer to text  */
			element = g_list_nth(history_list, h->wi.index);
			if(NULL !=element){
				c=(struct history_item *)(element->data);
				current_clipboard_text=p_strdup(c->text);
				/*g_printf("%s ",c->text); */
			}
		}	else{
			h->wi.tmp1=0;
			if( NULL != h->element_text){
				current_clipboard_text=p_strdup(h->element_text);
				
			}else{
				current_clipboard_text = gtk_clipboard_wait_for_text(clipboard);
			}	
		}
		
    
    if (current_clipboard_text != NULL)
    {
			TRACE(g_print("Got '%s'\n",current_clipboard_text));
      gtk_text_buffer_set_text(clipboard_buffer, current_clipboard_text, -1);
    }	 else	 {
			g_printf("NULL text to edit. Nothing to do.\n");
			return;
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
		
    if (gtk_dialog_run((GtkDialog*)dialog) == GTK_RESPONSE_ACCEPT) {
			GtkTextIter start, end;
      gtk_text_buffer_get_start_iter(clipboard_buffer, &start);
      gtk_text_buffer_get_end_iter(clipboard_buffer, &end);
      gchar* new_clipboard_text = gtk_text_buffer_get_text(clipboard_buffer, &start, &end, TRUE);
			
			if(h->wi.tmp1&EDIT_MODE_USE_RIGHT_CLICK){/**save changes to the history h.wi.index has the element */
				struct history_item *n=new_clip_item(c->type, strlen(new_clipboard_text), new_clipboard_text);
				n->flags=c->flags;
				g_free(element->data);
				element->data=n;
			}	else{ /* Save changes done to the clipboard */
				update_clipboard(clipboard, new_clipboard_text, H_MODE_NEW);
			}
			if(NULL != new_clipboard_text)
				g_free(new_clipboard_text);	
    }
    gtk_widget_destroy(dialog);
    g_free(current_clipboard_text);
  }
	else TRACE(g_print("gtk_grab_get_current returned !0\n"));

}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
gboolean history_item_right_click_on_edit(GtkWidget *menuitem, gpointer data)
{
	struct history_info *h=(struct history_info*)data;
	h->wi.tmp1|=EDIT_MODE_USE_RIGHT_CLICK;
	edit_selected((GtkMenuItem *)menuitem, data);
}

/***************************************************************************/
/** Paste all. Grab all of the history and paste it to the clipboard.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
gboolean history_item_right_click_on_copy_all(GtkWidget *menuitem, gpointer data)
{
	GList *element;
	gchar*str=NULL;
	gchar*last=NULL;
	gchar*delim= g_strcompress(get_pref_string("persistent_delim"));
	int which;
	struct history_info *h=(struct history_info*)data;
	/**if persistent and this history is persistent, only get persistent.  
	FIXME: this will only work with separarate set.
	*/
	which=(get_pref_int32("persistent_history") && h->histno == HIST_DISPLAY_PERSISTENT);
	for (element = history_list; element != NULL; element = element->next) {
		struct history_item *c=(struct history_item *)(element->data);
		if(which && !(c->flags&CLIP_TYPE_PERSISTENT))
			continue;
		if(NULL ==str)
			str=c->text;
		else {
			if(NULL == delim)
				str=g_strconcat(str,c->text,NULL);
			else
				str=g_strconcat(str,delim,c->text,NULL);
			if(NULL != last)
				g_free(last);
			last=str;
		}
	}	
	if(NULL != last){
		update_clipboards(str,H_MODE_IGNORE);
		g_free(last);
	}
		
	return TRUE;
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
gboolean  handle_history_item_right_click (int i, gpointer data)
{
  /* we passed the view as userdata when we connected the signal */
	struct history_info *h=(struct history_info*)data;
	struct history_item *c=NULL;
	if(NULL !=h ){
		GList* element = g_list_nth(history_list, h->wi.index);
		if(NULL !=element){
			c=(struct history_item *)(element->data);
			/*g_printf("%s ",c->text); */
		}
	}
	switch(i){
		case HIST_MOVE_TO_CANCEL:
/*			g_print("canceled\n"); */
			break;
		case HIST_MOVE_TO_OK:
/*			g_printf("Move to"); */
			handle_marking(h,h->wi.item,h->wi.index,OPERATE_PERSIST);
			break;
	}
	/*gtk_widget_grab_focus(h->menu); */
	return TRUE;
}
/**callback wrappers for the above function  */
gboolean  history_item_right_click_on_move (GtkWidget *menuitem, gpointer data)
{
	return handle_history_item_right_click(HIST_MOVE_TO_OK,data);
}
gboolean history_item_right_click_on_cancel (GtkWidget *menuitem, gpointer data)
{
	return handle_history_item_right_click(HIST_MOVE_TO_CANCEL,data);
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
if (event->type == GDK_BUTTON_PRESS  &&  event->button == 3)
	view_popup_menu(treeview, event, userdata);
	h->wi.index contains the element of the item clicked on.
****************************************************************************/
void  history_item_right_click (struct history_info *h, GdkEventKey *e, gint index)
{
  GtkWidget *menu, *menuitem;
  menu = gtk_menu_new();
	struct history_item *c=NULL;
	if(NULL !=h ){
		GList* element = g_list_nth(history_list, h->wi.index);
		if(NULL !=element){
			c=(struct history_item *)(element->data);
			/*g_printf("%s ",c->text); */
		}
	}
	menuitem = gtk_menu_item_new_with_label("Copy All to Clip");
  g_signal_connect(menuitem, "activate", (GCallback) history_item_right_click_on_copy_all, (gpointer)h);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	if(get_pref_int32("persistent_history")){
		/*g_printf("CreatehistR\n"); */
		if(NULL != c){
			if(c->flags & CLIP_TYPE_PERSISTENT)
				menuitem = gtk_menu_item_new_with_label("Move To Normal");
			else
				menuitem = gtk_menu_item_new_with_label("Move To Persistent");
		}	else
	    menuitem = gtk_menu_item_new_with_label("Move To?");
	
	  g_signal_connect(menuitem, "activate",(GCallback) history_item_right_click_on_move, (gpointer)h);
	  gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
		
		menuitem = gtk_menu_item_new_with_label("Edit");
	  g_signal_connect(menuitem, "activate", (GCallback) history_item_right_click_on_edit, (gpointer)h);
	  gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);	
		
		menuitem = gtk_menu_item_new_with_label("Cancel");
	  g_signal_connect(menuitem, "activate", (GCallback) history_item_right_click_on_cancel, (gpointer)h);
	  gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);	
	}

  gtk_widget_show_all(menu);

  /* Note: event can be NULL here when called from view_onPopupMenu;
   *  gdk_event_get_time() accepts a NULL argument */
   gtk_menu_popup(GTK_MENU(menu), h->menu, NULL, NULL, NULL,
									0,
                 /*(e != NULL) ? ((GdkEventButton *)e)->button : 0, */
                 gdk_event_get_time((GdkEvent*)e));
	/*gtk_widget_grab_focus(menu);  */
}

/* Called when Clear is selected from history menu */
static void clear_selected(GtkMenuItem *menu_item, gpointer user_data)
{
  /* Check for confirm clear option */
  if (get_pref_int32("confirm_clear"))  {
    GtkWidget* confirm_dialog = gtk_message_dialog_new(NULL,
                                                       GTK_DIALOG_MODAL,
                                                       GTK_MESSAGE_OTHER,
                                                       GTK_BUTTONS_OK_CANCEL,
                                                       _("Clear the history?"));
    
    if (gtk_dialog_run((GtkDialog*)confirm_dialog) == GTK_RESPONSE_OK)    {
      /* Clear history and free history-related variables */
			clear_history();
      
		  update_clipboard(primary, "", H_MODE_INIT);
      update_clipboard(clipboard, "", H_MODE_INIT);
    }
    gtk_widget_destroy(confirm_dialog);
  }
  else  {
    /* Clear history and free history-related variables */
    g_list_free(history_list);
    history_list = NULL;
    save_history();
    update_clipboard(primary, "", H_MODE_INIT);
    update_clipboard(clipboard, "", H_MODE_INIT);
  }
}

/* Called when About is selected from right-click menu */
static void show_about_dialog(GtkMenuItem *menu_item, gpointer user_data)
{
  /* This helps prevent multiple instances */
  if (!gtk_grab_get_current())
  {
    const gchar* authors[] = {"Gilberto \"Xyhthyx\" Miralla <xyhthyx@gmail.com>\nDoug Springer <gpib@rickyrockrat.net>", NULL};
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
    #ifdef HAVE_CONFIG_H	/**VER=555; sed "s#\(.*\)svn.*\".*#\1svn$VER\"#" config.h  */
    gtk_about_dialog_set_version((GtkAboutDialog*)about_dialog, VERSION);
    #endif
    gtk_about_dialog_set_comments((GtkAboutDialog*)about_dialog,
                                _("Lightweight GTK+ clipboard manager."));
    
    gtk_about_dialog_set_website((GtkAboutDialog*)about_dialog,
                                 "http://parcellite.sourceforge.net");
    
    gtk_about_dialog_set_copyright((GtkAboutDialog*)about_dialog, "Copyright (C) 2007, 2008 Gilberto \"Xyhthyx\" Miralla\n"
	   "Copyright (C) 2010-2013 Doug Springer");
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
  if (!gtk_grab_get_current()){
		 /* Show the preferences dialog */
    show_preferences(0);
	}

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
  g_signal_connect((GObject*)menu,"selection-done", (GCallback)gtk_widget_destroy, NULL);
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
    gtk_label_set_ellipsize((GtkLabel*)item_label, get_pref_int32("ellipsize"));
    gtk_label_set_width_chars((GtkLabel*)item_label, 30);
    /* Making bold... */
    gchar* bold_text = g_markup_printf_escaped("<b>%s</b>", text);
    gtk_label_set_markup((GtkLabel*)item_label, bold_text);
    g_free(bold_text);
    /* Append menu item */
    g_signal_connect((GObject*)menu_item, "select", (GCallback)gtk_menu_item_deselect, NULL);
    gtk_menu_shell_append((GtkMenuShell*)menu, menu_item);
	  g_free(text);
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
  gchar* path = g_build_filename(g_get_user_data_dir(), ACTIONS_FILE, NULL);
	printf("got path '%s'\n",path); fflush(NULL);
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
      
      if(0 ==fread(&size, 4, 1, actions_file))
      	g_print("3:got 0 items from fread\n");
      /* Read command */
      gchar* command = (gchar*)g_malloc(size + 1);
      if(0 ==fread(command, size, 1, actions_file))
      	g_print("4:got 0 items from fread\n");
      command[size] = '\0';
		  g_print("name='%s' cmd='%s'\n",name,command);
      if(0 ==fread(&size, 4, 1, actions_file))
      	g_print("5:got 0 items from fread\n");
      /* Append the action */
      gtk_menu_shell_append((GtkMenuShell*)menu, menu_item);
      g_signal_connect((GObject*)menu_item,        "activate",
                       (GCallback)action_selected, (gpointer)command);      
		  g_free(name);
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

/***************************************************************************/
/** Called just before destroying history menu.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
static gboolean selection_done(GtkMenuShell *menushell, gpointer user_data) 
{
	struct history_info *h=(struct history_info *)user_data;
	/*g_print("Got selection_done shell %p data %p\n",menushell,user_data);  */
	if(NULL != h && NULL != h->delete_list){/**have a list of items to delete.  */
		GList *i;
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
		if (get_pref_int32("save_history"))
		  save_history();
		goto done;
	}	
	/*g_print("selection_active=%d\n",selection_active); */
	/*g_print("Got selection_done\n"); */
	if(h->change_flag && get_pref_int32("save_history")){
		save_history();
		h->change_flag=0;
	}
		  
done:
	gtk_widget_destroy((GtkWidget *)menushell);
}

/***************************************************************************/
/** Set the background color of the widget.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
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

/**postition the history dialog  - should only be called if get_pref_int32("history_pos") is set */
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
		if(get_pref_int32("history_pos")){
			int xx,yy;
			if(get_pref_int32("history_x") > get_pref_int32("item_length") )
				xx=get_pref_int32("history_x")-get_pref_int32("item_length");
			else
				xx=1;
			if(get_pref_int32("history_y") > get_pref_int32("history_limit") )
				yy=get_pref_int32("history_y")-get_pref_int32("history_limit");
			else
				yy=1;
			if(NULL !=x) *x=xx;
			if(NULL !=y) *y=yy;	
			TRACE(g_print("x=%d, y=%d\n",xx,yy));
		}
		
	}
	
}

/***************************************************************************/
/** This handles events for the history menu, which is the parent of each
item.
\n\b Arguments:
\n\b Returns: FALSE if key was not handled, TRUE if it was.
You get two key presses if you return FALSE.
****************************************************************************/
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
	
	if(0&& NULL != e ){
		if(GDK_MOTION_NOTIFY==e->type)
			return FALSE;
    printf("krc (%x) S%x T%x C%x,SE%x, G%x, W%p, wdg%p",
		e->keyval,e->state,e->type,
		e->hardware_keycode,e->send_event,e->group,e->window,w);
		if(GDK_DRAG_ENTER == e->type || GDK_DRAG_LEAVE==e->type){
			printf(" Drag %s\n",GDK_DRAG_ENTER == e->type?"ENTER":"LEAVE");
		}
		if(GDK_BUTTON_RELEASE==e->type || GDK_BUTTON_PRESS==e->type  ){
			GdkEventButton *b=(GdkEventButton *)e;
			printf(" button %d State 0x%x\n",b->button, b->state);
			if(GDK_BUTTON_RELEASE==e->type && 3 == b->button){
				/*toggle-size-request", */
				/*allow item_selected to get called  */
				/*return FALSE;   */
			}
			/*return TRUE; */
		}
		printf("\n");
		fflush(NULL);
  }
	
	/**serves as init for keysearch  */
	if(NULL ==w && NULL==e && NULL == user){
		if(NULL != kstr)
			g_free(kstr);
		kstr=g_strnfill(KBUF_SIZE+8,0);
		idx=0;
		return FALSE;
	}else if(NULL == kstr){
		g_print("kstr null. Not init\n");
		return FALSE;
	}
  if(NULL == e){
    g_print("No Event!\n");
    return FALSE;
  }
		
  if(0 == get_pref_int32("type_search"))/**searching is turned off  */
    return FALSE;
	/**ignore left-clicks  */
	if(GDK_BUTTON_RELEASE==e->type && 3 == ((GdkEventButton *)e)->button)
		return FALSE;
		
  if(GDK_KEY_PRESS == e->type && ' ' == e->keyval) /**ignore space presses  */
    return TRUE;
	if(GDK_KEY_PRESS == e->type)
		return FALSE;
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
	/**ignore Ctrl-Alt  */
	if((GDK_CONTROL_MASK|GDK_MOD1_MASK)==(e->state & (GDK_CONTROL_MASK|GDK_MOD1_MASK)))
		return FALSE;
	if( e->state & GDK_MOD1_MASK){/**alt key pressed  */
		if(e->keyval == 'e'){
			TRACE(g_print("Alt-E\n"));
			gtk_grab_remove(w);
		  edit_selected((GtkMenuItem *)h, (gpointer)h);
		}
			
		else if(e->keyval == 'c'){
			TRACE(g_print("Alt-C\n"));
			clear_selected(NULL, NULL); 
		}	else{
			TRACE(g_print("Ignoring Alt-%c (0x%02x) state 0x%x",e->keyval,e->keyval,e->state));
		}
		return FALSE;
	}	/**end alt key pressed  */
	if(e->state & GDK_CONTROL_MASK)	/**ignore control keys  */
		return FALSE;
	if(e->state &GDK_SHIFT_MASK   && get_pref_int32("case_search"))	/**ignore shift   */
		return FALSE;
	if( GDK_EXPOSE== e->type || GDK_BUTTON_RELEASE==e->type)	/**fix bug 3560995, item 1/2, red clipboard.  */
		return FALSE;
	if(e->keyval == 0xff08){/**backspace  */
//		g_printf("0x%x bs %d ",e->type,idx);
		if(idx)
			--idx;
    else if( NULL != h->clip_item){
      gtk_menu_shell_select_item((GtkMenuShell *)h->menu,(GtkWidget *)h->clip_item);
    }
    set_widget_bg(NULL,h->menu);
		kstr[idx]=0;
	//	g_printf(" %d\n",idx);
		return TRUE;
	}	/**end backspace  */
	if( e->keyval == 0xffe1 || e->keyval == 0xffe2){
		printf("Ignoring key '%c' 0x%02x\n",e->keyval,e->keyval);
		TRACE(g_print("Ignoring key '%c' 0x%02x\n",e->keyval,e->keyval));	
		return FALSE;
	}
  if(e->keyval >= 0xff50 && e->keyval <= 0xff57) /**arrow keys, home,end,pgup,pgdwn  */
  	return FALSE;
  	
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
					if(get_pref_int32("case_search"))
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

/***************************************************************************/
/** Set clipboard from history list.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void set_clipboard_text(struct history_info *h, GList *element)
{
	/*g_mutex_lock(clip_lock); */
	if(NULL == find_h_item(h->delete_list,NULL,element)){	/**not in our delete list  */
		/**make a copy of txt, because it gets freed and re-allocated.  */
		gchar *txt=p_strdup(((struct history_item *)(element->data))->text);
		/*g_printf("set_clip_text %s\n",txt);  */
		if(get_pref_int32("use_copy") )
			update_clipboard(clipboard, txt, H_MODE_LIST);
		if(get_pref_int32("use_primary"))
	  	update_clipboard(primary, txt, H_MODE_LIST);	
		g_free(txt);
	}
  g_signal_emit_by_name ((gpointer)h->menu,"selection-done");
	/*g_printf("set_clip_text done\n");  */
	/*g_mutex_unlock(clip_lock); */
	
	if (get_pref_int32("automatic_paste")) { /** mousedown 2 */
		gchar *action=NULL;
		if(get_pref_int32("auto_mouse"))
			action="mousedown 2 && xdotool mouseup 2'";
		else if(get_pref_int32("auto_key"))
			action="key ctrl+v'";
		if(NULL == action)
			return;
		/**from clipit 1.4.1 */
    gchar* cmd = g_strconcat("/bin/sh -c 'xdotool ", action, NULL);
    GPid pid;
    gchar **argv;
    g_shell_parse_argv(cmd, NULL, &argv, NULL);
    g_free(cmd);
    g_spawn_async(NULL, argv, NULL, G_SPAWN_DO_NOT_REAP_CHILD, NULL, NULL, &pid, NULL);
    g_child_watch_add(pid, (GChildWatchFunc)action_exit, NULL);
    g_strfreev(argv);
  }/**end from clipit 1.4.1 */
}


/***************************************************************************/
/** This handles the events for each item in the history menu.
\n\b Arguments:	 user is element number
\n\b Returns:
****************************************************************************/
static gboolean my_item_event (GtkWidget *w,GdkEventKey *e, gpointer user)
{
	static struct history_info *h=NULL;
	GtkWidget *menu=NULL;
	
	if(NULL==w && NULL==e){
		h=(struct history_info *)user;
	  /*g_print("my_item_event: Set menu to %p\n",h); */
		return FALSE;
	}
	if(NULL == h)
		return FALSE;	
	menu=h->menu;
	/**check for enter  */
	if(GDK_MOTION_NOTIFY == e->type)
		return FALSE;
	/*printf("my_item_event: T 0x%x S 0x%x ",e->type,e->state); */
	if(NULL !=h && GDK_ENTER_NOTIFY ==e->type ){/**add to delete   */
		GdkEventCrossing *enter=(GdkEventCrossing *)e;
		/*printf("state 0x%x\n",enter->state); */
		/**use shift and right-click  */
		if(GDK_SHIFT_MASK&enter->state && GDK_BUTTON3_MASK&enter->state)
			handle_marking(h,w,GPOINTER_TO_INT(user),OPERATE_DELETE);
	}
	if(GDK_KEY_PRESS == e->type){
		GdkEventKey *k=	(GdkEventKey *)e;
		printf("key press %d (0x%x)\n",e->keyval,e->keyval); 
		fflush(NULL);
	}
	if(GDK_BUTTON_RELEASE==e->type){
		GdkEventButton *b=(GdkEventButton *)e;
		GList* element = g_list_nth(history_list, GPOINTER_TO_INT(user));
		/*printf("type %x State 0x%x val %x %p '%s'\n",e->type, b->state,b->button,w,(gchar *)((struct history_item *(element->data))->text));  */
		if(3 == b->button){ /**right-click  */
			gboolean rtn;
			if(GDK_CONTROL_MASK&b->state){
				handle_marking(h,w,GPOINTER_TO_INT(user),OPERATE_DELETE);
			}else{
				 if((GDK_CONTROL_MASK|GDK_SHIFT_MASK)&b->state)
					return FALSE;
				/*g_print("Calling popup\n");  */
	      h->wi.event=e;
	      h->wi.item=w;
				h->wi.index=GPOINTER_TO_INT(user);
				if(get_pref_int32("persistent_history"))
		      history_item_right_click(h,e,GPOINTER_TO_INT(user));
			}
			return TRUE;
		}else if( 1 == b->button){
		  /* Get the text from the right element and set as clipboard */
			set_clipboard_text(h,element);
		}	
		fflush(NULL);
	}
	
	return FALSE;
}

/***************************************************************************/
/** Attempt to handle enter key behaviour.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
static void item_selected(GtkMenuItem *menu_item, gpointer user_data)
{
	static struct history_info *h=NULL;
	if(NULL ==menu_item){
		h=(struct history_info *)user_data;
		return;
	}
		
		
	GdkEventKey *k=(GdkEventKey *)gtk_get_current_event();
	GList* element = g_list_nth(history_list, GPOINTER_TO_INT(user_data));
	/*g_print ("item_selected '%s' type %x val %x\n",(gchar *)((struct history_item *(element->data))->text),k->type, k->keyval);  */
	if(0xFF0d == k->keyval && GDK_KEY_PRESS == k->type){
		set_clipboard_text(h,element);
	}
}	

/***************************************************************************/
/** Write the elements to the menu.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void write_history_menu_items(GList *list, GtkWidget *menu)
{
	GList *element;
	if(NULL == list)
		return;
	for (element = list; element != NULL; element = element->next) 
			gtk_menu_shell_append((GtkMenuShell*)menu,element->data);	
}
/***************************************************************************/
/**  Called when status icon is left-clicked or action key hit.
\n\b Arguments:
\n\b Returns:
****************************************************************************/

static gboolean show_history_menu(gpointer data)
{
  /* Declare some variables */
  GtkWidget *menu,       *menu_item,
            *menu_image, *item_label;
  static struct history_info h;
	gint nok,pok;
	h.histno=GPOINTER_TO_INT(data);/**persistent or normal history  */
	h.change_flag=0;
	h.element_text=NULL;
  /**init our keystroke function  */
	key_release_cb(NULL,NULL,NULL);
	GList *element, *persistent=NULL;
	GList *lhist=NULL;
	int single_line=get_pref_int32("single_line");
	
  /* Create the menu */
  menu = gtk_menu_new();
	
  h.menu=hmenu=menu;
  h.clip_item=NULL;
	h.delete_list=NULL;
	h.persist_list=NULL;
	h.wi.tmp1=0; /** used to tell edit what we are to edit  */
	/*g_print("histmen %p\n",menu); */
	my_item_event(NULL,NULL,(gpointer)&h); /**init our function  */
	item_selected(NULL,(gpointer)&h);	/**ditto  */
	gtk_menu_shell_set_take_focus((GtkMenuShell *)menu,TRUE); /**grab keyboard focus  */
	/*g_signal_connect((GObject*)menu, "selection-done", (GCallback)selection_done, gtk_menu_get_attach_widget (menu));  */
	g_signal_connect((GObject*)menu, "cancel", (GCallback)selection_done, &h); 
	g_signal_connect((GObject*)menu, "selection-done", (GCallback)selection_done, &h); 
  /*g_signal_connect((GObject*)menu, "selection-done", (GCallback)gtk_widget_destroy, NULL); */
	/**Trap key events  */
/*	g_signal_connect((GObject*)menu, "key-release-event", (GCallback)key_release_cb, (gpointer)&h); */
  g_signal_connect((GObject*)menu, "event", (GCallback)key_release_cb, (gpointer)&h);
	/**trap mnemonic events  */
	/*g_signal_connect((GObject*)menu, "mnemonic-activate", (GCallback)key_release_cb, (gpointer)menu);  */

  /* -------------------- */
  /*gtk_menu_shell_append((GtkMenuShell*)menu, gtk_separator_menu_item_new()); */
  /* Items */
  if ((history_list != NULL) && (history_list->data != NULL)) {
    /* Declare some variables */
		
    gint element_number = 0;
    gchar* primary_temp = gtk_clipboard_wait_for_text(primary);
    gchar* clipboard_temp = gtk_clipboard_wait_for_text(clipboard);
    /* Reverse history if enabled */
    if (0 && get_pref_int32("reverse_history")) {
      /*history_list = g_list_reverse(history_list); */
      element_number = g_list_length(history_list) - 1;
    }
    /* Go through each element and adding each */
    for (element = history_list; element != NULL; element = element->next) {
			struct history_item *c=(struct history_item *)(element->data);
			gchar* hist_text=c->text;
			if(!(HIST_DISPLAY_PERSISTENT&h.histno) && (c->flags & CLIP_TYPE_PERSISTENT))
				goto next_loop;
			else if( !(HIST_DISPLAY_NORMAL&h.histno) && !(c->flags & CLIP_TYPE_PERSISTENT))
				goto next_loop;
      GString* string = g_string_new(hist_text);
		  glong len=g_utf8_strlen(string->str, string->len);
      /* Ellipsize text */
      if (len > get_pref_int32("item_length")) {
        switch (get_pref_int32("ellipsize")) {
          case PANGO_ELLIPSIZE_START:
          	string = g_string_erase(string, 0, g_utf8_offset_to_pointer(string->str, len - get_pref_int32("item_length")) - string->str);
            /*string = g_string_erase(string, 0, string->len-(get_pref_int32("item_length"))); */
            string = g_string_prepend(string, "...");
            break;
          case PANGO_ELLIPSIZE_MIDDLE:
          	{
						gchar* p1 = g_utf8_offset_to_pointer(string->str, get_pref_int32("item_length") / 2);
            gchar* p2 = g_utf8_offset_to_pointer(string->str, len - get_pref_int32("item_length") / 2);
            string = g_string_erase(string, p1 - string->str, p2 - p1);
            string = g_string_insert(string, p1 - string->str, "...");
            /** string = g_string_erase(string, (get_pref_int32("item_length")/2), string->len-(get_pref_int32("item_length")));
            string = g_string_insert(string, (string->len/2), "...");*/	
						}
            break;
          case PANGO_ELLIPSIZE_END:
          	string = g_string_truncate(string, g_utf8_offset_to_pointer(string->str, get_pref_int32("item_length")) - string->str);
            /*string = g_string_truncate(string, get_pref_int32("item_length")); */
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
		
  		g_signal_connect((GObject*)menu_item,      "event",
                       (GCallback)my_item_event, GINT_TO_POINTER(element_number));
		  g_signal_connect((GObject*)menu_item,      "activate",
                       (GCallback)item_selected, GINT_TO_POINTER(element_number));
		
      
      /* Modify menu item label properties */
      item_label = gtk_bin_get_child((GtkBin*)menu_item);
			if(single_line){
				/*gtk_label_set_line_wrap  */
				gtk_label_set_single_line_mode((GtkLabel*)item_label, TRUE);
			}	else{
				gtk_label_set_single_line_mode((GtkLabel*)item_label, FALSE);
			}
      
      /* Check if item is also clipboard text and make bold */
      if ((clipboard_temp) && (p_strcmp(hist_text, clipboard_temp) == 0))
      {
        gchar* bold_text = g_markup_printf_escaped("<b>%s</b>", string->str);
        gtk_label_set_markup((GtkLabel*)item_label, bold_text);
        g_free(bold_text);
        h.clip_item=menu_item;
				h.element_text=hist_text;
      }
      else if ((primary_temp) && (p_strcmp(hist_text, primary_temp) == 0))
      {
        gchar* italic_text = g_markup_printf_escaped("<i>%s</i>", string->str);
        gtk_label_set_markup((GtkLabel*)item_label, italic_text);
        g_free(italic_text);
        h.clip_item=menu_item;
			  h.element_text=hist_text;
      }
			if(get_pref_int32("persistent_history") && c->flags &CLIP_TYPE_PERSISTENT){
				persistent = g_list_prepend(persistent, menu_item);
				/*g_printf("persistent %s\n",c->text); */
			}	else{
				/* Append item */
				lhist = g_list_prepend(lhist, menu_item);
	      
			}
				
      
      /* Prepare for next item */
      g_string_free(string, TRUE);
next_loop:
      /** if (get_pref_int32("reverse_history"))
        element_number--;
      else*/
        element_number++;
    }	/**end of for loop for each history item  */
    /* Cleanup */
    g_free(primary_temp);
    g_free(clipboard_temp);
    /* Return history to normal if reversed */
    /** if (get_pref_int32("reverse_history"))
      history_list = g_list_reverse(history_list);*/
  }
  else
  {
    /* Nothing in history so adding empty */
    menu_item = gtk_menu_item_new_with_label(_("Empty"));
    gtk_widget_set_sensitive(menu_item, FALSE);
    gtk_menu_shell_append((GtkMenuShell*)menu, menu_item);
  }
	if (!get_pref_int32("reverse_history")) {
		lhist = g_list_reverse(lhist);
		persistent = g_list_reverse(persistent);
	}	
/**now actually add them from the list  */	
	if(get_pref_int32("persistent_history")){
		if(get_pref_int32("persistent_on_top")){
			write_history_menu_items(persistent,menu);
			gtk_menu_shell_append((GtkMenuShell*)menu, gtk_separator_menu_item_new()); 
			write_history_menu_items(lhist,menu);
		}	else{
			write_history_menu_items(lhist,menu);
			gtk_menu_shell_append((GtkMenuShell*)menu, gtk_separator_menu_item_new()); 
			write_history_menu_items(persistent,menu);
		}
	}else {	/**normal old operation, forget about persistence.  */
		write_history_menu_items(lhist,menu);
	}
	
  /* -------------------- */
  gtk_menu_shell_append((GtkMenuShell*)menu, gtk_separator_menu_item_new());
	
	if(get_pref_int32("type_search")){
    /* Edit clipboard */
		h.title_item = gtk_image_menu_item_new_with_label( _("Use Alt-E to edit, Alt-C to clear") );
    menu_image = gtk_image_new_from_stock(GTK_STOCK_EDIT, GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image((GtkImageMenuItem*)h.title_item, menu_image);
    gtk_menu_shell_append((GtkMenuShell*)menu, h.title_item);    
  }else{
    menu_item = gtk_image_menu_item_new_with_mnemonic(_("_Edit Clipboard"));
    menu_image = gtk_image_new_from_stock(GTK_STOCK_EDIT, GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image((GtkImageMenuItem*)menu_item, menu_image);
		g_signal_connect((GObject*)menu_item, "activate", (GCallback)edit_selected, (gpointer)&h);
    gtk_menu_shell_append((GtkMenuShell*)menu, menu_item);
		menu_item = gtk_image_menu_item_new_with_mnemonic(_("_Clear"));
		/* Clear */
	  menu_image = gtk_image_new_from_stock(GTK_STOCK_CLEAR, GTK_ICON_SIZE_MENU);
	  gtk_image_menu_item_set_image((GtkImageMenuItem*)menu_item, menu_image);
		g_signal_connect((GObject*)menu_item, "activate", (GCallback)clear_selected, NULL);
	  gtk_menu_shell_append((GtkMenuShell*)menu, menu_item);
  }
	g_list_free(lhist);
	g_list_free(persistent);
  /* Popup the menu... */
  gtk_widget_show_all(menu);
  gtk_menu_popup((GtkMenu*)menu, NULL, NULL, get_pref_int32("history_pos")?postition_history:NULL, NULL, 1, gtk_get_current_event_time());
	/**set last entry at first -fixes bug 2974614 */
	if(get_pref_int32("reverse_history") && NULL != h.clip_item)
		gtk_menu_shell_select_item((GtkMenuShell*)menu,h.clip_item);
	else	
		gtk_menu_shell_select_first((GtkMenuShell*)menu, TRUE);
  /* Return FALSE so the g_timeout_add() function is called only once */
  return FALSE;
}


/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void _show_history_menu (GtkMenuItem *m, gpointer data)
{
	g_timeout_add(POPUP_DELAY, show_history_menu, GINT_TO_POINTER(figure_histories()));
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
GtkWidget *create_parcellite_menu(guint button, guint activate_time)
{
  /* Declare some variables */
  GtkWidget *menu, *menu_item;
  
  /* Create the menu */
  menu = gtk_menu_new();
  /*g_signal_connect((GObject*)menu, "selection-done", (GCallback)gtk_widget_destroy, NULL); */
  /* About */
  menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ABOUT, NULL);
  g_signal_connect((GObject*)menu_item, "activate", (GCallback)show_about_dialog, NULL);
  gtk_menu_shell_append((GtkMenuShell*)menu, menu_item);
	
	/* Save History */
  menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_SAVE_AS, NULL);
  g_signal_connect((GObject*)menu_item, "activate", (GCallback)history_save_as, NULL);
	gtk_widget_set_tooltip_text(menu_item, _("Save History as a text file. Prepends xHIST_0000 to each entry. x is either P(persistent) or N (normal)"));
  gtk_menu_shell_append((GtkMenuShell*)menu, menu_item);
  /* Preferences */
  menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_PREFERENCES, NULL);
  g_signal_connect((GObject*)menu_item, "activate", (GCallback)preferences_selected, NULL);
  gtk_menu_shell_append((GtkMenuShell*)menu, menu_item);
	if(have_appindicator){
			/* History */
		GtkWidget *img=gtk_image_new_from_icon_name(PARCELLITE_ICON,GTK_ICON_SIZE_MENU); 
	  menu_item = gtk_image_menu_item_new_with_mnemonic(_("_History"));
		gtk_image_menu_item_set_image((GtkImageMenuItem *)menu_item,img);
	  g_signal_connect((GObject*)menu_item, "activate", (GCallback)_show_history_menu, NULL);
	  gtk_menu_shell_append((GtkMenuShell*)menu, menu_item);
	}
	
  /* -------------------- */
  gtk_menu_shell_append((GtkMenuShell*)menu, gtk_separator_menu_item_new());
  /* Quit */
  menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, NULL);
  g_signal_connect((GObject*)menu_item, "activate", (GCallback)quit_selected, NULL);
  gtk_menu_shell_append((GtkMenuShell*)menu, menu_item);
  /* Popup the menu... */
  gtk_widget_show_all(menu);
  gtk_menu_popup((GtkMenu*)menu, NULL, NULL, NULL, NULL, button, activate_time);	
	return menu;
}

/* Called when status icon is right-clicked */
static void  show_parcellite_menu(GtkStatusIcon *status_icon, guint button, guint activate_time,  gpointer data)
{
	create_parcellite_menu(button, activate_time);
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
gboolean show_parcellite_menu_wrapper(gpointer data)
{
	create_parcellite_menu(0, gtk_get_current_event_time());
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
gint figure_histories(void)
{
	gint i;
	if(get_pref_int32("persistent_history")){ 
		if(get_pref_int32("persistent_separate"))
			i=HIST_DISPLAY_NORMAL;
		else
			i=HIST_DISPLAY_PERSISTENT|HIST_DISPLAY_NORMAL;
	}else 
		i=HIST_DISPLAY_NORMAL;
	/*g_printf("Using history 0x%X\n",i); */
	return i;
}
#ifdef HAVE_APPINDICATOR
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void create_app_indicator(void)
{
	/* Create the menu */
	indicator_menu = create_parcellite_menu(0,gtk_get_current_event_time());
	/* check if we need to create the indicator or just refresh the menu */
	if(NULL == indicator) {
		indicator = app_indicator_new("parcellite", "parcellite", APP_INDICATOR_CATEGORY_APPLICATION_STATUS);
		app_indicator_set_status (indicator, APP_INDICATOR_STATUS_ACTIVE);
		
		app_indicator_set_attention_icon (indicator,"parcellite");
	}
	app_indicator_set_menu (indicator, GTK_MENU (indicator_menu));
}
#endif
	
/* Called when status icon is left-clicked */
static void status_icon_clicked(GtkStatusIcon *status_icon, gpointer user_data)
{
  /* Check what type of click was recieved */
  GdkModifierType state;
  gtk_get_current_event_state(&state);
  /* Control click */
  if (state == GDK_MOD2_MASK+GDK_CONTROL_MASK || state == GDK_CONTROL_MASK)
  {
		g_printf("Got Ctrl-click\n");
    if (actions_lock == FALSE)
    {
      g_timeout_add(POPUP_DELAY, show_actions_menu, NULL);
    }
  }
  /* Normal click */
  else
  {
    g_timeout_add(POPUP_DELAY, show_history_menu, GINT_TO_POINTER(figure_histories()));
  }
}

/* Called when history global hotkey is pressed */
void history_hotkey(char *keystring, gpointer user_data)
{
  g_timeout_add(POPUP_DELAY, show_history_menu, GINT_TO_POINTER(figure_histories()));
}
/* Called when persistent history global hotkey is pressed */
void phistory_hotkey(char *keystring, gpointer user_data)
{
	if(get_pref_int32("persistent_history") && get_pref_int32("persistent_separate"))
    g_timeout_add(POPUP_DELAY, show_history_menu, GINT_TO_POINTER(HIST_DISPLAY_PERSISTENT));
}

/* Called when actions global hotkey is pressed */
void actions_hotkey(char *keystring, gpointer user_data)
{
  g_timeout_add(POPUP_DELAY, show_actions_menu, NULL);
}

/* Called when actions global hotkey is pressed */
void menu_hotkey(char *keystring, gpointer user_data)
{
#ifdef HAVE_APPINDICATOR
	/*create_app_indicator(); */
	create_parcellite_menu(0, gtk_get_current_event_time());
	/** GtkWidget * w=create_parcellite_menu(0, gtk_get_current_event_time());
	app_indicator_set_menu (indicator, GTK_MENU (w));*/
#else
  show_parcellite_menu(status_icon, 0, 0, NULL);
#endif
}

/* Startup calls and initializations */
static void parcellite_init()
{
/* Create clipboard */
  primary = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
  clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
  
	if(FALSE ==g_thread_supported()){
		g_printf("g_thread not init!\n");
	}
	clip_lock= g_mutex_new();
	hist_lock= g_mutex_new();
	g_mutex_unlock(clip_lock);

  show_icon=!get_pref_int32("no_icon");
  /* Read history */
  if (get_pref_int32("save_history")){
		gchar *x;
		/*g_printf("Calling read_hist\n"); */
		read_history();
		if(NULL != history_list){
			struct history_item *c;
			c=(struct history_item *)(history_list->data);	
			if(NULL == (x=is_clipboard_empty(primary)))
				update_clipboard(primary,c->text,H_MODE_LIST);
			else g_free (x);
			if(NULL == (x=is_clipboard_empty(clipboard)))
				update_clipboard(clipboard,c->text,H_MODE_LIST);
			else g_free(x);
		}
	}
	
  g_timeout_add(CHECK_INTERVAL, check_clipboards_tic, NULL);  
#ifdef HAVE_APPINDICATOR
	check_for_appindictor(NULL);
	if(!have_appindicator) /**maybe it slept in, check for it every 30 seconds.  */
		g_timeout_add(CHECK_APPINDICATOR_INTERVAL, check_for_appindictor, NULL);  
#endif
  
  /* Bind global keys */
  keybinder_init();
	keybinder_bind(get_pref_string("phistory_key"), phistory_hotkey, NULL);
  keybinder_bind(get_pref_string("history_key"), history_hotkey, NULL);
  keybinder_bind(get_pref_string("actions_key"), actions_hotkey, NULL);
  keybinder_bind(get_pref_string("menu_key"), menu_hotkey, NULL);
  
  /* Create status icon */
  if (show_icon)
  {
#ifdef HAVE_APPINDICATOR
	if(have_appindicator)/* Indicator */
		create_app_indicator();
#endif
		if(!have_appindicator){
			status_icon = gtk_status_icon_new_from_icon_name(PARCELLITE_ICON); 
	    gtk_status_icon_set_tooltip((GtkStatusIcon*)status_icon, _("Clipboard Manager"));
	    g_signal_connect((GObject*)status_icon, "activate", (GCallback)status_icon_clicked, NULL);
	    g_signal_connect((GObject*)status_icon, "popup-menu", (GCallback)show_parcellite_menu, NULL);
		}
	  
  }
}


/***************************************************************************/
/** .
\n\b Arguments: 
which - which fifo we write to.
\n\b Returns:
****************************************************************************/
void write_stdin(struct p_fifo *fifo, int which)
{
  if (!isatty(fileno(stdin)))   {
    GString* piped_string = g_string_new(NULL);
    /* Append stdin to string */
    while (1)    {
      gchar* buffer = (gchar*)g_malloc(256);
      if (fgets(buffer, 256, stdin) == NULL)  {
        g_free(buffer);
        break;
      }
      g_string_append(piped_string, buffer);
      g_free(buffer);
    }
    /* Check if anything was piped in */
    if (piped_string->len > 0) {
      /* Truncate new line character */
      /* g_string_truncate(piped_string, (piped_string->len - 1)); */
      /* Copy to clipboard */
	   write_fifo(fifo,which,piped_string->str,piped_string->len); 
     /*sleep(10); */
      /* Exit */
    }
    g_string_free(piped_string, TRUE);
	 		
	}
}
	
/***************************************************************************/
	/** .
	\n\b Arguments:
	\n\b Returns:
	****************************************************************************/
int main(int argc, char *argv[])
{
	struct cmdline_opts *opts;
	int mode;
	
  bindtextdomain(GETTEXT_PACKAGE, PARCELLITELOCALEDIR);
  bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
  textdomain(GETTEXT_PACKAGE);
  
  /* Initiate GTK+ */
  gtk_init(&argc, &argv);
   /* Read preferences */
  read_preferences();
  /* Parse options */
	opts=parse_options(argc, argv);
  if(NULL == opts)
   	return 1;
	mode=PROC_MODE_EXACT;
	if(get_pref_int32("multi_user"))
	  mode|=PROC_MODE_USER_QUALIFY;
	/*g_printf("mode=0x%X\n",mode); */
	if(proc_find(PARCELLITE_PROG_NAME,mode,NULL)<2)	/**1 for me, and 1 for a running instance  */
		mode=PROG_MODE_DAEMON; /**first instance  */
	else
		mode=PROG_MODE_CLIENT; /**already running, just access fifos & exit.  */
	
	/**get options/cmd line not parsed.  */
	if( NULL != opts->leftovers)g_print("%s\n",opts->leftovers);
	/**init fifo should set up the fifo and the callback (if we are daemon mode)  */
		if(opts->primary)	{
			fifo=init_fifo(FIFO_MODE_PRI|mode);
			if(fifo->dbg) g_printf("Hit PRI opt!\n");
			
			if(PROG_MODE_CLIENT & mode){
				if(NULL != opts->leftovers){
					write_fifo(fifo,FIFO_MODE_PRI,opts->leftovers,strlen(opts->leftovers));
		      g_free(opts->leftovers);	
				}
				
				if(fifo->dbg) g_printf("checking stdin\n");
				write_stdin(fifo,FIFO_MODE_PRI);
				usleep(1000);
			}
			/* Grab primary */
	    GtkClipboard* prim = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
	    /* Print primary text (if any) */
	    gchar* prim_text = gtk_clipboard_wait_for_text(prim);
	    if (prim_text)
	      g_print("%s", prim_text);
	    g_free(prim_text);
			
	  }  else if(opts->clipboard){
			fifo=init_fifo(FIFO_MODE_CLI|mode);
			
			if(PROG_MODE_CLIENT & mode){
				if(NULL != opts->leftovers){
					write_fifo(fifo,FIFO_MODE_CLI,opts->leftovers,strlen(opts->leftovers));
		      g_free(opts->leftovers);
				}	
				write_stdin(fifo,FIFO_MODE_CLI);
				usleep(1000);
			}
	      
			GtkClipboard* clip = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
	    /* Print clipboard text (if any) */
	    gchar* clip_text = gtk_clipboard_wait_for_text(clip);
	    if (clip_text)
	      g_print("%s", clip_text);
	    g_free(clip_text);
	  }  else  	{ /*use CLIPBOARD*/
			GtkClipboard* clip = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
			fifo=init_fifo(FIFO_MODE_NONE|mode);
				/* Copy from unrecognized options */
			if(PROG_MODE_CLIENT & mode){
			  if(NULL != opts->leftovers){
			    write_fifo(fifo,FIFO_MODE_CLI,opts->leftovers,strlen(opts->leftovers));
		      g_free(opts->leftovers);
				}	
					 /* Check if stdin is piped */
		    write_stdin(fifo,FIFO_MODE_CLI);
				usleep(1000);
			}
			
			gchar* clip_text = gtk_clipboard_wait_for_text(clip);
    	if (clip_text)
      	g_print("%s", clip_text);
    	 g_free(clip_text);
	    
		}	
			  /* Run as daemon option */
  if (opts->daemon && (PROG_MODE_DAEMON & mode))  {
    init_daemon_mode();
  } 
	if(PROG_MODE_CLIENT & mode){
		close_fifos(fifo);
		return 0;
	}	
  
  /* Init Parcellite */
  parcellite_init(mode);
  /*g_printf("Start main loop\n"); */
  /* Run GTK+ loop */
  gtk_main();
  
#ifdef HAVE_APPINDICATOR
	if (have_appindicator & show_icon)
		app_indicator_set_status(indicator, APP_INDICATOR_STATUS_PASSIVE);
#endif
	
  /* Unbind keys */
	keybinder_unbind(get_pref_string("phistory_key"), phistory_hotkey);
  keybinder_unbind(get_pref_string("history_key"), history_hotkey);
  keybinder_unbind(get_pref_string("actions_key"), actions_hotkey);
  keybinder_unbind(get_pref_string("menu_key"), menu_hotkey);
  /* Cleanup */
	/**  
  g_free(prefs.history_key);
  g_free(prefs.actions_key);
  g_free(prefs.menu_key);
	*/
  g_list_free(history_list);
  close_fifos(fifo);
  /* Exit */
  return 0;
}
