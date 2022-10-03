#ifndef __PSC_H__
#define __PSC_H__

#include <stdint.h>

#include <string>
#include <iostream>
#include <vector>
#include <algorithm>

#include <epicsExport.h>
#include <asynPortDriver.h>
#include <asynOctetSyncIO.h>
#include <iocsh.h>

using std::string;
using std::cout;
using std::endl;
using std::vector;
using std::find;

#define MAX_PS	3

typedef uint32_t u32;
typedef uint16_t u16;

typedef struct {
	string name;
	asynParamType type;
	int address;
} parameter_t;

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
	vector<int> indices;
	vector<parameter_t> parameters;

private:
	asynUser* device;
};

#endif
