#ifndef __PSC_H__
#define __PSC_H__

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <unistd.h>

#include <string>
#include <iostream>

#include <epicsExport.h>
#include <asynPortDriver.h>
#include <asynOctetSyncIO.h>
#include <iocsh.h>
#include <drvAsynIPPort.h>

using std::string;
using std::cout;
using std::endl;

#pragma pack(2)

#define LOOP_LIMIT 4000
#define MAX_ADDRESSES       256
#define PACKET_LENGTH   10
#define TCP_PACKET_LENGTH   9
#define PS_ADDRESS_SHIFT    14
#define ETHERNET_ENABLE     0x1000000
#define ETHERNET_DISABLE    0x0

#define ADDRESS_PRIORITY               0x02
#define ADDRESS_SYSTEM_OPERATING_STATE 0x04
#define ADDRESS_DATA_TRANSFER_INIT     0x23
#define ADDRESS_DATA_TRANSFER          0x24
#define ADDRESS_DATA_BLOCK_DESTINATION 0x25
#define ADDRESS_DATA_SOURCE            0x26

#define MODE_MONITOR            0
#define MODE_DEVICE_OFF         1
#define MODE_DEVICE_ON          2
#define MODE_ADC_CAL            3
#define MODE_DEVICE_LOCKED      4
#define MODE_TRANSIENT          5
#define MODE_DEVICE_OFF_LOCKED  6
#define MODE_DOWNLOAD_DATA      8
#define MODE_SAVE_DATA          9
#define MODE_MODIFY_DATA        11

const int modes_count = 12;
const int states_count = 17;

static std::string modes[modes_count] = {
    "MONITOR",
    "DEVICE_OFF",
    "DEVICE_ON",
    "ADC_CAL",
    "DEVICE_LOCKED",
    "TRANSIENT",
    "DEVICE_OFF_LOCKED",
    "N/A",
    "DOWNLOAD_DATA",
    "SAVE_DATA",
    "N/A",
    "MODIFY_DATA"
};

static std::string state_names[states_count] = {
    "STATE_INIT",
    "STATE_CHECK_DOWNLOAD_MODE",
    "STATE_DATA_TRANSFER",
    "STATE_VERIFY_DATA",
    "STATE_EXIT_DOWNLOAD_MODE",
    "STATE_COPY_TO_FLASH",
    "STATE_WAIT_SAVE",
    "STATE_SAVE_FINISHED",
    "STATE_END_TRANSFER",
    "STATE_WAIT_DEVICE_OFF",
    "STATE_DEVICE_OFF",

    "STATE_CHECK_DATA_TRANSFER",
    "STATE_ENTER_TRANSIENT",
    "STATE_EXIT_TRANSIENT",
    "STATE_REQUEST_DATA",

    "STATE_STOP_LOGGER",

    "STATE_ERROR"
};

#define PSC_OK		0
#define PSC_SOCKET	1
#define PSC_READ	2
#define PSC_WRITE	3
#define PSC_CONNECT	4

#define LOAD_ASYN_ADDRESS \
    int address; \
    this->asyn = asyn; \
    getAddress(asyn, &address); \

#define ZERO(buffer) memset(buffer, 0, sizeof(buffer));

#define LOG(message, ...) \
        printf("%s:%d: " message "\n", __func__, __LINE__, ##__VA_ARGS__); \


typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;

static const int COMMAND_READ  = 0x0001;
static const int COMMAND_WRITE = 0x0002;

typedef struct
{
    u16 status;
    u16 command;
    u16 address;
    u32 data;
} packet_t;

typedef enum
{
    STATE_INIT = 0,

    // Write block states.
    STATE_CHECK_DOWNLOAD_MODE,
    STATE_DATA_TRANSFER,
    STATE_VERIFY_DATA,
    STATE_EXIT_DOWNLOAD_MODE,
    STATE_COPY_TO_FLASH,
    STATE_WAIT_SAVE,
    STATE_SAVE_FINISHED,
    STATE_END_TRANSFER,
    STATE_WAIT_DEVICE_OFF,
    STATE_DEVICE_OFF,

    // Read block states.
    STATE_CHECK_DATA_TRANSFER,
    STATE_ENTER_TRANSIENT,
    STATE_EXIT_TRANSIENT,
    STATE_REQUEST_DATA,

    STATE_STOP_LOGGER,

    STATE_ERROR
} state_t;

typedef union {
    u32   i_value;
    float f_value;
} raw32;

class PSController : public asynPortDriver
{
public:
    PSController(const char* name, const char* ip_port);
    ~PSController();

    asynStatus readInt32(asynUser* asyn, epicsInt32* value);
    asynStatus writeInt32(asynUser* asyn, epicsInt32 value);
    asynStatus readFloat64(asynUser* asyn, epicsFloat64* value);
    asynStatus writeFloat64(asynUser* asyn, epicsFloat64 value);
    asynStatus readUInt32Digital(asynUser *asyn, epicsUInt32 *value, 
                                 epicsUInt32 mask);
    asynStatus readInt32Array(asynUser *pasynUser, epicsInt32 *value, 
                              size_t nElements, size_t *nIn);
    asynStatus writeInt32Array(asynUser *pasynUser, epicsInt32 *value, 
                               size_t nElements);
    asynStatus readFloat32Array(asynUser *pasynUser, epicsFloat32 *value, 
                              size_t nElements, size_t *nIn);

protected:
    int ps[11];

private:
    asynUser* registerIO;
    asynUser* asyn;
    asynStatus writeRegister(u16 address, u32 value);
    asynStatus readRegister(u16 address, u32* value);

    template <typename T>
    asynStatus readArray(asynUser *asyn, T* value, size_t nElements,
                         size_t *nIn, bool is_float = false);

    asynStatus doRegisterIO(u16 address, int command, u32* value);
    asynStatus setEthernetState(u32 state);

    std::string ip_port;
};

#endif
