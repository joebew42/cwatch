#!/bin/sh

test_description="cwatch execute a command on moved_to event"

. ./libtest/util.sh
. ./libtest/sharness.sh

test_expect_success "touch a file on moved_to event" '
        mkdir box &&
        touch actual &&
        cwatch -d "box" -c "touch expected" -e moved_to &&
        sleep 0.5 &&
        mv actual box/actual
        sleep 1 &&
        kill_cwatch &&
        [ -e expected ]
    '
test_done
