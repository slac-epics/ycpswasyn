#!../../bin/linuxRT_glibc-x86_64/ycpswasyn

## You may have to change ycpswasyn to something else
## everywhere it appears in this file

#< envPaths

# ========================================================
# Support Large Arrays/Waveforms; Number in Bytes
# Please calculate the size of the largest waveform
# that you support in your IOC.  Do not just copy numbers
# from other apps.  This will only lead to an exhaustion
# of resources and problems with your IOC.
# The default maximum size for a channel access array is
# 16K bytes.
# ========================================================
# Uncomment and set appropriate size for your application:
epicsEnvSet("EPICS_CA_MAX_ARRAY_BYTES", "21000000")

## Register all support components
dbLoadDatabase("../../dbd/ycpswasyn.dbd",0,0)
ycpswasyn_registerRecordDeviceDriver(pdbbase) 

## Load record instances

## Configure asyn port driver
# YCPSWASYNConfig(
#    Port Name,                 # the name given to this port driver
#    Yaml Doc,                  # Path to the YAML file
#    Root Path                  # OPTIONAL: Root path to start the generation. If empty, the Yaml root will be used
#    IP Address,                # OPTIONAL: Target FPGA IP Address. If not given it is taken from the YAML file
#    Record name Prefix,        # Record name prefix
#    Record name Length Max,    # Record name maximum length (must be greater than lenght of prefix + 4)
#YCPSWASYNConfig("Atca2", "../../yaml/AmcCarrierSsrlEth2x_project.yaml/000TopLevel.yaml", "", "10.0.0.102", "yamlIOC1", 40)
YCPSWASYNConfig("Atca2", "../../yaml/AmcCarrierSsrlEth2x_project.yaml/000TopLevel.yaml", "", "", "yamlIOC1", 40)

#asynSetTraceMask(Atca1,, -1, 9)
#asynSetTraceIOMask(Atca1,, -1, 2)
asynSetTraceMask(Atca1,, -1, 0)

iocInit()

## Start any sequence programs
#seq sncExample,"user=jvasquez"

# This register gives timeout errors when it is read.
# For now let's not read it until this problem is fixed
# register path: /mmio/AmcCarrierSsrlRtmEth/AmcCarrierCore/Axi24LC64FT/MemoryArray
dbpf yamlIOC1:C:A24LC64FT:MemoryArray:Rd.SCAN "Passive"

