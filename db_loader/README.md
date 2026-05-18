# Titan Office Hours - Database Loader (for local clones)

## What this does
Reads a CSV file of office hours and loads it into `titan_office.db`. It uses a C instance of SQLite, so a C/C++ compiler is necessary. <br>

## CSV Format
name,day,start,end,room

#### Intended input:
"Smith, John",MW,11:00am,12:00pm,MH 123

## Compile
gcc -c sqlite3.c -o sqlite3.o<br>
g++ -std=c++11 -o office_hours_loader csv-to-db.cpp sqlite3.o<br>

## Usage
./office_hours_loader "College Name" "Department Name" data.csv<br>
./office_hours_loader "College Name" "Department Name" data.csv --replace<br>
#### When adding in updated office hours, use --replace at the end of your command.

## Example
./office_loader "Natural Sciences and Mathematics" "Mathematics" math_fall2026.csv<br>
./office_loader "Engineering and Computer Science" "Computer Science" cs_hours.csv --replace<br>
