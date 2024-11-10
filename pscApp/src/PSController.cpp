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

    status = drvAsynIPPortConfigure("TCP", tcp_host.c_str(), 1, 0, 0);
    if (status != asynSuccess) {
        cout << "Could not create port " << tcp_host << endl;
        return;
    }

    status = pasynOctetSyncIO->connect("TCP", 1, &this->blockIO, NULL);
    if(status != asynSuccess) {
        cout << "Could not connect to TCP port: " << ip_port << endl;
        return;
    }

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

	u32 mode;
	int status;
	int counter = 0;
    int reason;
    size_t rx_bytes;
    size_t tx_bytes;

    char tx_array[TCP_PACKET_LENGTH];
	char rx_array[256 * 4];

    static const u32 zero = 0;

	ZERO(tx_array);
	ZERO(rx_array);
	state_t state = STATE_INIT;
	state_t next_state;

    lock();
	while (state != STATE_DEVICE_OFF)
	{
        switch (state)
        {
            case STATE_INIT:
                status = doRegisterIO(ADDRESS_DATA_TRANSFER_INIT, COMMAND_WRITE, (u32*) &address);
                if (status != asynSuccess)
                    next_state = STATE_ERROR;
                else
                    next_state = STATE_CHECK_DATA_TRANSFER;
                break;

            case STATE_CHECK_DATA_TRANSFER:
                status = doRegisterIO(ADDRESS_DATA_TRANSFER_INIT, COMMAND_READ, &mode);
                if (status != PSC_OK)
                    next_state = STATE_ERROR;
                else if (mode != (u32) address)
                    next_state = STATE_CHECK_DATA_TRANSFER;
                else
                    next_state = STATE_ENTER_TRANSIENT;
                break;

            case STATE_ENTER_TRANSIENT:
                status = doRegisterIO(ADDRESS_SYSTEM_OPERATING_STATE, COMMAND_READ, &mode);
                if (status != PSC_OK)
                    next_state = STATE_ERROR;
                else if (mode != MODE_TRANSIENT && mode != MODE_MODIFY_DATA)
                    next_state = STATE_ENTER_TRANSIENT;
                else if (mode == MODE_TRANSIENT)
                    next_state = STATE_EXIT_TRANSIENT;
                else
                    next_state = STATE_DATA_TRANSFER;
                break;

            case STATE_EXIT_TRANSIENT:
                status = doRegisterIO(ADDRESS_SYSTEM_OPERATING_STATE, COMMAND_READ, &mode);
                if (status != PSC_OK)
                    next_state = STATE_ERROR;
                else if (mode == MODE_TRANSIENT)
                    next_state = STATE_EXIT_TRANSIENT;
                else
                    next_state = STATE_DATA_TRANSFER;
                break;

            case STATE_DATA_TRANSFER:
            	memcpy(tx_array + 0, &COMMAND_READ, sizeof(u8));
            	memcpy(tx_array + 1, &zero, sizeof(u32));
            	memcpy(tx_array + 5, &nElements, sizeof(u32));
                status = pasynOctetSyncIO->writeRead(this->blockIO, 
                                                     tx_array, TCP_PACKET_LENGTH, 
                                                     rx_array, 1024, 2, 
                                                     &tx_bytes, &rx_bytes, &reason);
                if(status != asynSuccess || 
                   tx_bytes != TCP_PACKET_LENGTH || rx_bytes != 1024) {
                    printf("Status: %d | Reason: %d | Bytes: %lu - %lu\n", 
                            status, reason, tx_bytes, rx_bytes);
                    next_state = STATE_ERROR;
                }
                else {
                    memcpy(value, rx_array, sizeof(rx_array));
                    *nIn = (rx_bytes / 4);
                    next_state = STATE_END_TRANSFER;
                }
                break;

            case STATE_END_TRANSFER:
                status = doRegisterIO(ADDRESS_DATA_TRANSFER_INIT, COMMAND_WRITE, &(u32[]){0}[0]);
                if (status != PSC_OK)
                    next_state = STATE_ERROR;
                else
                    next_state = STATE_WAIT_DEVICE_OFF;
                break;

            case STATE_WAIT_DEVICE_OFF:
                status = doRegisterIO(ADDRESS_SYSTEM_OPERATING_STATE, COMMAND_READ, &mode);
                if (status != PSC_OK)
                    next_state = STATE_ERROR;
                else if (mode != MODE_MONITOR && mode != MODE_DEVICE_OFF)
                    next_state = STATE_WAIT_DEVICE_OFF;
                else
                    next_state = STATE_DEVICE_OFF;
                break;

            default:
                goto exit;
                break;
        }

        counter++;
		usleep(1000);
		if (next_state != state) {
			printf("%- 25s -> %s \n", state_names[state], state_names[next_state]);
			counter = 0;
		}
		
		if (counter >= LOOP_LIMIT) {
			printf("State loop limit reached. [ %s ]\n", state_names[state]);
			status = -1;
			break;
		}

		state = next_state;
    }

exit:
    unlock();
    printf("Final status: %d\n", status);
    if (status != 0)
        return asynError;
    return asynSuccess;
}

asynStatus PSController::writeInt32Array(asynUser *asyn, epicsInt32 *value,
                                        size_t nElements)
{
    LOAD_ASYN_ADDRESS;

	u32 mode;
	asynStatus status;
	int counter = 0;
    int index = 0;

	state_t state = STATE_INIT;
	state_t next_state;

    u32 offset = address;
    u32 length = nElements;


    u32 init = (length << 8)|(offset & 0xff);

    static const u32 zero = 0;

    lock();
    while (state != STATE_DEVICE_OFF)
	{
		switch (state)
		{
			case STATE_INIT:
                status = doRegisterIO(ADDRESS_DATA_TRANSFER_INIT, COMMAND_WRITE,
                                      &init);
				if (status != PSC_OK)
					next_state = STATE_ERROR;
				else
					next_state = STATE_CHECK_DOWNLOAD_MODE;
				break;

			case STATE_CHECK_DOWNLOAD_MODE:
                status = doRegisterIO(ADDRESS_SYSTEM_OPERATING_STATE,
                        COMMAND_READ, &mode);
				if (status != PSC_OK)
					next_state = STATE_ERROR;
				else if (mode != MODE_DOWNLOAD_DATA)
					next_state = STATE_CHECK_DOWNLOAD_MODE;
				else
					next_state = STATE_DATA_TRANSFER;
				break;

			case STATE_DATA_TRANSFER:
                status = doRegisterIO(ADDRESS_DATA_TRANSFER, COMMAND_WRITE, (u32*)(value + index));
				usleep(1000);
				if (status != PSC_OK)
					next_state = STATE_ERROR;
				else
					next_state = STATE_VERIFY_DATA;
				break;

			case STATE_VERIFY_DATA:
                status = doRegisterIO(ADDRESS_DATA_TRANSFER, COMMAND_READ, &mode);
				if (status != PSC_OK)
					next_state = STATE_ERROR;
				else if (mode != (u32)value[index])
					next_state = STATE_VERIFY_DATA;
				else if (index >= 255)
					next_state = STATE_EXIT_DOWNLOAD_MODE;
				else {
					index++;
					next_state = STATE_DATA_TRANSFER;
				}
				break;

			case STATE_EXIT_DOWNLOAD_MODE:
                status = doRegisterIO(ADDRESS_SYSTEM_OPERATING_STATE, COMMAND_READ, &mode);
				if (status != PSC_OK)
					next_state = STATE_ERROR;
				else if (mode == MODE_DOWNLOAD_DATA)
					next_state = STATE_EXIT_DOWNLOAD_MODE;
				else
					next_state = STATE_COPY_TO_FLASH;
				break;

			case STATE_COPY_TO_FLASH:
                status = doRegisterIO(ADDRESS_DATA_BLOCK_DESTINATION,
                                      COMMAND_WRITE,
                                      &init);
				if (status != PSC_OK)
					next_state = STATE_ERROR;
				else
					next_state = STATE_WAIT_SAVE;
				break;

			case STATE_WAIT_SAVE:
                status = doRegisterIO(ADDRESS_SYSTEM_OPERATING_STATE, 
                        COMMAND_READ, &mode);
				if (status != PSC_OK)
					next_state = STATE_ERROR;
				else if (mode != MODE_SAVE_DATA)
					next_state = STATE_WAIT_SAVE;
				else
					next_state = STATE_SAVE_FINISHED;
				break;

			case STATE_SAVE_FINISHED:
                status = doRegisterIO(ADDRESS_SYSTEM_OPERATING_STATE, COMMAND_READ, &mode);
				if (status != PSC_OK)
					next_state = STATE_ERROR;
				else if (mode == MODE_SAVE_DATA)
					next_state = STATE_SAVE_FINISHED;
				else
					next_state = STATE_END_TRANSFER;
				break;

			case STATE_END_TRANSFER:
                status = doRegisterIO(ADDRESS_DATA_TRANSFER_INIT, COMMAND_WRITE, const_cast<u32*>(&zero));
				if (status != PSC_OK)
					next_state = STATE_ERROR;
				else
					next_state = STATE_WAIT_DEVICE_OFF;
				break;

			case STATE_WAIT_DEVICE_OFF:
                status = doRegisterIO(ADDRESS_SYSTEM_OPERATING_STATE, COMMAND_READ, &mode);
				if (status != PSC_OK)
					next_state = STATE_ERROR;
				else if (mode == MODE_DEVICE_OFF)
					next_state = STATE_DEVICE_OFF;
				else
					next_state = STATE_WAIT_DEVICE_OFF;
				break;

			default:
				goto exit;
				break;
		}

		counter++;
		usleep(1000);
		if (next_state != state) {
			printf("%- 25s -> %s \n", state_names[state], state_names[next_state]);
			counter = 0;
		}

		if (counter >= LOOP_LIMIT) {
			printf("State loop limit reached. [ %s ]\n", state_names[state]);
			status = asynError;
			break;
		}

		state = next_state;
	}

exit:
	return status;
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

    asynStatus status = doRegisterIO(address, COMMAND_WRITE, &temp);
    return status;
}

asynStatus PSController::writeFloat64(asynUser* asyn, epicsFloat64 value)
{
    LOAD_ASYN_ADDRESS;
    float temp = (float) value;

    asynStatus status = doRegisterIO(address, COMMAND_WRITE, reinterpret_cast<u32*>(&temp));

    std::cout << address << " - " << status << std::endl;

    return status;
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
