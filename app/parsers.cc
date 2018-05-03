#include "parsers.hh"

#include <regex>
#include <algorithm>
#include <cmath>

namespace bzzt {

namespace {

std::vector<char> whitespace_chars {' ', '\n', '\t', '\r'};
std::vector<char> group_begin_chars {'(', '[', '{'};
std::vector<char> group_end_chars {')', ']', '}'};

}

std::vector<std::string> parse_sections(std::string const& str) {
    std::vector<std::string> vec {};

    std::regex section_regex {"\\[([^\\]]+)\\]\\n([^\\[]+)"};
    std::sregex_iterator next {begin(str), end(str), section_regex};
    std::sregex_iterator end;

    for (; next != end; ++next) {
        auto match {*next};
        vec.push_back(match[1]);
        vec.push_back(match[2]);
    }

    return vec;
}

std::vector<std::string> parse_lines(std::string const& str) {
    std::vector<std::string> vec {};

    std::regex line_regex {"(.+?)\\n"};
    std::sregex_iterator next {begin(str), end(str), line_regex};
    std::sregex_iterator end;

    for (; next != end; ++next) {
        auto match {*next};
        vec.push_back(match[1]);
    }

    return vec;
}

std::vector<std::string> parse_whitespace_separated_values(std::string const& str) {
    std::vector<std::string> vec {};

    std::string value {};
    auto parsing_value {false};
    unsigned int indent_level {0};

    for (unsigned int i {0}; i < str.size(); ++i) {
        if (!parsing_value && std::find(std::begin(whitespace_chars), std::end(whitespace_chars), str[i]) == std::end(whitespace_chars)) {
            parsing_value = true;
        }

        if (parsing_value) {
            if (std::find(std::begin(group_begin_chars), std::end(group_begin_chars), str[i]) != std::end(group_begin_chars)) {
                indent_level += 1;
            }
            if (std::find(std::begin(group_end_chars), std::end(group_end_chars), str[i]) != std::end(group_end_chars) && indent_level > 0) {
                indent_level -= 1;
            }
            if (std::find(std::begin(whitespace_chars), std::end(whitespace_chars), str[i]) != std::end(whitespace_chars) && indent_level == 0) {
                parsing_value = false;
                vec.push_back(value);
                value = "";
                continue;
            }

            value.push_back(str[i]);
        }
    }

    if (value.size() > 0) {
        vec.push_back(value);
    }

    return vec;
}

unsigned int parse_unsigned_int(std::string const& str) {
    unsigned int val {0};

    for (unsigned int i {0}; i < str.size() && str[i] >= '0' && str[i] <= '9'; ++i) {
        val *= 10;
        val += str[i] - '0';
    }

    return val;
}

float parse_float(std::string const& str) {
    float val {0.0f};

    unsigned dot_pos {0};
    bool dot_found {false};

    for (unsigned int i {0}; i < str.size() && ((str[i] >= '0' && str[i] <= '9') || str[i] == '.'); ++i) {
        if (str[i] == '.') {
            if (dot_found) {
                return 0.0f;
            }
            dot_pos = i;
            dot_found = true;
        } else {
            val *= 10;
            val += str[i] - '0';
        }
    }

    auto exponent {str.size() - dot_pos};
    exponent -= exponent > 0 ? 1 : 0;
    val = val / (dot_found ? std::pow(10, exponent) : 1);

    return val;
}

}