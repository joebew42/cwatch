#!/bin/sh

test_description="cwatch execute a command on close_write event"

. ./libtest/util.sh
. ./libtest/sharness.sh

test_expect_success "touch a file on close_write event" '
        mkdir box &&
        touch box/actual &&
        cwatch -d "box" -c "touch expected" -e close_write &&
        sleep 0.5 &&

        # opening in write mode
        echo "a" >> box/actual &&

        sleep 1 &&
        kill_cwatch &&
        [ -e expected ]
    '
test_done
