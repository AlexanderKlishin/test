#include <sstream>
#include <iterator>

#include "ClicDb.h"
#include "types.h"
#include "clic_printer.h"
#include "clic_parser.h"

ClicDb::ClicDb(const char* dbFilename) : db(NULL, 0)
{
    try {
        db.set_error_stream(&std::cerr);
        db.open(NULL, dbFilename, NULL, DB_BTREE, DB_CREATE, 0);
    } catch(DbException &e) {
        std::cerr << "Exception thrown: " << e.what() << std::endl;
        exit(1);
    }
}

ClicDb::~ClicDb() {
    db.close(0);
}

void ClicDb::clear() {
    try {
        db.truncate(0, 0, 0);
    } catch(DbException &e) {
        std::cerr << "Exception thrown: " << e.what() << std::endl;
        exit(1);
    }
}

void ClicDb::set(const std::string& usr, const std::set<std::string>& locations) {
    std::stringstream ss;

    printLocations(ss, locations);
    std::string locationsString = ss.str();
    Dbt key(const_cast<char*>(usr.c_str()), usr.size());
    Dbt value(const_cast<char*>(locationsString.c_str()), locationsString.size());
    db.put(NULL, &key, &value, 0);
}

std::set<std::string> ClicDb::get(const std::string& usr) {
    Dbt key(const_cast<char*>(usr.c_str()), usr.size());
    Dbt value;

    if (db.get(NULL, &key, &value, 0) == DB_NOTFOUND)
        return std::set<std::string>();
    std::string str((char*)value.get_data(), value.get_size());

    std::set<std::string> res;
    for (const auto &i : split(str, '\t'))
        res.insert(i);
    return res;
}

void ClicDb::addMultiple(const std::string& usr, const std::set<std::string>& locationsToAdd) {
    if (locationsToAdd.empty())
        return;

    std::set<std::string> storedLocations = get(usr);
    std::copy(locationsToAdd.begin(), locationsToAdd.end(), std::inserter(storedLocations, storedLocations.begin()));
    set(usr, storedLocations);
}

void ClicDb::rmMultiple(const std::string& usr, const std::set<std::string> &locationsToRemove) {
    std::set<std::string> storedLocations = get(usr);
    size_t originalCount = storedLocations.size();
    for (const auto &loc : locationsToRemove)
        storedLocations.erase(loc);
    if (storedLocations.size() < originalCount)
        set(usr, storedLocations);
}
