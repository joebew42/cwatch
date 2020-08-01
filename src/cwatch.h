/* cwatch.h
 * Monitor file system activity using the inotify linux kernel library
 *
 * Copyright (C) 2012, Joe Bew <joebew42@gmail.com>,
 *                     Vincenzo Di Cicco <enzodicicco@gmail.com>
 *
 * This file is part of cwatch
 *
 * cwatch is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * cwatch is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef __CWATCH_H
#define __CWATCH_H

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <signal.h>
#include <stdarg.h>
#include <syslog.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <dirent.h>
#include <regex.h>
#include <sys/inotify.h>
#include <sys/param.h>
#include <sys/stat.h>

#include "bstrlib.h"
#include "list.h"

#define PROGRAM_NAME "cwatch"
#define PROGRAM_VERSION "1.2.3"
#define PROGRAM_STAGE "experimental"

#define EVENT_SIZE (sizeof(struct inotify_event))
#define EVENT_BUF_LEN (1024 * (EVENT_SIZE + 16))

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

/* List of pattern that will be replaced during the command execution
 * Note: See their initialization in the monitor() function
 *
 * _ROOT  (%r) when cwatch execute the command, will be replaced with the
 *             root monitored directory
 * _PATH  (%p) when cwatch execute the command, will be replaced with the
 *             absolute full path of the file or directory where the
 *             event occurs
 * _FILE  (%f) when cwatch execute the command, will be replaced with the
 *             name of the file or directory that triggered
 *             the event.
 * _EVENT (%e) when cwatch execute the command, will be replaced with the
 *             event type occured
 * _REGEX (%x) when cwatch execute the command, will be replaced with the
 *             first regex occurence that match with the regular expression
 *             suited by -X --regex-catch option
 * _COUNT (%n) when cwatch execute the command, will be replaced with the
 *             count of the events
 */
bstring COMMAND_PATTERN_ROOT;
bstring COMMAND_PATTERN_PATH;
bstring COMMAND_PATTERN_FILE;
bstring COMMAND_PATTERN_EVENT;
bstring COMMAND_PATTERN_REGEX;
bstring COMMAND_PATTERN_COUNT;

typedef enum
{
    FALSE,
    TRUE
} bool_t;

/* used to store information about watched resource */
typedef struct wd_data_s
{
    int wd;       /* inotify watch descriptor */
    char *path;   /* absolute real path of the directory */
    Queue *links; /* list of symlinks that point to this resource */
} WD_DATA;

/* used to store information about symbolic link */
typedef struct link_data_s
{
    char *path;       /* absolute real path of the symbolic link */
    WD_DATA *wd_data; /* a pointer to it wd_data */
} LINK_DATA;

/* used to describe an event in the events LUT.
 * see the complete LUT definition int cwatch.c
 */
struct event_t
{
    char *name;
    uint32_t mask;
    int (*handler)(struct inotify_event *, char *, int, Queue *);
};

char *root_path;              /* root path that cwatch is monitoring */
bstring command;              /* the command to be execute, defined by -c option*/
bstring format;               /* a string containing the output format defined by -F option */
bstring tmp_command;          /* temporary command used by execute_command */
struct bstrList *split_event; /* list of events parsed from command line */
uint32_t event_mask;          /* the resulting event_mask */
regex_t *exclude_regex;       /* the posix regular expression defined by -x option */
regex_t *user_catch_regex;    /* the posix regular expression defined by -X option */
regmatch_t p_match[2];        /* store the matched regular expression by -X option */

int exec_c;         /* the number of times command is executed */
char exec_cstr[10]; /* used as conversion of exec_c to cstring */

bool_t nosymlink_flag;
bool_t recursive_flag;
bool_t verbose_flag;
bool_t syslog_flag;

/* function pointer to inotify_add_watch
 *
 * @param  int          : file descriptor
 * @param  const char * : absolute real path
 * @param  uint32_t     : event mask
 * @return int          : watch descriptor
 */
int (*watch_descriptor_from)(int, const char *, uint32_t);

/* function pointer to inotify_rm_watch
 *
 * @param  int          : file descriptor
 * @param  int          : watch descriptor
 * @return int          : 0 if success, -1 otherwise
 */
int (*remove_watch_descriptor)(int, int);

/* print the version of the program and exit */
void print_version();

/* print out the help and exit
 *
 * @param int    : exit code
 * @param char * : message to print at exit
 */
void help(int, char *);

/* log message via syslog or via standard output
 * this function act like a printf
 *
 * @param char * : message to log
 * @param ...    : additional characters
 */
void log_message(char *, ...);

/* resolve the real path of a symbolic or relative path
 *
 * @param  const char * : path of the symbolic link to resolve
 * @return char *
 */
char *
resolve_real_path(const char *);

/*
 * checks if the given path is a directory.
 * If the path is a symlink checks the pointed resource.
 *
 * @param  const char * : path
 * @return bool_t
 */
bool_t
is_dir(const char *);

/*
 * Return a new string appending a directory path
 * to another.
 * Ensure that the path has *one* trailing slash.
 *
 * Warning: the function doesn't checks if the given
 * paths are malformed.
 *
 * @param const char * : first path
 * @param const cahr * : directory path to append
 * @return char *      : the compete path, or NULL
 *                       if insufficient memory
 */
char *
append_dir(const char *, const char *);

/*
 * Return a new string appending a filename to a
 * path.
 * Ensure that the path doesn't has the trailing slash.
 *
 * Warning: the function doesn't checks if the given
 * path is malformed.
 *
 * @param const char * : path
 * @param const cahr * : filename to append
 * @return char *      : the compete path, or NULL
 *                       if insufficient memory
 */
char *
append_file(const char *, const char *);

/* searchs and returns the node of the specified path
 *
 * @param  const char * : absolute path to find
 * @param  Queue *       : list of watched resources
 * @return QueueElement *
 */
QueueElement *
get_node_from_path(const char *, Queue *);

/* searchs and returns the node of the specified watch descriptor
 *
 * @param  const int   : wd to find
 * @param  Queue *      : list of watched resources
 * @return QueueElement *
 */
QueueElement *
get_node_from_wd(const int, Queue *);

/* creates a wd_data
 *
 * @param char *     : absolute real path
 * @param int        : wd
 * @return WD_DATA *
 */
WD_DATA *
create_wd_data(char *, int);

/* searchs and returns the list_node from symlink path
 *
 * @param  const char * : absolute path to find
 * @param  Queue *       : list of watched resources
 * @return QueueElement *
 */
QueueElement *
get_link_node_from_path(const char *, Queue *);

/* checks if a path stored in the list of
 * watched resources is a symbolic link or not
 *
 * @param  char * : path
 * @param  Queue * : list of watched resources
 * @return bool_t
 */
bool_t
is_symlink(char *, Queue *);

/* searchs and returns the link_data from symlink path
 * of a specified WD_DATA
 *
 * @param  const char *    : absolute path to find
 * @param  const WD_DATA * : WD_DATA pointer in which search
 * @return LINK_DATA *
 */
LINK_DATA *
get_link_data_from_wd_data(const char *, const WD_DATA *);

/* searchs and returns the link_data from symlink path
 *
 * @param  const char * : absolute path to find
 * @param  Queue *       : list of watched resources
 * @return LINK_DATA *
 */
LINK_DATA *
get_link_data_from_path(const char *, Queue *);

/* creates a LINK_DATA
 *
 * @param char * : absolute path of symbolic link
 * @WD_DATA *    : WD_DATA in which symbolic link will be attached
 * @return LINK_DATA *
 */
LINK_DATA *
create_link_data(char *, WD_DATA *);

/* checks whetever a string is contained in a list
 * as a substring
 *
 * @param char * : string to check
 * @param Queue * : list in which performs search
 * @return boolt_t
 */
bool_t
is_listed_as_child(char *, Queue *);

/* returns TRUE if the second path is a child of the second one
 * FALSE otherwise
 *
 * @param  const char * : first path
 * @param  const char * : second path
 * @return bool_t
 */
bool_t
is_child_of(const char *, const char *);

/* checks whetever a string match the regular
 * expression pattern defined with -x option
 * See: exclude_regex
 *
 * @param  char * : string to check
 * @return bool_t
 */
bool_t
excluded(char *);

/* checks whetever a pattern match the regular
 * expression pattern defined with -X option
 * See: user_catch_regex
 *
 * @param  char * : string to check
 * @return bool_t
 */
bool_t
regex_catch(char *);

/* return the subexpression matched by regex_catch
 *
 * @param  char * : string to check
 * @return char * : matched subexpression
 */
char *
get_regex_catch(char *);

/* replace all occurrences in the command or format string
 * specified by the user, with the special characters
 *
 * @param  char *  : command (-c) or the format (-F) defined by user
 * @param  char *  : full path in which event was triggered
 * @param  char *  : name of the file or directory that triggered the event
 * @param  char *  : event name
 * @return bstring
 */
bstring
format_command(char *, char *, char *, char *);

/* parse the command line
 *
 * @param  int     : number of arguments
 * @param  char ** : arguments
 * @return int
 */
int parse_command_line(int, char **);

/* it performs a breadth-first-search to visit a directory tree
 * and call the add_to_watch_list(path) for each directory,
 * either if it's pointed by a symbolic link or not.
 *
 * @param  char *   : absolute path of directory to watch
 * @param  char *   : symbolic link that point to the path
 * @param  bool_t * : traverse directory recursively or not
 * @param  int      : inotify file descriptor
 * @param  Queue *   : list of watched resources
 * @return int      : -1 (An error occurred), 0 (Resource added correctly)
 */
int watch_directory_tree(char *, char *, bool_t, int, Queue *);

/* add a directory into watch Queue
 *
 * @param  char *      : absolute path of the directory to watch
 * @param  char *      : symbolic link that points to the absolute path
 * @param  int         : inotify file descriptor
 * @param  Queue *      : list of watched resources
 * @return QueueElement * : pointer of the node added in the watch list
 */
QueueElement *
add_to_watch_list(char *, char *, int, Queue *);

/* given a real path unwatch a directory from the watch list
 *
 * @param char *  : absolute path of the resource to remove
 * @param int     : inotify file descriptor
 * @param Queue *  : list of watched resources
 */
void unwatch_path(char *, int, Queue *);

/* searches for all symbolic links that are contained
 * in a path, and put them into another list
 *
 * @param char * : path to check
 * @param Queue * : list of all watched resources
 * @param Queue * : list of symbolic links found
 */
void all_symlinks_contained_in(char *, Queue *, Queue *);

/* from a given list of symbolic links,
 * extract all of them that are contained in a path
 *
 * @param char * : parent path to check against
 * @param Queue * : list of symbolic links to check
 * @param Queue * : list of symbolic links found
 */
void symlinks_contained_in(char *, Queue *, Queue *);

/* if there is no other symbolic links that point to the
 * watched resource and the watched resource is not a child
 * of the the root path unwatch it and relative orphan directories
 *
 * @param WD_DATA * : watch descriptor
 * @param int       : inotify file descriptor
 * @param Queue *    : list of watched resources
 */
void remove_unreachable_resources(WD_DATA *, int, Queue *);

/* returns a Queue of paths that holds:
 * - each path is related with some other path
 * - each path is referenced by a symbolic link
 *
 * @param const char * : path to inspect
 * @param Queue *       : list of referenced paths
 */
Queue *
common_referenced_paths_for(const char *, Queue *);

/* returns TRUE if a path is related to another,
 * FALSE, otherwise
 *
 * @param  const char * : first path
 * @param  const char * : second path
 * @return bool_t
 */
bool_t
is_related_to(const char *, const char *);

/* removes from the watch list all resources that are no
 * longer referenced by symbolic links and are outside
 * from the root_path
 *
 * @param const char * : absolute path to remove
 * @param Queue *       : list of all path that are referenced by symbolic link
 * @param int          : inotify file descriptor
 * @param Queue *       : list of watched resources
 */
void remove_orphan_watched_resources(const char *, Queue *, int, Queue *);

/* given a symbolic link unwatch a directory from the watch list
 *
 * @param char *  : symbolic link of the resource to remove
 * @param int     : inotify file descriptor
 * @param Queue *  : list of watched resources
 */
void unwatch_symlink(char *, int, Queue *);

/* start monitoring of inotify event on watched resources
 *
 * @param int          : inotify file descriptor
 * @param Queue *       : list of watched resources
 */
int monitor(int, Queue *);

/* COMMAND EXECUTION HANDLER
 *
 * _inline   : called when the -c --command option is given
 * _embedded : called when the -F --format  option is given
 *
 * These functions handles the execution of a command
 *
 * @param  char *  : the inotify event name
 * @param  char *  : the name of file/directory that triggered the event
 * @param  char *  : the path where event occured
 * @return int    : -1 in case of error, 0 otherwise
 */
int (*execute_command)(char *, char *, char *);
int execute_command_inline(char *, char *, char *);
int execute_command_embedded(char *, char *, char *);

/* get the inotify event handler from the event mask
 *
 * @param const uint32_t : inotify event mask
 */
struct event_t *
get_inotify_event(const uint32_t);

/* EVENT HANDLER DEFINITION
 *
 * handler functions called when an event occurs
 *
 * @param struct inotify_event * : inotify event
 * @param char *                 : the path of file or directory that triggered
 *                                 the event
 * @param Queue *                 : the list of all watched resources
 * @param int                    : file descriptor
 * @return int                   : -1 if errors occurs, 0 otherwise
 */
int event_handler_undefined(struct inotify_event *, char *, int, Queue *);
int event_handler_create(struct inotify_event *, char *, int, Queue *);
int event_handler_delete(struct inotify_event *, char *, int, Queue *);
int event_handler_moved_from(struct inotify_event *, char *, int, Queue *);
int event_handler_moved_to(struct inotify_event *, char *, int, Queue *);

/* handler function called when a signal occurs
 *
 * @param int : signal identifier
 */
void signal_callback_handler(int);
#endif /* !__CWATCH_H */
