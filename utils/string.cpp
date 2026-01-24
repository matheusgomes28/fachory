#include <utils/string.hpp>

#include <string>
#include <vector>

namespace fachory::utils::string {

    std::vector<std::string> split(std::string const& text, char delim) {
        if (text.empty()) {
            return {};
        }

        std::vector<std::string> ret;

        std::string::size_type current_pos = 0;
        while ((current_pos != std::string::npos) && (current_pos <= text.size())) {
            auto const next_pos = text.find(delim, current_pos);

            // Found the last one
            if ((current_pos != std::string::npos) && (next_pos == std::string::npos)) {
                auto const substr_size = text.size() - current_pos;
                ret.push_back(text.substr(current_pos, substr_size));
                break;
            }

            // Found a valid item
            if ((current_pos != std::string::npos) && (next_pos != std::string::npos)) {
                auto const substr_size = next_pos - current_pos;
                ret.push_back(text.substr(current_pos, substr_size));
            }

            current_pos = next_pos + 1;
        }

        return ret;
    }
} // namespace fachory::utils::string
