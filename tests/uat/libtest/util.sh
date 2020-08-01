#!/bin/sh
#
# Copyright (C) 2012, Joe Bew <joebew42@gmail.com>,
#                     Vincenzo Di Cicco <enzodicicco@gmail.com>
#
# This file is part of cwatch
#
# cwatch is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# cwatch is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

CWATCH_PID=""
CWATCH_DIR=".."

cwatch() {
    $CWATCH_DIR/cwatch "$@" &
    CWATCH_PID=$!
}

kill_cwatch() {
    if [ $CWATCH_PID != "" ]; then
        kill $CWATCH_PID
    fi
}
