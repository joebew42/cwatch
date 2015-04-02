#!/bin/sh

test_description="cwatch execute a command on attrib event"

. ./libtest/util.sh
. ./libtest/sharness.sh

test_expect_success "touch a file on attrib event" '
        mkdir box &&
        touch box/actual &&
        cwatch -d "box" -c "touch expected" -e attrib &&
        sleep 0.5 &&
        chmod -w box/actual
        sleep 1 &&
        kill_cwatch &&
        [ -e expected ]
    '
test_done
