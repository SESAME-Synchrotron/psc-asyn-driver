TOP=../..

include $(TOP)/configure/CONFIG

LIBRARY_IOC += psc
DBD         += psc.dbd
psc_SRCS    += PSController.cpp
psc_SRCS    += PSCRegister.cpp

CXXFLAGS += -std=c++11 -fPIC
CFLAGS   += -fPIC

psc_LIBS += $(EPICS_BASE_IOC_LIBS)

include $(TOP)/configure/RULES

