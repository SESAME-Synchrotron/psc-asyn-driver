TOP=../..

include $(TOP)/configure/CONFIG

LIBRARY_IOC += psc
DBD         += psc.dbd
psc_SRCS    += PSController.cpp

CXXFLAGS += -std=c++11

psc_LIBS += $(EPICS_BASE_IOC_LIBS)

include $(TOP)/configure/RULES

