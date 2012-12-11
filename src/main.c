/* Cwatch.
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

#include "list.h"
#include "cwatch.h"


/* Environment variables */
char *program_name = "cwatch";
char *program_version = "0.0 00/00/0000"; // maj.rev MM/DD/YYYY
char *path = NULL;
char *command = NULL;
int fd;
LIST *list_wd;

/* Global option */
bool be_syslog;
bool be_verbose;
bool be_easter;

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

    /* File descriptor inotify */
    fd = inotify_init();

    /* List of all watch directories */
    list_wd = list_init();

    /* Watch the path */
    if (watch(path, NULL) == -1)
    {
        printf("An error occured while adding \"%s\" as watched resource!\n", path);
        return -1;
    }

    /* Start monitoring */
    return monitor();
}


