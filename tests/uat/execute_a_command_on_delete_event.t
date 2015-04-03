#!/bin/sh

test_description="cwatch execute a command on delete event"

. ./libtest/util.sh
. ./libtest/sharness.sh

test_expect_success "touch a file on delete event" '
        mkdir box &&
        touch box/actual &&
        cwatch -d "box" -c "touch expected" -e delete &&
        sleep 0.5 &&
        rm box/actual &&
        sleep 1 &&
        kill_cwatch &&
        [ -e expected ]
    '
test_done
