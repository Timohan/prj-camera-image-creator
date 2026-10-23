#ifndef INI_FILE_H
#define INI_FILE_H

#include <map>
#include <string>
#include "../math/math3d.h"

/*!
 * \brief IniFile
 * Reads key=value lines, lines starting with # are comments.
 */
class IniFile
{
public:
    bool read(const std::string &filename);

    const std::string &getFilename() const;
    bool hasKey(const std::string &key) const;
    bool getFloat(const std::string &key, float *value) const;
    // vector is written as x/y/z
    bool getVector(const std::string &key, VECTOR3 *vector) const;
    // list is written as a/b/c..., it must have exactly count values
    bool getFloats(const std::string &key, float *values, int count) const;

private:
    std::string m_filename;
    std::map<std::string, std::string> m_values;
};

#endif // INI_FILE_H
