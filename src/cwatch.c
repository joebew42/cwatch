/* cwatch.c
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

#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <sys/param.h>
#include <syslog.h>

#include "list.h"

#define EVENT_SIZE      ( sizeof (struct inotify_event) )
#define EVENT_BUF_LEN   ( 1024 * ( EVENT_SIZE + 16 ) )

uint32_t mask = IN_ISDIR | IN_CREATE | IN_DELETE;

// Boolean data type
typedef char bool;

// Used to maintain information about watched resource
typedef struct wd_data_s
{
    int wd;     // Watch Descriptor
    char *path;
} WD_DATA;

// Used to mantain information about link and a references count
typedef struct wd_link_s
{
    int count;          // Count references
    LIST_NODE *node;   // Pointer to a LIST_NODE
} WD_LINK;

// Global option
bool be_syslog;
bool be_verbose;
bool be_easter;

// Environment variables
char *program_name = "cwatch";
char *program_version = "1.0 05/15/2012"; // maj.rev MM/DD/YYYY
char *path = NULL;
char *command = NULL;
int fd;
LIST *list_wd;
LIST *list_link;

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
 * Watch a directory
 *
 * It performs a breath-first-search to traverse a directory and
 * call the add_to_watch_list(path) for each directory, either if it's pointed by a symbolic link or not.
 * @param char* : The path of directory to watch
 * @param bool : specify if the watched path is a link or not
 */
void watch(char *, bool);

/**
 * Add a directory into watch list
 *
 * This function is used to append a directory into watch list
 * @param char* : The absolute path of the directory to watch
 * @return LIST_NODE* : the pointer of the node of the watch list
 */
LIST_NODE *add_to_watch_list(char *);

/**
 * Unwatch a directory
 * 
 * Used to remove a file or directory
 * from the list of watched resources
 * @param char* : the path of the resource to remove
 * @return: see inotify_rm_watch() returns zero on success, or -1
 *          if an error occurred
 */
void unwatch(char *);

/**
 * Add a link to the list of symbolic links
 *
 * Search if a link already exists into link list.
 * @param LIST_NODE* : the node of the watch_list
 * @return int : the reference count of the link
 */
int add_to_link_list(LIST_NODE *);

/**
 * Start monitoring
 * 
 * Used to monitor inotify event on watched resources
 */
int monitor();

/**
 * MAIN
 */
int main(int argc, char *argv[])
{   
    
    if (argc == 1)
    {
        help();
        return -1;
    }

    // Handle command line arguments
    while (argc > 1)
    {	
        // Parsing '-' options
        if (argv[1][0] == '-')
        {
            // Single option
            switch (argv[1][1])
            {
                case 'c':
                    // Move at command
                    if (argc > 2)
                    {
                        ++argv;
                        --argc;
                    }
                    else
                    {
                        help();
                        return -1;
                    }
                    
                    // Check for a valid command
                    if (strcmp(argv[1], "") == 0 || argv[1][0] == '-')
                    {
                        help();
                        return -1;
                    }
                    
                    // Store command
                    command = malloc(sizeof(char) * strlen(argv[1]) + 1);
                    strcpy(command, argv[1]);
                    break;
                case 'l':
                    // Enable syslog
                    be_syslog = 1;
                    break;
                case 'v':
                    // Be verbose
                    be_verbose = 1;
                    break;
                case 'V':
                    // Print version and exit
                    printf("%s - Version: %s\n", program_name, program_version);
                    return 0;
                case 'n':
                    be_easter = 1;
                    break;
                case 'h':
                default:
                    help();
                    return -1;
            }
        }
        else
        {
            // Directory to watch???
            // A command is specified???
            if (argc != 2 || command == NULL)
            {
                help();
                return -1;
            }

            // Check if the path isn't empty
            if (strcmp(argv[1],"") == 0)
            {
                help();
                return -1;
            }
    
            // Check if the path has the final slash 
            if (argv[1][strlen(argv[1])-1] != '/')
            {
                path = (char*) malloc(sizeof(char) * (strlen(argv[1]) + 2));
                strcpy(path, argv[1]);
                strcat(path, "/");
            }
            else
            {
                path = (char*) malloc(sizeof(char) * strlen(argv[1]));
                strcpy(path, argv[1]);
            }
      
            // Check if it is a directory 
            DIR *dir = opendir(path);
            if (dir == NULL)
            {
                help();
                return -1;
            }
            closedir(dir);
           
            // Check if the path is absolute or not. 
            if( path[0] != '/' )
            {
                char *real_path = resolve_real_path(path);
                free(path);
                path = real_path;
            }
        }

        // Next argument
        --argc;
        ++argv;
    }
    
    if (path == NULL)
    {
        help();
        return -1;
    }

    // File descriptor inotify 
    fd = inotify_init();
    
    // List of all watch directories 
    list_wd = list_init();

    // List of all symbolic links
    list_link = list_init();

    // Watch the path
    watch(path, 0); 
    
    // Start monitoring
    return monitor();
}

void help()
{
    printf(
"Usage: %s -c COMMAND [OPTIONS] DIRECTORY\n"
"Monitors changes in a directory through inotify system call and executes"
" the specified COMMAND with the -c option\n\n"
"  -c\t\tthe command to executes when changes happens\n"
"  -l\t\tLog all messages through syslog\n"
"  -v\t\tBe verbose\n"
"  -h\t\tOutput this help and exit\n"
"  -V\t\tOutput version and exit\n",
    program_name);
}

void log_message(char *message)
{
    if (be_verbose)
        printf("%s\n", message);

    if (be_syslog)
    {
        openlog(program_name, LOG_PID, LOG_LOCAL1);
        syslog(LOG_INFO, message);
        closelog();
    }

    free(message);
}

void print_list(LIST *list_wd)
{
    // Traverse example
    LIST_NODE *node = list_wd->first;
    while (node)
    {
        WD_DATA *wd_data = (WD_DATA *) node->data;
        printf("%s, WD:%d\n", wd_data->path, wd_data->wd);
        node = node->next;
    }
}

char *resolve_real_path(const char *path)
{
    // DEBUG ONLY:
    printf("Resolving: \"%s\" ...\n", path);

    char *resolved = malloc(sizeof(char) * MAXPATHLEN + 1);
    
    realpath(path, resolved);
    
    if (resolved == NULL)
        return NULL;
   
    strcat(resolved, "/");
     
    return resolved;
}

LIST_NODE *get_from_path(char *path)
{
    LIST_NODE *node = list_wd->first;
    while (node)
    {
        WD_DATA *wd_data = (WD_DATA *) node->data;
        if (strcmp(path, wd_data->path) == 0)
            return node;
        node = node->next;
    }
    return NULL;
}

LIST_NODE *get_from_wd(int wd)
{
    LIST_NODE *node = list_wd->first;
    while (node)
    {
        WD_DATA *wd_data = (WD_DATA *) node->data;
        if (wd == wd_data->wd)
            return node;
        node = node->next;    
    }
    
    return NULL;
}

void watch(char *path, bool is_link)
{
    // Add initial path to the watch list
    LIST_NODE *node = get_from_path(path);
    if (node == NULL)
        node = add_to_watch_list(path);
    
    // Searchs and increments the reference counter
    // of the link in list_link
    if (is_link)
        add_to_link_list(node);
    
    // Temporary list to perform breath-first-search
    LIST *list = list_init();
    list_push(list, (void *) path);

    // Traverse directory
    DIR *dir_stream;
    struct dirent *dir;

    while (list->first != NULL)
    {
        // Directory to watch
        char *p = (char*) list_pop(list);
        
        // Traverse directory
        dir_stream = opendir(p);
        
        while (dir = readdir(dir_stream))
        {
            if (dir->d_type == DT_DIR &&
                strcmp(dir->d_name, ".") != 0 &&
                strcmp(dir->d_name, "..") != 0)
            {
                char *path_to_watch = (char*) malloc(sizeof(char) * (strlen(p) + strlen(dir->d_name) + 2));
                strcpy(path_to_watch, p);
                strcat(path_to_watch, dir->d_name);
                strcat(path_to_watch, "/");

                // Add to the watch list
                if (get_from_path(path_to_watch) == NULL)
                    add_to_watch_list(path_to_watch);
                
                // Continue directory traversing
                list_push(list, (void*) path_to_watch);
            }
            // Resolve symbolic link
            else if (dir->d_type == DT_LNK)
            {
                char *path_to_watch = (char*) malloc(sizeof(char) * (strlen(p) + strlen(dir->d_name) + 1));
                strcpy(path_to_watch, p);
                strcat(path_to_watch, dir->d_name);
                
                char *real_path = resolve_real_path(path_to_watch);
                
                // Test for:
                // 1. is a real path
                // 2. is a directory
                if (real_path != NULL && opendir(real_path) != NULL)
                {
                    // Add to the watch list if it's not present
                    LIST_NODE *node = get_from_path(real_path);
                    if (node == NULL)
                        node = add_to_watch_list(real_path);
                    
                    // Searchs and increments the reference counter
                    // of the link in list_link
                    add_to_link_list(node);
                    
                    // Continue directory traversing
                    list_push(list, (void*) real_path);
                }
            }
        }
        closedir(dir_stream);
    }
    
    // Free memory
    list_free(list);
}

LIST_NODE *add_to_watch_list(char *path)
{
    // Append directory to watchlist
    int wd = inotify_add_watch(fd, path, mask);
    
    // Check limit in:
    // /proc/sys/fs/inotify/max_user_watches
    if (wd == -1)
    {
        printf("AN ERROR OCCURRED WHILE ADDING PATH %s:\n", path);
        printf("Please, take in consideration these possibilities:\n");
        printf(" - Max user's watches reached! See /proc/sys/fs/inotify/max_user_watches\n");
        printf(" - Resource is no more available!?\n");
        printf(" - An infinite loop generated by cyclic symbolic link?\n");
        return wd;
    }
    
    WD_DATA *wd_data = malloc(sizeof(WD_DATA));
    wd_data->wd = wd;
    wd_data->path = path;
    
    LIST_NODE *list_node = list_push(list_wd, (void *) wd_data);
    
    // Log Message
    if (be_verbose || be_syslog)
    {
        char *message = malloc(sizeof(char) * MAXPATHLEN);
        sprintf(message, "WATCHING: (fd:%d,wd:%d)\t\t%s", fd, wd_data->wd, path);
        log_message(message);
    }
    
    return list_node;
}

void unwatch(char *path)
{
    // Retrieve the watch descriptor id from path
    LIST_NODE *node = get_from_path(path);
    if (node != NULL)
    {   
        WD_DATA *wd_data = (WD_DATA *) node->data;
        
        // Log Message
        if (be_verbose || be_syslog)
        {
            char *message = malloc(sizeof(char) * MAXPATHLEN);
            sprintf(message, "UNWATCHING: (fd:%d,wd:%d)\t\t%s", fd, wd_data->wd, path);
            log_message(message);
        }
        
        // Remove inotify_watch and node
        // XXX: It's probabily that in newer kernel versions,
        //      the call to inotify_rm_watch is called automatically
        //      when a resource is deleted.
        //      inotify_rm_watch is used when we want to remove explicity
        //      a watch upon an existing watched resource.
        // int i = inotify_rm_watch(fd, wd_data->wd);
        list_remove(list_wd, node);
    }
}

int add_to_link_list(LIST_NODE *list_node)
{
    if (list_node == NULL)
        return -1;
    
    WD_LINK *wd_link;
    
    LIST_NODE *node = list_link->first;
    while (node)
    {
        wd_link = (WD_LINK *) node->data;
        
        if (list_node == wd_link->node)
        {
            ++wd_link->count;
            printf("AGGIORNATO LINK SIMBOLICO IN LISTA, %d\n", wd_link->count);
            return wd_link->count;
        }
        
        node = node->next;
    }

    wd_link = malloc(sizeof(WD_LINK));
    wd_link->count = 1;
    wd_link->node = list_node;

    printf("AGGIUNTO LINK SIMBOLICO IN LISTA\n");
    
    list_push(list_link, (void *) wd_link);
    
    return 1;
}

int monitor()
{
    // Buffer for File Descriptor 
    char buffer[EVENT_BUF_LEN];
    // inotify_event of the event 
    struct inotify_event *event = NULL;
    // The path of touched directory or file
    char *path = NULL;
    int len;

    // Wait for events 
    while (len = read(fd, buffer, EVENT_BUF_LEN))
    {
        if (len < 0)
        {
            printf("Read() error");
            return -1;
        }

        // index of the event into file descriptor 
        int i = 0;
        while (i < len)
        {
            // inotify_event of the event 
            event = (struct inotify_event*) &buffer[i];

            // Build the full path of the directory or symbolic link
            LIST_NODE *node = get_from_wd(event->wd);
            if (node != NULL)
            {
                WD_DATA *wd_data = (WD_DATA *) node->data;
                
                path = malloc(sizeof(char) * (strlen(wd_data->path) + strlen(event->name) + 2));
                strcpy(path, wd_data->path);
                strcat(path, event->name);
                strcat(path, "/");
            }
            else
                continue;
            
            // IN_CREATE Event 
            if (event->mask & IN_CREATE)
            {
                // Check if it is a folder. If yes watch it
                if (event->mask & IN_ISDIR)
                    watch(path, 0);
                else
                {
                    // check if it is a link. if yes watch it. 
                    DIR *dir_stream = opendir(path);
                    if (dir_stream != NULL)
                    {
                        // resolve symbolic link
                        char *realpath = resolve_real_path(path);
                        watch(realpath, 1);
                    }
                    closedir(dir_stream);
                }
            }
            // IN_DELETE event 
            else if (event->mask & IN_DELETE)
            {
                // Starts build the full path of the
                
                // Check if it is a folder. If yes unwatch it 
                if (event->mask & IN_ISDIR)
                    unwatch(path);
                else
                {
                    // Resolve the real path of the symbolic link
                    char* resolved = resolve_real_path(path);
                    
                    printf("PATH TO DELETE: %s\n", resolved);
                }
            }

            // Next event 
            i += EVENT_SIZE + event->len;
        }
    }

    return 0;
}
