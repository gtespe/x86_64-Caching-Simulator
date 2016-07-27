#x86_64 Caching Simulator

1. create a valgrind trace file (example file included):
    
linux> valgrind --log-fd=1 --tool=lackey -v --trace-mem=yes <YourCommandHere> > <filename>

2. Compile and link:

make

3. run:

Usage: ./csim [-hv] -s <num> -E <num> -b <num> -t <file>
Options:
-h         Print this help message.
-v         Optional verbose flag.
-s <num>   Number of set index bits.
-E <num>   Number of lines per set.
-b <num>   Number of block offset bits.
-t <file>  Trace file.

Examples:
linux>  ./csim -s 4 -E 1 -b 4 -t valgrind.txt
linux>  ./csim -v -s 8 -E 2 -b 4 -t valgrind.txt


Note: To clean, execute 'make clean'
