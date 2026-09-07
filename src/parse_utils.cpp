#include "parse_utils.hpp"

#include <iterator>
#include <algorithm>
#include <sstream>

namespace bears_chess {

KeyedArgs group_by_keywords(const std::vector<std::string>& args, const std::vector<std::string>& keywords) {
    KeyedArgs result;
    std::string current = "";
    result[current];

    for (auto& word : args) {
        if (std::find(keywords.begin(), keywords.end(), word) != keywords.end()) {
            current = word;
            result[current];
        } else {
            result[current].push_back(word);
        }
    }
    return result;
}

std::vector<std::string> split(const std::string& line) {
    std::istringstream iss(line);
    std::vector<std::string> tokens{
        std::istream_iterator<std::string>{iss},
        std::istream_iterator<std::string>{}
    };
    return tokens;
}

std::string join(const std::vector<std::string>& words, const std::string& delimiter) {
    if (words.empty()) {
        return "";
    }
    std::string result = words[0];
    for (auto it = words.cbegin() + 1; it != words.cend(); ++it) {
        result += delimiter + *it;
    }
    return result;
}

std::string to_lower(const std::string& text) {
    std::string text_lower;
    text_lower.reserve(text.size());
    for (auto ch : text) {
        text_lower += static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return text_lower;
}

bool contains(const std::vector<std::string>& words, const std::string& value) {
    return std::find(words.begin(), words.end(), value) != words.end();
}

} // bears_chess