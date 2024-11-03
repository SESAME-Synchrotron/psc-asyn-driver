#include <asynPortDriver.h>
#include <epicsExport.h>
#include <iocsh.h>
#include <PSController.h>

extern "C" asynStatus PSCConfigure(const char* port_name, const char* ip_port)
{
    new PSController(port_name, ip_port);
    return asynSuccess;
}

static const iocshArg arg0 = {"Port Name", iocshArgString };
static const iocshArg arg1 = {"IP address", iocshArgString };
static const iocshArg* args[] = {&arg0, &arg1};
static const iocshFuncDef configureFunction = {"PSCConfigure", 2, args};

static void  configureFunctionCall(const iocshArgBuf* args)
{
    PSCConfigure(args[0].sval, args[1].sval);
}

void PSCRegister(void)
{
    iocshRegister(&configureFunction, configureFunctionCall);
}

extern "C" {
    epicsExportRegistrar(PSCRegister);
}

