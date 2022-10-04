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

	createParam("indices_1", asynParamInt32,   &indices[0]);
	createParam("indices_2", asynParamInt32,   &indices[1]);
	createParam("indices_3", asynParamInt32,   &indices[2]);
	createParam("indices_1", asynParamFloat64, &indices[4]);
	createParam("indices_2", asynParamFloat64, &indices[5]);
	createParam("indices_3", asynParamFloat64, &indices[6]);
}

asynStatus PSController::readInt32(asynUser* asyn, epicsInt32* value)
{
	asynStatus status = performIO(asyn, reinterpret_cast<u32*>(value));
	return status;
}

asynStatus PSController::readFloat64(asynUser* asyn, epicsFloat64* value)
{
	asynStatus status = performIO(asyn, reinterpret_cast<u32*>((float*)value));
	return status;
}

asynStatus PSController::writeInt32(asynUser* asyn, epicsInt32 value)
{
	u32* _value = (u32*) &value;
	asynStatus status = performIO(asyn, _value);
	return status;
}

asynStatus PSController::writeFloat64(asynUser* asyn, epicsFloat64 value)
{
	u32* raw = (u32*) malloc(sizeof(u32));
	float _value = (float) value;
	memcpy(raw, &_value, sizeof(u32));
	asynStatus status = performIO(asyn, raw);
	return status;
}

asynStatus PSController::performIO(asynUser* asyn, u32* value)
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

	getParamName(function, &parameter_name);
	getAddress(asyn, &address);

	tx.status  = (u16) 0x0000;
	tx.command = (u16) COMMAND_READ;
	tx.address = (u16) address;
	tx.data    = (u32) 0;
	status = pasynOctetSyncIO->writeRead(asyn, reinterpret_cast<char*>(&tx), PACKET_LENGTH, reinterpret_cast<char*>(&rx), PACKET_LENGTH, 2, &tx_bytes, &rx_bytes, &reason);
	if(status != asynSuccess || tx_bytes != PACKET_LENGTH || rx_bytes != PACKET_LENGTH) {
		printf("Could not perform IO | Name: %s | Address: %d\n", parameter_name, address);
		return asynError;
	}

	*value = rx.data;
	return asynSuccess;
}
