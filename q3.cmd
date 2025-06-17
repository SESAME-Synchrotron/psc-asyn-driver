#!bin/linux-x86_64/ioc

dbLoadDatabase "dbd/ioc.dbd"
ioc_registerRecordDeviceDriver pdbbase

dbLoadRecords("db/optics-q3.db")

PSCConfigure("C09-QF1", "10.2.2.105:9322")
PSCConfigure("C09-QD1", "10.2.2.106:9322")
PSCConfigure("C09-QF2", "10.2.2.107:9322")
PSCConfigure("C09-QD2", "10.2.2.108:9322")

PSCConfigure("C10-QF1", "10.2.2.109:9322")
PSCConfigure("C10-QD1", "10.2.2.110:9322")
PSCConfigure("C10-QF2", "10.2.2.111:9322")
PSCConfigure("C10-QD2", "10.2.2.112:9322")

PSCConfigure("C11-QF1", "10.2.2.113:9322")
PSCConfigure("C11-QD1", "10.2.2.114:9322")
PSCConfigure("C11-QF2", "10.2.2.115:9322")
PSCConfigure("C11-QD2", "10.2.2.116:9322")

PSCConfigure("C12-QF1", "10.2.2.117:9322")
PSCConfigure("C12-QD1", "10.2.2.118:9322")
PSCConfigure("C12-QF2", "10.2.2.119:9322")
PSCConfigure("C12-QD2", "10.2.2.120:9322")


iocInit

