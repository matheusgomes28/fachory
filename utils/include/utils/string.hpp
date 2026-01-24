#ifndef UTILS_STRING_H
#define UTILS_STRING_H

#include <string>
#include <vector>

namespace fachory::utils::string {
    std::vector<std::string> split(std::string const& text, char delim);
} // namespace fachory::utils::string

#endif // UTILS_STRING_H
//
