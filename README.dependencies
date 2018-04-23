# YCPSWASYN Dependencies

This module depends on the following external packages:
- CPSW (framework)
- BOOST
- YAML-CPP

and on the following EPICS modules:
- Asyn
- yamlLoader

In order to use this module you need to add the following definitions, or something similar, into your IOC application:

## configure/CONFIG_SITE

```
CPSW_FRAMEWORK_PACKAGE_NAME=cpsw/framework
CPSW_FRAMEWORK_VERSION=R3.6.4
CPSW_FRAMEWORK_TOP=$(PACKAGE_SITE_TOP)/$(CPSW_FRAMEWORK_PACKAGE_NAME)/$(CPSW_FRAMEWORK_VERSION)
CPSW_FRAMEWORK_LIB = $(CPSW_FRAMEWORK_TOP)/$(PKG_ARCH)/lib
CPSW_FRAMEWORK_INCLUDE = $(CPSW_FRAMEWORK_TOP)/$(PKG_ARCH)/include

YAML_PACKAGE_NAME=yaml-cpp
YAML_VERSION=yaml-cpp-0.5.3
YAML_TOP=$(PACKAGE_SITE_TOP)/$(YAML_PACKAGE_NAME)/$(YAML_VERSION)
YAML_LIB= $(YAML_TOP)/$(PKG_ARCH)/lib
YAML_INCLUDE=$(YAML_TOP)/$(PKG_ARCH)/include

BOOST_PACKAGE_NAME=boost
BOOST_VERSION=1.63.0
BOOST_TOP=$(PACKAGE_SITE_TOP)/$(BOOST_PACKAGE_NAME)/$(BOOST_VERSION)
BOOST_LIB = $(BOOST_TOP)/$(PKG_ARCH)/lib
BOOST_INCLUDE = $(BOOST_TOP)/$(PKG_ARCH)/include
```

## configure/CONFIG_SITE.Common.linuxRT-x86_64

```
PKG_ARCH=$(LINUXRT_BUILDROOT_VERSION)-x86_64
```

## configure/CONFIG_SITE.Common.rhel6-x86_64

```
PKG_ARCH=$(LINUX_VERSION)-x86_64
```

## configure/RELEASE

```
ASYN_MODULE_VERSION=R4.30-0.3.0
YAMLLOADER_MODULE_VERSION=R1.0.3

ASYN=$(EPICS_MODULES)/asyn/$(ASYN_MODULE_VERSION)
YAMLLOADER=$(EPICS_MODULES)/yamlLoader/$(YAMLLOADER_MODULE_VERSION)
```

## xxxApp/src/Makefile

```
# =====================================================
# Path to "NON EPICS" External PACKAGES: USER INCLUDES
# =====================================================
USR_INCLUDES = $(addprefix -I,$(BOOST_INCLUDE) $(CPSW_FRAMEWORK_INCLUDE) $(YAML_INCLUDE))
# =====================================================

# ======================================================
#PATH TO "NON EPICS" EXTERNAL PACKAGES: USER LIBRARIES
# ======================================================
cpsw_DIR = $(CPSW_FRAMEWORK_LIB)
yaml-cpp_DIR = $(YAML_LIB)
# ======================================================

# ======================================================
# LINK "NON EPICS" EXTERNAL PACKAGE LIBRARIES STATICALLY
# ======================================================
USR_LIBS_Linux += cpsw yaml-cpp
# ======================================================

# ayn yamlLoader DBD
xxx_DBD += asyn.dbd
xxx_DBD += yamlLoader.dbd

# =====================================================
# Link in the libraries from other EPICS modules
# =====================================================
xxx_LIBS += yamlLoader
xxx_LIBS += asyn
```

## xxxApp/Db/Makefile

```
# ==========================================
# YCPSWASYN databases
# ==========================================
## For automatic generated records
DB_INSTALLS += $(YCPSWASYN)/db/ai.template
DB_INSTALLS += $(YCPSWASYN)/db/ao.template
DB_INSTALLS += $(YCPSWASYN)/db/longin.template
DB_INSTALLS += $(YCPSWASYN)/db/longout.template
DB_INSTALLS += $(YCPSWASYN)/db/waveform_in.template
DB_INSTALLS += $(YCPSWASYN)/db/waveform_out.template
DB_INSTALLS += $(YCPSWASYN)/db/waveform_8_in.template
DB_INSTALLS += $(YCPSWASYN)/db/waveform_8_out.template
DB_INSTALLS += $(YCPSWASYN)/db/waveform_in_float.template
DB_INSTALLS += $(YCPSWASYN)/db/waveform_out_float.template
DB_INSTALLS += $(YCPSWASYN)/db/mbbi.template
DB_INSTALLS += $(YCPSWASYN)/db/mbbo.template
DB_INSTALLS += $(YCPSWASYN)/db/bo.template
DB_INSTALLS += $(YCPSWASYN)/db/waveform_stream16.template
DB_INSTALLS += $(YCPSWASYN)/db/waveform_stream32.template
```
