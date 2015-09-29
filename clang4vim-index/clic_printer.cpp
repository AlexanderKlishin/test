#include "clic_printer.h"

void printLocations(std::ostream& out, const std::set<std::string>& locations) {
    bool first = true;
    for(const auto &it : locations) {
        if (!first)
            out << '\t';
        out << it;
        first = false;
    }
}

void printIndex(std::ostream& out, const ClicIndex& index)
{
    for(const auto &it : index) {
        if (!it.first.empty()) {
            out << it.first << '\t';
            printLocations(out, it.second);
            out << std::endl;
        }
    }
}

