#pragma once

#include <istream>
#include <memory>

#include "types.h"

class istream_iterator {
public:
    typedef istream_iterator self_type;
    typedef ClicIndexItem value_type;
    typedef ClicIndexItem& reference;
    typedef ClicIndexItem* pointer;
    typedef std::forward_iterator_tag iterator_category;
    typedef size_t difference_type;

public:
    explicit istream_iterator(std::istream& strm)
        : strm_(strm), pos_(0), val_() {
        read_line();
    }

    istream_iterator end() {
        istream_iterator res = *this;
        res.pos_ = -1;

        return res;
    }

    self_type operator++() {
        read_line();
        if (pos_ != -1)
            pos_++;
        return *this;
    }

    reference operator*() { return val_; }
    pointer operator->() { return &val_; }

    bool operator==(const self_type& rhs) {
        return pos_ == rhs.pos_;
    }
    bool operator!=(const self_type& rhs) {
        return !(*this == rhs);
    }

private:
    void read_line();

private:
    std::istream& strm_;
    ssize_t pos_;
    value_type val_;
};

class istream_range {
public:
    istream_range(std::istream& strm) : it_(strm) { }
    istream_iterator begin() { return it_; }
    istream_iterator end() { return it_.end(); }
private:
    istream_iterator it_;
};

