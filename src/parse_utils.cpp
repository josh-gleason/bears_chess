#include "parse_utils.hpp"

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

bool contains(const std::vector<std::string>& words, const std::string& value) {
    return std::find(words.begin(), words.end(), value) != words.end();
}

} // bears_chess