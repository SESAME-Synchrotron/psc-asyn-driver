#include "PSController.h"

PSController::PSController(const char* name, const char* ip_port)
    : asynPortDriver(
            name,
            MAX_ADDRESSES,
            asynInt32Mask | asynFloat64Mask | asynUInt32DigitalMask | asynInt32ArrayMask | asynDrvUserMask,
            asynInt32Mask | asynFloat64Mask | asynUInt32DigitalMask | asynInt32ArrayMask,
            ASYN_MULTIDEVICE | ASYN_CANBLOCK,
            1, 0, 0)
{
    int status;
    this->ip_port = ip_port;
    std::string udp_host = std::string(ip_port) + " UDP";
    std::string tcp_host = std::string(ip_port) + " TCP";

    status = drvAsynIPPortConfigure("UDP", udp_host.c_str(), 1, 0, 0);
    if (status != asynSuccess) {
        cout << "Could not create port " << udp_host << endl;
        return;
    }

    status = pasynOctetSyncIO->connect("UDP", 1, &this->registerIO, NULL);
    if(status != asynSuccess) {
        cout << "Could not connect to port " << udp_host << endl;
        return;
    }

//    status = drvAsynIPPortConfigure("TCP", tcp_host.c_str(), 1, 0, 0);
//    if (status != asynSuccess) {
//        cout << "Could not create port " << tcp_host << endl;
//        return;
//    }
//
//    status = pasynOctetSyncIO->connect("TCP", 1, &this->blockIO, NULL);
//    if(status != asynSuccess) {
//        cout << "Could not connect to TCP port: " << ip_port << endl;
//        return;
//    }

    createParam("i_1", asynParamInt32,   &ps[0]);
    createParam("i_2", asynParamInt32,   &ps[1]);
    createParam("i_3", asynParamInt32,   &ps[2]);
    createParam("f_1", asynParamFloat64, &ps[3]);
    createParam("f_2", asynParamFloat64, &ps[4]);
    createParam("f_3", asynParamFloat64, &ps[5]);

	createParam("b_1", asynParamUInt32Digital, &ps[6]);
	createParam("b_2", asynParamUInt32Digital, &ps[7]);
	createParam("b_3", asynParamUInt32Digital, &ps[8]);

    createParam("parameters_1", asynParamInt32Array, &ps[9]);
    createParam("write_parameters_1", asynParamInt32Array, &ps[10]);

    setEthernetState(ETHERNET_ENABLE);
}

asynStatus PSController::readInt32Array(asynUser *asyn, epicsInt32 *value,
                                        size_t nElements, size_t *nIn)
{
    LOAD_ASYN_ADDRESS;

	asynStatus status;
	u32 mode;
    u32 i = 0;
    u32 zero = 0;
    u32 offset = (u32) address;

    int counter = 0;

    setEthernetState(ETHERNET_ENABLE);

    // Write sector and initialize data transfer.
    printf("writing sector %d\n", address);
    status = doRegisterIO(ADDRESS_DATA_TRANSFER_INIT, COMMAND_WRITE, &offset);
    if (status != asynSuccess) {
        printf("%s:%d: unable to init data transfer\n", __func__, __LINE__);
        return asynError;
    }
    LOG("Wrote sector %d", offset);

    // Enter transient mode
    do {
        status = doRegisterIO(ADDRESS_SYSTEM_OPERATING_STATE, COMMAND_READ, &mode);
        LOG("enter transient from: %s", modes[mode]);
        if (status != asynSuccess) {
            printf("%s:%d: unable to read state\n", __func__, __LINE__);
            return asynError;
        }
        usleep(1000);
    } while (mode != MODE_TRANSIENT && mode != MODE_MODIFY_DATA && 
             counter++ < LOOP_LIMIT);
    if (counter >= LOOP_LIMIT) {
        printf("%s:%d: enter transient mode timeout.\n", __func__, __LINE__);
        return asynError;
    }
    LOG("entered mode %s", modes[mode]);

    // Exit transient mode
    counter = 0;
    if (mode == MODE_TRANSIENT) {
        do {
            status = doRegisterIO(ADDRESS_SYSTEM_OPERATING_STATE, 
                                  COMMAND_READ, &mode);
            printf("exit transient\n");
            if (status != asynSuccess) {
                printf("%s:%d: unable to read state\n", __func__, __LINE__);
                return asynError;
            }
            usleep(10000);
        } while (mode == MODE_TRANSIENT && counter++ < LOOP_LIMIT);
        if (counter >= LOOP_LIMIT) {
            printf("%s:%d: exit transient mode timeout. mode = %s\n", 
                    __func__, __LINE__, modes[mode]);
            return asynError;
        }
    }

    LOG("uploading data ...");
    // Iterate over data
    mode = 0;
    for(i = 0; i < nElements; i++) {

        // Write index
        status = doRegisterIO(ADDRESS_DATA_SOURCE, COMMAND_WRITE, (u32*) &i);
        if (status != asynSuccess) {
            printf("%s:%d: unable to set data index\n", __func__, __LINE__);
            return asynError;
        }

        // Wait for index written
        counter = 0;
        do {
            status = doRegisterIO(ADDRESS_DATA_SOURCE, COMMAND_READ, &mode);
            printf("index write: %u - %u\n", i, mode);
            if (status != asynSuccess) {
                printf("%s:%d: unable to read data index\n", __func__, __LINE__);
                return asynError;
            }
            usleep(1000);
        } while (mode != i && counter++ < LOOP_LIMIT);
        if (counter > LOOP_LIMIT) {
            printf("%s:%d: index readback timeout.\n", __func__, __LINE__);
            return asynError;
        }

        // Read data
        status = doRegisterIO(ADDRESS_DATA_TRANSFER, COMMAND_READ, (u32*) &value[i]);
        if (status != asynSuccess) {
            printf("%s:%d: unable to read data[%d]\n", __func__, __LINE__, i);
            return asynError;
        }
    }

    // End data transfer.
    status = doRegisterIO(ADDRESS_DATA_TRANSFER_INIT, COMMAND_WRITE, &zero);
    if (status != asynSuccess) {
        printf("%s:%d: unable to end data transfer\n", __func__, __LINE__);
        return status;
    }

    // Wait for reset or device off
    counter = 0;
    do {
        status = doRegisterIO(ADDRESS_SYSTEM_OPERATING_STATE, COMMAND_READ, &mode);
        printf("mode off: mode[%d] = %s\n", mode, modes[mode]);
        if (status != asynSuccess) {
            printf("%s:%d: unable to read state\n", __func__, __LINE__);
            return status;
        }
        usleep(1000);
    } while (mode != MODE_MONITOR && mode != MODE_DEVICE_OFF && counter++ < LOOP_LIMIT);
    if (counter >= LOOP_LIMIT) {
        LOG("wait for device off timeout");
        return asynError;
    }

    return asynSuccess;
}

asynStatus PSController::writeInt32Array(asynUser *asyn, epicsInt32 *value,
                                        size_t nElements)
{
    LOAD_ASYN_ADDRESS;

	u32 mode;
	asynStatus status;
	int counter = 0;

    u32 offset = address;
    u32 length = nElements;
    u32 init   = (length << 8)|(offset & 0xff);
    u32 zero   = 0;

    LOG("enabling ethernet ... \n");
    setEthernetState(ETHERNET_ENABLE);
    LOG("ethernet enabled.\n");

    // Write sector and initialize data transfer.
    status = doRegisterIO(ADDRESS_DATA_TRANSFER_INIT, COMMAND_WRITE, &init);
    if (status != asynSuccess) {
        printf("%s:%d: could not initialize data transfer\n", 
                __func__, __LINE__);
        return asynError;
    }
    LOG("Wrote sector %d", offset);

    // Enter download mode.
    do {
        status = doRegisterIO(ADDRESS_SYSTEM_OPERATING_STATE, 
                              COMMAND_READ, &mode);
        if (status != asynSuccess) {
            printf("%s:%d: could not read system state\n", __func__, __LINE__);
            return asynError;
        }
        usleep(1000);
    } while (mode != MODE_DOWNLOAD_DATA && counter++ < LOOP_LIMIT);
    if (counter >= LOOP_LIMIT) {
        printf("%s:%d: could not enter download mode. mode = %s\n",
                __func__, __LINE__, modes[mode]);
        return asynError;
    }
    LOG("entered download mode.");

    // Write data.
    for(size_t i = 0; i < nElements; i++) {

        // Write word
        status = doRegisterIO(ADDRESS_DATA_TRANSFER, 
                              COMMAND_WRITE, 
                              (u32*) &value[i]);
        if (status != asynSuccess) {
            printf("%s:%d: could not write data at index %zu\n", 
                    __func__, __LINE__, i);
            return asynError;
        }

        // Wait for word to be written.
        counter = 0;
        do {
            status = doRegisterIO(ADDRESS_DATA_TRANSFER, COMMAND_READ, &mode);
            if (status != asynSuccess) {
                printf("%s:%d: could not read data at index %zu\n", 
                        __func__, __LINE__, i);
                return asynError;
            }
        } while (mode != (u32) value[i] && counter++ < LOOP_LIMIT);
        if (counter >= LOOP_LIMIT) {
            printf("%s:%d: could not readback data at index %zu\n",
                    __func__, __LINE__, i);
            return asynError;
        }

        LOG("wrote index: %zu", i);
    }

    // Exit download mode.
    do {
        status = doRegisterIO(ADDRESS_SYSTEM_OPERATING_STATE, COMMAND_READ, 
                              &mode);
        if (status != asynSuccess) {
            printf("%s:%d: could not read system state\n", __func__, __LINE__);
            return asynError;
        }
    } while (mode == MODE_DOWNLOAD_DATA && counter++ < LOOP_LIMIT);
    if (counter >= LOOP_LIMIT) {
        printf("%s:%d: could not exit download mode. mode = %s", 
                __func__, __LINE__, modes[mode]);
        return asynError;
    }
    LOG("exited download mode.");

    // Copy to flash.
    status = doRegisterIO(ADDRESS_DATA_BLOCK_DESTINATION, COMMAND_WRITE, &init);
    if (status != asynSuccess) {
        printf("%s:%d: could not copy data to flash.\n", __func__, __LINE__);
        return asynError;
    }
    LOG("copy to flash started.");

    // Wait to enter save data mode.
    counter = 0;
    do {
        status = doRegisterIO(ADDRESS_SYSTEM_OPERATING_STATE, COMMAND_READ, 
                              &mode);
        if (status != asynSuccess) {
            printf("%s:%d: could not read system state to copy to flash.\n", 
                    __func__, __LINE__);
            return asynError;
        }
    } while (mode != MODE_SAVE_DATA && counter++ < LOOP_LIMIT);
    if (counter >= LOOP_LIMIT) {
        printf("%s:%d: could not enter save data mode.\n", __func__, __LINE__);
        return asynError;
    }
    LOG("entered save data mode.");

    // Wait to exit save data mode
    counter = 0;
    do {
        status = doRegisterIO(ADDRESS_SYSTEM_OPERATING_STATE, COMMAND_READ,
                              &mode);
        if (status != asynSuccess) {
            printf("%s:%d: could not read system state to copy to flash.\n",
                    __func__, __LINE__);
            return asynError;
        }
        usleep(1000);
    } while (mode == MODE_SAVE_DATA && counter++ < LOOP_LIMIT);
    LOG("mode: %s | counter: %d", modes[mode], counter);
    if (counter >= LOOP_LIMIT) {
        printf("%s:%d: could not copy data to flash.\n", __func__, __LINE__);
        return asynError;
    }
    LOG("exited save data mode.");
 
    // End transfer.
    status = doRegisterIO(ADDRESS_DATA_TRANSFER_INIT, COMMAND_WRITE, &zero);
    if (status != asynSuccess) {
        printf("%s:%d: could not end data transfer.\n", __func__, __LINE__);
        return asynError;
    }
    LOG("data transfer ended.");

    // Wait for device off.
    counter = 0;
    do {
        status = doRegisterIO(ADDRESS_SYSTEM_OPERATING_STATE, COMMAND_READ, 
                              &mode);
        if (status != asynSuccess) {
            printf("%s:%d: could not read system state to copy to flash.\n", 
                    __func__, __LINE__);
            return asynError;
        }
        LOG("mode = %s", modes[mode]);
        usleep(1000);
    } while (mode != MODE_DEVICE_OFF && counter++ < LOOP_LIMIT);
    if (counter >= LOOP_LIMIT) {
        printf("%s:%d: could not wait for device off..\n", __func__, __LINE__);
        return asynError;
    }
    LOG("device is off.");

	return asynSuccess;
}

asynStatus PSController::readUInt32Digital(asynUser* asyn, epicsUInt32 *value, epicsUInt32 mask)
{
    LOAD_ASYN_ADDRESS;

    asynStatus status = doRegisterIO(address, COMMAND_READ, (u32*) value);
	return status;
}

asynStatus PSController::readInt32(asynUser* asyn, epicsInt32* value)
{
    LOAD_ASYN_ADDRESS;

    asynStatus status = doRegisterIO(address, COMMAND_READ, (u32*) value);
    return status;
}

asynStatus PSController::readFloat64(asynUser* asyn, epicsFloat64* value)
{
    u32   raw;
    float temp;
   
    LOAD_ASYN_ADDRESS;
    
    asynStatus status = doRegisterIO(address, COMMAND_READ, &raw);
    memcpy(&temp, &raw, sizeof(u32));
    *value = temp;

    return status;
}

asynStatus PSController::writeInt32(asynUser* asyn, epicsInt32 value)
{
    LOAD_ASYN_ADDRESS;
    u32 temp = (u32) value;

    setEthernetState(ETHERNET_ENABLE);
    asynStatus status = doRegisterIO(address, COMMAND_WRITE, &temp);
    return status;
}

asynStatus PSController::writeFloat64(asynUser* asyn, epicsFloat64 value)
{
    LOAD_ASYN_ADDRESS;
    float temp = (float) value;

    setEthernetState(ETHERNET_ENABLE);
    asynStatus status = doRegisterIO(address, COMMAND_WRITE, reinterpret_cast<u32*>(&temp));

    std::cout << address << " - " << status << std::endl;

    return status;
}

inline asynStatus PSController::writeRegister(u16 address, u32 value)
{
    return doRegisterIO(address, COMMAND_WRITE, &value);
}

inline asynStatus PSController::readRegister(u16 address, u32* value)
{
    return doRegisterIO(address, COMMAND_READ, value);
}

asynStatus PSController::doRegisterIO(u16 address, int command, u32* value)
{
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
    u16 ps = parameter_name[ strlen(parameter_name) - 1 ] - 0x30;

    tx = { 
        .status  = 0x0, 
        .command = (u16) (command),
        .address = (u16) (address | (ps << PS_ADDRESS_SHIFT)), 
        .data    = command == COMMAND_READ ? 0 : *value
    };
    memcpy(tx_array, &tx, sizeof(tx_array));
    status = pasynOctetSyncIO->writeRead(this->registerIO, 
                                         tx_array, PACKET_LENGTH, 
                                         rx_array, PACKET_LENGTH, 1, 
                                         &tx_bytes, &rx_bytes, &reason);
    if(status != asynSuccess || tx_bytes != PACKET_LENGTH || rx_bytes != PACKET_LENGTH) {
        printf("Status: %d | Reason: %d | Bytes: %lu - %lu\n", status, reason, tx_bytes, rx_bytes);
        return asynError;
    }

    if (command == COMMAND_READ) {
        memcpy(&rx, rx_array, sizeof(rx_array));
        *value = rx.data;
    }
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
        .address = ADDRESS_PRIORITY | (1 << PS_ADDRESS_SHIFT),
        .data    = state,
    };
    memcpy(tx_array, &tx, sizeof(tx_array));
    status = pasynOctetSyncIO->writeRead(this->registerIO, 
                                         tx_array, PACKET_LENGTH, 
                                         rx_array, PACKET_LENGTH, 1, 
                                         &tx_bytes, &rx_bytes, &reason);
    if(status != asynSuccess || tx_bytes != PACKET_LENGTH || rx_bytes != PACKET_LENGTH) {
        printf("Ethernet faile. Status: %d | Reason: %d | Bytes: %lu - %lu\n", status, reason, tx_bytes, rx_bytes);
        return asynError;
    }

    return asynSuccess;
}
