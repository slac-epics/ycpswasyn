#!/bin/bash
edm -x -m "P=221-ATCA1" trigger.edl &
edm -x -m "P=221-ATCA1" buffers.edl &
edm -x -m "P=221-ATCA1" attenuators.edl &
edm -x -m "P=221-ATCA1" dacs.edl &
