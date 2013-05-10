/* cwatch.h
 * Monitor file system activity using the inotify linux kernel library
 *
 * Copyright (C) 2012, Giuseppe Leone <joebew42@gmail.com>,
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

#include "bstrlib.h"
#include "list.h"

#define PROGRAM_NAME    "cwatch"
#define PROGRAM_VERSION "1.2.3"
#define PROGRAM_STAGE   "experimental"

#define EVENT_SIZE      (sizeof (struct inotify_event))
#define EVENT_BUF_LEN   (1024 * ( EVENT_SIZE + 16 ))

/*
 * List of pattern that will be replaced during the command execution
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
const_bstring COMMAND_PATTERN_ROOT;
const_bstring COMMAND_PATTERN_PATH;
const_bstring COMMAND_PATTERN_FILE;
const_bstring COMMAND_PATTERN_EVENT;
const_bstring COMMAND_PATTERN_REGEX;
const_bstring COMMAND_PATTERN_COUNT;

typedef enum {FALSE,TRUE} bool_t;

/* Used to store information about watched resource */
typedef struct wd_data_s
{
    int    wd;            /* watch descriptor */
    char   *path;         /* absolute real path of the directory */
    LIST   *links;        /* list of symlinks that point to this resource */
} WD_DATA;

/* Used to store information about symbolic link */
typedef struct link_data_s
{
    char    *path;         /* absolute real path of the symbolic link */
    WD_DATA *wd_data;      /* a pointer to it wd_data */
} LINK_DATA;

/*
 * Used to describe an event in the events LUT.
 * See the complete LUT definition int cwatch.c
 */
struct event_t
{
    char *name;           /* the event name (delete,create,modify,etc ...) */
    int (*handler)(
        struct inotify_event *,
        char *
        );                /* function handler called when the event occurs */
};

char *root_path;                /* root path that cwatch is monitoring */
bstring command;                /* the command to be execute, defined by -c option*/
bstring format;                 /* a string containing the output format defined by -F option */
bstring tmp_command;            /* temporary command used by execute_command */
int (*execute_command)(
    char *,
    char *,
    char *);                    /* the command to be executed when an event is triggered */
struct bstrList *split_event;   /* list of events parsed from command line */
uint32_t event_mask;            /* the resulting event_mask */
regex_t *exclude_regex;         /* the posix regular expression defined by -x option */
regex_t *user_catch_regex;      /* the posix regular expression defined by -X option */
regmatch_t p_match[2];          /* store the matched regular expression by -X option */

int fd;                         /* inotify file descriptor */
LIST *list_wd;                  /* the list of all watched resource */

int exec_c;                      /* the number of times command is executed */
char exec_cstr[10];              /* used as conversion of exec_c to cstring */

bool_t nosymlink_flag;
bool_t recursive_flag;
bool_t verbose_flag;
bool_t syslog_flag;

/**
 * Print the version of the program and exit
 */
void print_version();

/**
 * Help
 *
 * @param int : if > 0 help return with exit(int), 0 otherwise
 * Print out the help
 */
int help(int);

/**
 * Log
 *
 * Log message via syslog or via standard output
 * @param char * : Message to log
 */
void log_message(char *, ...);

/**
 * Resolve the real path
 *
 * This function is used to resolve the
 * absolute real_path of a symbolic link or a relative path.
 * @param char *  : the path of the symbolic link to resolve
 * @return char * : the resolved real_path, NULL otherwise
 */
char *resolve_real_path(const char *);

/**
 * Searchs and returns the node for the path passed
 * as argument.
 * @param char *       : The path to find
 * @return LIST_NODE * : A pointer to node, NULL otherwise.
 */
LIST_NODE *get_node_from_path(const char *);

/**
 * Searchs and returns the node for the wd passed
 * as argument.
 * @param int          : The wd to find
 * @return LIST_NODE * : a pointer to node, NULL otherwise.
 */
LIST_NODE *get_node_from_wd(const int);

/**
 * Create a WD_DATA
 *
 * @param char *     : The real path
 * @param int *      : the watch descriptor
 * @return WD_DATA * : a pointer to WD_DATA, NULL otherwise.
 */
WD_DATA *create_wd_data(char *, int);

/**
 * Searchs and returns the list_node from symlink path
 *
 * @param char *       : The path to find
 * @return LIST_NODE * : a pointer to list_node, NULL otherwise.
 */
LIST_NODE *get_link_node_from_path(const char *);

/**
 * Searchs and returns the link_data from symlink path
 * of a specified WD_DATA
 *
 * @param char *       : The path to find
 * @param WD_DATA *    : a pointer to WD_DATA in which search
 * @return LINK_DATA * : a pointer to link data, NULL otherwise.
 */
LINK_DATA *get_link_data_from_wd_data(const char *, const WD_DATA *);

/**
 * Searchs and returns the link_data from symlink path
 *
 * @param char *       : The path to find
 * @return LINK_DATA * : a pointer to link data, NULL otherwise.
 */
LINK_DATA *get_link_data_from_path(const char *);

/**
 * Create a LINK_DATA
 *
 * @param char *       : The path of symlink
 * @param WD_DATA *    : The wd_data in which symlink will be attached
 * @return LINK_DATA * : a pointer to link data, NULL otherwise.
 */
LINK_DATA *create_link_data(char *, WD_DATA *);

/**
 * Returns TRUE if the first path is a child
 * of the second one.
 * @param char * : first path  (the child)
 * @param char * : second path (the parent)
 * @return bool_t
 */
bool_t is_child_of(const char *, const char *);

/**
 * Checks whetever a string is contained in a list
 *
 * @param char *  : string to check
 * @param LIST *  : list containing string
 * @return bool_t : TRUE if string is contained, FALSE otherwise
 */
bool_t is_listed_in(char *, LIST *);

/**
 * Checks whetever a string match the regular
 * expression pattern defined with -x option
 * See: exclude_regex
 *
 * @param char * : string to check
 */
bool_t excluded(char *);

/**
 * Checks whetever a pattern match the regular
 * expression pattern defined with -X option
 * See: user_catch_regex
 *
 * @param char * : string to check
 */
bool_t regex_catch(char *);

/**
 * Return the subexpression matched by regex_catch
 *
 * @param  char * : string to check
 * @return char * : matched subexpression
 */
char *get_regex_catch(char *);

/**
 * Replace all occurrences in the command/format
 * specified by the user, with the special characters
 *
 * @param char *   : the command (-c) or the format (-F) defined by user
 * @param char *   : the full path in which event was triggered
 * @param char *   : the name of the file or directory that triggered the event
 * @param char *   : the event name
 * @return bstring : the resulting string with all occurrences replaced
 */
bstring format_command(char *, char *, char *, char *);

/**
 * Parse command line
 *
 * This function is used to parse command line and initialize some environment variables
 * @param int     : arguments count
 * @param char ** : arguments value
 * @return int
 */
int parse_command_line(int, char **);

/**
 * Watch a directory
 *
 * It performs a breath-first-search to traverse a directory and
 * call the add_to_watch_list(path) for each directory, either if it's pointed by a symbolic link or not.
 * @param char * : The path of directory to watch
 * @param char * : The symbolic link that point to the path
 * @return int   : -1 (An error occurred), 0 (Resource added correctly)
 */
int watch(char *, char *);

/**
 * Add a directory into watch list
 *
 * This function is used to append a directory into watch list
 * @param char* : The absolute path of the directory to watch
 * @param char* : The symbolic link that point to the path
 * @return LIST_NODE* : the pointer of the node of the watch list
 */
LIST_NODE *add_to_watch_list(char *, char *);

/**
 * Unwatch a directory
 *
 * Used to remove a file or directory
 * from the list of watched resources
 * @param char * : the path of the resource to remove
 * @param bool_t : TRUE if the path to unwatch is a symlink, FALSE otherwise.
 */
void unwatch(char *, bool_t);

/**
 * Return a list of path that is referenced by a symbolic link
 * and is child or a parent of the pathe given as argument
 * @param char * : the path to inspect
 * @return LIST  : the list of all paths
 */
LIST *list_of_referenced_path(const char *);

/**
 * Remove from the watch list all resources that are no
 * longer referenced by symbolic links and are extern
 * from the root_path
 * @param char * : the path to remove
 * @return LIST  : the list of all path that are referenced by symlink
 */
void remove_orphan_watched_resources(const char *, LIST *);

/**
 * Unwatch a symbolic link from the watched resources
 *
 * @param LIST_NODE * : the list_node of the symbolic link to unwatch
 */
void unwatch_symbolic_link(LIST_NODE *);

/**
 * Start monitoring
 *
 * Used to monitor inotify event on watched resources
 */
int monitor();

/**
 * Execute a command
 * *_inline   : called when the -c --command option is given
 * *_embedded : called when the -F --format  option is given
 *
 * This function handle the execution of a command
 * @param char *  : the inotify event name
 * @param char *  : the name of file/directory that triggered the event
 * @param char *  : the path where event occured
 * @return int    : -1 in case of error, 0 otherwise
 */

int execute_command_inline(char *, char *, char *);
int execute_command_embedded(char *, char *, char *);

/**
 * Get the inotify event handler from the event mask
 *
 * @param uint32_t : the inotify event mask
 * @return struct event_t  : the event
 */
struct event_t *get_inotify_event(const uint32_t);

/**
 * EVENT HANDLER DEFINITION
 *
 * Handler functions called when an event occurs
 *
 * @param struct inotify_event * : inotify event
 * @param char *                 : the path of file or directory that triggered the event
 * @return int                   : -1 if errors occurs, 0 otherwise
 */

int event_handler_undefined(struct inotify_event *, char *);   /* NO ACTION, it always return 0 */
int event_handler_create(struct inotify_event *, char *);      /* IN_CREATE */
int event_handler_delete(struct inotify_event *, char *);      /* IN_DELETE */
int event_handler_moved_from(struct inotify_event *, char *);  /* IN_MOVED_FROM */
int event_handler_moved_to(struct inotify_event *, char *);    /* IN_MOVED_TO */

/* SIGNAL HANDLER DEFINITION
 *
 * Handler function called when a signal occurs
 *
 * @param int : ID of signal
 */

void signal_callback_handler(int);
#endif /* !__CWATCH_H */
