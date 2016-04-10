/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2016 ObdDiag.Net. All rights reserved.
 *
 */

//
// Lightweight string class
//

#ifndef __LSTRING_H__ 
#define __LSTRING_H__

#include <cstdint>

using namespace std;

//#define LSTRING_VALIDATE

namespace util {

class string {
public:
    const static uint32_t STRING_SIZE = 50;
    const static uint32_t npos = 0xFFFFFFFF;
    string(uint32_t size = STRING_SIZE);
    string(const char* s);
    string(const string& other);
    string(uint32_t count, char ch);
    ~string();
    void resize(uint32_t count);
    void resize(uint32_t count, char ch);
    string& append(const char* s);
    string& append(const char* s, uint32_t count);
    string& append(uint32_t count, char ch);
    string& assign(uint32_t count, char ch);
    void clear();
    uint32_t copy(char* dest, uint32_t count, uint32_t pos = 0) const;
    const char* c_str() const { return data_; }
    bool empty() const { return (length_ == 0); }
    uint32_t find(const string& str, uint32_t pos = 0) const;
    uint32_t find(char ch, uint32_t pos = 0) const;
    uint32_t length() const { return length_; }
    string substr(uint32_t pos, uint32_t count = npos) const;
    string& operator+=(const string& other);
    string& operator+=(const char* s);
    string& operator+=(char ch);
    char operator[](uint32_t pos) const { return data_[pos]; }
    char& operator[](uint32_t pos) { return data_[pos]; }
    string& operator=(const string& str);
    string& operator=(const char* s);
private:
    void init(uint32_t size);
    char* data_;
    uint16_t length_;
    uint16_t allocatedLength_;
#ifdef LSTRING_VALIDATE
    void validate(uint32_t size);
#endif
};

bool operator==(const string& lhs, const char* rhs);
bool operator!=(const string& lhs, const char* rhs);
bool operator==(const string& lhs, const string& rhs);
string operator+(const string& lhs, const string& rhs);
string operator+(const char* lhs, const string& rhs);
string operator+(char ch, const string& rhs);
string operator+(const string& lhs, const char* rhs);
string operator+(const string& lhs, char ch);

}

#endif //__LSTRING_H__


