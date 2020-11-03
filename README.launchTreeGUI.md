# How to launch the cpswTreeGUI

## Overview

The cpswTreeGUI is a Qt application for inspecting and modifying firmware registers at runtime. The register heirarchy is defined in YAML.

The TreeGUI is normally launched with a set of command line parameters to connect to the correct FPGA. These parameters are static to an IOC so they can be stored in an EPICS database and looked up when, say, a button on another screen is clicked. This way you can simply pass macros to a display so that it can look up the values in PVs at runtime.

## Background

You should be familiar with CPSW and how hierarchies are defined in YAML.

Go to the CPSW and YAML go to the official [confluence page about CPSW](https://confluence.slac.stanford.edu/display/ppareg/CPSW%3A+HowTo+User+Guide), or take a look at the README files in the CPSW framework package area.


## Requirements

You should include the database located in <TOP>/ycpswasyn/Db/treeGui.db in your application. In your <TOP>/<Name>App/Db/Makefile add

`DB_INSTALLS += $(YCPSWASYN)/db/treeGui.db`

Additionally, the TreeGUI requires a 'backdoor' YAML tarball if you want to run it alongside your IOC. After getting the backdoor tarball from the system firmware engineer, place it in <TOP>/firmware

### Example and Display

A database example is given as a reference. It it located in `<TOP>/ycpswasyn/Db/treeGui.db`

An example PyDM display that you can use with this is located in `$TOOLS/pydm/display/util/`. There are a pair of files, `fpga_config.ui` and `fpga_config.py`
