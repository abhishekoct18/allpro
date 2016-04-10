/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2016 ObdDiag.Net. All rights reserved.
 *
 */

#include <cctype>
#include <cstdlib>
#include "algorithms.h"

using namespace std;

namespace util {

/**
 * Convert string to lowercase
 * @param[in,out] str String to convert
 */
void to_lower(string& str)
{
    for (int i = 0; i < str.length(); i++) {
        str[i] = tolower(str[i]);
    }
}

/**
 * Convert string to uppercase
 * @param[in,out] str String to convert
 */
void to_upper(string& str)
{
    for (int i = 0; i < str.length(); i++) {
        str[i] = toupper(str[i]);
    }
}

/**
 * Checks if string is a valid ASCII hex string
 * @param[in] str String to validate
 * @return 1 if valid, 0 otherwise
 */
bool is_xdigits(const string& str)
{
    int len = str.length();
    if (len == 0 || len % 2) {
        return false; // Invalid string
    }

    for (int i = 0; i < len; i++) {
        if (!isxdigit(str[i])) {
            return false;
        }
    }
    return true;
}

/**
 * Do string space compression and uppercase conversion
 * @param[in,out] str String to perform action to
 */
void remove_space(string& str)
{
    int len = str.length();
    int j = 0;
    for (int i = 0; i < len; i++) {
        if (str[i] > ' ') { // Skip space or non-printable
            str[j++] = str[i];
        }
    }
    str.resize(j);
}

/**
 * Standard library stoul implementation
 * @param[in] str String to perform action to
 * @param[out] pos The result length
 * @param[in] base Base
 * @return The result value
 */
uint32_t stoul(const string& str, uint32_t* pos, int base)
{
    if (pos)
        *pos = str.length();
    return strtoul(str.c_str(), 0, base);;
}

/**
 * Byte to Ascii converter
 * @param[in] byte Byte to convert
 * @return An ASCII characher
 */
char to_ascii(uint8_t byte)
{
    const char dispthTable[] = { '0', '1', '2', '3', '4', '5', '6', '7',
                                 '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
    return (byte <= 0xF) ? dispthTable[byte] : 0;
}

}
