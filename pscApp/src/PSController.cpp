#include "PSController.h"

PSController::PSController(const char* name, const char* asyn_name)
	: asynPortDriver(
			name,
			MAX_PS,
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

	this->parameters.push_back({"I_load",    asynParamFloat64, 148});
	this->parameters.push_back({"V_mon",     asynParamFloat64, 158});
	this->parameters.push_back({"reference", asynParamFloat64, 144});

	int i = 0;
	this->indices.reserve(parameters.size());
	for(auto p : this->parameters) {
		createParam(p.name.c_str(), p.type, &indices[i]);
		i++;
	}
}

asynStatus PSController::readInt32(asynUser* asyn, epicsInt32* value)
{
	return asynSuccess;
}

asynStatus PSController::readFloat64(asynUser* asyn, epicsFloat64* value)
{
	return asynSuccess;
}

asynStatus PSController::writeInt32(asynUser* asyn, epicsInt32 value)
{
	return asynSuccess;
}

asynStatus PSController::writeFloat64(asynUser* asyn, epicsFloat64 value)
{
	return asynSuccess;
}


