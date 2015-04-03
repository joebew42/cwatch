#!/bin/sh

test_description="cwatch execute a command on close_nowrite event"

. ./libtest/util.sh
. ./libtest/sharness.sh

test_expect_success "touch a file on close_nowrite event" '
        mkdir box &&
        touch box/actual &&
        cwatch -d "box" -c "touch expected" -e close_nowrite &&
        sleep 0.5 &&

        # opening in read mode
        cat box/actual &> /dev/null &&

        sleep 1 &&
        kill_cwatch &&
        [ -e expected ]
    '
test_done
