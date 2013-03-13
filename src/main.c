/* main.c
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

#include "cwatch.h"

/**
 * MAIN
 */
int main(int argc, char *argv[])
{ 
    if (parse_command_line(argc, argv) == 0) {
        /* File descriptor inotify */
        fd = inotify_init();

        /* List of all watch directories */
        list_wd = list_init();

        /* Watch the path */
        if (watch(root_path, NULL) == -1) {
            printf("An error occured while adding \"%s\" as watched resource!\n", root_path);
            return -1;
        }

        /* Start monitoring */
        return monitor();
    }
    
    return -1;
}
