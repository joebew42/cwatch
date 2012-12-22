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

#include "cwatch.h"

/* Define program name and version */
char *program_name = "cwatch";
char *program_version = "0.0 00/00/0000"; // maj.rev mm/dd/yyyy

/* Events mask */
uint32_t mask = IN_ISDIR | IN_CREATE | IN_DELETE;

void print_version()
{
    printf("%s - Version: %s\n", program_name, program_version);
}

void help()
{
    printf ("Usage: %s -c COMMAND [OPTIONS] DIRECTORY\n", program_name);
    printf ("Monitors changes in a directory through inotify system call and executes");
    printf (" the specified COMMAND with the -c option\n\n");
    printf (" -c\t\tthe command to executes when changes happens\n");
    printf (" -l\t\tLog all messages through syslog\n");
    printf (" -v\t\tBe verbose\n");
    printf (" -h\t\tOutput this help and exit\n");
    printf (" -V\t\tOutput version and exit\n");
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
    LIST_NODE *node = list_wd->first;
    while (node)
    {
        WD_DATA *wd_data = (WD_DATA *) node->data;
        printf("%s, WD:%d, LNK:%d\n", wd_data->path, wd_data->wd, wd_data->symbolic_link);

        /* print the content of links list */
        if (wd_data->symbolic_link == 1)
        {
            LIST_NODE *n_node = wd_data->links->first;
            printf ("\tList of links that point to this path:\n");

            while (n_node)
            {
                char *p = (char*) n_node->data;
                printf("\t\t%s\n", p);
                n_node = n_node->next;
            }

        }
        node = node->next;
    }
}

char *resolve_real_path(const char *path)
{

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

int parse_command_line(int argc, char *argv[])
{
    if (argc == 1)
    {
        help();
        return -1;
    }

    /* Handle command line arguments */
    while (argc > 1)
    {
        /* Parsing '-' options */
        if (argv[1][0] == '-')
        {
            /* Single option */
            switch (argv[1][1])
            {
                case 'c':
                    /* Move at command */
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

                    /* Check for a valid command */
                    if (strcmp(argv[1], "") == 0 || argv[1][0] == '-')
                    {
                        help();
                        return -1;
                    }

                    /* Store command */
                    command = malloc(sizeof(char) * strlen(argv[1]) + 1);
                    strcpy(command, argv[1]);
                    break;

                case 'l':
                    /* Enable syslog */
                    be_syslog = 1;
                    break;

                case 'v':
                    /* Be verbose */
                    be_verbose = 1;
                    break;

                case 'V':
                    /* Print version and exit */
                    print_version();
                    return -1;

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
            /* Check if errors occurred */
            if (argc != 2 || command == NULL)
            {
                help();
                return -1;
            }

            /* Check if the path isn't empty */
            if (strcmp(argv[1], "") == 0)
            {
                help();
                return -1;
            }

            /* Check if the path has the final slash */
            if (argv[1][strlen(argv[1])-1] != '/')
            {
                path = (char*) malloc(sizeof(char) * (strlen(argv[1]) + 2));
                strcpy(path, argv[1]);
                strcat(path, "/");

                /* Is a dir? */
                DIR *dir = opendir(path);
                if (dir == NULL)
                {
                    help();
                    return -1;
                }
                closedir(dir);
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

            /* Check if the path is absolute or not */
            if( path[0] != '/' )
            {
                char *real_path = resolve_real_path(path);
                free(path);
                path = real_path;
            }
        }

        /* Next argument */
        --argc;
        ++argv;
    }

    if (path == NULL)
    {
        help();
        return -1;
    }

    return 0; // Everything gone ok!
}

int watch(char *real_path, char *symlink)
{
    /* Add initial path to the watch list */
    LIST_NODE *node = add_to_watch_list(real_path, symlink);
    if (node == NULL)
        return -1; // An error occurred!!!

    /* Temporary list to perform breath-first-search */
    LIST *list = list_init();
    list_push(list, (void *) real_path);

    /* Traverse directory */
    DIR *dir_stream;
    struct dirent *dir;
    
    while (list->first != NULL)
    {
        /* Directory to watch */
        char *p = (char*) list_pop(list);
        
        /* Traverse directory */
        dir_stream = opendir(p);

        if (dir_stream == NULL)
        {
            printf("UNABLE TO OPEN DIRECTORY:\t\"%s\" -> %d\n", p, errno);
            return -1;
        }
        
        while (dir = readdir(dir_stream))
        {
            if (dir->d_type == DT_DIR
                && strcmp(dir->d_name, ".") != 0
                && strcmp(dir->d_name, "..") != 0)
            {
                /* Absolute path to watch */
                char *path_to_watch = (char*) malloc(sizeof(char) * (strlen(p) + strlen(dir->d_name) + 2));
                strcpy(path_to_watch, p);
                strcat(path_to_watch, dir->d_name);
                strcat(path_to_watch, "/");
                
                /* Append to watched resources */
                add_to_watch_list(path_to_watch, NULL);
				                
                /* Continue directory traversing */
                list_push(list, (void*) path_to_watch);
            }
            /* Resolve symbolic link */
            else if (dir->d_type == DT_LNK)
            {
                /* Absolute path of symlink */
                char *symlink = (char*) malloc(sizeof(char) * (strlen(p) + strlen(dir->d_name) + 2));
                strcpy(symlink, p);
                strcat(symlink, dir->d_name);
                strcat(symlink, "/");
                
                char *real_path = resolve_real_path(symlink);
                
                /**
                 * Test for:
                 * 1. is a real path
                 * 2. is a directory
                 */
                if (real_path != NULL && opendir(real_path) != NULL)
                {
                    /* Append to watched resources */
                    add_to_watch_list(real_path, symlink);

                    /* Continue directory traversing */
                    list_push(list, (void*) real_path);
                }
            }
        }
        closedir(dir_stream);
    }
    
    list_free(list);

    return 0;
}

LIST_NODE *add_to_watch_list(char *real_path, char *symlink)
{
    /* Check if the resource is already in the watch_list */
    LIST_NODE *node = get_from_path(real_path);
    
    /* If the resource is not watched yet, then add it into the watch_list */
    if (node == NULL)
    {
        /* Append directory to watch_list */
        int wd = inotify_add_watch(fd, real_path, mask);
    
        /**
         * Check limit in:
         * proc/sys/fs/inotify/max_user_watches
         */
        if (wd == -1)
        {
            printf("AN ERROR OCCURRED WHILE ADDING PATH %s:\n", real_path);
            printf("Please, take in consideration these possibilities:\n");
            printf(" - Max user's watches reached! See /proc/sys/fs/inotify/max_user_watches\n");
            printf(" - Resource is no more available!?\n");
            printf(" - An infinite loop generated by cyclic symbolic link?\n");
            return NULL;
        }
        
        /* Create the entry */
        WD_DATA *wd_data = malloc(sizeof(WD_DATA));
        wd_data->wd = wd;
        wd_data->path = real_path;
        wd_data->links = list_init();
        wd_data->symbolic_link = (strncmp(path, real_path, strlen(path)) == 0) ? 0 : 1;
        
        /* Check if set symbolic link flag to true */
        // printf("is a link: %d\n", wd_data->symbolic_link);

        node = list_push(list_wd, (void*) wd_data);
    
        /* Log Message */
        char *message = malloc(sizeof(char) * MAXPATHLEN);
        sprintf(message, "WATCHING: (fd:%d,wd:%d)\t\t\"%s\"", fd, wd_data->wd, real_path);
        log_message(message);
    }

    /* Check to add symlink (if any) to the symlink list of the watched resource */
    if (node != NULL && symlink != NULL)
    {
        WD_DATA *wd_data = (WD_DATA*) node->data;

        bool found = 0;
        LIST_NODE *node_link = wd_data->links->first;
        while (node_link)
        {
            char *link = (char*) node_link->data;
            if (strcmp(link, symlink) == 0)
            {
                found = 1;
                break;
            }

            node_link = node_link->next;
        }

        if (found == 0)
        {
            list_push(wd_data->links, (void*) symlink);

            /* Log Message */
            char *message = malloc(sizeof(char) * MAXPATHLEN);
            sprintf(message, "ADDED SYMBOLIC LINK:\t\t\"%s\" -> \"%s\"", symlink, real_path);
            log_message(message);
        }
    }
    
    return node;
}

int exists(char* child_path, LIST *parents)
{
    if (parents == NULL || parents->first == NULL)
        return 0;

    LIST_NODE *node = parents->first;
    while(node)
    {
        char* parent_path = (char*) node->data;
        if (strlen(parent_path) <= strlen(child_path)
            && strncmp(parent_path, child_path, strlen(parent_path)) == 0)
        {
            return 1;
        }
        node = node->next;
    }
    return 0;
}

void unwatch(char *path, bool is_link)
{
    /* Remove a real path from watched resources */
    if (is_link == 0)
    {
        /* Retrieve the watch descriptor id from path */
        LIST_NODE *node = get_from_path(path);
        if (node != NULL)
        {   
            WD_DATA *wd_data = (WD_DATA *) node->data;
        
            /* Log Message */
            char *message = malloc(sizeof(char) * MAXPATHLEN);
            sprintf(message, "UNWATCHING: (fd:%d,wd:%d)\t\t%s", fd, wd_data->wd, path);
            log_message(message);
            
            inotify_rm_watch(fd, wd_data->wd);
            list_remove(list_wd, node);
        }
    }
    /* Remove a symbolic link from watched resources */
    else
    {
        LIST_NODE *node = list_wd->first;
        while (node)
        {
            WD_DATA *wd_data = (WD_DATA*) node->data;
            
            LIST_NODE *link_node = wd_data->links->first;
            while (link_node)
            {
                char *p = (char*) link_node->data;
                if (strcmp(path, p) == 0)
                {
                    /* Log Message */
                    char *message = malloc(sizeof(char) * MAXPATHLEN);
                    sprintf(message, "UNWATCHING SYMLINK: \t\t%s -> %s", path, wd_data->path);
                    log_message(message);

                    list_remove(wd_data->links, link_node);
                    
                    /** 
                     * if there is no other symbolic links that point to the
                     * watched resources then unwatch it
                     */
                    if (wd_data->links->first == NULL && wd_data->symbolic_link == 1)
                    {
                        // printf("I have to unwatch it!\n");

                        /**
                         * Temporary list of resources that are pointed
                         * by symbolic links.
                         */
                        LIST *tmp_linked_path = list_init();

                        /**
                         * Descend to all subdirectories of wd_data->path and unwatch them all
                         * only if they are not pointed by some symbolic link, anymore
                         * (check if parent is not pointed by symlinks too).
                         */
                        LIST_NODE *sub_node = list_wd->first;
                        while (sub_node)
                        {
                            WD_DATA *sub_wd_data = (WD_DATA*) sub_node->data;
                            if (strncmp(wd_data->path, sub_wd_data->path, strlen(wd_data->path)) == 0
                                && exists(sub_wd_data->path, tmp_linked_path) == 0)
                            {
                                if (sub_wd_data->links->first == NULL)
                                {
                                    /* Log Message */
                                    char *message = malloc(sizeof(char) * MAXPATHLEN);
                                    sprintf(message, "UNWATCHING: (fd:%d,wd:%d)\t\t%s", fd, sub_wd_data->wd, sub_wd_data->path);
                                    log_message(message);
            
                                    inotify_rm_watch(fd, sub_wd_data->wd);
                                    list_remove(list_wd, sub_node);
                                }
                                else
                                {
                                    /* Save current path into linked_path */
                                    list_push(tmp_linked_path, (void*) sub_wd_data->path);
                                }
                            }
                            sub_node = sub_node->next;
                        }
                        list_free(tmp_linked_path);
                    }
                    return;
                }
                link_node = link_node->next;
            }
            node = node->next;
        }
    }
}

int monitor()
{
    /* Buffer for File Descriptor */
    char buffer[EVENT_BUF_LEN];

    /* inotify_event of the event */
    struct inotify_event *event = NULL;

    /* The real path of touched directory or file */
    char *path = NULL;
    int len;
    int i;
    
    /* Temporary node information */
    LIST_NODE *node = NULL;

    /* Wait for events */
    while (len = read(fd, buffer, EVENT_BUF_LEN))
    {
        if (len < 0)
        {
            printf("ERROR: UNABLE TO READ INOTIFY QUEUE EVENTS!!!\n");
            return -1;
        }

        /* index of the event into file descriptor */
        i = 0;
        while (i < len)
        {
            /* inotify_event */
            event = (struct inotify_event*) &buffer[i];

            /* Build the full path of the directory or symbolic link */
            node = get_from_wd(event->wd);
            if (node != NULL)
            {
                WD_DATA *wd_data = (WD_DATA *) node->data;
                
                path = malloc(sizeof(char) * (strlen(wd_data->path) + strlen(event->name) + 2));
                strcpy(path, wd_data->path);
                strcat(path, event->name);
                strcat(path, "/");
            }
            else
            {
                /* Next event */
                i += EVENT_SIZE + event->len;
                continue;
            }
            
            /* IN_CREATE Event */
            if (event->mask & IN_CREATE)
            {
                /* Check if it is a folder. If yes watch it */
                if (event->mask & IN_ISDIR)
                    watch(path, NULL);
                else
                {
                    /* check if it is a link. if yes watch it. */
                    bool is_dir = 0;
                    DIR *dir_stream = opendir(path);
                    if (dir_stream != NULL)
                        is_dir = 1;
                    closedir(dir_stream);

                    if (is_dir == 1)
                    {
                        /* resolve symbolic link */
                        char *real_path = resolve_real_path(path);

                        /* check if the real path is already monitored */
                        LIST_NODE *node = get_from_path(real_path);
                        if (node == NULL)
                        {
                            watch(real_path, path);
                        }
                        else
                        {
                            /** 
                             * Append the new symbolic link
                             * to the watched resource
                             */
                            WD_DATA *wd_data = (WD_DATA*) node->data;
                            list_push(wd_data->links, (void*) path);

                            /* Log Message */
                            char *message = malloc(sizeof(char) * MAXPATHLEN);
                            sprintf(message, "ADDED SYMBOLIC LINK:\t\t\"%s\" -> \"%s\"", path, real_path);
                            log_message(message);
                        }
                    }
                }
            }
            /* IN_DELETE event */
            else if (event->mask & IN_DELETE)
            {
                /* Check if it is a folder. If yes unwatch it */
                if (event->mask & IN_ISDIR)
                    unwatch(path, 0);
                else
                {
                    /**
                     * XXX Since it is not possible to know if the
                     * inotify event belongs to a file or a symbolic link,
                     * the unwatch function will be called for each file.
                     * This is a big computational issue to be treated.
                     */
                    unwatch(path, 1);
                }
            }

            /* Next event */
            i += EVENT_SIZE + event->len;
        }
    }

    return 0;
}
