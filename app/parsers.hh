#pragma once

#include <vector>
#include <string>
#include <array>

namespace bzzt {

// [ <section> <contents> ] ...
std::vector<std::string> parse_sections(std::string const& str);

// <line> ...
std::vector<std::string> parse_lines(std::string const& str);

// <value> ...
// Parenthesized sections are treated as one single value.
std::vector<std::string> parse_whitespace_separated_values(std::string const& str);

unsigned int parse_unsigned_int(std::string const& str);

float parse_float(std::string const& str);

}