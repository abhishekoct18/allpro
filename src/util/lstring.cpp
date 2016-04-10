/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2016 ObdDiag.Net. All rights reserved.
 *
 */

#include <cstring>
#include <cstdlib>
#include <stdexcept>
#include <cctype>
#include <algorithm>
#include "lstring.h"

#ifdef LSTRING_VALIDATE
    #define VALIDATE(size) validate(size)
#else
    #define VALIDATE(size)
#endif

using namespace std;
    
namespace util {

void string::init(uint32_t size)
{
    allocatedLength_ = size > STRING_SIZE ? size : STRING_SIZE;
    allocatedLength_++;
    data_ = new char[allocatedLength_]; // Including null terminator
}

string::string(uint32_t size)
{
    init(size);
    data_[0] = 0;
    length_ = 0;
}

//
// Validate the string memory overrun, throws exception
//
#ifdef LSTRING_VALIDATE
void string::validate(uint32_t size)
{
    if (size >= allocatedLength_) {
        abort();
    }
}
#endif

string::string(const char* s)
{
    length_ = strlen(s);
    init(length_);
    strcpy(data_, s);
}

string::string(const string& str)
{
    length_ = str.length_;
    init(length_);
    strcpy(data_, str.data_);
}

string::string(uint32_t count, char ch)
{
    length_ = count;
    init(length_);
    memset(data_, ch, length_);
    data_[length_] = 0;
}

string::~string()
{
    delete[] data_; 
}

void string::resize(uint32_t count)
{
    if (count < length_) {
        length_ = count;
        data_[length_] = 0;
    }
    else {
        ; // nothing
    }
}

void string::resize(uint32_t count, char ch)
{
    if (count < length_) {
        length_ = count;
        data_[length_] = 0;
    }
    else if (count > length_) {
        VALIDATE(count);
        memset(data_ + length_, ch, count - length_);
        length_ = count;
        data_[length_] = 0;
    }
}

string& string::append(const char* s)
{
    int len = strlen(s);
    VALIDATE(length_ + len); 
    strcpy(data_ + length_, s);
    length_ += len;
    return *this;
}

string& string::append(const char* s, uint32_t count)
{
    VALIDATE(length_ + count);
    memcpy(data_ + length_, s, count);
    length_ += count;
    data_[length_] = 0;
    return *this;
}

string& string::append(uint32_t count, char ch)
{
	VALIDATE(length_ + count);
    for (uint32_t i = 0; i < count; i++) {
        (*this) += ch;
    }
    return *this;
}

string& string::assign(uint32_t count, char ch)
{
    length_ = count;
    VALIDATE(length_ + count);
    memset(data_, ch, count);
    data_[count] = 0;
    return *this;
}

string& string::operator+=(const string& str)
{
    return append(str.data_, str.length_);
}

string& string::operator+=(const char* s)
{
    return append(s);
}

string& string::operator+=(char ch)
{
    VALIDATE(length_ + 1);
    data_[length_] = ch;
    data_[++length_] = 0;
    return *this;
}

uint32_t string::find(const string& str, uint32_t pos) const
{
    const char* p = strstr(data_ + pos, str.data_);
    return p ? (p - data_) : npos;
}

uint32_t string::find(char ch, uint32_t pos) const
{
    const char* p = strchr(data_ + pos, ch);
    return p ? (p - data_) : npos;
}

string string::substr(uint32_t pos, uint32_t count) const
{
    uint32_t p = (pos >= length_) ? 0 : pos; // should be an exception
    uint32_t l = (length_ < count) ? (length_ - p) : count;
    string s(l + 1);
    s.append(data_ + p, l);
    return s;
}

string& string::operator=(const string& str)
{
    VALIDATE(str.length_);
    memcpy(data_, str.data_, str.length_ + 1); // including null terminator
    length_ = str.length_;
    return *this;
}

string& string::operator=(const char* s)
{
    length_ = strlen(s);
    VALIDATE(length_);
    strcpy(data_, s);
    return *this;
}
void string::clear() 
{
	length_ = 0;
	data_[0] = 0;
}

uint32_t string::copy(char* dest, uint32_t count, uint32_t pos) const
{
	memcpy(dest, data_ + pos, count);
    return count;
}

bool operator==(const string& lhs, const char* rhs)
{
    return (strcmp(lhs.c_str(), rhs) == 0);
}

bool operator!=(const string& lhs, const char* rhs)
{
    return (strcmp(lhs.c_str(), rhs) != 0);
}

bool operator==(const string& lhs, const string& rhs)
{
    return (strcmp(lhs.c_str(), rhs.c_str()) == 0);
}

string operator+(const string& lhs, const string& rhs)
{
    string s(lhs);
    s += rhs;
    return s;
}

string operator+(const char* lhs, const string& rhs)
{
    int len = strlen(lhs) + rhs.length();
    string s(len + 1);
    s += lhs;
    s += rhs;
    return s;
}

string operator+(char ch, const string& rhs)
{
    int len = rhs.length() + 1;
    string s(len + 1);
    s += ch;
    s += rhs;
    return s;
}

string operator+(const string& lhs, const char* rhs)
{
    int len = lhs.length() + strlen(rhs);
    string s(len + 1);
    s += lhs;
    s += rhs;
    return s;
}

string operator+(const string& lhs, char ch)
{
    int len = lhs.length() + 1;
    string s(len + 1);
    s += lhs;
    s += ch;
    return s;
}

} // end util namespace
