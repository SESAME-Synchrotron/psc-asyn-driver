#!bin/linux-x86_64/ioc

dbLoadDatabase "dbd/ioc.dbd"
ioc_registerRecordDeviceDriver pdbbase

< cmd/q1.cmd

iocInit

