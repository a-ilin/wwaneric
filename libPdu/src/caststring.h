
#ifndef CAST_STRING_H
#define CAST_STRING_H

#include <sstream>
#include <string>

template  <class in_value>
const std::string cast_string(const in_value &t)
{
 std::ostringstream ss;
 ss << t;
 return ss.str();
}

#endif

