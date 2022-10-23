#include "PSController.h"

PSController::PSController(const char* name, const char* asyn_name)
    : asynPortDriver(
            name,
            MAX_ADDRESSES,
            asynInt32Mask | asynFloat64Mask | asynDrvUserMask,
            asynInt32Mask | asynFloat64Mask,
            ASYN_MULTIDEVICE | ASYN_CANBLOCK,
            1, 0, 0)
{
    int status = pasynOctetSyncIO->connect(asyn_name, 1, &this->device, NULL);
    if(status != asynSuccess) {
        cout << "Could not connect to port " << asyn_name << endl;
        return;
    }

    createParam("i_1", asynParamInt32,   &ps[0]);
    createParam("i_2", asynParamInt32,   &ps[1]);
    createParam("i_3", asynParamInt32,   &ps[2]);
    createParam("f_1", asynParamFloat64, &ps[3]);
    createParam("f_2", asynParamFloat64, &ps[4]);
    createParam("f_3", asynParamFloat64, &ps[5]);
}

asynStatus PSController::readInt32(asynUser* asyn, epicsInt32* value)
{
    asynStatus status = performIO(asyn, (u32*) value);
    return status;
}

asynStatus PSController::readFloat64(asynUser* asyn, epicsFloat64* value)
{
    u32   raw;
    float temp;

    asynStatus status = performIO(asyn, &raw);
    memcpy(&temp, &raw, sizeof(u32));
    *value = temp;
    return status;
}

asynStatus PSController::writeInt32(asynUser* asyn, epicsInt32 value)
{
    u32 temp = (u32) value;

    setEthernetState(ETHERNET_ENABLE);
    asynStatus status = performIO(asyn, &temp, COMMAND_WRITE);
    setEthernetState(ETHERNET_DISABLE);
    return status;
}

asynStatus PSController::writeFloat64(asynUser* asyn, epicsFloat64 value)
{
    float temp = (float) value;

    setEthernetState(ETHERNET_ENABLE);
    asynStatus status = performIO(asyn, reinterpret_cast<u32*>(&temp), COMMAND_WRITE);
    setEthernetState(ETHERNET_DISABLE);
    return status;
}

asynStatus PSController::performIO(asynUser* asyn, u32* value, command_t command)
{
    int address;
    int status;
    int reason;
    int function = asyn->reason;
    size_t tx_bytes;
    size_t rx_bytes;
    const char* parameter_name;
    packet_t tx;
    packet_t rx; 
    char tx_array[PACKET_LENGTH];
    char rx_array[PACKET_LENGTH];

    getParamName(function, &parameter_name);
    getAddress(asyn, &address);
    u16 ps = parameter_name[ strlen(parameter_name) - 1 ] - 0x30;

    tx = { 
        .status  = 0x0, 
        .command = (u16) (command),
        .address = (u16) (address | (ps << PS_ADDRESS_SHIFT)), 
        .data    = command == COMMAND_READ ? 0 : *value
    };
    memcpy(tx_array, &tx, sizeof(tx_array));
    status = pasynOctetSyncIO->writeRead(this->device, 
                                         tx_array, PACKET_LENGTH, 
                                         rx_array, PACKET_LENGTH, 1, 
                                         &tx_bytes, &rx_bytes, &reason);
    if(status != asynSuccess || tx_bytes != PACKET_LENGTH || rx_bytes != PACKET_LENGTH) {
        printf("Status: %d | Reason: %d | Bytes: %lu - %lu\n", status, reason, tx_bytes, rx_bytes);
        return asynError;
    }

    memcpy(&rx, rx_array, sizeof(rx_array));
    *value = rx.data;
    return asynSuccess;
}

asynStatus PSController::setEthernetState(u32 state)
{
    int status;
    int reason;
    size_t tx_bytes;
    size_t rx_bytes;
    char tx_array[PACKET_LENGTH];
    char rx_array[PACKET_LENGTH];
    packet_t tx;

    tx = {
        .status  = 0x0,
        .command = COMMAND_WRITE,
        .address = ADDRESS_PRIORITY | (3 << PS_ADDRESS_SHIFT),
        .data    = state,
    };
    memcpy(tx_array, &tx, sizeof(tx_array));
    status = pasynOctetSyncIO->writeRead(this->device, 
                                         tx_array, PACKET_LENGTH, 
                                         rx_array, PACKET_LENGTH, 1, 
                                         &tx_bytes, &rx_bytes, &reason);
    if(status != asynSuccess || tx_bytes != PACKET_LENGTH || rx_bytes != PACKET_LENGTH) {
        printf("Ethernet faile. Status: %d | Reason: %d | Bytes: %lu - %lu\n", status, reason, tx_bytes, rx_bytes);
        return asynError;
    }

    return asynSuccess;
}
