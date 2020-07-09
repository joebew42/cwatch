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

int main(int argc, char *argv[])
{
    signal(SIGINT, signal_callback_handler);

    if (parse_command_line(argc, argv) == 0)
    {
        int fd = inotify_init();
        LIST *list_wd = list_init();

        watch_descriptor_from = inotify_add_watch;
        remove_watch_descriptor = inotify_rm_watch;

        if (watch_directory_tree(root_path, NULL, recursive_flag, fd, list_wd) == -1)
        {
            printf("An error occured while adding \"%s\" as watched resource!\n", root_path);
            return EXIT_FAILURE;
        }

        return monitor(fd, list_wd);
    }

    return EXIT_SUCCESS;
}
