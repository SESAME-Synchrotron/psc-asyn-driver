#include "PSController.h"

PSController::PSController(const char* name, const char* ip_port)
    : asynPortDriver(
            name,
            MAX_ADDRESSES,
            asynInt32Mask      | asynFloat64Mask      | asynUInt32DigitalMask |
            asynInt32ArrayMask | asynFloat32ArrayMask | asynDrvUserMask,
            asynInt32Mask      | asynFloat64Mask      | asynUInt32DigitalMask |
            asynInt32ArrayMask | asynFloat32ArrayMask,
            ASYN_MULTIDEVICE | ASYN_CANBLOCK,
            1, 0, 0)
{
    int status;
    this->ip_port = ip_port;
    std::string udp_host = std::string(ip_port) + " UDP";

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

    createParam("i32",      asynParamInt32,         &p_i32);
    createParam("f32",      asynParamFloat64,       &p_f32);
    createParam("b16",      asynParamUInt32Digital, &p_b16);
    createParam("i32array", asynParamInt32Array,    &p_i32array);
    createParam("f32array", asynParamFloat32Array,  &p_f32array);
}

PSController::~PSController()
{
    pasynOctetSyncIO->disconnect(this->registerIO);
}

asynStatus PSController::readInt32Array(asynUser *asyn, epicsInt32 *value,
                                        size_t nElements, size_t *nIn)
{
    return readArray(asyn, value, nElements, nIn);
}

asynStatus PSController::readFloat32Array(asynUser *asyn, epicsFloat32 *value,
                                        size_t nElements, size_t *nIn)
{
    return readArray(asyn, value, nElements, nIn);
}

template <typename T>
asynStatus PSController::readArray(asynUser *asyn, T* value, size_t nElements,
                                   size_t *nIn)
{
	asynStatus status;

    parameter_t info = *(parameter_t*) asyn->drvUser;
	u32 mode;
    u32 i = 0;
    u32 zero = 0;
    u32 sector = (u32) info.address;

    int counter = 0;

    status = writeRegister(ADDRESS_PRIORITY, ETHERNET_ENABLE);
    if (status != asynSuccess) {
        LOG("ethernet priority error");
        return asynError;
    }

    // Resetting data transfer.
    LOG("resetting block transfer");
    status = writeRegister(ADDRESS_DATA_TRANSFER_INIT, zero);
    if (status != asynSuccess) {
        LOG("unable to reset block transfer");
        return status;
    }

    counter = 0;
    do {
        status = readRegister(ADDRESS_SYSTEM_OPERATING_STATE, &mode);
        if (status != asynSuccess) {
            LOG("unable to read state");
            return status;
        }
        usleep(1000);
    } while (mode != MODE_MONITOR    && 
             mode != MODE_DEVICE_OFF && 
             counter++ < LOOP_LIMIT);
    if (counter >= LOOP_LIMIT) {
        LOG("wait for device off timeout");
        return asynError;
    }
    LOG("block transfer reset done");

    // Write sector and initialize data transfer.
    status = writeRegister(ADDRESS_DATA_TRANSFER_INIT, sector);
    if (status != asynSuccess) {
        LOG("unable to start data transfer");
        return asynError;
    }
    LOG("Wrote sector %d", sector);

    // Enter transient mode
    LOG("entering transient mode");
    do {
        status = readRegister(ADDRESS_SYSTEM_OPERATING_STATE, &mode);
        if (status != asynSuccess) {
            LOG("unable to read state");
            return asynError;
        }
        usleep(1000);
    } while (mode != MODE_TRANSIENT   &&
             mode != MODE_MODIFY_DATA &&
             counter++ < LOOP_LIMIT);
    if (counter >= LOOP_LIMIT) {
        LOG("transient mode timeout.");
        return asynError;
    }
    LOG("entered mode %s", modes[mode].c_str());

    // Exit transient mode
    counter = 0;
    if (mode == MODE_TRANSIENT) {
        LOG("exiting transient mode");
        do {
            status = readRegister(ADDRESS_SYSTEM_OPERATING_STATE, &mode);
            if (status != asynSuccess) {
                LOG("unable to read state");
                return asynError;
            }
            usleep(10000);
        } while (mode == MODE_TRANSIENT && counter++ < LOOP_LIMIT);
        if (counter >= LOOP_LIMIT) {
            LOG("exit transient mode timeout, mode = %s", modes[mode].c_str());
            return asynError;
        }
        LOG("exited transient mode to %s", modes[mode].c_str());
    }

    LOG("uploading data ...");
    // Iterate over data
    mode = 0;
    for(i = 0; i < nElements; i++) {

        // Write index
        status = writeRegister(ADDRESS_DATA_SOURCE, i + info.offset);
        if (status != asynSuccess) {
            LOG("unable to write index %u sector %u", i, info.address);
            return asynError;
        }

        // Wait for index written
        counter = 0;
        do {
            status = readRegister(ADDRESS_DATA_SOURCE, &mode);
            if (status != asynSuccess) {
                LOG("unable to check index %u sector %u", i, info.address);
                return asynError;
            }
            usleep(1000);
        } while (mode != i && counter++ < LOOP_LIMIT);
        if (counter > LOOP_LIMIT) {
            LOG("index readback timeout");
            return asynError;
        }

        // Read data
        u32 temp;
        status = readRegister(ADDRESS_DATA_TRANSFER, &temp);
        if (status != asynSuccess) {
            LOG("unable to read data at %u", i);
            return asynError;
        }

        memcpy(&value[i], &temp, sizeof(u32));
    }
    LOG("upload completed.");

    // End data transfer.
    status = writeRegister(ADDRESS_DATA_TRANSFER_INIT, zero);
    if (status != asynSuccess) {
        printf("%s:%d: unable to end data transfer\n", __func__, __LINE__);
        return status;
    }
    LOG("data transfer ended, waiting for device off/reset");

    // Wait for reset or device off
    counter = 0;
    do {
        status = readRegister(ADDRESS_SYSTEM_OPERATING_STATE, &mode);
        if (status != asynSuccess) {
            LOG("unable to read state");
            return status;
        }
        usleep(1000);
    } while (mode != MODE_MONITOR &&
             mode != MODE_DEVICE_OFF &&
             counter++ < LOOP_LIMIT);
    if (counter >= LOOP_LIMIT) {
        LOG("wait for device off timeout");
        return asynError;
    }
    LOG("device off, block transfer completed");
    *nIn = nElements;

    return asynSuccess;
}

asynStatus PSController::writeInt32Array(asynUser *asyn, epicsInt32 *value,
                                        size_t nElements)
{
    return writeArray(asyn, value, nElements);
}

asynStatus PSController::writeFloat32Array(asynUser *asyn, epicsFloat32 *value,
                                        size_t nElements)
{
    return writeArray(asyn, value, nElements);
}

template <typename T>
asynStatus PSController::writeArray(asynUser *asyn, T* value, size_t nElements)
{
	asynStatus status;
	int counter = 0;

    parameter_t info = *(parameter_t*)(asyn->drvUser);
	u32 mode;
    u32 sector = info.address;
    u32 length = nElements;
    u32 init   = (length << 8)|(sector & 0xff);
    u32 zero   = 0;

    status = writeRegister(ADDRESS_PRIORITY, ETHERNET_ENABLE);
    if (status != asynSuccess) {
        LOG("ethernet priority error");
        return asynError;
    }

    // Resetting data transfer.
    LOG("resetting block transfer");
    status = writeRegister(ADDRESS_DATA_TRANSFER_INIT, zero);
    if (status != asynSuccess) {
        LOG("unable to reset block transfer");
        return status;
    }

    counter = 0;
    do {
        status = readRegister(ADDRESS_SYSTEM_OPERATING_STATE, &mode);
        if (status != asynSuccess) {
            LOG("unable to read state");
            return status;
        }
        usleep(1000);
    } while (mode != MODE_MONITOR    &&
             mode != MODE_DEVICE_OFF &&
             counter++ < LOOP_LIMIT);
    if (counter >= LOOP_LIMIT) {
        LOG("wait for device off timeout");
        return asynError;
    }
    LOG("block transfer reset done");

    // Write sector and initialize data transfer.
    status = writeRegister(ADDRESS_DATA_TRANSFER_INIT, init);
    if (status != asynSuccess) {
        LOG("could not initialize data transfer for sector %d.", sector);
        return asynError;
    }
    LOG("Wrote sector %d", sector);

    // Enter download mode.
    do {
        status = readRegister(ADDRESS_SYSTEM_OPERATING_STATE, &mode);
        if (status != asynSuccess) {
            LOG("could not read system state");
            return asynError;
        }
        usleep(1000);
    } while (mode != MODE_DOWNLOAD_DATA && counter++ < LOOP_LIMIT);
    if (counter >= LOOP_LIMIT) {
        LOG("could not enter download mode, mode = %s",
                mode >= 0 && mode <= 11 ? modes[mode].c_str() : "N/A");
        return asynError;
    }
    LOG("entered download mode.");

    // Write data.
    size_t i = 0;
    for(i = 0; i < nElements; i++) {

        // Write word
        status = writeRegister(ADDRESS_DATA_TRANSFER, *(u32*) &value[i]);
        if (status != asynSuccess) {
            LOG("could not write data at %zu", i);
            return asynError;
        }

        // Wait for word to be written.
        counter = 0;
        do {
            status = readRegister(ADDRESS_DATA_TRANSFER, &mode);
            if (status != asynSuccess) {
                LOG("could not read data at %zu", i);
                return asynError;
            }
        } while (mode != *(u32*) &value[i] && counter++ < LOOP_LIMIT);
        if (counter >= LOOP_LIMIT) {
            LOG("could not readback data at %zu", i);
            return asynError;
        }
    }
    LOG("wrote %zu words out of %zu", i, nElements);

    // Exit download mode.
    do {
        status = readRegister(ADDRESS_SYSTEM_OPERATING_STATE, &mode);
        if (status != asynSuccess) {
            LOG("could not read system state");
            return asynError;
        }
    } while (mode == MODE_DOWNLOAD_DATA && counter++ < LOOP_LIMIT);
    if (counter >= LOOP_LIMIT) {
        LOG("could not exit download mode, mode = %s",
                mode >= 0 && mode <= 11 ? modes[mode].c_str() : "N/A");
        return asynError;
    }
    LOG("exited download mode.");

    // Copy to flash.
    status = writeRegister(ADDRESS_DATA_BLOCK_DESTINATION, init);
    if (status != asynSuccess) {
        LOG("could not copy data to flash");
        return asynError;
    }
    LOG("copy to flash started.");

    // Wait to enter save data mode.
    counter = 0;
    do {
        status = readRegister(ADDRESS_SYSTEM_OPERATING_STATE, &mode);
        if (status != asynSuccess) {
            LOG("could not read system state");
            return asynError;
        }
    } while (mode != MODE_SAVE_DATA && counter++ < LOOP_LIMIT);
    if (counter >= LOOP_LIMIT) {
        LOG("could not enter save data mode");
        return asynError;
    }
    LOG("entered save data mode.");

    // Wait to exit save data mode
    counter = 0;
    do {
        status = readRegister(ADDRESS_SYSTEM_OPERATING_STATE, &mode);
        if (status != asynSuccess) {
            LOG("could not read system state");
            return asynError;
        }
        usleep(1000);
    } while (mode == MODE_SAVE_DATA && counter++ < LOOP_LIMIT);
    if (counter >= LOOP_LIMIT) {
        LOG("copy to flash failed");
        return asynError;
    }
    LOG("copy to flash done, exited save data mode.");
 
    // End transfer.
    status = writeRegister(ADDRESS_DATA_TRANSFER_INIT, zero);
    if (status != asynSuccess) {
        LOG("could not end data transfer");
        return asynError;
    }
    LOG("data transfer ended, waiting for device off/reset");

    // Wait for device off.
    counter = 0;
    mode = 1;
    do {
        status = readRegister(ADDRESS_SYSTEM_OPERATING_STATE, &mode);
        if (status != asynSuccess) {
            LOG("could not read system state");
            return asynError;
        }
        usleep(1000);
    } while (mode != MODE_DEVICE_OFF && counter++ < LOOP_LIMIT);
    if (counter >= LOOP_LIMIT) {
        LOG("device could not enter OFF mode. mode = %s", modes[mode].c_str());
        return asynError;
    }
    LOG("device is off, block write completed");

	return asynSuccess;
}

asynStatus PSController::readUInt32Digital(asynUser* asyn, epicsUInt32 *value, epicsUInt32 mask)
{
    parameter_t info = *(parameter_t*) asyn->drvUser;
	return readRegister(info.address, (u32*) value);
}

asynStatus PSController::readInt32(asynUser* asyn, epicsInt32* value)
{
    parameter_t p;

    p = *(parameter_t*) asyn->drvUser;
	return readRegister(p.address, (u32*) value);
}

asynStatus PSController::readFloat64(asynUser* asyn, epicsFloat64* value)
{
    raw32       raw;
    asynStatus  status;
    parameter_t p;

    p = *(parameter_t*) asyn->drvUser;
    status = readRegister(p.address, &raw.i_value);
    *value = raw.f_value;
    return status;
}

asynStatus PSController::writeInt32(asynUser* asyn, epicsInt32 value)
{
    asynStatus  status;
    u32         temp;
    parameter_t p;

    status = writeRegister(ADDRESS_PRIORITY, ETHERNET_ENABLE);
    if (status != asynSuccess) {
        LOG("ethernet priority error");
        return asynError;
    }

    p    = *(parameter_t*) asyn->drvUser;
    temp = (u32) value;
    return writeRegister(p.address, temp);
}

asynStatus PSController::writeFloat64(asynUser* asyn, epicsFloat64 value)
{
    asynStatus  status;
    raw32       raw;
    parameter_t p;

    status = writeRegister(ADDRESS_PRIORITY, ETHERNET_ENABLE);
    if (status != asynSuccess) {
        LOG("ethernet priority error");
        return asynError;
    }

    p = *(parameter_t*) asyn->drvUser;
    raw.f_value = (float) value;
    return writeRegister(p.address, raw.i_value);
}

asynStatus PSController::writeRegister(u16 address, u32 value)
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
    size_t tx_bytes;
    size_t rx_bytes;
    packet_t tx;
    packet_t rx; 
    char tx_array[PACKET_LENGTH];
    char rx_array[PACKET_LENGTH];

    u16 ps = 1;

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
    if(status != asynSuccess || tx_bytes != PACKET_LENGTH ||
                                rx_bytes != PACKET_LENGTH) {
        LOG("io failed for adddress %d | status = %d | tx = %lu | rx = %lu", 
             address, status, tx_bytes, rx_bytes);
        return asynError;
    }

    if (command == COMMAND_READ) {
        memcpy(&rx, rx_array, sizeof(rx_array));
        *value = rx.data;
    }
    return asynSuccess;
}

asynStatus PSController::drvUserCreate(asynUser* pasynUser, const char* drvInfo,
                                     const char** pptypeName, size_t* psize)
{
    parameter_t* info = new parameter_t();
    std::string driver_info = drvInfo;
    std::string driver_input = "";
    std::string substring = "";
    std::string parameter = "";

    size_t next_pos;
    size_t pos;

    pos = driver_info.find(" ");
    if (pos == std::string::npos) {
        LOG("could not parse parameter name, drvinfo: %s", drvInfo);
        return asynError;
    }

    parameter = driver_info.substr(0, pos);
    driver_input = driver_info.substr(pos + 1);
    pos = driver_input.find("address");
    if (pos == std::string::npos)
    {
        LOG("address not found in drvInfo: %s", drvInfo);
        return asynError;
    }
    next_pos  = driver_input.find_first_of(' ', pos + 1);
    if (next_pos == std::string::npos)
        next_pos = driver_input.length();
    substring = driver_input.substr(pos + 8, next_pos - (pos + 8));
    info->address = std::atoi(substring.c_str());

    pos = driver_input.find("ps");
    if (pos == std::string::npos)
    {
        LOG("ps not found in drvInfo: %s", drvInfo);
        return asynError;
    }
    next_pos  = driver_input.find_first_of(' ', pos + 1);
    if (next_pos == std::string::npos)
        next_pos = driver_input.length();
    substring = driver_input.substr(pos + 3, next_pos - (pos + 3));
    info->ps = std::atoi(substring.c_str());

    pos = driver_input.find("offset");
    if (pos == std::string::npos)
    {
        info->offset = 0;
    }
    else
    {
        next_pos = driver_input.find_first_of(' ', pos + 1);
        if (next_pos == std::string::npos)
            next_pos = driver_input.length();
        substring = driver_input.substr(pos + 3, next_pos - (pos + 3));
        info->offset = std::atoi(substring.c_str());
    }

    pasynUser->drvUser = info;
    return asynPortDriver::drvUserCreate(pasynUser, parameter.c_str(), pptypeName,
                                         psize);
}

