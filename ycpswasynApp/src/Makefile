TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#========================================

# <name>.dbd will be created from <name>Include.dbd
DBD += ycpswasyn.dbd
DBD += ycpswasynSupport.dbd

INC += drvYCPSWASYN.h

LIBRARY_IOC += ycpswasyn
LIB_SRCS += drvYCPSWASYN.cpp
LIB_LIBS += asyn
LIB_LIBS += seq pv

LIB_LDFLAGS += -L$(BOOST_LIB) -L$(CPSW_FRAMEWORK_LIB) -L$(YAML_LIB)
LIB_SYS_LIBS += cpsw boost_system yaml-cpp
USR_INCLUDES = -I$(BOOST_INCLUDE) -I$(CPSW_FRAMEWORK_INCLUDE) -I$(YAML_INCLUDE)

# ================================================
# Build the IOC application
# ================================================
PROD_IOC = ycpswasynApp

# ======================================================================
# <name>_registerRecordDeviceDriver.cpp will be created from <name>.dbd
PROD_SRCS_DEFAULT += ycpswasyn_registerRecordDeviceDriver.cpp ycpswasynMain.cpp
PROD_SRCS_vxWorks += -nil-

PROD_LIBS += ycpswasyn
PROD_LIBS += asyn
PROD_LIBS += seq pv
PROD_LIBS += $(CEXP_LIBS)
PROD_LIBS += $(EPICS_BASE_IOC_LIBS)

PROD_LDFLAGS += -L$(BOOST_LIB) -L$(CPSW_FRAMEWORK_LIB) -L$(YAML_LIB)
PROD_SYS_LIBS += cpsw boost_system yaml-cpp
USR_INCLUDES = -I$(BOOST_INCLUDE) -I$(CPSW_FRAMEWORK_INCLUDE) -I$(YAML_INCLUDE)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

