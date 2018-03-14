TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#========================================

# <name>.dbd will be created from <name>Include.dbd
DBD += ycpswasyn.dbd

INC += drvYCPSWASYN.h

LIBRARY_IOC += ycpswasyn
LIB_SRCS += drvYCPSWASYN.cpp
LIB_LIBS += asyn
LIB_LIBS += yamlLoader

# =================================================
# Need to link these external libraries statically
# =================================================
USR_INCLUDES = $(addprefix -I,$(BOOST_INCLUDE) $(CPSW_FRAMEWORK_INCLUDE) $(YAML_INCLUDE))
cpsw_DIR = $(CPSW_FRAMEWORK_LIB)
boost_system_DIR = $(BOOST_LIB)
yaml-cpp_DIR = $(YAML_LIB)
USR_LIBS_Linux += cpsw boost_system yaml-cpp

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
