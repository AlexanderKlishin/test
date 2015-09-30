#include <cassert>
#include <iostream>

#include <boost/config/warning_disable.hpp>
#include <boost/fusion/adapted/std_pair.hpp>
#include <boost/fusion/include/std_pair.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/spirit/include/qi.hpp>

#include "clic_parser.h"

namespace spirit = boost::spirit;
namespace ascii = boost::spirit::ascii;
namespace qi = boost::spirit::qi;
namespace phoenix = boost::phoenix;

template <typename Iterator>
struct Grammar : qi::grammar<Iterator, ClicIndexItem ()>
{
    Grammar() : Grammar::base_type(line)
    {
        using qi::_val;
        using qi::_1;
        using qi::char_;
        using qi::lit;
        using qi::eps;
        using ascii::space;

        word    = eps[_val = ""]            >> (+ (char_ - (lit('\t') | lit('\n')))[_val += _1]);
        wordSet = eps[phoenix::clear(_val)] >> (+word[phoenix::insert(_val, _1)] % '\t');
        line    %= word >> '\t' >> wordSet >> '\n';
    }

    qi::rule<Iterator, std::string ()> word;
    qi::rule<Iterator, std::set<std::string> ()> wordSet;
    qi::rule<Iterator, ClicIndexItem ()> line;
};

Grammar<spirit::istream_iterator> grammar;

IndexItemIterator::IndexItemIterator() : i(-1) {}

IndexItemIterator::IndexItemIterator(std::istream& in) :
    value(std::make_shared<ClicIndexItem>()),
    i(0),
    in(&in),
    inputBegin(in)
{
    in.unsetf(std::ios::skipws);
}

const ClicIndexItem& IndexItemIterator::dereference() const {
    return *value;
}

bool IndexItemIterator::equal(const IndexItemIterator& other) const {
    return i == other.i;
}

void IndexItemIterator::increment() {
    assert(i != -1);
    bool ok = qi::parse(inputBegin, inputEnd, grammar, *value);
    if (ok) {
        i++;
    } else {
        i = -1; // reached the end

        if (inputBegin != inputEnd) {
            std::cerr << "Trailing characters:" << std::endl;
            while (!in->eof()) {
                char c;
                *in >> c;
                std::cerr << c;
            }
        }
    }
}

boost::iterator_range<IndexItemIterator> parseIndex(std::istream& in) {
    return boost::make_iterator_range(IndexItemIterator(in), IndexItemIterator());
}
