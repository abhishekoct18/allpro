/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2016 ObdDiag.Net. All rights reserved.
 *
 */

#include <adaptertypes.h>
#include <algorithms.h>
#include "ecumsg.h"

using namespace util;

const int HEADER_SIZE = 3;

class EcumsgISO9141 : public Ecumsg {
    friend class Ecumsg;
public:
    virtual void addHeaderAndChecksum();
    virtual bool stripHeaderAndChecksum();
private:
    EcumsgISO9141(uint32_t size) : Ecumsg(ISO9141, size) {
        const uint8_t header[] = { 0x68, 0x6A, 0xF1 };
        memcpy(header_, header, sizeof(header));
    }
};

class EcumsgISO14230 : public Ecumsg {
    friend class Ecumsg;
public:
    virtual void addHeaderAndChecksum();
    virtual bool stripHeaderAndChecksum();
private:    
    EcumsgISO14230(uint32_t size) : Ecumsg(ISO14230, size) {
        const uint8_t header[] = { 0xC0, 0x33, 0xF1 };
        memcpy(header_, header, sizeof(header));
    }
};

class EcumsgVPW : public Ecumsg {
    friend class Ecumsg;
public:
    virtual void addHeaderAndChecksum();
    virtual bool stripHeaderAndChecksum();
private:    
    EcumsgVPW(uint32_t size) : Ecumsg(VPW, size) {
        const uint8_t header[] = { 0x68, 0x6A, 0xF1 };
        memcpy(header_, header, sizeof(header));
    }
};

class EcumsgPWM : public Ecumsg {
    friend class Ecumsg;    
public:
    virtual void addHeaderAndChecksum();
    virtual bool stripHeaderAndChecksum();
private:
    EcumsgPWM(uint32_t size) : Ecumsg(PWM, size) {
        const uint8_t header[] = { 0x61, 0x6A, 0xF1 };
        memcpy(header_, header, sizeof(header));
    }
};

/**
 * Factory method for adapter protocol messages
 * @param[in] type Message type
 * @return The message instance 
 */
Ecumsg* Ecumsg::instance(uint8_t type)
{
    const uint32_t size = 255;
    Ecumsg* instance = 0;
    switch(type) {
        case ISO9141:
            instance = new EcumsgISO9141(size);
            break;
        case ISO14230:
            instance = new EcumsgISO14230(size);
            break;
        case VPW:
            instance = new EcumsgVPW(size);
            break;
        case PWM:
            instance = new EcumsgPWM(size);
            break;
    }
    
    if (instance) {
        // if header
        const ByteArray* bytes = AdapterConfig::instance()->getBytesProperty(PAR_HEADER_BYTES);
        if (bytes->length)
            instance->setHeader(bytes->data);
    }
    return instance;
}

/**
 *  Adds the checksum to ISO 9141/14230 message
 *  @param[in,out] data Data bytes
 *  @param[in,out] length Data length
 */
static void IsoAddChecksum(uint8_t* data, uint8_t& length)
{
    uint8_t sum = 0;
    for (int i = 0; i < length; i++) {
        sum += data[i];
    }
    data[length++] = sum;
}

/**
 * Strip the header from ISO9141/14230 or J1850 message
 * @param[in,out] data Data bytes
 * @param[in,out] length Data length
 */
static void StripHeader(uint8_t* data, uint8_t& length)
{
    length -= HEADER_SIZE;
    memmove(&data[0], &data[HEADER_SIZE], length);
}

/*
 * Add the checksum to J1850 message
 * @param[in,out] data Data bytes
 * @param[in,out] length Data length
 */
static void J1850AddChecksum(uint8_t* data, uint8_t& length)
{
    const uint8_t* ptr = data;
    int len = length;
    uint8_t chksum = 0xFF;  // start with all one's
    
    while (len--) {
        int i = 8;
        uint8_t val = *(ptr++);
        while (i--) {
            if (((val ^ chksum) & 0x80) != 0) {
                chksum ^= 0x0E;
                chksum = (chksum << 1) | 1;
            } 
            else {
                chksum = chksum << 1;
            }
            val = val << 1;
        }
    }
    data[length++] = ~chksum;
}

/**
 * Strip the checksum from J1850 message
 * @param[in,out] data Data bytes
 * @param[in,out] length Data length
 */
static void J1850StripChecksum(uint8_t* data, uint8_t& length)
{
    length--;
}

/**
 * Strip the checksum from ISO 9141/1423 message
 * @param[in,out] data Data bytes
 * @param[in,out] length Data length
 */
static void ISOStripChecksum(uint8_t* data, uint8_t& length)
{
    length--;
}

/**
 * Construct Ecumsg object
 */
Ecumsg::Ecumsg(uint8_t type, uint32_t size) : type_(type), length_(0), size_(size)
{
    data_ = new uint8_t[size];
}

/**
 * Destructor
 */
Ecumsg::~Ecumsg()
{
    delete[] data_;
}

/**
 * Get the string representation of message bytes
 * @param[out] str The output string
 */
void Ecumsg::toString(string& str) const
{
    to_ascii(data_, length_, str);
}

/**
 * Set the message data bytes
 * @param[in] data Data bytes
 * @param[in] length Data length
 */
void Ecumsg::setData(const uint8_t* data, uint8_t length)
{
    length_ = length;
    memcpy(data_, data, length);
}

/**
 * Set header bytes
 * @param[in] header The header bytes
 */
void Ecumsg::setHeader(const uint8_t* header)
{
    memcpy(header_, header, sizeof(header_)); 
}

/**
 * Adds the header/checksum to ISO 9141 message
 */
void EcumsgISO9141::addHeaderAndChecksum()
{
    // Shift data on 3 bytes to accommodate the header
    memmove(&data_[HEADER_SIZE], &data_[0], length_);
    length_ += HEADER_SIZE;
    memcpy(&data_[0], header_, HEADER_SIZE);
    
    IsoAddChecksum(data_, length_);
}

/**
 * Adds the header/checksum to ISO 14230 message
 */
void EcumsgISO14230::addHeaderAndChecksum()
{
    // Shift data on 3 bytes to accommodate the header
    memmove(&data_[HEADER_SIZE], &data_[0], length_);
    uint8_t len = length_;
    length_ += HEADER_SIZE;
    memcpy(&data_[0], header_, HEADER_SIZE);
    
    // The length is in the 1st byte
    data_[0] = (data_[0] & 0xC0) | len;
    
    IsoAddChecksum(data_, length_);
}

/**
 * Adds the header/checksum to J1850 VPW message
 */
void EcumsgVPW::addHeaderAndChecksum()
{
    // Shift data on 3 bytes to accommodate the header
    memmove(&data_[HEADER_SIZE], &data_[0], length_);
    length_ += HEADER_SIZE;
    memcpy(&data_[0], header_, HEADER_SIZE);
    
    J1850AddChecksum(data_, length_);
}

/**
 * Adds the header/checksum to J1850 PWM message
 */
void EcumsgPWM::addHeaderAndChecksum()
{
    // Shift data on 3 bytes to accommodate the header
    memmove(&data_[HEADER_SIZE], &data_[0], length_);
    length_ += HEADER_SIZE;
    memcpy(&data_[0], header_, HEADER_SIZE);
    
    J1850AddChecksum(data_, length_);
}

/**
 * Strips the header/checksum from ISO 9141 message
 * @return true if header is valid, false otherwise
 */
bool EcumsgISO9141::stripHeaderAndChecksum()
{
    StripHeader(data_, length_);
    ISOStripChecksum(data_, length_);
    return true;
}

/**
 * Strips the header/checksum from ISO 14230 message
 * @return true if header is valid, false otherwise
 */
bool EcumsgISO14230::stripHeaderAndChecksum()
{
    StripHeader(data_, length_);
    ISOStripChecksum(data_, length_);
    return true;
}

/**
 * Strips the header/checksum from J1850 VPW message
 * @return true if header is valid, false otherwise
 */
bool EcumsgVPW::stripHeaderAndChecksum()
{
    StripHeader(data_, length_);
    J1850StripChecksum(data_, length_);
    return true;
}

/**
 * Strips the header/checksum from J1850 PWM message
 * @return true if header is valid, false otherwise
 */
bool EcumsgPWM::stripHeaderAndChecksum()
{
    StripHeader(data_, length_);
    J1850StripChecksum(data_, length_);
    return true;
}

