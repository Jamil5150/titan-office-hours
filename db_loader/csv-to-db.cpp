#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <sstream>
#include <cstdlib>
#include "sqlite3.h"

std::string trimmer(const std::string& s) {
    size_t first = s.find_first_not_of(" \t\n\r\"");
    if (first == std::string::npos) return "";
    size_t last = s.find_last_not_of(" \t\n\r\"");
    return s.substr(first, last - first + 1);
}

std::string readCSVField(std::stringstream& ss) {
    std::string field;
    // Skip any leading whitespace (spaces, tabs)
    while (ss.peek() == ' ' || ss.peek() == '\t') {
        ss.get();
    }
    if (ss.peek() == '"') {
        ss.get();  // consume the opening quote
        while (ss.peek() != EOF) {
            char c = ss.get();
            if (c == '"') {
                // If next char is also a quote, it's an escaped quote; add one quote and continue
                if (ss.peek() == '"') {
                    field += '"';
                    ss.get();
                } else {
                    // End of quoted field
                    break;
                }
            } else {
                field += c;
            }
        }
        while (ss.peek() != EOF && ss.peek() != ',') {
            ss.get();
        }
        if (ss.peek() == ',') ss.get();
    } else {
        std::getline(ss, field, ',');
    }
    return field;
}

int main(int argc, char* argv[]) {
    std::vector<std::string> command_line(argv, argv + argc);

    bool replace = false;
    if (command_line.size() >= 5 && command_line.at(4) == "--replace") {
        replace = true;
        command_line.pop_back();   // size now 4
    }

    if (command_line.size() != 4) {
        std::cerr << "Usage: " << command_line.at(0)
                  << " <college> <department> <csv_file> [--replace]\n";
        std::cerr << "Example: " << command_line.at(0)
                  << " \"Engineering and Computer Science\" \"Computer Science\" cs_office_hours.csv\n";
        return 1;
    }

    std::string collegeName = command_line.at(1);
    std::string deptName    = command_line.at(2);
    std::string csvFilename = command_line.at(3);

    sqlite3* db;
    char* errMsg = 0;
    int rc = sqlite3_open("titan_office.db", &db);
    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        return 1;
    }
    std::cout << "Opened database successfully.\n";

    // Create tables
    sqlite3_exec(db,
        "CREATE TABLE IF NOT EXISTS colleges ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT UNIQUE NOT NULL);", 0, 0, &errMsg);

    sqlite3_exec(db,
        "CREATE TABLE IF NOT EXISTS departments ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT UNIQUE NOT NULL, "
        "college_id INTEGER, "
        "FOREIGN KEY (college_id) REFERENCES colleges(id));", 0, 0, &errMsg);

    sqlite3_exec(db,
        "CREATE TABLE IF NOT EXISTS faculty ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT NOT NULL, "
        "dept_id INTEGER, "
        "FOREIGN KEY (dept_id) REFERENCES departments(id));", 0, 0, &errMsg);

    sqlite3_exec(db,
        "CREATE TABLE IF NOT EXISTS office_hours ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "faculty_id INTEGER, "
        "day TEXT, "
        "start_time TEXT, "
        "end_time TEXT, "
        "room TEXT, "
        "FOREIGN KEY (faculty_id) REFERENCES faculty(id));", 0, 0, &errMsg);

    // Insert college (if not already present)
    {
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db,
            "INSERT OR IGNORE INTO colleges (name) VALUES (?1);",
            -1, &stmt, 0);
        sqlite3_bind_text(stmt, 1, collegeName.c_str(), -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    // Insert department (linked to college)
    {
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db,
            "INSERT OR IGNORE INTO departments (name, college_id) "
            "VALUES (?1, (SELECT id FROM colleges WHERE name = ?2));",
            -1, &stmt, 0);
        sqlite3_bind_text(stmt, 1, deptName.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, collegeName.c_str(), -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    // Open CSV
    std::ifstream file(csvFilename);
    if (!file.is_open()) {
        std::cerr << "Could not open " << csvFilename << std::endl;
        sqlite3_close(db);
        return 1;
    }

    std::string line;
    std::getline(file, line); // skip header

    // Prepare insert statements
    sqlite3_stmt* insertFacultyStmt = NULL;
    sqlite3_prepare_v2(db,
        "INSERT OR IGNORE INTO faculty (name, dept_id) "
        "VALUES (?1, (SELECT id FROM departments WHERE name = ?2));",
        -1, &insertFacultyStmt, 0);

    sqlite3_stmt* insertHourStmt = NULL;
    sqlite3_prepare_v2(db,
        "INSERT INTO office_hours (faculty_id, day, start_time, end_time, room) "
        "VALUES ((SELECT id FROM faculty WHERE name = ?1 AND dept_id = "
        "(SELECT id FROM departments WHERE name = ?2)), ?3, ?4, ?5, ?6);",
        -1, &insertHourStmt, 0);

    // Replace mode: delete old hours for this department
    if (replace) {
        const char* deleteSQL = R"(
            DELETE FROM office_hours
            WHERE faculty_id IN (
                SELECT id FROM faculty
                WHERE dept_id = (SELECT id FROM departments WHERE name = ?1)
            );
        )";
        sqlite3_stmt* delStmt;
        sqlite3_prepare_v2(db, deleteSQL, -1, &delStmt, 0);
        sqlite3_bind_text(delStmt, 1, deptName.c_str(), -1, SQLITE_STATIC);
        sqlite3_step(delStmt);
        sqlite3_finalize(delStmt);
        std::cout << "Cleared existing hours for " << deptName << ".\n";
    }

    // Process CSV rows
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string name, day, start, end, room;

        name  = readCSVField(ss);
        day   = readCSVField(ss);
        start = readCSVField(ss);
        end   = readCSVField(ss);
        room  = readCSVField(ss);

        name  = trimmer(name);
        day   = trimmer(day);
        start = trimmer(start);
        end   = trimmer(end);
        room  = trimmer(room);

        // Insert faculty
        sqlite3_bind_text(insertFacultyStmt, 1, name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(insertFacultyStmt, 2, deptName.c_str(), -1, SQLITE_STATIC);
        sqlite3_step(insertFacultyStmt);
        sqlite3_reset(insertFacultyStmt);

        // Insert office hour
        sqlite3_bind_text(insertHourStmt, 1, name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(insertHourStmt, 2, deptName.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(insertHourStmt, 3, day.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(insertHourStmt, 4, start.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(insertHourStmt, 5, end.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(insertHourStmt, 6, room.c_str(), -1, SQLITE_STATIC);
        sqlite3_step(insertHourStmt);
        sqlite3_reset(insertHourStmt);

        std::cout << "Loaded: [" << deptName << "] " << name << " "
                  << day << " " << start << "-" << end << " " << room << std::endl;
    }

    sqlite3_finalize(insertFacultyStmt);
    sqlite3_finalize(insertHourStmt);
    file.close();
    sqlite3_close(db);

    std::cout << "Successful! Data from " << csvFilename << " added to titan_office.db :) \n";
    return 0;
}
