# How does the automatic generation of PV works?

## Overview

YCPSWASYN uses CPSW for the communication with FPGA and AsynPortDriver for its integration into EPICS.

This document describes how the automatic generation of PVs works.

## Background

You should be familiar with CPSW and how hierarchies are defined in YAML.

Go to the CPSW and YAML go to the official [confluence page about CPSW](https://confluence.slac.stanford.edu/display/ppareg/CPSW%3A+HowTo+User+Guide), or take a look at the README files in the CPSW framework package area.

## PV Naming

PVs are automatically generated for all registers defined on the YAML files. There are 2 mode of autogeneratio of the PV names, depending on the *AUTO_GEN_MODE* mode passed to the driver (see **README.configureDriver**):

### PV names based on mappings (AUTO_GEN_MODE = 1)

The PV names are calculated based on the register path, following the mapping defined in these 2 files:
- map
- map_top

These file location can be especified with the driver's argument *MAP_FILE_PATH*. If the argument is empty, the default location is <TOP>/yaml.

Each device name, from right to left, in the path is substitute by its map on the “map” or “map_top” file. The difference is that if it finds the name on “map_top” the substitution ends and the PV name is completed.

If the device is not found on the maps, the device name is truncated to 3 chars.

The register name at the tip of the path is left unmodified.

A post-fix is added to the PV name based on the type of register:
- ":Rd" for RO registers
- ":St" for RW registers
- ":Ex" for Command registers
- ":16" for stream data interpreted as 16-bit words
- ":32" for stream data interpreted as 32-bit words

**Note:** RW register must be written using its *:St PV and read back using its *:Rd PV


For example, the PV for the register

        /mmio/DigFpga/AmcCarrierCore/AxiVersion/BuildStamp

is

        ${P}C:AV:BuildStamp:Rd

**Note:** the macro ${P} is defined via argument *PREFIX* on CPSWASYNConfig():
         if the respective argument is a non-empty string then ':' is added.

### Hashed PV Names (AUTO_GEN_MODE = 2)

The driver computes a SHA1 hash over a string composed by:
- The prefix especified using the *PREFIX* driver's argument,
- the complete register path,
- the post-fix, based in the type of register
  - "Rd" for RO registers
  - "St" for RW registers
  - "Ex" for Command registers
  - "16" for stream data interpreted as 16-bit words
  - "32" for stream data interpreted as 32-bit words

        [PREFIX]REGISTER_PATH[POST-FIX]

The prefix and post-fix are used verbatim, i.e. not ":" is added.

The resulting SHA1 hash is converted to a all-uppercase hex-representation (which is 40 characters
long). The final PV name is obtained by truncating the SHA1 (at the end) to the name limit
(as passed to YCPSWASYNConfig).

E.g., a prefix `PREFIX` and RO register path `/mmio/something[2]/reg[0-15]` computes to a SHA1
sum (over `PREFIX/mmio/something[2]/reg[0-15]Rd`)

         DD9B9EAAB711EB22FE04B7690BE42AC5A35C29C5

If the register is RW, then an additional PV will be generated, and its name will be computed over
`PREFIX/mmio/something[2]/reg[0-15]St` resulting in:

         DED03BD0F70CEE1ADA33FCD83A8FCE59B6112FB9

Knowing the full register path and prefix, an application may thus compute the hashed record name.

## Arrays of Hubs

Arrays of Hubs, as in

         /mmio/somehub[0-3]/somereg[0-15]

are flattened out. In the above example four PVs would be generated (one for each instance of `somehub`),
each holding 16 values.

Registers, i.e., 'leaves' are always left as arrays.

## Stream data PVs

For each stream data interface, two templates will be loaded. One template will present the data as 16-bit values, while the other one will present the same data as 32-bit values. In both case the data will be available in waveform PVs.

For each waveform PV, a subArray PV will also be loaded, so a subset of data points can be selected. The subArray related to the 16-bit waveform will have a post-fix "SS" (Subarray Short) while the subArray related to the 32-bit waveform will have a post-fix "SL" (Subarray Long).

## Debug information

when the IOC runs, it creates 4 files on the /tmp directory:
- `<PORT_NAME>_<PREFIX>_regMap.txt`         with all the register that it found on the yaml file
- `<PORT_NAME>_<PREFIX>_regMap.yaml`        with all the register that it found on the yaml file (alternate, yaml format)
- `<PORT_NAME>_<PREFIX>_pvList.txt`         with all the names of the PVs that it created
- `<PORT_NAME>_<PREFIX>_keysNotFound.txt`   with the name of devices that it didn’t find in the maps

<PORT_NAME> is the port name passed to YCPSWASYNConfig(). <PREFIX> is the record prefix
passed to YCPSWASYNConfig() (`_` separator is omitted if PREFIX is empty).

## Record types and fields

The driver will automatically create an appropriate record type for the type of register it find. Also, it will take information for the YAML description files and copied it into the appropriate record fields.

The driver will load the records from templates located on the <TOP>/db folder, according to the following table

| Register class         | nelms  | Encoding   | Enum  | Register mode   | template
|:-----------------------|:-------|:----------:|:-----:|:---------------:|:-------------------------
| IntField               | 1      |            |       | RO              | longin.template
| IntField               | >1     |            |       | RO              | waveform_in.template, waveform_8_in.template
| IntField               | 1      |            |       | RW              | longout.template
| IntField               | >1     |            |       | RW              | waveform_out.template, waveform_8_out.template
| IntField               | N/A    |            | Yes   | RO              | bi.template, mbbi.template
| IntField               | N/A    |            | Yes   | RW              | bo.template, mbbo.template
| IntField               | N/A    | IEEE_754   |       | RO              | ai.template
| IntField               | N/A    | IEEE_754   |       | RW              | ao.template
| SequenceCommand        | N/A    |            |       | N/A             | bo.template
| IntField (stream port) | N/A    |            |       | N/A             | waveform_stream32.template, waveform_stream16.template
