dbLoadRecords("db/optics-q1.db")

PSCConfigure("C01-QF1", "10.2.2.73:9322")
PSCConfigure("C01-QD1", "10.2.2.74:9322")
PSCConfigure("C01-QF2", "10.2.2.75:9322")
PSCConfigure("C01-QD2", "10.2.2.76:9322")

PSCConfigure("C02-QF1", "10.2.2.77:9322")
PSCConfigure("C02-QD1", "10.2.2.78:9322")
PSCConfigure("C02-QF2", "10.2.2.79:9322")
PSCConfigure("C02-QD2", "10.2.2.80:9322")

PSCConfigure("C03-QF1", "10.2.2.81:9322")
PSCConfigure("C03-QD1", "10.2.2.82:9322")
PSCConfigure("C03-QF2", "10.2.2.83:9322")
PSCConfigure("C03-QD2", "10.2.2.84:9322")

PSCConfigure("C04-QF1", "10.2.2.85:9322")
PSCConfigure("C04-QD1", "10.2.2.86:9322")
PSCConfigure("C04-QF2", "10.2.2.87:9322")
PSCConfigure("C04-QD2", "10.2.2.88:9322")

dbLoadRecords("db/asynRecord.db", "PORT=C01-QF1_UDP,P=SRC01-PS-QF1:,R=AsynIO,ADDR=,IMAX=,OMAX=")
dbLoadRecords("db/asynRecord.db", "PORT=C01-QD1_UDP,P=SRC01-PS-QD1:,R=AsynIO,ADDR=,IMAX=,OMAX=")
dbLoadRecords("db/asynRecord.db", "PORT=C01-QF2_UDP,P=SRC01-PS-QF2:,R=AsynIO,ADDR=,IMAX=,OMAX=")
dbLoadRecords("db/asynRecord.db", "PORT=C01-QD2_UDP,P=SRC01-PS-QD2:,R=AsynIO,ADDR=,IMAX=,OMAX=")

dbLoadRecords("db/asynRecord.db", "PORT=C02-QF1_UDP,P=SRC02-PS-QF1:,R=AsynIO,ADDR=,IMAX=,OMAX=")
dbLoadRecords("db/asynRecord.db", "PORT=C02-QD1_UDP,P=SRC02-PS-QD1:,R=AsynIO,ADDR=,IMAX=,OMAX=")
dbLoadRecords("db/asynRecord.db", "PORT=C02-QF2_UDP,P=SRC02-PS-QF2:,R=AsynIO,ADDR=,IMAX=,OMAX=")
dbLoadRecords("db/asynRecord.db", "PORT=C02-QD2_UDP,P=SRC02-PS-QD2:,R=AsynIO,ADDR=,IMAX=,OMAX=")

dbLoadRecords("db/asynRecord.db", "PORT=C03-QF1_UDP,P=SRC03-PS-QF1:,R=AsynIO,ADDR=,IMAX=,OMAX=")
dbLoadRecords("db/asynRecord.db", "PORT=C03-QD1_UDP,P=SRC03-PS-QD1:,R=AsynIO,ADDR=,IMAX=,OMAX=")
dbLoadRecords("db/asynRecord.db", "PORT=C03-QF2_UDP,P=SRC03-PS-QF2:,R=AsynIO,ADDR=,IMAX=,OMAX=")
dbLoadRecords("db/asynRecord.db", "PORT=C03-QD2_UDP,P=SRC03-PS-QD2:,R=AsynIO,ADDR=,IMAX=,OMAX=")

dbLoadRecords("db/asynRecord.db", "PORT=C04-QF1_UDP,P=SRC04-PS-QF1:,R=AsynIO,ADDR=,IMAX=,OMAX=")
dbLoadRecords("db/asynRecord.db", "PORT=C04-QD1_UDP,P=SRC04-PS-QD1:,R=AsynIO,ADDR=,IMAX=,OMAX=")
dbLoadRecords("db/asynRecord.db", "PORT=C04-QF2_UDP,P=SRC04-PS-QF2:,R=AsynIO,ADDR=,IMAX=,OMAX=")
dbLoadRecords("db/asynRecord.db", "PORT=C04-QD2_UDP,P=SRC04-PS-QD2:,R=AsynIO,ADDR=,IMAX=,OMAX=")

