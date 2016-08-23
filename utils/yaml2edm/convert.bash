#!/bin/bash

cd caqtdm

for f in ../edm/*.edl; do
	edl2ui $f
done

sed -i -e 's/.edl/.ui/g' menu.ui
sed -i -e 's/.ui<\/string>/.ui<\/string><\/property><property name=\"args\"><string notr=\"true\">P=$(P)<\/string>/g' menu.ui
