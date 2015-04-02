#!/bin/sh

test_description="cwatch execute a command on open event"

. ./libtest/util.sh
. ./libtest/sharness.sh

test_expect_success "touch a file on open event" '
        mkdir box &&
        touch box/actual &&
        cwatch -d "box" -c "touch expected" -e open &&
        sleep 0.5 &&
        cat box/actual &> /dev/null
        sleep 1 &&
        kill_cwatch &&
        [ -e expected ]
    '
test_done
