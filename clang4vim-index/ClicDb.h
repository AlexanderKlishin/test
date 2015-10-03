#pragma once

#include <db_cxx.h>

#include <set>
#include <string>

class ClicDb {
public:
    ClicDb(const char* dbFilename);
    ~ClicDb();

    void clear();

    void set(const std::string& usr, const std::set<std::string>& locations);
    std::set<std::string> get(const std::string& usr);

    void addMultiple(const std::string &usr,
                     const std::set<std::string> &locationsToAdd);
    void rmMultiple(const std::string &usr,
                    const std::set<std::string> &locationsToRemove);

private:
    /*
    class ClicCursor {
    public:
        explicit ClicCursor(Db& db) : cursor(nullptr) {
            db.cursor(0, &cursor, 0);
        }
        void rm(Dbt* key, Dbt* value) {
            if (cursor->get(key, value, DB_GET_BOTH) != DB_NOTFOUND)
                cursor->del(0);
        }
        ~ClicCursor() {
            cursor->close();
        }
    private:
        Dbc* cursor;
    };*/

    Db db;
};
