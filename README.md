[![Build Status](https://travis-ci.org/joebew42/cwatch.svg?branch=dev)](https://travis-ci.org/joebew42/cwatch)

# cwatch

A program to monitor the filesystem activity through the inotify Linux kernel library and executes a user defined command when specific events occurs. Useful for automatic backup system, alert monitor for directories and files, continuous testing and more.

This is a **experimental release**. At this stage we are testing the software as best as we can. If you would like to help us to test and improve this software, feel free to do everything you want.

A Quick-start:

1. Clone the project:

         $git clone https://github.com/joebew42/cwatch.git

2. Compile it:

         $cd cwatch/

         $autoreconf --install

         $./configure

         $make

3. Run all tests:

         $make -s check

4. Start use it:

   4.1. Read the manual

        ./src/cwatch --help

   4.2. *Share with us your use case*

*Note*: This README is just a draft. In the next days we will provide a new one well organized.
