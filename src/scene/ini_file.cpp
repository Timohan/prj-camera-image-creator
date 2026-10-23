#include "ini_file.h"
#include <fstream>
#include <iostream>
#include <cstdlib>

static std::string trim(const std::string &text)
{
    const char *spaces = " \t\r\n";
    const size_t begin = text.find_first_not_of(spaces);
    if (begin == std::string::npos) {
        return "";
    }
    return text.substr(begin, text.find_last_not_of(spaces) - begin + 1);
}

static bool parseFloat(const std::string &text, float *value)
{
    char *end = nullptr;
    *value = static_cast<float>(strtod(text.c_str(), &end));
    return !text.empty() && *end == '\0';
}

/*!
 * \brief IniFile::read
 * \param filename full path of the ini file
 * \return true on success
 */
bool IniFile::read(const std::string &filename)
{
    std::ifstream file(filename);
    std::string line;

    m_filename = filename;
    m_values.clear();
    if (!file.is_open()) {
        std::cerr << "Cannot open " << filename << std::endl;
        return false;
    }
    while (std::getline(file, line)) {
        line = trim(line);
        const size_t equal = line.find('=');
        if (line.empty() || line[0] == '#' || equal == std::string::npos) {
            continue;
        }
        m_values[trim(line.substr(0, equal))] = trim(line.substr(equal + 1));
    }
    return true;
}

const std::string &IniFile::getFilename() const
{
    return m_filename;
}

bool IniFile::getFloat(const std::string &key, float *value) const
{
    auto it = m_values.find(key);
    return it != m_values.end() && parseFloat(it->second, value);
}

bool IniFile::getVector(const std::string &key, VECTOR3 *vector) const
{
    auto it = m_values.find(key);
    if (it == m_values.end()) {
        return false;
    }
    const std::string &text = it->second;
    const size_t first = text.find('/');
    const size_t second = first == std::string::npos ? first : text.find('/', first + 1);
    if (second == std::string::npos) {
        return false;
    }
    return parseFloat(trim(text.substr(0, first)), &vector->x)
        && parseFloat(trim(text.substr(first + 1, second - first - 1)), &vector->y)
        && parseFloat(trim(text.substr(second + 1)), &vector->z);
}
