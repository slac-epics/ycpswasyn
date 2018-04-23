# How to manually create PV for your registers

## Overview

YCPSWASYN uses CPSW for the communication with FPGA and AsynPortDriver for its integration into EPICS.

This document describes the steps required to map register from the FPGA space, which definition comes in YAML files from the hardware engineers, into EPICS PVs with appropriate type and LCLS-compatible name. 

## Background

You should be familiar with CPSW and how hierarchies are defined in YAML. 

Go to the CPSW and YAML go to the official [confluence page about CPSW](https://confluence.slac.stanford.edu/display/ppareg/CPSW%3A+HowTo+User+Guide), or take a look at the README files in the CPSW framework package area.


## Mapping registers to EPICS PVs

### 1. Assign Asyn parameter names to registers

For each register that you want to expose as a PV, you need to assign an Asyn parameter name to it. For this, you need to create a dictionary file. This is a text file with 2 columns describing: the register's path (as describe in the application YAML files and as it is expose by CPSW) on the first column, and its associated desired Asyn parameter name. The parameter name must be unique inside your application. 

This is an example of how the dictionary file looks like:

        #################################################################
        # Register path                           # Asyn parameter name #
        #################################################################
        /root/DeviceIdRegister                    DEV_ID
        /root/device1/subdevice2/register3        DEV1_SUB2_REG3
        /root/deviceArray[0]/registersArray[3]    DEVA0_REGA3

You can add comments starting with '#'.

An dictionary example is given with this module as a reference: <TOP>/yaml/example.dict

### 2. Create EPICS records attached to the corresponding Asyn parameter

Once you have defined parameter names for all you register you need to attached then to appropriate records. This is done in the standard way AsynPortDriver does it, that is: use an Asyn device type on the record DTYP field, and define the INP or OUT fields following the AsynPortDriver convention as @asyn(PORT, ADDR, TIMEOUT)ASYN_PARAMETER_NAME or @asynMask(PORT, ADDR, MASK)ASYN_PARAMETER_NAME. You must use the parameter name you defined on your dictionary in the INP/OUT filed.

The Asyn device type (which goes on DTYP) and ADDR depends on the type of register you are using:

| Register class         | nelms  | Encoding   | Enum  | Register mode   | DTYP               | ADDR  |
|:-----------------------|:-------|:----------:|:-----:|:---------------:|:-------------------|:-----:|
| IntField               | 1      |            |       | RO              | asynInt32          | 0     |
| IntField               | >1     |            |       | RO              | asynInt32ArrayIn   | 0     |
| IntField               | 1      |            |       | RW              | asynInt32          | 1     |
| IntField               | >1     |            |       | RW              | asynInt32ArrayOut  | 1     |
| IntField               | N/A    |            | Yes   | RO              | asynUInt32Digital  | 0     |
| IntField               | N/A    |            | Yes   | RW              | asynUInt32Digital  | 1     |
| IntField               | N/A    | IEEE_754   |       | RO              | asynFloat64        | 2     |
| IntField               | N/A    | IEEE_754   |       | RW              | asynFloat64        | 3     |
| SequenceCommand        | N/A    |            |       | N/A             | asynUInt32Digital  | 4     |
| IntField (stream port) | N/A    |            |       | N/A             | asynInt32ArrayIn   | 5     |


The register information is available on the YAML definition.

For example, for a register with class IntField and mode RO, for which you have defined a parameter name MY_REGISTER in you dictionary, you can defined a record like this:

        record(longin,  "$(PV_NAME)") {
            field(DTYP, "asynInt32")
            field(DESC, "My register")
            field(SCAN, "1 second")
            field(INP,  "@asyn($(PORT),0)MY_REGISTER")
        }

A *almost* complete list of example templates with its substitution file (example.substitutions) are given on <TOP>/ycpswasynApp/Db:

| Register class         | nelms  | Encoding   | Enum  | Register mode   | Example template
|:-----------------------|:-------|:----------:|:-----:|:---------------:|:----------------------
| IntField               | 1      |            |       | RO              | RegisterIn.template,
| IntField               | >1     |            |       | RO              | RegisterArrayIn.template
| IntField               | 1      |            |       | RW              | RegisterOut.template, RegisterOutRBV.template
| IntField               | >1     |            |       | RW              | RegisterArrayOut.template
| IntField               | N/A    |            | Yes   | RO              | RegisterEnumBIn.template, RegisterEnumMBBIn.template 
| IntField               | N/A    |            | Yes   | RW              | RegisterEnumBOut.template, RegisterEnumMBBOut.template, RegisterDoubleOutRBV.template, RegisterEnumMBBOutRBV.template
| IntField               | N/A    | IEEE_754   |       | RO              | RegisterDoubleIn.template
| IntField               | N/A    | IEEE_754   |       | RW              | RegisterDoubleOut.template, RegisterEnumBOutRBV.te
| SequenceCommand        | N/A    |            |       | N/A             | RegisterCommand.template
| IntField (stream port) | N/A    |            |       | N/A             | RegisterStream.template, RegisterStream16.template

**Notes:**
- *RBV templates show how to implement a read-back from a register with R/W access.
- For Stream ports, an additional parameter is automatically created and the name is generated adding ":16" to the original parameter name. This gives access to the same stream data, but as 16-bit words which is the case for ADC samples for example. The template RegisterStream16.template shows how to use this feature.
