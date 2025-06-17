#!bin/linux-x86_64/ioc

dbLoadDatabase "dbd/ioc.dbd"
ioc_registerRecordDeviceDriver pdbbase

< q1.cmd

iocInit

