# Titan Office Hours

Hey there! This is a searchable database of CSUF faculty office hours, available as a command-line tool and web interface.

**Live Site:** [jamil5150.github.io/titan-office-hours](https://jamil5150.github.io/titan-office-hours/)

## Features
- Search by professor name, department, or college
- Filter by today's hours
- Room locations included
- Mobile-friendly web interface
- CLI search tool for power users
- Semester-updatable database with `--replace` flag (if used locally)

## Project Structure
- **`/`** — Web interface (HTML, CSS, JavaScript, SQLite via sql.js)
- **`/cli/`** — Command-line search tool (`search.cpp`)
- **`/db_loader/`** — Database loader tool (`main.cpp`) that builds `titan_office.db` from CSV files

## How It Works
1. **Collect office hours** into a CSV file (see format below)
2. **Load into the database** using `db_loader/office_loader`
3. **Search** via the CLI (`cli/search`)

Just want to access faculty office hours without any fuss? Visit [my webpage](https://jamil5150.github.io/titan-office-hours/).<br>
Want to add office hours for someone? Fill out the submission form located [here](https://docs.google.com/forms/d/e/1FAIpQLSeJCflSEG24At4COMb1PJ6WOMLlqGLS2HTF7gksi4HaL8wOww/viewform?usp=dialog) or on my site! 

## CSV Format
```csv
name,day,start,end,room
"Smith, John",MW,11:00am,12:00pm,MH 123
