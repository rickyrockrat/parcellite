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

#ifndef UTILS_H
#define UTILS_H

G_BEGIN_DECLS
#include "parcellite.h"
#define CONFIG_DIR  PARCELLITE_PROG_NAME
#define DATA_DIR    PARCELLITE_PROG_NAME

struct cmdline_opts {
	gboolean icon;    
	gboolean clipboard;
	gboolean primary;
	gboolean exit;
	gboolean version;
	gchar *leftovers;
	gint appindicator;
};

gchar *p_strdup( const gchar *str );
void check_dirs( void );

gboolean is_hyperlink(gchar* link);

struct cmdline_opts *parse_options(int argc, char* argv[]);

#define PROG_MODE_CLIENT 2
#define PROG_MODE_DAEMON 1
#define FIFO_MODE_NONE 0x10
#define FIFO_MODE_PRI  0x20 
#define FIFO_MODE_CLI  0x40
#define FIFO_MODE_CMD  0x80

#define PROC_MODE_EXACT 1
#define PROC_MODE_STRSTR 2
#define PROC_MODE_USER_QUALIFY 4

struct p_fifo {
	int whoami;					/**client or daemon  */
	int fifo_p;					/**primary fifo  */
	int fifo_c;					/**clipboard fifo  */
	int fifo_cmd;				/**command interface (in to daemon) */
	int fifo_dat;			/**data out  */
	GIOChannel *g_ch_p; /**so we can add watches in the main loop  */
	GIOChannel *g_ch_c;	/**so we can add watches in the main loop  */
	GIOChannel *g_ch_cmd;	/** cmd watcher */
	gchar *cbuf; /**cmd buffer  */
	gchar *dbuf; /**data buffer  */
	gchar *buf;	 /**the data itself  */
	gint clen;  /**cmd len  */
	gint tclen;  /**allocated cmd len  */
	gint dlen;  /**data len  */
	gint len;		/**total len of buffer (alloc amount)  */
	gint rlen;	/**received lenght  */
	gint which;	/**which clipboard the buffer should be written to  */
	gint dbg;		/**set to debug fifo system  */
};


int proc_find(const char* name, int mode, pid_t *pid);
int create_fifo(void);
int open_fifos(struct p_fifo *fifo);
int read_fifo(struct p_fifo *f, int which);
int write_fifo(struct p_fifo *f, int which, char *buf, int len);
struct p_fifo* init_fifo(int mode);
void close_fifos(struct p_fifo *p);
void show_gtk_dialog(gchar *message, gchar *title);
G_END_DECLS

#endif
