#!/bin/sh

test_description="cwatch execute a command on modify event"

. ./libtest/util.sh
. ./libtest/sharness.sh

test_expect_success "touch a file on modify event" '
        mkdir box &&
        touch box/actual &&
        cwatch -d "box" -c "touch expected" -e modify &&
        sleep 0.5 &&
        echo "a" >>box/actual &&
        sleep 1 &&
        kill_cwatch &&
        [ -e expected ]
    '
test_done
