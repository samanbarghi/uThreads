# uThreads
A concurrent user-level thread library implemented in C++. uThreads only supports Linux on x86_64 platforms. 

# Dependencies
 * gcc > 4.8 

# Installation
* Change the destination directory in Makefile (DEST_DIR) to point to where you want the library files be installed. Default location is `/usr/local`
* `make`
* `[sudo] make install`
* `export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:DEST_DIR` 

# Usage

Include "uThreads/uThreads.h" in your source file. Link your program with uThreads library (-luThreads). You can find examples under `test` directory. Perform `make test` to compile them and the binary will be created under bin directory. 

