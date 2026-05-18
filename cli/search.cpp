#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <regex>
#include "sqlite3.h"

// College abbreviation lookup
const std::map<std::string, std::string> collegeAbbrev = {
    {"ECS",  "Engineering and Computer Science"},
    {"HSS",  "Humanities and Social Sciences"},
    {"NSM",  "Natural Sciences and Mathematics"},
    {"CBE",  "College of Business and Economics"},
    {"COMM", "College of Communications"},
    {"EDUC", "College of Education"},
    {"HHD",  "College of Health and Human Development"},
    {"ARTS", "College of the Arts"}
};

const std::map<std::string, std::string> deptAbbrev = {
{"ACCT", "Accountancy"},
{"AFAM", "African American Studies"},
{"AGNG", "Aging Studies Program"},
{"AMST", "American Studies"},
{"ANTH", "Anthropology"},
{"ART", "Art"},
{"ASAM", "Asian American Studies"},
{"BIOL", "Biological Science"},
{"BUAD", "Business Administration"},
{"CAS", "Child and Adolescent Studies"},
{"CHEM", "Chemistry and Biochemistry"},
{"CHIC", "Chicana and Chicano Studies"},
{"COMD", "Communication Sciences and Disorders"},
{"COMM", "Communications"},
{"COUN", "Counseling"},
{"CPSC", "Computer Science"},
{"CTVA", "Cinema and Television Arts"},
{"ECON", "Economics"},
{"EDAD", "Educational Leadership"},
{"EDEL", "Elementary and Bilingual Education"},
{"EDSC", "Secondary Education"},
{"EGCE", "Civil and Environmental Engineering"},
{"EGEC", "Electrical and Computer Engineering"},
{"EGME", "Mechanical Engineering"},
{"EGGN", "Engineering Program"},
{"ENGL", "English, Comparative Literature and Linguistics"},
{"ENST", "Environmental Studies Program"},
{"FIN", "Finance"},
{"GEOG", "Geography and the Environment"},
{"GEOL", "Geological Sciences"},
{"HCOM", "Human Communication Studies"},
{"HIST", "History"},
{"HONR", "University Honors Program"},
{"HPAO", "Health Professions Advising Office"},
{"HUSR", "Human Services"},
{"IB", "International Business Program"},
{"IDT", "Instructional Design and Technology Program"},
{"IEE", "International Education and Engagement"},
{"ISDS", "Information Systems and Decision Sciences"},
{"KNES", "Kinesiology"},
{"LBST", "Liberal Studies"},
{"LTAM", "Latin American Studies Program"},
{"MATH", "Mathematics"},
{"MGMT", "Management"},
{"MKTG", "Marketing"},
{"MLNG", "Modern Languages and Literatures"},
{"MLSC", "Military Science Program"},
{"MSW", "Social Work"},
{"MUS", "Music"},
{"NURS", "Nursing"},
{"PHIL", "Philosophy"},
{"PHYS", "Physics"},
{"POSC", "Politics, Administration and Justice"},
{"PSYC", "Psychology"},
{"PUBH", "Public Health"},
{"READ", "Literacy and Reading Education"},
{"RLST", "Religious Studies"},
{"SCED", "Science Education Program"},
{"SOCI", "Sociology"},
{"SPED", "Special Education"},
{"THTR", "Theatre and Dance"},
{"UEXT", "University Extension"},
{"WGST", "Gender and Sexuality Studies"},
// Add more when needed
};

// Resolve abbreviation (returns full name if found, else original)
std::string resolveCollege(const std::string& input) {
    auto it = collegeAbbrev.find(input);
    return (it != collegeAbbrev.end()) ? it->second : input;
}

std::string resolveDept(const std::string& input) {
    auto it = deptAbbrev.find(input);
    return (it != deptAbbrev.end()) ? it->second : input;
}

std::string today_sys() {
    std::time_t t = std::time(nullptr);
    std::tm* now = std::localtime(&t);
    const char* days[] = {"Sunday", "Monday", "Tuesday", "Wednesday",
                          "Thursday", "Friday", "Saturday"};
    return days[now->tm_wday];
}

std::string formatTime(const std::string& timeStr) {
    if (timeStr.empty()) return "";
    
    std::string t = timeStr;
    // Remove any existing spaces
    t.erase(std::remove(t.begin(), t.end(), ' '), t.end());
    
    // Extract hours, minutes, and am/pm
    size_t colonPos = t.find(':');
    if (colonPos == std::string::npos) return timeStr;
    
    std::string hours = t.substr(0, colonPos);
    std::string minutes = t.substr(colonPos + 1, 2);
    std::string ampm;
    
    if (t.find("am") != std::string::npos || t.find("AM") != std::string::npos) {
        ampm = "AM";
    } else if (t.find("pm") != std::string::npos || t.find("PM") != std::string::npos) {
        ampm = "PM";
    } else {
        return timeStr;
    }
    
    return hours + ":" + minutes + " " + ampm;
}

std::string formatTimeRange(const std::string& hoursStr) {
    std::string result = hoursStr;
    std::regex timePattern(R"((\d{1,2}:\d{2})(am|pm)-(\d{1,2}:\d{2})(am|pm))");
    std::smatch match;
    
    std::string::const_iterator searchStart(result.cbegin());
    while (std::regex_search(searchStart, result.cend(), match, timePattern)) {
        std::string formatted = match[1].str() + " " + 
                               (match[2].str() == "am" ? "AM" : "PM") + 
                               " - " + 
                               match[3].str() + " " + 
                               (match[4].str() == "am" ? "AM" : "PM");
        
        result.replace(match.position(), match.length(), formatted);
        searchStart = result.cbegin() + match.position() + formatted.length();
    }
    
    return result;
}
// Prints college/department hierarchy
void listCollegesAndDepartments(sqlite3* db, const std::string& collegeFilter) {
    std::string sql;
    std::string bindParam;

    if (collegeFilter.empty()) {
        sql = R"(
            SELECT c.name, d.name
            FROM colleges c
            LEFT JOIN departments d ON c.id = d.college_id
            ORDER BY c.name, d.name;
        )";
    } else {
        sql = R"(
            SELECT c.name, d.name
            FROM colleges c
            LEFT JOIN departments d ON c.id = d.college_id
            WHERE c.name LIKE ?1
            ORDER BY c.name, d.name;
        )";
        bindParam = collegeFilter;
    }

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << sqlite3_errmsg(db) << std::endl;
        return;
    }

    if (!bindParam.empty()) {
        sqlite3_bind_text(stmt, 1, bindParam.c_str(), -1, SQLITE_STATIC);
    }

    std::string currentCollege;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char* col = sqlite3_column_text(stmt, 0);
        const unsigned char* dep = sqlite3_column_text(stmt, 1);

        std::string c = col ? reinterpret_cast<const char*>(col) : "";
        std::string d = dep ? reinterpret_cast<const char*>(dep) : "";

        if (c != currentCollege) {
            std::cout << c << ":\n";
            currentCollege = c;
        }
        if (!d.empty()) {
            std::cout << "  - " << d << "\n";
        } else {
            std::cout << "  (no departments loaded)\n";
        }
    }

    sqlite3_finalize(stmt);
}

// Print office hours with college grouping
void printOfficeHours(sqlite3* db,
                      const std::string& lastName,
                      const std::string& dept,
                      const std::string& college,
                      bool todayOnly) {
    std::string sql = R"(
        SELECT c.name AS College, d.name AS Department,
               f.name AS Faculty, oh.day,
               GROUP_CONCAT(oh.start_time || '-' || oh.end_time, ', ') AS Hours
        FROM office_hours oh
        JOIN faculty f ON oh.faculty_id = f.id
        JOIN departments d ON f.dept_id = d.id
        JOIN colleges c ON d.college_id = c.id
        WHERE 1=1
    )";

    std::vector<std::string> params;
    int bindIdx = 1;

    if (!lastName.empty()) {
        sql += " AND f.name LIKE ?" + std::to_string(bindIdx++);
        params.push_back("%" + lastName + "%");
    }
    if (!dept.empty()) {
        sql += " AND d.name LIKE ?" + std::to_string(bindIdx++);
        params.push_back(dept);
    }
    if (!college.empty()) {
        sql += " AND c.name LIKE ?" + std::to_string(bindIdx++);
        params.push_back(college);
    }
    if (todayOnly) {
        sql += " AND oh.day = ?" + std::to_string(bindIdx++);
        params.push_back(today_sys());
    }

    sql += " GROUP BY c.name, d.name, f.name, oh.day "
           "ORDER BY c.name, d.name, f.name, oh.day;";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << sqlite3_errmsg(db) << std::endl;
        return;
    }

    for (size_t i = 0; i < params.size(); ++i) {
        sqlite3_bind_text(stmt, (int)(i + 1), params[i].c_str(), -1, SQLITE_STATIC);
    }

    std::string currentCollege, currentDept, currentFaculty;
    bool first = true;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char* col  = sqlite3_column_text(stmt, 0);
        const unsigned char* dep  = sqlite3_column_text(stmt, 1);
        const unsigned char* fac  = sqlite3_column_text(stmt, 2);
        const unsigned char* day  = sqlite3_column_text(stmt, 3);
        const unsigned char* hrs  = sqlite3_column_text(stmt, 4);

        std::string c = col ? reinterpret_cast<const char*>(col) : "";
        std::string d = dep ? reinterpret_cast<const char*>(dep) : "";
        std::string f = fac ? reinterpret_cast<const char*>(fac) : "";
        std::string dy= day ? reinterpret_cast<const char*>(day) : "";
        std::string h = hrs ? reinterpret_cast<const char*>(hrs) : "";

        if (c != currentCollege) {
            if (!first) std::cout << "\n";
            std::cout << c << ":\n";
            currentCollege = c;
            currentDept.clear();
        }
        if (d != currentDept) {
            std::cout << "  " << d << ":\n";
            currentDept = d;
            currentFaculty.clear();
        }
        if (f != currentFaculty) {
            std::cout << "    " << f << ":\n";
            currentFaculty = f;
        }
        std::cout << "      " << dy << "    " << formatTimeRange(h) << "\n";
        first = false;
    }

    sqlite3_finalize(stmt);
}
int main(int argc, char* argv[]) {
    std::vector<std::string> command_line(argv, argv + argc);

    std::string lastNameFilter;
    std::string deptFilter;
    std::string collegeFilter;
    bool todayOnly = false;
    bool listMode = false;

    for (size_t i = 1; i < command_line.size(); ++i) {
        if (command_line[i] == "--help" || command_line[i] == "-h") {
            std::cout << "Usage: " << command_line[0] << " [options]\n"
                      << "Options:\n"
                      << "  --lastname NAME      partial faculty name (case insensitive)\n"
                      << "  --department DEPT    department name or abbreviation\n"
                      << "  --college COLLEGE    college name or abbreviation\n"
                      << "  --today              show only today's hours\n"
                      << "  --help               this message\n";
            return 0;
        } else if (command_line[i] == "--lastname" && i + 1 < command_line.size()) {
            lastNameFilter = command_line[++i];
        } else if (command_line[i] == "--department" && i + 1 < command_line.size()) {
            deptFilter = resolveDept(command_line[++i]);
        } else if (command_line[i] == "--college" && i + 1 < command_line.size()) {
            collegeFilter = resolveCollege(command_line[++i]);
        } else if (command_line[i] == "--today") {
            todayOnly = true;
        } else if (command_line[i] == "--list"){
            listMode = true;
        }
        else {
            std::cerr << "Unknown option: " << command_line[i] << "\n";
            return 1;
        }
    }

    sqlite3* db;
    int rc = sqlite3_open("titan_office.db", &db);
    if (rc) {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        return 1;
    }

    if (listMode){
        listCollegesAndDepartments(db, collegeFilter);
     }else{
      if (todayOnly) {
        std::cout << "Showing hours for " << today_sys() << " only.\n\n";
    }

    printOfficeHours(db, lastNameFilter, deptFilter, collegeFilter, todayOnly);
     }
    sqlite3_close(db);
    return 0;
}
