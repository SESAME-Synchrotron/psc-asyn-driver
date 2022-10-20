#ifndef __PSC_H__
#define __PSC_H__

#include <cstdint>
#include <cstdio>
#include <cstring>

#include <string>
#include <iostream>
#include <vector>
#include <algorithm>
#include <iterator>

#include <epicsExport.h>
#include <asynPortDriver.h>
#include <asynOctetSyncIO.h>
#include <iocsh.h>

using std::string;
using std::cout;
using std::endl;
using std::vector;
using std::find;

#pragma pack(2)

#define MAX_ADDRESSES        256
#define PACKET_LENGTH        10
#define PS_ADDRESS_SHIFT    14
#define ADDRESS_PRIORITY    0x2
#define ETHERNET_ENABLE     0x1000000
#define ETHERNET_DISABLE    0x0

typedef uint32_t u32;
typedef uint16_t u16;

typedef enum {
    COMMAND_READ  = 0x0001,
    COMMAND_WRITE = 0x0002
} command_t;

typedef struct
{
    u16 status;
    u16 command;
    u16 address;
    u32 data;
} packet_t;

class PSController : public asynPortDriver
{
public:
    PSController(const char* name, const char* asyn_name);
    asynStatus readInt32(asynUser* asyn, epicsInt32* value);
    asynStatus writeInt32(asynUser* asyn, epicsInt32 value);
    asynStatus readFloat64(asynUser* asyn, epicsFloat64* value);
    asynStatus writeFloat64(asynUser* asyn, epicsFloat64 value);

protected:
    int ps[6];

private:
    asynUser* device;
    asynStatus performIO(asynUser* asyn, u32* value, command_t command = COMMAND_READ);
    asynStatus setEthernetState(u32 state);
};

#endif
