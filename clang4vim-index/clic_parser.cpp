#include "clic_parser.h"

void istream_iterator::read_line() {
    std::string str;

    if (pos_ == -1)
        return;

    std::getline(strm_, str);
    if (strm_.eof()) {
        pos_ = -1;
        return;
    }

    auto elems = split(str, '\t');
    if (elems.empty()) {
        pos_ = -1;
        return;
    }

    val_.first = elems.front();
    elems.pop_front();
    for (const auto& e : elems)
        val_.second.insert(e);
}

