TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the IOC application

PROD_IOC = ioc
# ioc.dbd will be created and installed
DBD += ioc.dbd

# ioc.dbd will be made up from these files:
ioc_DBD += base.dbd

# Include dbd files from all support applications:
#ioc_DBD += xxx.dbd

# Add all the support libraries needed by this IOC
#ioc_LIBS += xxx

# ioc_registerRecordDeviceDriver.cpp derives from ioc.dbd
ioc_SRCS += ioc_registerRecordDeviceDriver.cpp

# Build the main IOC entry point on workstation OSs.
ioc_SRCS_DEFAULT += iocMain.cpp
ioc_SRCS_vxWorks += -nil-

# Add support from base/src/vxWorks if needed
#ioc_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# Finally link to the EPICS Base libraries
ioc_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

