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
#include <sys/wait.h>

void setup_icon( void ); /**in main.c  */

#define MAX_HISTORY 1000

#define INIT_HISTORY_KEY      NULL
#define INIT_ACTIONS_KEY      NULL
#define INIT_MENU_KEY         NULL

#define DEF_USE_COPY          TRUE
#define DEF_USE_PRIMARY       FALSE
#define DEF_SYNCHRONIZE       FALSE
#define DEF_SAVE_HISTORY      TRUE
#define DEF_HISTORY_LIMIT     25
#define DEF_HYPERLINKS_ONLY   FALSE
#define DEF_CONFIRM_CLEAR     TRUE
#define DEF_SINGLE_LINE       TRUE
#define DEF_REVERSE_HISTORY   FALSE
#define DEF_ITEM_LENGTH       50
#define DEF_ITEM_LENGTH_MAX   200
#define DEF_ELLIPSIZE         2
#define DEF_PHISTORY_KEY      "<Ctrl><Alt>X"
#define DEF_HISTORY_KEY       "<Ctrl><Alt>H"
#define DEF_ACTIONS_KEY       "<Ctrl><Alt>A"
#define DEF_MENU_KEY          "<Ctrl><Alt>P"
#define DEF_NO_ICON           FALSE

/**allow lower nibble to become the number of items of this type  */
#define PREF_TYPE_TOGGLE 0x10
#define PREF_TYPE_SPIN   0x20
#define PREF_TYPE_COMBO  0x30
#define PREF_TYPE_ENTRY  0x40 /**gchar *  */
#define PREF_TYPE_ALIGN  0x50 /**label, then align box  */
#define PREF_TYPE_SPACER 0x60
#define PREF_TYPE_FRAME  0x70 /**frame for each section  */
#define PREF_TYPE_MASK	 0xF0 
#define PREF_TYPE_NMASK	 0xF
#define PREF_TYPE_SINGLE_LINE 1

#define PREF_SEC_NONE 0
#define PREF_SEC_CLIP 1
#define PREF_SEC_HIST 2
#define PREF_SEC_MISC 3
#define PREF_SEC_DISP 4

#define PREF_SEC_ACT	5
#define PREF_SEC_XMISC 6

#define RC_VERSION_NAME "RCVersion"
#define RC_VERSION      1

struct myadj {
	gdouble lower;
	gdouble upper;
	gdouble step;
	gdouble page;
};
static char *icon_name="parcellite";
struct myadj align_hist_x={1,100,1,10};
struct myadj align_hist_y={1,100,1,10};
struct myadj align_data_lim={0,1000000,1,10};
struct myadj align_hist_lim={5, MAX_HISTORY, 1, 10};
struct myadj align_line_lim={5, DEF_ITEM_LENGTH_MAX, 1, 5};
struct pref_item {
	gchar *name;		/**name/id to find pref  */
	gint32 val;			/**int val  */
	float fval;
	gchar *cval;	 /**char val  */
	GtkWidget *w;	 /**widget in menu  */
	gint type;		/**PREF_TYPE_  */
	gchar *desc; /**shows up in menu  */
	gchar *tip; /**tooltip */
	gint sec;	/**clipboard,history, misc,display,hotkeys  */
	gchar *sig;			 /**signal, if any  */
	GCallback sfunc; /**function to call  */
	struct myadj *adj;
};
static struct pref_item dummy[2];
static void check_toggled(GtkToggleButton *togglebutton, gpointer user_data);
static void search_toggled(GtkToggleButton *b, gpointer user);
static gint dbg=0;
static int tool_bitfield=0;
static int tool_bitfield_check=0;
struct pref2int *pref2int_mapper=NULL;
struct tool_flag tool_flags[]={
	{.flag=TOOL_XDOTOOL,.name="xdotool"},
	{.flag=0,.name=NULL },
	/*{.flag=,.name= }, */
};
/**hot key list, mainly for easy sanity checks.  */
struct keys keylist[]={
	{.name="menu_key",.keyval=DEF_MENU_KEY,.keyfunc=(void *)menu_hotkey},
	{.name="history_key",.keyval=DEF_HISTORY_KEY,.keyfunc=(void *)history_hotkey},
	{.name="phistory_key",.keyval=DEF_PHISTORY_KEY,.keyfunc=(void *)phistory_hotkey},
	{.name="actions_key",.keyval=DEF_ACTIONS_KEY,.keyfunc=(void *)actions_hotkey},
	{.name=NULL,.keyval=NULL,.keyfunc=(void *)0},
};
/**must be in same order as above struct array  */
gchar *def_keyvals[]={ DEF_MENU_KEY,DEF_HISTORY_KEY,DEF_PHISTORY_KEY,DEF_ACTIONS_KEY};
struct pref_item myprefs[]={
/**Behaviour  */	
	/**Clipboards  */
	{.adj=NULL,.cval=NULL,.sig=NULL,.sfunc=NULL,.sec=PREF_SEC_CLIP,.name=NULL,.type=PREF_TYPE_FRAME,.desc="<b>Clipboards</b>",.tip=NULL,.val=0}, 
  {.adj=NULL,.cval=NULL,.sig="toggled",.sfunc=(GCallback)check_toggled,.sec=PREF_SEC_CLIP,.name="use_copy",.type=PREF_TYPE_TOGGLE,.desc="Use _Copy (Ctrl-C)",.tip="If checked, Use the clipboard, which is Ctrl-C, Ctrl-V",.val=DEF_USE_COPY},
  {.adj=NULL,.cval=NULL,.sig="toggled",.sfunc=(GCallback)check_toggled,.sec=PREF_SEC_CLIP,.name="use_primary",.type=PREF_TYPE_TOGGLE,.desc="Use _Primary (Selection)",.tip="If checked, Use the primary clipboard (mouse highlight-copy, middle mouse button paste)",.val=DEF_USE_PRIMARY},
  {.adj=NULL,.cval=NULL,.sig=NULL,.sec=PREF_SEC_CLIP,.name="synchronize",.type=PREF_TYPE_TOGGLE,.desc="S_ynchronize clipboards",.tip="If checked, will keep both clipboards with the same content. If primary is pasted, then copy will have the same data.",.val=DEF_SYNCHRONIZE},
  /**History  */	
  {.adj=NULL,.cval=NULL,.sig=NULL,.sfunc=NULL,.sec=PREF_SEC_HIST,.name=NULL,.type=PREF_TYPE_FRAME,.desc="<b>History</b>",.tip=NULL,.val=0},
  {.adj=NULL,.cval=NULL,.sig=NULL,.sec=PREF_SEC_HIST,.name="save_history",.type=PREF_TYPE_TOGGLE,.desc="Save history",.tip="Save history to a file.",.val=DEF_SAVE_HISTORY},
  {.adj=NULL,.cval=NULL,.sig=NULL,.sec=PREF_SEC_HIST,.name="history_pos",.type=PREF_TYPE_TOGGLE|PREF_TYPE_SINGLE_LINE,.desc="Position history",.tip="If checked, use X, Y to position the history list",.val=0},
  {.adj=&align_hist_x,.cval=NULL,.sig=NULL,.sec=PREF_SEC_HIST,.name="history_x",.type=PREF_TYPE_SPIN|PREF_TYPE_SINGLE_LINE,.desc="<b>X</b>",.tip=NULL,.val=1},
  {.adj=&align_hist_y,.cval=NULL,.sig=NULL,.sec=PREF_SEC_HIST,.name="history_y",.type=PREF_TYPE_SPIN|PREF_TYPE_SINGLE_LINE,.desc="<b>Y</b>",.tip="Position in pixels from the top of the screen",.val=1},
	{.adj=&align_hist_lim,.cval=NULL,.sig=NULL,.sec=PREF_SEC_HIST,.name="history_limit",.type=PREF_TYPE_SPIN,.desc="Items in history",.tip="Maximum number of clipboard entries to keep",.val=DEF_HISTORY_LIMIT},
  {.adj=&align_data_lim,.cval=NULL,.sig=NULL,.sec=PREF_SEC_HIST,.name="data_size",.type=PREF_TYPE_SPIN,.desc="Max Data Size(KB)",.tip="Maximum data size of entire history list",.val=0},
  {.adj=&align_hist_lim,.cval=NULL,.sig=NULL,.sec=PREF_SEC_HIST,.name="item_size",.type=PREF_TYPE_SPIN,.desc="Max Item Size(KB)",.tip="Maximum data size of one item",.val=0},
	{.adj=NULL,.cval=NULL,.sig=NULL,.sec=PREF_SEC_HIST,.name="nop",.type=PREF_TYPE_SPACER,.desc=" ",.tip=NULL},
	{.adj=NULL,.cval=NULL,.sig=NULL,.sec=PREF_SEC_HIST,.name="automatic_paste",.type=PREF_TYPE_TOGGLE|PREF_TYPE_SINGLE_LINE,.desc="Auto Paste",.tip="If checked, will use xdotool to paste wherever the mouse is.\nNOTE! Package xdotool MUST BE INSTALLED for this to work.",.val=0},
	{.adj=NULL,.cval=NULL,.sig=NULL,.sec=PREF_SEC_HIST,.name="auto_key",.type=PREF_TYPE_TOGGLE|PREF_TYPE_SINGLE_LINE,.desc="Key",.tip="If checked, will use Ctrl-V paste.",.val=0},
	{.adj=NULL,.cval=NULL,.sig=NULL,.sec=PREF_SEC_HIST,.name="auto_mouse",.type=PREF_TYPE_TOGGLE|PREF_TYPE_SINGLE_LINE,.desc="Mouse",.tip="If checked, will use middle mouse to paste.",.val=1},
	{.adj=NULL,.cval=NULL,.sig=NULL,.sec=PREF_SEC_HIST,.name="key_input",.type=PREF_TYPE_TOGGLE,.desc="Keyboard Input",.tip="If checked, will emit the history entry via the keyboard entry.",.val=0},
	{.adj=NULL,.cval=NULL,.sig=NULL,.sec=PREF_SEC_HIST,.name="restore_empty",.type=PREF_TYPE_TOGGLE,.desc="Restore Empty",.tip="If checked, will restore clipboard entry on application exit.",.val=1},
	{.adj=NULL,.cval=NULL,.sig=NULL,.sec=PREF_SEC_HIST,.name="rc_edit",.type=PREF_TYPE_TOGGLE,.desc="Right Click Edit",.tip="If checked, edit the history item clicked on.",.val=0},
  
  /**Miscellaneous  */  
	{.adj=NULL,.cval=NULL,.sig=NULL,.sfunc=NULL,.sec=PREF_SEC_MISC,.name=NULL,.type=PREF_TYPE_FRAME,.desc="<b>Miscellaneous</b>",.tip=NULL,.val=0},
	{.adj=NULL,.cval=NULL,.sig="toggled",.sfunc=(GCallback)search_toggled,.sec=PREF_SEC_MISC,.name="type_search",.type=PREF_TYPE_TOGGLE,.desc="Search As You Type",.tip="If checked, does a search-as-you-type. Turns red if not found. Goes to top (Alt-E) line when no chars are entered for search"},
  {.adj=NULL,.cval=NULL,.sig="toggled",.sfunc=(GCallback)search_toggled,.sec=PREF_SEC_MISC,.name="case_search",.type=PREF_TYPE_TOGGLE,.desc="Case Sensitive Search",.tip="If checked, does case sensitive search"},
	{.adj=NULL,.cval=NULL,.sig=NULL,.sec=PREF_SEC_MISC,.name="ignore_whiteonly",.type=PREF_TYPE_TOGGLE,.desc="Ignore Whitespace Only",.tip="If checked, will ignore any clipboard additions that contain only whitespace."},
	{.adj=NULL,.cval=NULL,.sig=NULL,.sec=PREF_SEC_MISC,.name="trim_wspace_begend",.type=PREF_TYPE_TOGGLE,.desc="Trim Whitespace",.tip="If checked, will trim whitespace from beginning and end of entry."},
	{.adj=NULL,.cval=NULL,.sig=NULL,.sec=PREF_SEC_MISC,.name="trim_newline",.type=PREF_TYPE_TOGGLE,.desc="Trim Newlines",.tip="If checked, will replace newlines with spaces."},
	{.adj=NULL,.cval=NULL,.sig=NULL,.sec=PREF_SEC_MISC,.name="hyperlinks_only",.type=PREF_TYPE_TOGGLE,.desc="Capture hyperlinks only",.tip=NULL,.val=DEF_HYPERLINKS_ONLY},
	{.adj=NULL,.cval=NULL,.sig=NULL,.sec=PREF_SEC_MISC,.name="confirm_clear",.type=PREF_TYPE_TOGGLE,.desc="Confirm before clearing history",.tip=NULL,.val=DEF_CONFIRM_CLEAR},
	
/**Display  add icon here...*/
	{.adj=NULL,.cval=NULL,.sig=NULL,.sfunc=NULL,.sec=PREF_SEC_DISP,.name=NULL,.type=PREF_TYPE_FRAME,.desc="<b>Items</b>",.tip=NULL,.val=0},
	{.adj=NULL,.cval=NULL,.sig=NULL,.sec=PREF_SEC_DISP,.name="current_on_top",.type=PREF_TYPE_TOGGLE,.desc="Current entry on top",.tip="If checked, places current clipboard entry at top of list. If not checked, history does not get sorted.",.val=TRUE},
  {.adj=NULL,.cval=NULL,.sig=NULL,.sec=PREF_SEC_DISP,.name="single_line",.type=PREF_TYPE_TOGGLE,.desc="Show in a single line",.tip=NULL,.val=DEF_SINGLE_LINE},
  {.adj=NULL,.cval=NULL,.sig=NULL,.sec=PREF_SEC_DISP,.name="reverse_history",.type=PREF_TYPE_TOGGLE,.desc="Show in reverse order",.tip="If checked, the current entry will be on the bottom instead of the top.",.val=DEF_REVERSE_HISTORY},
	{.adj=NULL,.cval=NULL,.sig=NULL,.sec=PREF_SEC_DISP,.name="nop",.type=PREF_TYPE_SPACER,.desc=" ",.tip=NULL},
	{.adj=&align_line_lim,.cval=NULL,.sig=NULL,.sec=PREF_SEC_DISP,.name="item_length",.type=PREF_TYPE_SPIN,.desc="  Character length of items",.tip=NULL,.val=DEF_ITEM_LENGTH},  
  {.adj=NULL,.cval=NULL,.sig=NULL,.sec=PREF_SEC_DISP,.name="nop",.type=PREF_TYPE_SPACER,.desc=" ",.tip=NULL},
	{.adj=NULL,.cval=NULL,.sig=NULL,.sec=PREF_SEC_DISP,.name="persistent_history",.type=PREF_TYPE_TOGGLE,.desc="Persistent History",.tip="If checked, enables the persistent history.",.val=FALSE},
	{.adj=NULL,.cval=NULL,.sig=NULL,.sec=PREF_SEC_DISP,.name="persistent_separate",.type=PREF_TYPE_TOGGLE,.desc="Persistent As Separate List",.tip="If checked, puts the persistent history in a new list.",.val=FALSE},
	{.adj=NULL,.cval=NULL,.sig=NULL,.sec=PREF_SEC_DISP,.name="persistent_on_top",.type=PREF_TYPE_TOGGLE,.desc="Persistent On Top",.tip="If checked, puts the persistent history at the top of the history list.",.val=FALSE},
	{.adj=NULL,.cval="\\n",.sig=NULL,.sec=PREF_SEC_DISP,.name="persistent_delim",.type=PREF_TYPE_ENTRY,.desc="Paste All Entry Delimiter",.tip="This string will be inserted between each line of history for paste all."},
	{.adj=NULL,.cval=NULL,.sig=NULL,.sec=PREF_SEC_DISP,.name="nonprint_disp",.type=PREF_TYPE_TOGGLE,.desc="Alternate Non-printing Display",.tip="If checked, will display tabs with Right arrow (utf8,\\2192), newlines with pharagrph(\\204b), and spaces with square-u(\\2423).", .val=FALSE},
	{.adj=NULL,.cval=NULL,.sig=NULL,.sec=PREF_SEC_DISP,.name="nop",.type=PREF_TYPE_SPACER,.desc=" ",.tip=NULL},
  {.adj=NULL,.cval=NULL,.sig=NULL,.sec=PREF_SEC_NONE,.name="ellipsize",.type=PREF_TYPE_COMBO,.desc="Omit items in the:",.tip=NULL,.val=DEF_ELLIPSIZE}, 
	
/**miscellaneous that doesn't fit elswhew  */	
  {.adj=NULL,.cval=NULL,.sig=NULL,.sfunc=NULL,.sec=PREF_SEC_XMISC,.name=NULL,.type=PREF_TYPE_FRAME,.desc="<b>Miscellaneous</b>",.tip=NULL,.val=0},
	{.adj=NULL,.cval=NULL,.sig=NULL,.sec=PREF_SEC_XMISC,.name="multi_user",.type=PREF_TYPE_TOGGLE,.desc="Multiuser",.tip="If checked, enables checking multiple concurrent user logic. Use if several different users are logged in at the same time.",.val=TRUE},
	{.adj=NULL,.cval=PARCELLITE_ICON,.sig=NULL,.sec=PREF_SEC_XMISC,.name="icon_name",.type=PREF_TYPE_ENTRY,.desc="Parcellite Icon Name",.tip="Name of Parcellite icon. If this is mis-typed, icon will not appear.",.val=TRUE},
#ifdef	DEBUG_UPDATE
	{.adj=NULL,.cval=NULL,.sig=NULL,.sec=PREF_SEC_XMISC,.name="debug_update",.type=PREF_TYPE_TOGGLE,.desc="DebugUpdate",.tip="If checked, enables debug prints on clipboard update logic. This only takes effect when enabled at start up, and may be disabled at compile time.",.val=FALSE},
#endif
	
/**Action Keys  */
	{.adj=NULL,.cval=NULL,.sig=NULL,.sec=PREF_SEC_ACT,.name="menu_key",.type=PREF_TYPE_ENTRY,.desc="Menu key combination",.tip=NULL},	
  {.adj=NULL,.cval=NULL,.sig=NULL,.sec=PREF_SEC_ACT,.name="history_key",.type=PREF_TYPE_ENTRY,.desc="History key combination:",.tip=NULL},
	{.adj=NULL,.cval=NULL,.sig=NULL,.sec=PREF_SEC_ACT,.name="phistory_key",.type=PREF_TYPE_ENTRY,.desc="Persistent history key:",.tip=NULL},
  {.adj=NULL,.cval=NULL,.sig=NULL,.sec=PREF_SEC_ACT,.name="actions_key",.type=PREF_TYPE_ENTRY,.desc="Actions key combination:",.tip=NULL},
	{.adj=NULL,.cval=NULL,.sig=NULL,.sec=PREF_SEC_NONE,.name="no_icon",.val=FALSE},
	{.adj=NULL,.cval=NULL,.sig=NULL,.sec=PREF_SEC_NONE,.name=NULL,.desc=NULL},
};

GtkListStore* actions_list;
GtkTreeSelection* actions_selection;

/***************************************************************************/
/** Set up which tools are available on the system.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
static void check_for_tools_exit(GPid pid, gint status, gpointer data)
{
	int flag= GPOINTER_TO_INT(data);
  g_spawn_close_pid(pid);
	if( WIFEXITED(status) ){
		if(0 ==WEXITSTATUS(status) )
			tool_bitfield|=flag;
		
	}
	tool_bitfield_check &= ~(flag);
	g_fprintf(stderr,"Flag 0x%04x, status %d, EXIT %d STAT %d\n",flag,status,WIFEXITED(status),WEXITSTATUS(status) ); 
		
}
/***************************************************************************/
/** Check for installed tools and set flags accordingly.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void check_for_tools(void )
{
	GPid pid;
  gchar **argv;
	int i;
	for (i=0; NULL != tool_flags[i].name; ++i){
		gchar cmd[100];
		tool_bitfield_check|=tool_flags[i].flag;
		sprintf(cmd,"/bin/sh -c 'which %s'>/dev/null\n",tool_flags[i].name);
		g_shell_parse_argv(cmd, NULL, &argv, NULL);
		g_spawn_async(NULL, argv, NULL, G_SPAWN_DO_NOT_REAP_CHILD, NULL, NULL, &pid, NULL);
		g_child_watch_add(pid, (GChildWatchFunc)check_for_tools_exit, GINT_TO_POINTER(tool_flags[i].flag));
		g_strfreev(argv);
	}
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns: Structure of item
****************************************************************************/
struct pref_item* get_pref(char *name)
{
	int i;
	for (i=0;NULL != myprefs[i].desc; ++i){
		if(NULL == myprefs[i].name)
			continue;
		if(!g_strcmp0(myprefs[i].name,name))
			return &myprefs[i];
	}	
	return &dummy[0];
}
/***************************************************************************/
/** Set an array that is updated every time the preferences are changed.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void pref_mapper (struct pref2int *m, int mode)
{
	int i;
	if(PM_INIT == mode){
		pref2int_mapper=m;
		return;
	}
	for (i=0; pref2int_mapper[i].name != NULL && pref2int_mapper[i].val != NULL; ++i){
		struct pref_item *p=get_pref(pref2int_mapper[i].name);
		*pref2int_mapper[i].val=p->val;
	}
	
}
/***************************************************************************/
/** .
\n\b Arguments: pointer of widget
\n\b Returns: Structure of item
****************************************************************************/
struct pref_item* get_pref_by_widget(GtkWidget *w)
{
	int i;
	for (i=0;NULL != myprefs[i].desc; ++i){
		if(NULL == myprefs[i].name)
			continue;
		if(w == myprefs[i].w)
			return &myprefs[i];
	}	
	return &dummy[0];
}
/***************************************************************************/
/** Find first item in section.
\n\b Arguments:
\n\b Returns: index into struct where found, or end of list
****************************************************************************/
int get_first_pref(int section)
{
	int i;
	for (i=0;NULL != myprefs[i].desc; ++i){
		if(section == myprefs[i].sec){
			if(dbg)g_printf("gfp:returning sec %d, '%s\n",section, myprefs[i].desc);
			return i;
		}
			
	}	
	if(dbg)g_printf("Didn't find section %d\n",i);
	return i;
}                 
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int init_pref( void )
{
	GdkScreen *s;
	gint sx,sy;
	s=gdk_screen_get_default();
	sx= gdk_screen_get_width(s);
	sy= gdk_screen_get_height(s);
	dummy[0].cval="dummy String";
	dummy[0].w=NULL;
	dummy[0].desc="Dummy desc";
	dummy[0].tip="Dummy tip";
	dummy[1].name=NULL;
	dummy[1].sec=PREF_SEC_NONE;
	align_hist_y.upper=sy-100;
	align_hist_x.upper=sx-100;
	return 0;
}
/***************************************************************************/
/** set the wideget of item.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int set_pref_widget (char *name, GtkWidget *w)
{
	struct pref_item *p=get_pref(name);
	if(NULL == p)
		return -1;
	p->w=w;
	return 0;
}
/***************************************************************************/
/** get the char * value of string.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
GtkWidget *get_pref_widget (char *name)
{
	struct pref_item *p=get_pref(name);
	if(NULL == p)
		return dummy[0].w;
	return p->w;
 }
/***************************************************************************/
/** Set the int value of name.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int set_pref_int32(char *name, gint32 val)
{
	struct pref_item *p=get_pref(name);
	if(NULL == p)
		return -1;
	p->val=val;
	return 0;
}

/***************************************************************************/
/** get the int value of name.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
gint32 get_pref_int32 (char *name)
{
	struct pref_item *p=get_pref(name);
	if(NULL == p)
		return -1;
	return p->val;
}
/***************************************************************************/
/** Set the float value of name.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int set_pref_float(char *name, float fval)
{
	struct pref_item *p=get_pref(name);
	if(NULL == p)
		return -1;
	p->fval=fval;
	return 0;
}

/***************************************************************************/
/** get the float value of name.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
float get_pref_float (char *name)
{
	struct pref_item *p=get_pref(name);
	if(NULL == p)
		return -1;
	return p->fval;
}

/***************************************************************************/
/** set the char *value of string.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int set_pref_string (char *name, char *string)
{
	struct pref_item *p=get_pref(name);
	if(NULL == p)
		return -1;
	if(p->cval != NULL)
		g_free(p->cval);
	p->cval=g_strdup(string);
	return 0;
}

/***************************************************************************/
/** get the char * value of string.
\n\b Arguments:
\n\b Returns:
****************************************************************************/
gchar *get_pref_string (char *name)
{
	struct pref_item *p=get_pref(name);
	if(NULL == p)
		return dummy[0].cval;
	return p->cval;
 }

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void unbind_itemkey(char *name, void *fhk )
{
	
	struct pref_item *p=get_pref(name);
	if(NULL == p){
		if(dbg)g_printf("pref:null found for %s\n",name);
		return;
	}
	keybinder_unbind(p->cval, fhk);
	g_free(p->cval);
	p->cval=NULL;
	
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void bind_itemkey(char *name, void (fhk)(char *, gpointer) )
{
	struct pref_item *p=get_pref(name);
	if(NULL ==p){
		if(dbg)g_printf("pref2:null found for %s\n",name);
		return;
	}
	if(NULL != p->cval && 0 != p->cval)
		keybinder_bind(p->cval, fhk, NULL);
}


/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void set_key_entry(gchar *name, gchar *val)
{
	int i;
	for (i=0;NULL != keylist[i].name; ++i){
		if(!g_strcmp0(keylist[i].name,name)){
		  if(NULL == val)
		  	keylist[i].keyval="";
			else
				keylist[i].keyval=val;
			return;
		}
	}
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void set_keys_from_prefs( void )
{
	int i,l;
	for (i=0;NULL != keylist[i].name; ++i){
		/**NOTE: do not set up default keys here! User may WANT them null */
			/** call egg_accelerator_parse_virtual to validate? */
		set_key_entry(keylist[i].name,get_pref_string(keylist[i].name));
		/*g_fprintf(stderr,"key '%s' val '%s'\n",keylist[i].name, keylist[i].keyval); */
	}	
	/**now go through and make sure we have no duplicates */
	for (i=0;NULL != keylist[i].name; ++i){
		if(0 !=  keylist[i].keyval[0]){
			/**see if it exists elsewhere  */
			for (l=0;NULL != keylist[l].name; ++l){
				if(l!=i && 0 != keylist[l].keyval[0]){
					if(!g_strcmp0(keylist[i].keyval, keylist[l].keyval)) { /**conflict!, delete second  */
						g_fprintf(stderr,"Error! Hot keys have same key '%s': '%s' and '%s'. Ignoring second entry\n",keylist[i].keyval,keylist[i].name,keylist[l].name);
						set_key_entry(keylist[l].name,"");
						set_pref_string(keylist[l].name,"");
					}
				}	
			}	
		}
	}	
	
}
/***************************************************************************/
/** .
\n\b Arguments:
mode - if 0, do not display helper missing dialog.
\n\b Returns:
****************************************************************************/
void check_sanity(int mode)
{
	gint32 x,y;
	gchar *val;
	check_for_tools(); /**update the list of tools parcellite needs.  */
	while(tool_bitfield_check){
		g_main_context_iteration(NULL, TRUE);
		usleep(100000);
	}
	val=get_pref_string("icon_name");
	if(NULL != val && strcmp(icon_name, val)){
		setup_icon( );
		icon_name=strdup(val);
	}
	x=get_pref_int32("history_x");
	y=get_pref_int32("history_y");
	postition_history(NULL,&x,&y,NULL, 0); /**have function limit x,y according to screen limits */
  set_pref_int32("history_x",x);
  set_pref_int32("history_y",y);
 	x=get_pref_int32("history_limit");
  if ((!x) || (x > MAX_HISTORY) || (x < 0))
    set_pref_int32("history_limit",DEF_HISTORY_LIMIT);
	x=get_pref_int32("item_length");
  if ((!x) || (x > DEF_ITEM_LENGTH_MAX) || (x < 0))
    set_pref_int32("item_length",DEF_ITEM_LENGTH);
	x=get_pref_int32("ellipsize");
  if ((!x) || (x > 3) || (x < 0))
     set_pref_int32("ellipsize",DEF_ELLIPSIZE);
	set_keys_from_prefs();

	
	if(get_pref_int32("persistent_history")){
	 	if(get_pref_int32("persistent_separate"))
	 		set_pref_int32("persistent_on_top",0);
	}else{
		set_pref_int32("persistent_separate",0);
	  set_pref_int32("persistent_on_top",0);
	}
	if(get_pref_int32("automatic_paste")){
		if(!(tool_bitfield&TOOL_XDOTOOL)){ 
			g_fprintf(stderr,"tool_bitfield=0x%x\n",tool_bitfield);
			set_pref_int32("automatic_paste",0);
			if( mode )
				show_gtk_dialog("xdotool is not installed\nParcellite's auto-paste will not function without xdotool.","xdotool Not Installed" ); 
		} else{
			if(get_pref_int32("auto_key") && get_pref_int32("auto_mouse"))
				set_pref_int32("auto_key",0);
			if(!get_pref_int32("auto_key") && !get_pref_int32("auto_mouse"))
				set_pref_int32("auto_mouse",1);	
		}
	}
}
/* Apply the new preferences */
static void apply_preferences()
{
	int i;
  /* Unbind the keys before binding new ones */
	for (i=0;NULL != keylist[i].name; ++i)
	  unbind_itemkey(keylist[i].name,keylist[i].keyfunc);
	
	
  for (i=0;NULL != myprefs[i].desc; ++i){
		if(NULL == myprefs[i].name)
			continue;
		switch(myprefs[i].type&PREF_TYPE_MASK){
			case PREF_TYPE_TOGGLE:
				myprefs[i].val=gtk_toggle_button_get_active((GtkToggleButton*)myprefs[i].w);
				break;
			case PREF_TYPE_SPIN:
				myprefs[i].val=gtk_spin_button_get_value_as_int((GtkSpinButton*)myprefs[i].w);
				myprefs[i].fval=(float)gtk_spin_button_get_value((GtkSpinButton*)myprefs[i].w);
				break;
			case PREF_TYPE_COMBO:
				myprefs[i].val=gtk_combo_box_get_active((GtkComboBox*)myprefs[i].w) + 1;
				break;
			case PREF_TYPE_ENTRY:
				myprefs[i].cval=g_strdup(gtk_entry_get_text((GtkEntry*)myprefs[i].w ));
				break;
			case PREF_TYPE_SPACER:
				break;
			default: if(dbg)g_printf("apply_pref:don't know how to handle type %d\n",myprefs[i].type);
				break;
		}
	}
  check_sanity(1);
  /* Bind keys and apply the new history limit */
	for (i=0;NULL != keylist[i].name; ++i)
	  bind_itemkey(keylist[i].name,keylist[i].keyfunc);	
  truncate_history();
	pref_mapper(NULL, PM_UPDATE);
}


/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
static void save_preferences()
{
	int i;
  /* Create key */
  GKeyFile* rc_key = g_key_file_new();
	g_key_file_set_integer(rc_key, "rc", RC_VERSION_NAME, RC_VERSION);
  if(0 == get_pref_int32("type_search"))
    set_pref_int32("case_search",0);
  /* Add values */
	for (i=0;NULL != myprefs[i].desc; ++i){
		if(NULL == myprefs[i].name)
			continue;
		switch(myprefs[i].type&PREF_TYPE_MASK){
			case PREF_TYPE_TOGGLE:
				g_key_file_set_boolean(rc_key, "rc", myprefs[i].name, myprefs[i].val);
				break;
			case PREF_TYPE_COMBO:
			case PREF_TYPE_SPIN:
				g_key_file_set_integer(rc_key, "rc", myprefs[i].name, myprefs[i].val);
				break;
			case PREF_TYPE_ENTRY:
				g_key_file_set_string(rc_key, "rc", myprefs[i].name, myprefs[i].cval);
				break;
			case PREF_TYPE_SPACER:
				break;
			default: if(dbg)g_printf("save_pref:don't know how to handle type %d\n",myprefs[i].type);
				break;
		}
	}
  /* Check config and data directories */
  check_dirs();
  /* Save key to file */
  gchar* rc_file = g_build_filename(g_get_user_config_dir(), PREFERENCES_FILE, NULL);
  g_file_set_contents(rc_file, g_key_file_to_data(rc_key, NULL, NULL), -1, NULL);
  g_key_file_free(rc_key);
  g_free(rc_file);
}


/***************************************************************************/
/** Read the parcelliterc file.
\n\b Arguments:
mode - 0 to not display missing helper warnings 
\n\b Returns:
****************************************************************************/
void read_preferences(int mode)
{
	gchar *c,*rc_file = g_build_filename(g_get_user_config_dir(), PREFERENCES_FILE, NULL);
  gint32 z;
  GError *err=NULL;
	struct pref_item *p;
	init_pref();
  /* Create key */
  GKeyFile* rc_key = g_key_file_new();
  if (g_key_file_load_from_file(rc_key, rc_file, G_KEY_FILE_NONE, NULL)) {
		int i;
		i=g_key_file_get_integer(rc_key, "rc", RC_VERSION_NAME,&err); /**this begins in 1.1.8  */
		/* Load values */
		for (i=0;NULL != myprefs[i].desc; ++i){
			if(NULL == myprefs[i].name)
				continue;
		  err=NULL;
			switch(myprefs[i].type&PREF_TYPE_MASK){
				case PREF_TYPE_TOGGLE:
				  z=g_key_file_get_boolean(rc_key, "rc", myprefs[i].name,&err);
				  if( NULL ==err)
							myprefs[i].val=z;
					break;
				case PREF_TYPE_COMBO:
				case PREF_TYPE_SPIN:
				  z=g_key_file_get_integer(rc_key, "rc", myprefs[i].name,&err);
				  if( NULL ==err)
							myprefs[i].val=z;
					break;
				case PREF_TYPE_ENTRY:
				  c=g_key_file_get_string(rc_key, "rc", myprefs[i].name, &err);
				  if( NULL ==err)
						myprefs[i].cval=c;
					break;
				case PREF_TYPE_SPACER:
				break;
				default: 
					if(dbg) g_printf("read_pref:don't know how to handle type %d for '%s'\n",myprefs[i].type,myprefs[i].name);
					continue;
					break;
			}
			if(NULL != err)
				g_printf("Unable to load pref '%s'\n",myprefs[i].name);
			if(dbg)g_printf("rp:Set '%s' to %d (%s)\n",myprefs[i].name, myprefs[i].val, myprefs[i].cval);
		}
    p=get_pref("type_search");
    if(0 == p->val){
			p=get_pref("case_search");
			p->val=0;
		}
      
    /* Check for errors and set default values if any */
    check_sanity(mode);
  }
  else  { /* Init default keys on error */
	  int i;
		
		for (i=0;NULL != keylist[i].name; ++i)
			set_pref_string(keylist[i].name,def_keyvals[i]);
  }
	pref_mapper(NULL, PM_UPDATE);
  g_key_file_free(rc_key);
  g_free(rc_file);
}

/* Read ~/.parcellite/actions into the treeview */
static void read_actions()
{
  /* Open the file for reading */
  gchar* path = g_build_filename(g_get_user_data_dir(), ACTIONS_FILE, NULL);
  FILE* actions_file = fopen(path, "rb");
  g_free(path);
  /* Check that it opened and begin read */
  if (actions_file)
  {
    /* Keep a row reference */
    GtkTreeIter row_iter;
    /* Read the size of the first item */
    gint size=0;
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
static void save_actions()
{
  /* Check config and data directories */
  check_dirs();
  /* Open the file for writing */
  gchar* path = g_build_filename(g_get_user_data_dir(), ACTIONS_FILE, NULL);
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
static void check_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
  if (gtk_toggle_button_get_active((GtkToggleButton*)get_pref_widget("use_copy")) &&
      gtk_toggle_button_get_active((GtkToggleButton*)get_pref_widget("use_primary")))
  {
    /* Only allow synchronize option if both primary and clipboard are enabled */
    gtk_widget_set_sensitive((GtkWidget*)get_pref_widget("synchronize"), TRUE);
  }
  else
  {
    /* Disable synchronize option */
    gtk_toggle_button_set_active((GtkToggleButton*)get_pref_widget("synchronize"), FALSE);
    gtk_widget_set_sensitive((GtkWidget*)get_pref_widget("synchronize"), FALSE);

  }
}

static void search_toggled(GtkToggleButton *b, gpointer user)
{
	struct pref_item *u,*p;
	u=get_pref_by_widget((GtkWidget *)user);
	p=get_pref("case_search");
	
  if(u == p){
    if(TRUE == gtk_toggle_button_get_active((GtkToggleButton*)p->w) && 
      FALSE == gtk_toggle_button_get_active((GtkToggleButton*)get_pref_widget("type_search")) )
      gtk_toggle_button_set_active((GtkToggleButton*)get_pref_widget("type_search"), TRUE);
  }else if( u == get_pref("type_search")){
    if(FALSE == gtk_toggle_button_get_active((GtkToggleButton*)get_pref_widget("type_search")) &&
      TRUE == gtk_toggle_button_get_active((GtkToggleButton*)p->w) )
      gtk_toggle_button_set_active((GtkToggleButton*)p->w, FALSE);    
  }

}

/* Called when Add... button is clicked */
static void add_action(GtkButton *button, gpointer user_data)
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
static void remove_action(GtkButton *button, gpointer user_data)
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
static void move_action_up(GtkButton *button, gpointer user_data)
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
static void move_action_down(GtkButton *button, gpointer user_data)
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
static void delete_key_pressed(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
  /* Check if DEL key was pressed (keyval: 65535) */
  if (event->keyval == 65535)
    remove_action(NULL, NULL);
}

/* Called when a cell is edited */
static void edit_action(GtkCellRendererText *renderer, gchar *path, gchar *new_text,  gpointer cell)
{
  GtkTreeIter sel_iter;
  /* Check if selected */
  if (gtk_tree_selection_get_selected(actions_selection, NULL, &sel_iter))
  {
    /* Apply changes */
    gtk_list_store_set(actions_list, &sel_iter, GPOINTER_TO_INT(cell), new_text, -1);
  }
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int update_pref_widgets( void)
{
	int i,rtn=0;
	for (i=0;NULL !=myprefs[i].desc; ++i){
		if(NULL != myprefs[i].name){
			switch (myprefs[i].type&PREF_TYPE_MASK){
				case PREF_TYPE_TOGGLE:
					gtk_toggle_button_set_active((GtkToggleButton*)myprefs[i].w, myprefs[i].val);
					break;
				case PREF_TYPE_SPIN:
					gtk_spin_button_set_value((GtkSpinButton*)myprefs[i].w, (gdouble)myprefs[i].val);
					break;
				case PREF_TYPE_COMBO:
					gtk_combo_box_set_active((GtkComboBox*)myprefs[i].w, myprefs[i].val - 1);
					break;
				case PREF_TYPE_ENTRY:
					gtk_entry_set_text((GtkEntry*)myprefs[i].w, myprefs[i].cval);
					break;
				case PREF_TYPE_SPACER:
					break;
				default: 
					if(dbg)g_printf("apply_pref:don't know how to handle type %d\n",myprefs[i].type);
					rtn=-1;
					continue;
					break;
			}
			if(dbg)g_printf("up:Set '%s' to %d (%s)\n",myprefs[i].name, myprefs[i].val, myprefs[i].cval);	
		}
		
	}	
	
	return rtn;
}
/***************************************************************************/
/** .
\n\b Arguments:
section is the section to add, parent is the box to put it in.
\n\b Returns: -1 on error;
****************************************************************************/
int add_section(int sec, GtkWidget *parent)
{
	int i,rtn=0;
	int single_st, single_is;
	gint connect;
	GtkWidget *hbox, *label, *child, *vbox, *alignment;
	GtkWidget* packit;
	vbox=parent;
	single_st=single_is=0;
	for (i=get_first_pref(sec);sec==myprefs[i].sec; ++i){
		connect=1;
		single_st=(myprefs[i].type&(PREF_TYPE_NMASK|PREF_TYPE_SINGLE_LINE)); /**deterimine if we are in single line  */
		
		if(single_st && !single_is){ /**start of single line  */
			hbox = gtk_hbox_new(FALSE, 2);  /**create hbox  */
			/*g_printf("alloc %p hbox\n",hbox); */
			single_is=1;
		}
		
		switch (myprefs[i].type&PREF_TYPE_MASK){
			case PREF_TYPE_FRAME:/**must be first in section, since it sets vbox.  */
					myprefs[i].w= gtk_frame_new(NULL);
				  gtk_frame_set_shadow_type((GtkFrame*)	myprefs[i].w, GTK_SHADOW_NONE);
				  label = gtk_label_new(NULL);							/**<b>myprefs[i].desc  */
				  gtk_label_set_markup((GtkLabel*)label, _(myprefs[i].desc));
				  gtk_frame_set_label_widget((GtkFrame*)	myprefs[i].w, label);
				  alignment = gtk_alignment_new(0.50, 0.50, 1.0, 1.0);
				  gtk_alignment_set_padding((GtkAlignment*)alignment, 12, 0, 12, 0);
				  gtk_container_add((GtkContainer*)	myprefs[i].w, alignment);
				  vbox = gtk_vbox_new(FALSE, 2);
				  gtk_container_add((GtkContainer*)alignment, vbox);
					gtk_box_pack_start((GtkBox*)parent,	myprefs[i].w,FALSE,FALSE,0);	
				continue;
				break;
			case PREF_TYPE_TOGGLE:
				myprefs[i].w=gtk_check_button_new_with_mnemonic(_(myprefs[i].desc));
				packit=myprefs[i].w;	
				break;
			
			case PREF_TYPE_SPIN:
				if(!single_is)
					hbox = gtk_hbox_new(FALSE, 4);  
  			label = gtk_label_new(NULL);
				gtk_label_set_markup((GtkLabel*)label, _(myprefs[i].desc));
				gtk_box_pack_start((GtkBox*)hbox, label, FALSE, FALSE, 0);
  			myprefs[i].w=gtk_spin_button_new((GtkAdjustment*)gtk_adjustment_new ( \
				myprefs[i].val,myprefs[i].adj->lower,myprefs[i].adj->upper,myprefs[i].adj->step,myprefs[i].adj->page,0 ),10,0); 
  			gtk_box_pack_start((GtkBox*)hbox, myprefs[i].w, FALSE, FALSE, 0);
  			gtk_spin_button_set_update_policy((GtkSpinButton*)myprefs[i].w, GTK_UPDATE_IF_VALID);
  			if(NULL != myprefs[i].tip) gtk_widget_set_tooltip_text(label, _(myprefs[i].tip));
  			packit=hbox;
				break;
			
			case PREF_TYPE_ENTRY:
				if(!single_is)
					hbox = gtk_hbox_new(TRUE, 4);
			  label = gtk_label_new(NULL);
				gtk_label_set_markup((GtkLabel*)label, _(myprefs[i].desc));
			  gtk_misc_set_alignment((GtkMisc*)label, 0.0, 0.50);
			  gtk_box_pack_start((GtkBox*)hbox, label, TRUE, TRUE, 0);
			  myprefs[i].w = gtk_entry_new();
			  gtk_entry_set_width_chars((GtkEntry*)myprefs[i].w, 10);
				gtk_box_pack_end((GtkBox*)hbox,myprefs[i].w, TRUE, TRUE, 0);
			  packit=hbox;
				break;
			case PREF_TYPE_COMBO: /**handled in show_preferences, only one so  */
				continue;
				break;
			case PREF_TYPE_SPACER:
				packit=myprefs[i].w=gtk_menu_item_new_with_label("");
				child=gtk_bin_get_child((GtkBin *)packit);
				gtk_misc_set_padding((GtkMisc *)child,0,0);
				gtk_label_set_markup ((GtkLabel *)child, "<span size=\"0\"> </span>");
				break;
			
			default: 
				if(dbg)g_printf("add_sec:don't know how to handle type %d\n",myprefs[i].type);
				rtn=-1;
				continue;
				break;
		}
		
		/**tooltips are set on the label of the spin box, not the widget and are handled above */
		if(PREF_TYPE_SPIN != myprefs[i].type && NULL != myprefs[i].tip)
		  gtk_widget_set_tooltip_text(myprefs[i].w, _(myprefs[i].tip));
		
		if(NULL != myprefs[i].sig && connect)
			g_signal_connect((GObject*)myprefs[i].w, myprefs[i].sig, (GCallback)myprefs[i].sfunc, myprefs[i].w);
		
		if(dbg)g_printf("Packing %s\n",myprefs[i].name);
		if(single_is){
			if(packit != hbox){
				/*g_printf("Packed a slwidget %p<-%p\n",hbox, myprefs[i].w); */
				gtk_box_pack_start((GtkBox*)hbox,myprefs[i].w , FALSE, FALSE, 0);	
			}
			/**else already packed above.  */
		}	else
																							/**espand fill padding  */
			gtk_box_pack_start((GtkBox*)vbox, packit, TRUE, TRUE, 0);
	/**check for end of single line.  */
		single_st=(myprefs[i+1].type&(PREF_TYPE_NMASK|PREF_TYPE_SINGLE_LINE)); /**deterimine if we are in single line  */
		if(single_is && !single_st){/**end of single line  */
																							/**exp fill padding  */
			gtk_box_pack_start((GtkBox*)vbox, hbox, TRUE, TRUE, 0);		/**pack the hbox into parent  */
			/*g_printf("pack %p<-%p hbox\n",vbox,hbox); */
			single_is=0;
		}
	}
	if(dbg)g_printf("Ending on %d '%s'\n",i,myprefs[i].name);
	return rtn;
	
}

/* Shows the preferences dialog on the given tab */
void show_preferences(gint tab)
{
  /* Declare some variables */
  GtkWidget *frame,*label,*alignment,*hbox, *vbox;
	struct pref_item *p;
  GtkTreeViewColumn *tree_column;
	init_pref();
  
  /* Create the dialog */
  GtkWidget* dialog = gtk_dialog_new_with_buttons(_("Preferences"),     NULL,
                                                   (GTK_DIALOG_MODAL  + GTK_DIALOG_NO_SEPARATOR),
                                                    GTK_STOCK_CANCEL,   GTK_RESPONSE_REJECT,
                                                    GTK_STOCK_OK,       GTK_RESPONSE_ACCEPT, NULL);
  
  gtk_window_set_icon((GtkWindow*)dialog, gtk_widget_render_icon(dialog, GTK_STOCK_PREFERENCES, -1, NULL));
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
  
  /* Build the clipboards frame & copy section */
	add_section(PREF_SEC_CLIP,vbox_behavior);
	/* Build the history frame */
	add_section(PREF_SEC_HIST,vbox_behavior);
  /* Build the miscellaneous frame */
	add_section(PREF_SEC_MISC,vbox_behavior);
  
  /* Build the display page */
  GtkWidget* page_display = gtk_alignment_new(0.50, 0.50, 1.0, 1.0);
  gtk_alignment_set_padding((GtkAlignment*)page_display, 12, 6, 12, 6);
  gtk_notebook_append_page((GtkNotebook*)notebook, page_display, gtk_label_new(_("Display")));
  GtkWidget* vbox_display = gtk_vbox_new(FALSE, 12);
  gtk_container_add((GtkContainer*)page_display, vbox_display);
  
  /* Build the items frame */
	add_section(PREF_SEC_DISP,vbox_display);
	
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
	p=get_pref("ellipsize");
  label = gtk_label_new(_(p->desc));
  gtk_misc_set_alignment((GtkMisc*)label, 0.0, 0.50);
  gtk_box_pack_start((GtkBox*)hbox, label, FALSE, FALSE, 0);
  p->w = gtk_combo_box_new_text();
  gtk_combo_box_append_text((GtkComboBox*)p->w, _("Beginning"));
  gtk_combo_box_append_text((GtkComboBox*)p->w, _("Middle"));
  gtk_combo_box_append_text((GtkComboBox*)p->w, _("End"));
  gtk_box_pack_start((GtkBox*)hbox, p->w, FALSE, FALSE, 0);
  gtk_box_pack_start((GtkBox*)vbox_display, frame, FALSE, FALSE, 0);
	
	/* Build the misc Display frame */
	add_section(PREF_SEC_XMISC,vbox_display);
  
  /* Build the actions page */
  GtkWidget* page_actions = gtk_alignment_new(0.50, 0.50, 1.0, 1.0);
  gtk_alignment_set_padding((GtkAlignment*)page_actions, 6, 6, 6, 6);
  gtk_notebook_append_page((GtkNotebook*)notebook, page_actions, gtk_label_new(_("Actions")));
  GtkWidget* vbox_actions = gtk_vbox_new(FALSE, 6);
  gtk_container_add((GtkContainer*)page_actions, vbox_actions);
  
  /* Build the actions label */
  label = gtk_label_new(_("Control-click Parcellite's tray icon to use actions,"));
	gtk_widget_set_tooltip_text(label, _("Use %% if you want a literal %. See the man page for gnuC printf (man 2 printf) for more information."));
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
	
	label=gtk_label_new(_("Action"));
	gtk_widget_set_tooltip_text(label,"This is the Action Name. \nDO NOT put commands here.");
	gtk_widget_show (label);
  tree_column = gtk_tree_view_column_new_with_attributes(_("Action"), name_renderer, "text", 0, NULL);
  gtk_tree_view_column_set_widget(tree_column,label); 
	
  gtk_tree_view_column_set_resizable(tree_column, TRUE);
  gtk_tree_view_append_column((GtkTreeView*)treeview, tree_column);
  GtkCellRenderer* command_renderer = gtk_cell_renderer_text_new();
  g_object_set(command_renderer, "editable", TRUE, NULL);
  g_object_set(command_renderer, "ellipsize-set", TRUE, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  g_signal_connect((GObject*)command_renderer, "edited", (GCallback)edit_action, (gpointer)1);
	
	label=gtk_label_new(_("Command"));
	gtk_widget_set_tooltip_text(label,"Put the full commands here. ex echo \"parcellite gave me %s\">>$HOME/ptest");
	gtk_widget_show (label);
  tree_column = gtk_tree_view_column_new_with_attributes(_("Command"), command_renderer, "text", 1, NULL);
	gtk_tree_view_column_set_widget(tree_column,label); 
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
	add_section(PREF_SEC_ACT,vbox);
	gtk_box_pack_start((GtkBox*)vbox_extras,frame,FALSE,FALSE,0);
  
  /* Make widgets reflect current preferences */
	update_pref_widgets();
  
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

