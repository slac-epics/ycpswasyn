#!/bin/bash
#for i in $( cat 000TopLevel.yaml | tr ' ' '\n' | grep .yaml ); do
#  
#end
#
#
expandYaml () {
  for i in $( cat $1 | tr ' ' '\n' | grep .yaml ); do
    expandYaml $i
    #arr=("${arr[@]}" "$i")
#    cat $i >> expanded.yaml
  done
#  cat $1 >> expanded.yaml
  arr=("${arr[@]}" "$1")
  return 0
}

declare -a arr
rm -f merge.yaml

expandYaml 000TopLevel.yaml
#make sort for unique elements, don't change order
arr=($(echo ${arr[@]} | tr [:space:] '\n' | awk '!a[$0]++'))
for i in ${arr[@]}; do
  cat $i >> merge.yaml
  echo -e '\n' >> merge.yaml
done
