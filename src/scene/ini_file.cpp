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
    m_listValues.clear();
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
        const std::string key = trim(line.substr(0, equal));
        const std::string value = trim(line.substr(equal + 1));
        m_values[key] = value;
        m_listValues.push_back(std::make_pair(key, value));
    }
    return true;
}

const std::string &IniFile::getFilename() const
{
    return m_filename;
}

bool IniFile::hasKey(const std::string &key) const
{
    return m_values.find(key) != m_values.end();
}

bool IniFile::getFloat(const std::string &key, float *value) const
{
    auto it = m_values.find(key);
    return it != m_values.end() && parseFloat(it->second, value);
}

bool IniFile::getVector(const std::string &key, VECTOR3 *vector) const
{
    float values[3];
    if (!getFloats(key, values, 3)) {
        return false;
    }
    vector->x = values[0];
    vector->y = values[1];
    vector->z = values[2];
    return true;
}

bool IniFile::getFloats(const std::string &key, float *values, int count) const
{
    auto it = m_values.find(key);
    return it != m_values.end() && parseFloats(it->second, values, count);
}

bool IniFile::parseFloats(const std::string &text, float *values, int count)
{
    size_t begin = 0;
    for (int i=0;i<count;i++) {
        const size_t end = text.find('/', begin);
        if ((end == std::string::npos) != (i == count-1)) {
            // too few or too many values
            return false;
        }
        if (!parseFloat(trim(text.substr(begin, end == std::string::npos ? end : end - begin)), &values[i])) {
            return false;
        }
        begin = end + 1;
    }
    return true;
}

std::vector<std::string> IniFile::getValues(const std::string &key) const
{
    std::vector<std::string> values;
    for (const auto &keyValue : m_listValues) {
        if (keyValue.first == key) {
            values.push_back(keyValue.second);
        }
    }
    return values;
}
