# Titan Office Hours Search - CLI Instance

For personal usage, you must have a C/C++ compiler installed. As the SQLite3 instance that I use is in C, making objects is necessary. 

## Compile
gcc -c sqlite3.c -o sqlite3.o
g++ -std=c++11 -c search.cpp -o search.o
g++ -std=c++11 search search.o sqlite3.o

## Usage
./search --lastname Smith --today
