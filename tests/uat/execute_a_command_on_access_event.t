#!/bin/sh

test_description="cwatch execute a command on access event"

. ./libtest/util.sh
. ./libtest/sharness.sh

test_expect_success "touch a file on access event" '
        mkdir box &&
        #the file must have content
        echo "a" > box/actual &&
        cwatch -d "box" -c "touch expected" -e access &&
        sleep 0.5 &&

        # access is triggered from read(), execve() ...
        cat < box/actual

        sleep 1 &&
        kill_cwatch &&
        [ -e expected ]
    '
test_done
