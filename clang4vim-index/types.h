#pragma once

#include <string>
#include <list>
#include <map>
#include <set>

typedef std::map<std::string, std::set<std::string>> ClicIndex;
typedef std::pair<std::string, std::set<std::string>> ClicIndexItem;

inline std::list<std::string> split(const std::string& str, int delim = ' '){
    std::list<std::string> res;
    std::string::size_type i = 0;
    std::string::size_type j = str.find(delim);

    while (j != std::string::npos) {
        res.push_back(str.substr(i, j-i));
        i = ++j;
        j = str.find(delim, j);

        if (j == std::string::npos)
            res.push_back(str.substr(i, str.length()));
    }

    return res;
}
