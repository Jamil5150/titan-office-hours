# Titan Office Hours Search - CLI Instance

For personal CLI usage, you must have a C/C++ compiler installed. As the SQLite3 instance that I use is in C, making objects is necessary. 

## Compile
gcc -c sqlite3.c -o sqlite3.o<br>
g++ -std=c++11 -c search.cpp -o search.o<br>
g++ -std=c++11 search search.o sqlite3.o<br>

## Usage
Usage is straightforward; here are some examples:

./search --lastname Smith --today<br>
./search --help <br>
