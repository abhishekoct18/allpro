/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2016 ObdDiag.Net. All rights reserved.
 *
 */

#ifndef __ALGORITHMS_H__ 
#define __ALGORITHMS_H__

#include "lstring.h"

namespace util {
    
    void to_lower(string& str);
    void to_upper(string& str);
    void remove_space(string& str);
    uint32_t stoul(const string& str, uint32_t* pos = 0, int base = 10);
    bool is_xdigits(const string& str);
    char to_ascii(uint8_t byte);
    
}

#endif //__ALGORITHMS_H__
