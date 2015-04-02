#!/bin/sh

test_description="cwatch execute a command on moved_from event"

. ./libtest/util.sh
. ./libtest/sharness.sh

test_expect_success "touch a file on moved_from event" '
        mkdir box &&
        touch box/actual &&
        cwatch -d "box" -c "touch expected" -e moved_from &&
        sleep 0.5 &&
        mv box/actual .
        sleep 1 &&
        kill_cwatch &&
        [ -e expected ]
    '
test_done
