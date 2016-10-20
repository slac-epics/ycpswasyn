#!/bin/bash
edm -x -m "P=yamlIOC1" trigger.edl &
edm -x -m "P=yamlIOC1" buffers.edl &
edm -x -m "P=yamlIOC1" attenuators.edl &
edm -x -m "P=yamlIOC1" dacs.edl &
edm -x -m "P=yamlIOC1" defaults.edl &
