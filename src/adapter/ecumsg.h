/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2016 ObdDiag.Net. All rights reserved.
 *
 */

#ifndef __ECUMSG_H__ 
#define __ECUMSG_H__

#include <cstdint>
#include <lstring.h>

using namespace std;

class Ecumsg {
public:
	const static uint8_t ISO9141  = 1;
	const static uint8_t ISO14230 = 2;
	const static uint8_t PWM      = 3;
	const static uint8_t VPW      = 4;
	
	static Ecumsg* instance(uint8_t type);
    virtual ~Ecumsg();
	const uint8_t* data() const { return data_; }
    uint8_t* data() { return data_; }
	uint8_t type() const { return type_; }
	uint8_t length() const { return length_; }
    void length(uint8_t length) { length_ = length; }
	virtual void addHeaderAndChecksum() = 0;
	virtual bool stripHeaderAndChecksum() = 0;
	Ecumsg& operator+=(uint8_t byte) { data_[length_++] = byte; return *this; }
	void setData(const uint8_t* data, uint8_t length);
	void toString(util::string& str) const;
protected:
	Ecumsg(uint8_t type, uint32_t size);
	void setHeader(const uint8_t* data);
	
	uint8_t* data_;
	uint8_t type_;
    uint8_t length_;
	uint8_t size_;
	uint8_t header_[3];
};

#endif //__ECUMSG_H__
