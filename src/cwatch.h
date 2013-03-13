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
#include <syslog.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <dirent.h>
#include <sys/inotify.h>
#include <sys/param.h>

#include "list.h"

#define PROGRAM_NAME    "cwatch"
#define PROGRAM_VERSION "1.0"
#define PROGRAM_STAGE   "experimental"

#define EVENT_SIZE      sizeof (struct inotify_event)
#define EVENT_BUF_LEN   1024 * ( EVENT_SIZE + 16 )

/*
 * _ROOT when cwatch execute the command will be replaced with the
 *       root monitored directory
 * _FILE when cwatch execute the command will be replaced with the
 *       absolute full path of the file where the event occured.
 * _TYPE when cwatch execute the command will be replaced with the
 *       event type occured
 */
#define COMMAND_PATTERN_ROOT   "%r"
#define COMMAND_PATTERN_FILE   "%f"
#define COMMAND_PATTERN_EVENT  "%e"

typedef enum {FALSE,TRUE} bool_t;

/* Used to store information about watched resource */
typedef struct wd_data_s
{
    int    wd;            /* watch descriptor */
    char   *path;         /* absoulete real path of the directory */
    bool_t symbolic_link; /* used to know if is reached by symbolic link */
    LIST   *links;        /* list of sym links that point to this resource */
} WD_DATA;

/* Used by str_split() (see function definition below) */
typedef struct str_split_s
{
    unsigned short size;
    char           **substring;
} STR_SPLIT_S;

/*
 * Used to describe an event in the events LUT.
 * See the complete LUT definition ad the end of this file.
 */
struct event_t
{
    char *name;           /* the event name (delete,create,modify,etc ...) */
    int (*handler)(
        struct inotify_event *,
        char *
        );                /* function handler called when the event occurs */
};

char *root_path;          /* root path that cwatch is monitoring */
char *command;            /* the full string of command to be execute */
STR_SPLIT_S *scommand;    /* the splitted command, used in execute_command() */
STR_SPLIT_S *sevents;     /* the list of event_mask to monitor */
uint32_t event_mask;      /* the resulting event_mask */

int fd;                   /* inotify file descriptor */
LIST *list_wd;            /* the list of all watched resource */

bool_t recursive_flag;
bool_t all_flag;
bool_t verbose_flag;
bool_t syslog_flag;

/**
 * Print the version of the program and exit
 */
void print_version();

/**
 * Help
 * 
 * Print out the help
 */
void help();

/**
 * Log
 * 
 * Log message via syslog or via standard output
 * @param char * : Message to log
 */
void log_message(char *);

/**
 * Print List
 * 
 * Only an help fuction that print list_wd
 * @param LIST * : the list_wd you want print
 */
void print_list(LIST *);

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
LIST_NODE *get_from_path(const char *);

/**
 * Searchs and returns the node for the wd passed
 * as argument.
 * @param int          : The wd to find
 * @return LIST_NODE * : a pointer to node, NULL otherwise.
 */
LIST_NODE *get_from_wd(const int);

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
 * Checks whetever a string exists in a list
 *
 * @param char * : string to check
 * @param LIST * : list containing string
 */
int exists(char *, LIST *);

/**
 * Start monitoring
 * 
 * Used to monitor inotify event on watched resources
 */
int monitor();

/**
 * str_split
 *
 * subdivide a string into n-substring, that are separate
 * by a specified separator symbol.
 * @param char *      : the string to subdivide
 * @param char *      : separator symbol
 * @param STR_SPLIT_R : a data structure contains the splitted string
 */
STR_SPLIT_S *str_split(char *, char *);

/**
 * Execute a command
 *
 * This function handle the execution of a command 
 * @param char *  : the inotify event name
 * @param char *  : the path where event occured
 * @return int    : -1 in case of error, 0 otherwise
 */
int execute_command(char *, char *);

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
 * @param char *                 : the path where event occurs
 * @return int                   : -1 if errors occurs, 0 otherwise
 */

int event_handler_undefined(struct inotify_event *, char *);   /* NO ACTION, it always return -1 */
int event_handler_create(struct inotify_event *, char *);      /* IN_CREATE */
int event_handler_delete(struct inotify_event *, char *);      /* IN_DELETE */
int event_handler_moved_from(struct inotify_event *, char *);  /* IN_MOVED_FROM */
int event_handler_moved_to(struct inotify_event *, char *);    /* IN_MOVED_TO */

#endif /* !__CWATCH_H */
