/* cwatch.h
 * Monitor file system activity using the inotify linux kernel library
 *
 * Copyright (C) 2012, Giuseppe Leone <joebew42@gmail.com>,
 *                     Vincenzo Di Cicco <enzodicicco@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
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
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <sys/param.h>
#include <syslog.h>
#include <errno.h>

#include "list.h"

/* Size of an event */
#define EVENT_SIZE      ( sizeof (struct inotify_event) )
#define EVENT_BUF_LEN   ( 1024 * ( EVENT_SIZE + 16 ) )

/* Boolean data type */
typedef char bool;

/* Environment variables */
char *program_name;
char *program_version;
char *path;
char *command;
int fd;
LIST *list_wd;

/* Global option */
bool be_syslog;
bool be_verbose;
bool be_easter;

/* Used to maintain information about watched resource */
typedef struct wd_data_s
{
    int wd;     // Watch Descriptor
    char *path;
    bool symbolic_link;
    LIST *links;
} WD_DATA;

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
 * @param char* : Message to log
 */
void log_message(char *);

/**
 * Print List
 * 
 * Only an help fuction that print list_wd
 * @param LIST* : the list_wd you want print
 */
void print_list(LIST *);

/**
 * Resolve the real path
 * 
 * This function is used to resolve the
 * absolute real_path of a symbolic link or a relative path.
 * @param char* : the path of the symbolic link to resolve
 * @return char* : the resolved real_path, NULL otherwise
 */
char *resolve_real_path(const char *);

/**
 * Searchs and returns the node for the path passed
 * as argument.
 * @param char* : The path to find
 * @return LIST_NODE* : a pointer to node, NULL otherwise.
 */
LIST_NODE *get_from_path(char *);

/**
 * Searchs and returns the node for the wd passed
 * as argument.
 * @param int : The wd to find
 * @return LIST_NODE* : a pointer to node, NULL otherwise.
 */
LIST_NODE *get_from_wd(int);

/**
 * Parse command line
 *
 * This function is used to parse command line and initialize some environment variables
 * @param int : arguments count
 * @param char** : arguments value
 * @return int
 */
int parse_command_line(int, char**);

/**
 * Watch a directory
 *
 * It performs a breath-first-search to traverse a directory and
 * call the add_to_watch_list(path) for each directory, either if it's pointed by a symbolic link or not.
 * @param char* : The path of directory to watch
 * @param char* : The symbolic link that point to the path
 * @return int : -1 (An error occurred), 0 (Resource added correctly)
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
 * Jump of a level up in directory hierarchy.
 *
 * @param char* : the absolute path
 * */
char *levelUp(char *);

/**
 * Search recursively if a level up directory of path is in watching.
 *
 * @param char* : the path to process
 */
bool isAlone(char *);

/**
 * Unwatch a directory
 * 
 * Used to remove a file or directory
 * from the list of watched resources
 * @param char* : the path of the resource to remove
 * @param bool : true (1) if the path to unwatch is a symlink, false (0) otherwise.
 */
void unwatch(char *, bool);

/**
 * Start monitoring
 * 
 * Used to monitor inotify event on watched resources
 */
int monitor();

#endif /* !__CWATCH_H */
