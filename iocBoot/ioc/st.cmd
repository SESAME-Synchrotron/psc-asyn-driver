#!../../bin/linux-x86_64/ioc

dbLoadDatabase "dbd/ioc.dbd"
ioc_registerRecordDeviceDriver pdbbase

dbLoadRecords("db/correctors.db")
dbLoadRecords("db/optics.db")

drvAsynIPPortConfigure("PS_H_1", "10.2.2.34:9322 UDP", 0, 0, 0) # 628
drvAsynIPPortConfigure("PS_QF1_1", "10.2.2.73:9322 UDP", 0, 0, 0) # 628

# drvAsynIPPortConfigure("PS_H_2", "10.2.2.34:9322", 0, 0, 0) # 605
# drvAsynIPPortConfigure("PS_H_3", "10.2.2.35:9322", 0, 0, 0) # 665
# 
# drvAsynIPPortConfigure("PS_V_1", "10.2.2.36:9322", 0, 0, 0) # 624
# drvAsynIPPortConfigure("PS_V_2", "10.2.2.37:9322", 0, 0, 0) # 582 <=> 592
# drvAsynIPPortConfigure("PS_V_3", "10.2.2.38:9322", 0, 0, 0) # 667
# 
# drvAsynIPPortConfigure("PS_H_4", "10.2.2.39:9322", 0, 0, 0) # 602
# drvAsynIPPortConfigure("PS_H_5", "10.2.2.40:9322", 0, 0, 0) # 604
# drvAsynIPPortConfigure("PS_H_6", "10.2.2.41:9322", 0, 0, 0) # 671
# 
# drvAsynIPPortConfigure("PS_V_4", "10.2.2.42:9322", 0, 0, 0) # 676
# drvAsynIPPortConfigure("PS_V_5", "10.2.2.43:9322", 0, 0, 0) # 687
# drvAsynIPPortConfigure("PS_V_6", "10.2.2.44:9322", 0, 0, 0) # 650
# 
# drvAsynIPPortConfigure("PS_H_7", "10.2.2.45:9322", 0, 0, 0) # 609
# drvAsynIPPortConfigure("PS_H_8", "10.2.2.46:9322", 0, 0, 0) # 631 <=> 652
# drvAsynIPPortConfigure("PS_H_9", "10.2.2.47:9322", 0, 0, 0) # 651 
# 
# drvAsynIPPortConfigure("PS_V_7", "10.2.2.48:9322", 0, 0, 0) # 677
# drvAsynIPPortConfigure("PS_V_8", "10.2.2.49:9322", 0, 0, 0) # 589
# drvAsynIPPortConfigure("PS_V_9", "10.2.2.50:9322", 0, 0, 0) # 581
# 
# drvAsynIPPortConfigure("PS_H_10", "10.2.2.51:9322", 0, 0, 0) # 658
# drvAsynIPPortConfigure("PS_H_11", "10.2.2.52:9322", 0, 0, 0) # 649
# drvAsynIPPortConfigure("PS_H_12", "10.2.2.53:9322", 0, 0, 0) # 596
# 
# drvAsynIPPortConfigure("PS_V_10", "10.2.2.54:9322", 0, 0, 0) # 683
# drvAsynIPPortConfigure("PS_V_11", "10.2.2.55:9322", 0, 0, 0) # 598
# drvAsynIPPortConfigure("PS_V_12", "10.2.2.56:9322", 0, 0, 0) # 632
 
PSCConfigure("HC1", "PS_H_1")
PSCConfigure("QF1_1", "PS_QF1_1")

iocInit

