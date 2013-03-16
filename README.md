cwatch(1)
=========

A lightweight program that monitors the filesystem activity through the inotify Linux kernel library and executes a user defined command.

This is a working experimental release (ver.1.0). In this stage we are testing the software as best as we can. If you would like to help us to test and improve this software, feel free to do everything you want.

A Quick-start:

1. Clone the project:

           $git clone https://github.com/joebew42/cwatch.git

2. Compile it:

           $cd cwatch/

           $git checkout 1.0experimental

           $aclocal && automake --add-missing --copy && autoconf

           $./configure

           $make

3. Use the software:  
	
	3.1. Read the manual
	
                ./src/cwatch --help
 
	3.2. *Do some testing*

*Note*: This README is just a draft. In the next days we will provide a new one well organized.