#!/bin/sh

test_description="cwatch execute a command on create event"

. ./libtest/util.sh
. ./libtest/sharness.sh

test_expect_success "touch a file on create event" '
        mkdir box &&
        cwatch -d "box" -c "touch expected" -e create &&
        sleep 0.5 &&
        touch box/actual &&
        sleep 1 &&
        kill_cwatch &&
        [ -e expected ]
    '
test_done
