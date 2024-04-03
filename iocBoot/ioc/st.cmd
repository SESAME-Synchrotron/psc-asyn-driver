#!../../bin/linux-x86_64/ioc

dbLoadDatabase "dbd/ioc.dbd"
ioc_registerRecordDeviceDriver pdbbase

dbLoadRecords("db/correctors.db")
dbLoadRecords("db/optics.db")

drvAsynIPPortConfigure("PS_H_1", "10.2.2.34:9322 UDP", 0, 0, 0) # 628
drvAsynIPPortConfigure("PS_QF1_1", "10.2.2.73:9322 UDP", 0, 0, 0) # 628

PSCConfigure("HC1", "PS_H_1")
PSCConfigure("QF1_1", "PS_QF1_1")

iocInit

