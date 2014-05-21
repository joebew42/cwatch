#!/bin/sh

travis()
{
    echo "Installing check 0.9.12..."
    wget http://downloads.sourceforge.net/project/check/check/0.9.12/check-0.9.12.tar.gz
    tar zxf check-0.9.12.tar.gz
    cd check-0.9.12/ && ./configure && make && sudo make install && sudo ldconfig
    cd ..

    echo "Building cwatch..."
    mkdir m4 && autoreconf -vi && ./configure && make && make -s check
}

usage()
{
    echo "Usage $0 {travis}"
}

case $1 in
    travis)
        travis
        ;;
    *)
        usage
    exit 1
esac
