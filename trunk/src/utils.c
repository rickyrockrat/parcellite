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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>


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
  gchar* data_dir = g_build_path(G_DIR_SEPARATOR_S, g_get_user_data_dir(), DATA_DIR,  NULL);
  gchar* config_dir = g_build_path(G_DIR_SEPARATOR_S, g_get_user_config_dir(), CONFIG_DIR,  NULL);
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
struct cmdline_opts *parse_options(int argc, char* argv[])
{
	struct cmdline_opts *opts=g_malloc0(sizeof(struct cmdline_opts));
	if(NULL == opts){
		g_printf("Unable to malloc cmdline_opts\n");
		return NULL;
	}
  if (argc <= 1) 
		return opts;	
  GOptionEntry main_entries[] = 
  {
    {
      "daemon", 'd',
      0,
      G_OPTION_ARG_NONE,
      &opts->daemon, _("Run as daemon"),
      NULL
    },
    {
      "no-icon", 'n',
      0,
      G_OPTION_ARG_NONE,
      &opts->icon, _("Do not use status icon (Ctrl-Alt-P for menu)"),
      NULL
    },
    {
      "clipboard", 'c',
      0,
      G_OPTION_ARG_NONE,
      &opts->clipboard, _("Print clipboard contents"),
      NULL
    },
    {
      "primary", 'p',
      0,
      G_OPTION_ARG_NONE,
      &opts->primary, _("Print primary contents"),
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
                                   "Report bugs to <gpib@rickyrockrat.net>."));
  /* Add entries and parse options */
  g_option_context_add_main_entries(context, main_entries, NULL);
  g_option_context_parse(context, &argc, &argv, NULL);
  g_option_context_free(context);
	opts->leftovers = g_strjoinv(" ", argv + 1);
  /* Check which options were parseed */
  
  /* Do not display icon option */
  if (opts->icon)  {
    prefs.no_icon = TRUE;
  }
	return opts;
}


/***************************************************************************/
/** Return a PID given a name. Used to check a if a process is running..
if 2 or greater, process is running
\n\b Arguments:
\n\b Returns:	number of instances of name found
****************************************************************************/
int proc_find(const char* name, pid_t *pid) 
{
	DIR* dir;
	FILE* fp;
	struct dirent* ent;
  char* endptr;
  char buf[PATH_MAX];
	int instances=0;

  if (!(dir = opendir("/proc"))) {
  	perror("can't open /proc");
    return -1;
  }

  while((ent = readdir(dir)) != NULL) {
    /* if endptr is not a null character, the directory is not
     * entirely numeric, so ignore it */
    long lpid = strtol(ent->d_name, &endptr, 10);
    if (*endptr != '\0') {
        continue;
    }
    /* try to open the cmdline file */
    snprintf(buf, sizeof(buf), "/proc/%ld/cmdline", lpid);

    if ((fp= fopen(buf, "r"))) {
      if (fgets(buf, sizeof(buf), fp) != NULL) {
				gchar *b=g_path_get_basename(buf);
	      if (!g_strcmp0(b, name)) {
					++instances;
					if(NULL !=pid)
					  *pid=lpid;
	      }
      }
      fclose(fp);
    }

  }

  closedir(dir);
  return instances;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
gboolean fifo_read_cb (GIOChannel *src,  GIOCondition cond, gpointer data)
{
	struct p_fifo *f=(struct p_fifo *)data;
	int which;
	if(src == f->g_ch_p)
		which=FIFO_MODE_PRI;
	else if(src == f->g_ch_c)
		which=FIFO_MODE_CLI;
	else{
		g_printf("Unable to determine fifo!!\n");
		return 0;
	}
	if(cond & G_IO_HUP){
	  if(f->dbg) g_printf("gothup ");
		return FALSE;
	}
	if(cond & G_IO_NVAL){
	  if(f->dbg) g_printf("readnd ");
		return FALSE;
	}
	if(cond & G_IO_IN){
	  if(f->dbg) g_printf("norm ");
	}
	
  if(f->dbg) g_printf("0x%X Waiting on chars\n",cond);
	f->rlen=0;
/** (	while (1) {*/
		int s;
		
		s=read_fifo(f,which);
/**  		usleep(100);
		if(-1 == s){
			g_printf("Error reading fifo\n");
			return 0;
		} else if(0 == s)
			break;
	}    
	if(f->rlen>0){
		g_printf("Setting fifo which\n");
		f->which=FIFO_MODE_PRI==which?ID_PRIMARY:ID_CLIPBOARD;
	}    */
		
	return TRUE;
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
gint _create_fifo(gchar *f)
{
	int i=0;
	if(0 == access(f,F_OK)	)
		unlink(f);
	if(-1 ==mkfifo(f,0660)){
		perror("mkfifo ");
		i= -1;
	}
	g_free(f);
	return i;
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int create_fifo(void)
{
	gchar *f;
	int i=0;
	f=g_build_filename(g_get_user_data_dir(), FIFO_FILE_C, NULL);
	if(-1 ==  _create_fifo(f) )
		--i;
	f=g_build_filename(g_get_user_data_dir(), FIFO_FILE_P, NULL);
	if(-1 ==  _create_fifo(f) )
		--i;
	return i;
}


/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int _open_fifo(char *path, int flg)
{
	int fd;
	mode_t mode=0660;
	fd=open(path,flg,mode);
	if(fd<3){
		perror("Unable to open fifo");
	}
	g_free(path);
	return fd;
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int open_fifos(struct p_fifo *fifo)
{
	int flg;
	gchar *f;
	
	if(FIFO_CLIENT == fifo->whoami)
		flg=O_WRONLY|O_NONBLOCK;
	else {/**daemon  if you set O_RDONLY, you get 100%cpu usage from HUP*/
		flg=O_RDWR|O_NONBLOCK;/*|O_EXCL; */
	}	
	
	f=g_build_filename(g_get_user_data_dir(), FIFO_FILE_P, NULL);
	if( (fifo->fifo_p=_open_fifo(f,flg))>2 && FIFO_DAEMON == fifo->whoami){
		if(fifo->dbg) g_printf("PRI fifo %d\n",fifo->fifo_p);
		fifo->g_ch_p=g_io_channel_unix_new (fifo->fifo_p);
		g_io_add_watch (fifo->g_ch_p,G_IO_IN|G_IO_HUP,fifo_read_cb,(gpointer)fifo);
	}
		
	f=g_build_filename(g_get_user_data_dir(), FIFO_FILE_C, NULL);
	if( (fifo->fifo_c=_open_fifo(f,flg)) >2 && FIFO_DAEMON == fifo->whoami){
		fifo->g_ch_c=g_io_channel_unix_new (fifo->fifo_c);
		g_io_add_watch (fifo->g_ch_c,G_IO_IN|G_IO_HUP,fifo_read_cb,(gpointer)fifo);
	}
	if(fifo->dbg) g_printf("CLI fifo %d PRI fifo %d\n",fifo->fifo_c,fifo->fifo_p);
	if(fifo->fifo_c <3 || fifo->fifo_p <3)
		return -1;
	return 0;
}


/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int read_fifo(struct p_fifo *f, int which)
{
	int i,t, fd;
	i=t=0;
	
	switch(which){
		case FIFO_MODE_PRI:
			fd=f->fifo_p;
			f->which=ID_PRIMARY;
			break;
		case FIFO_MODE_CLI:
			fd=f->fifo_c;
			f->which=ID_CLIPBOARD;
			break;
		default:
			g_printf("Unknown fifo %d!\n",which);
			return -1;
	}
	if(NULL ==f || fd <3 ||NULL == f->buf|| f->len <=0)
		return -1;
	while(1){
		i=read(fd,f->buf,f->len-t);
		if(i>0)
			t+=i;
		else 
			break;
	}
	if( -1 == i){
		if( EAGAIN != errno){
			perror("read_fifo");
			return -1;
		}
	}
	f->buf[t]=0;
	f->rlen=t;
	if(t>0)
	  if(f->dbg) g_printf("%s: Got %d '%s'\n",f->fifo_p==fd?"PRI":"CLI",t,f->buf);
	return t;
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int write_fifo(struct p_fifo *f, int which, char *buf, int len)
{
	int i, l,fd;
	l=0;
	switch(which){
		case FIFO_MODE_PRI:
			if(f->dbg) g_printf("Using pri fifo for write\n");
			fd=f->fifo_p;
			break;
		case FIFO_MODE_CLI:
			if(f->dbg) g_printf("Using cli fifo for write\n");
			fd=f->fifo_c;
			break;
		default:
			g_printf("Unknown fifo %d!\n",which);
			return -1;
	}
	if(NULL ==f || fd <3 || NULL ==buf)
		return -1;
	if(f->dbg) g_printf("writing '%s'\n",buf);
	while(len){																							
		i=write(fd,buf,len);
		if(i>0)
			len-=i;
		++l;
		if(l>0x7FFE){
			g_printf("Maxloop hit\n");
			break;
		}
			
	}
	if( -1 == i){
		if( EAGAIN != errno){
			perror("write_fifo");
			return -1;
		}
	}
	return 0;
}




/***************************************************************************/
/** Figure out who we are, then open the fifo accordingly.
return the fifo file descriptor, or -1 on error
GIOChannel*         g_io_channel_unix_new               (int fd);

guint               g_io_add_watch                      (GIOChannel *channel,
                                                         G_IO_IN    GIOFunc func,
                                                         gpointer user_data);
                                                         
g_io_channel_shutdown(channel,TRUE,NULL);
\n\b Arguments:
\n\b Returns:	allocated struct or NULL on fail
****************************************************************************/
struct p_fifo *init_fifo(int mode)
{
	struct p_fifo *f=g_malloc0(sizeof(struct p_fifo));
	if(NULL == f){
		g_printf("Unable to allocate for fifo!!\n");
		return NULL;
	}
	if(NULL == (f->buf=(gchar *)g_malloc0(8000)) ){
		g_printf("Unable to alloc 8k buffer for fifo! Command Line Input will be ignored.\n");
		g_free(f);
		return NULL;
	}
	/**set debug here for debug messages ( f->dbg=1)  */
	f->len=7999;
/*	f->dbg=1; */
/*	g_printf("My PID is %d\n",getpid()); */
	/**we are daemon, and will launch  */
	if(proc_find("parcellite",NULL)<2){
		f->whoami=FIFO_DAEMON;
		if(f->dbg) g_printf("parcellite not found\n");
		if(create_fifo() < 0 ){
			g_printf("error creating fifo\n");
		  goto err;
		}	
		if(open_fifos(f) < 0 ){
			g_printf("Error opening fifo. Exit\n");
			goto err;
		}	
		return f;
		  /**We are the client  */
	}	else{
		f->whoami=FIFO_CLIENT;
		if(f->dbg) g_printf("parcellite found\n");
		if(open_fifos(f) < 0 ){
			g_printf("Error opening fifo. Exit\n");
			goto err;
		}
		return f;
	}
	
err:
	close_fifos(f);
	return NULL;
	
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void close_fifos(struct p_fifo *f)
{
	if(NULL ==f)
		return;
	
	if(NULL != f->g_ch_p)
		g_io_channel_shutdown(f->g_ch_p,TRUE,NULL);
	if(f->fifo_p>0)
		close(f->fifo_p);
	
	if(NULL != f->g_ch_c)
		g_io_channel_shutdown(f->g_ch_c,TRUE,NULL);
	if(f->fifo_c>0)
		close(f->fifo_c);
	g_free(f);
}

