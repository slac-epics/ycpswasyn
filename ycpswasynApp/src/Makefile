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

# =================================================
# Need to link these external libraries statically
# =================================================
USR_INCLUDES = -I$(BOOST_INCLUDE) -I$(CPSW_FRAMEWORK_INCLUDE) -I$(YAML_INCLUDE)
cpsw_DIR = $(CPSW_FRAMEWORK_LIB)
boost_system_DIR = $(BOOST_LIB)
yaml-cpp_DIR = $(YAML_LIB)
USR_LIBS_Linux += cpsw boost_system yaml-cpp

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

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

