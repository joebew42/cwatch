/* commandline.c
 * Command line parsing utility for cwatch
 *
 * Copyright (C) 2014, Joe Bew <joebew42@gmail.com>,
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

#include "commandline.h"

CMDLINE_OPTS *
commandline_parse(char *commandline)
{
    if (strlen(commandline) > 0)
    {
        CMDLINE_OPTS *cmdline_opts = (CMDLINE_OPTS *)malloc(sizeof(CMDLINE_OPTS));
        cmdline_opts->directory = strdup(commandline);
        return cmdline_opts;
    }
    return NULL;
}
