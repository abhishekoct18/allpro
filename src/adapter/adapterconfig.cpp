/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2016 ObdDiag.Net. All rights reserved.
 *
 */

#include <cstring>
#include <adaptertypes.h>

using namespace std;
const uint64_t onebit = 1;

//
// Configuration settings, storing/retrieving properties
//

AdapterConfig::AdapterConfig() : values_(0)
{
    memset(intProps_, 0, sizeof(intProps_));
}
    
void AdapterConfig::setBoolProperty(int id, bool val)
{
    if (id > 64) 
        return;
    values_ = val ? (values_ | (onebit << id)) : (values_ & ~(onebit << id));
}

bool AdapterConfig::getBoolProperty(int id) const
{
    if (id > 64) 
        return false;
    return values_ & (onebit << id);
}

void AdapterConfig::setIntProperty(int id, uint32_t val) 
{
    int idx = id - INT_PROPS_START;
    intProps_[idx] = val;
}

uint32_t AdapterConfig::getIntProperty(int id) const
{
    int idx = id - INT_PROPS_START;
    return intProps_[idx];
}

void AdapterConfig::setBytesProperty(int id, const ByteArray* bytes)
{
    int idx = id - BYTES_PROPS_START;
    bytesProps_[idx] = *bytes;
}

const ByteArray* AdapterConfig::getBytesProperty(int id) const
{
    int idx = id - BYTES_PROPS_START;
    return &bytesProps_[idx];
}

AdapterConfig* AdapterConfig::instance()
{
    static AdapterConfig instance;
    return &instance;
}
