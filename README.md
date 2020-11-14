# Faux-Character-Driver
Simple Character driver to read and write to memory

To build and load the module:
 1. make
 2. insmod fcd.ko
 
 This will load the module creating a device file ( /dev/fcd)
 simple write to and read from /dev/fcd
 
 
 Features
 
 1. read 
 2. write
 3. seek
 
This is a simple character driver which creates a memory space of 512 bytes in memory
into which we can write ,read and do seek
