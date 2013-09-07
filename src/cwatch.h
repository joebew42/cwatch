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
bstring COMMAND_PATTERN_ROOT;
bstring COMMAND_PATTERN_PATH;
bstring COMMAND_PATTERN_FILE;
bstring COMMAND_PATTERN_EVENT;
bstring COMMAND_PATTERN_REGEX;
bstring COMMAND_PATTERN_COUNT;

/* event names */
bstring B_ACCESS;
bstring B_MODIFY;
bstring B_ATTRIB;
bstring B_CLOSE_WRITE;
bstring B_CLOSE_NOWRITE;
bstring B_CLOSE;
bstring B_OPEN;
bstring B_MOVED_FROM;
bstring B_MOVED_TO;
bstring B_MOVE;
bstring B_CREATE;
bstring B_DELETE;
bstring B_DELETE_SELF;
bstring B_UNMOUNT;
bstring B_Q_OVERFLOW;
bstring B_IGNORED;
bstring B_ISDIR;
bstring B_ONESHOT;
bstring B_DEFAULT;
bstring B_ALL_EVENTS;

typedef enum {FALSE,TRUE} bool_t;

/* Used to store information about watched resource */
typedef struct wd_data_s
{
    int    wd;            /* inotify watch descriptor */
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

/* TODO: GlobalVariablesAreEvil - Get Rid of Them */
/* Consider to push them down and passing as parameter to function */
/* that use them. */
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

int exec_c;                     /* the number of times command is executed */
char exec_cstr[10];             /* used as conversion of exec_c to cstring */

/* function pointer to inotify_add_watch */
int (*watch_descriptor_from)(
    int,                        /* file descriptor */
    const char *,               /* absolute full path */
    uint32_t );                 /* event mask */

bool_t nosymlink_flag;
bool_t recursive_flag;
bool_t verbose_flag;
bool_t syslog_flag;

/* print the version of the program and exit */
void
print_version();

/* print out the help and exit */
void
help(
    int,                        /* exit code */
    char * );                   /* message to print at exit */

/* log message via syslog or via standard output */
void
log_message(
    char *,                     /* message to log */
    ... );                      /* additional characters like printf */

/* resolve the real path of a symbolic or relative path */
char *                          /* resolved real path, NULL otherwise */
resolve_real_path(
    const char * );             /* path of the symbolic link to resolve */

/* searchs and returns the node of the specified path */
LIST_NODE *                     /* LIST_NODE pointer, NULL otherwise */
get_node_from_path(
    const char *,               /* absolute path to find */
    LIST * );                   /* list of watched resources */

/* searchs and returns the node of the specified wd */
LIST_NODE *                     /* LIST_NODE pointer, NULL otherwise */
get_node_from_wd(
    const int,                  /* wd to find */
    LIST * );                   /* list of watched resources */

/* creates a wd_data */
WD_DATA *                       /* WD_DATA pointer, NULL otherwise */
create_wd_data(
    char *,                     /* absolute real path */
    int );                      /* inotify watch descriptor (wd) */

/* searchs and returns the list_node from symlink path */
LIST_NODE *                     /* LIST_NODE pointer, NULL otherwise */
get_link_node_from_path(
    const char *,               /* absolute path to find */
    LIST * );                   /* list of watched resources */

/* searchs and returns the link_data from symlink path */
/* of a specified WD_DATA */
LINK_DATA *                     /* LINK_DATA pointer, NULL otherwise */
get_link_data_from_wd_data(
    const char *,               /* absolute path to find */
    const WD_DATA * );          /* WD_DATA pointer in which search */

/* searchs and returns the link_data from symlink path */
LINK_DATA *                     /* LINK_DATA pointer, NULL otherwise */
get_link_data_from_path(
    const char *,               /* absolute path to find */
    LIST * );                   /* list of watched resources */

/* creates a LINK_DATA */
LINK_DATA *                     /* LINK_DATA pointer, NULL otherwise */
create_link_data(
    char *,                     /* absolute path of symbolic link */
    WD_DATA * );                /* WD_DATA in which symbolic link */
                                /* will be attached */

/* returns TRUE if the first path is a child of the second one */
/* FALSE otherwise */
bool_t
is_child_of(
    const char *,               /* first path */
    const char * );             /* second path */

/* checks whetever a string is contained in a list */
bool_t                          /* TRUE if found, FALSE otherwise */
is_listed_in(
    char *,                     /* string to check */
    LIST * );                   /* list in which perform search */

/* checks whetever a string match the regular */
/* expression pattern defined with -x option */
/* See: exclude_regex */
bool_t
excluded(
    char * );                   /* string to check */

/* checks whetever a pattern match the regular */
/* expression pattern defined with -X option */
/* See: user_catch_regex */
bool_t
regex_catch(
    char * );                   /* string to check */

/* return the subexpression matched by regex_catch */
char *                          /* matched subexpression */
get_regex_catch(
    char * );                   /* string to check */

/* replace all occurrences in the command or format string */
/* specified by the user, with the special characters */
bstring                         /* resulting string with all occurrences replaced */
format_command(
    char *,                     /* command (-c) or the format (-F) defined by user */
    char *,                     /* full path in which event was triggered */
    char *,                     /* name of the file or directory that triggered the event */
    char * );                   /* event name */

/* parse the command line */
int
parse_command_line(
    int,                        /* arguments count */
    char ** );                  /* argument values */

/* it performs a breadth-first-search to visit a directory tree */
/* and call the add_to_watch_list(path) for each directory, */
/* either if it's pointed by a symbolic link or not. */
int                             /* -1 (An error occurred), 0 (Resource added correctly) */
watch_directory_tree(
    char *,                     /* absolute path of directory to watch */
    char *,                     /* symbolic link that point to the path */
    bool_t,                     /* traverse directory recursively or not */
    int,                        /* inotify file descriptor */
    LIST * );                   /* list of watched resources */

/* add a directory into watch list */
LIST_NODE *                     /* pointer of the node added in the watch list*/
add_to_watch_list(
    char *,                     /* absolute path of the directory to watch */
    char *,                     /* symbolic link that points to the absolute path */
    int,                        /* inotify file descriptor */
    LIST * );                   /* list of watched resources */

/* unwatch a directory */
void
unwatch(
    char *,                     /* absolute path of the resource to remove */
    bool_t );                   /* TRUE if is path is a symbolic link, FALSE otherwise */

/* returns a LIST of path that is referenced by a symbolic link */
LIST *
list_of_referenced_path(
    const char * );             /* absolute path to inspect */

/* removes from the watch list all resources that are no */
/* longer referenced by symbolic links and are extern */
/* from the root_path */
void
remove_orphan_watched_resources(
    const char *,               /* absolute path to remove */
    LIST * );                   /* LIST of all path that are referenced */
                                /* by symbolic link */

/* unwatch a symbolic link from the watched resources */
void
unwatch_symbolic_link(
    LIST_NODE * );              /* LIST_NODE of the symbolic link to unwatch */

/* start monitoring of inotify event on watched resources */
int
monitor(
    int,                        /* inotify file descriptor */
    LIST *);                    /* list of watched resources */

/* COMMAND EXECUTION HANDLER
 *
 * _inline   : called when the -c --command option is given
 * _embedded : called when the -F --format  option is given
 *
 * These functions handles the execution of a command
 *
 * @param char *  : the inotify event name
 * @param char *  : the name of file/directory that triggered the event
 * @param char *  : the path where event occured
 * @return int    : -1 in case of error, 0 otherwise
 */
int execute_command_inline(char *, char *, char *);
int execute_command_embedded(char *, char *, char *);

/* get the inotify event handler from the event mask */
struct event_t *                /* event handler descriptor */
get_inotify_event(
    const uint32_t );           /* inotify event mask */

/* EVENT HANDLER DEFINITION
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

/* handler function called when a signal occurs */
void
signal_callback_handler(
    int );                      /* signal identifier */
#endif /* !__CWATCH_H */
