/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2016 ObdDiag.Net. All rights reserved.
 *
 */

#include <lstring.h>
#include <algorithms.h>
#include <adaptertypes.h>
#include <romapi_15xx.h>

using namespace std;
using namespace util;

/**
 * IAP API call to get CPU UID
 * @parameter[in] uid UID 3 x uint32_t array to filled in
 */
static void IAPReadUID(uint32_t uid[3])
{
    unsigned int command[5], result[4];

    command[0] = IAP_READ_UID_CMD;
    ((IAP_ENTRY_T) IAP_ENTRY_LOCATION)(command , result);

    uid[0] = result[1];
    uid[1] = result[2];
    uid[2] = result[3];
}

/**
 * Format the UID as string
 * @paramer[in] uid UID 3 x uint32_t array
 * @return UID as a string
 */
static string UIDToString(uint32_t uid[3])
{
    string str(30);

    for (int j = 0; j < 3; j++) {
        NumericType value(uid[j]);

        for (int i = 3; i >= 0; i--) {
            str += to_ascii(value.bvalue[i] >> 4);
            str += to_ascii(value.bvalue[i] & 0x0F);
        }
        str += '-';
    }
    str.resize(str.length() - 1);
    return str;
}

/**
 * Display the LPC15XX CPU UID 
 */
void AdptReadSerialNum()
{
    uint32_t uid[3];

    IAPReadUID(uid);
    AdptSendReply(UIDToString(uid));
}

/**
 * Defines the low power mode
 */
void AdptPowerModeConfigure()
{
    const uint32_t SLEEP = 0;

	LPC_PWRD_API->power_mode_configure(SLEEP, 0x0);
}
