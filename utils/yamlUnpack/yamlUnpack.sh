#! /bin/sh

export PATH=/afs/slac/g/lcls/package/python/python2.7.9/linux-x86_64/bin:$PATH
export LD_LIBRARY_PATH=/afs/slac/g/lcls/package/python/python2.7.9/linux-x86_64/lib:$LD_LIBRARY_PATH

. "`pwd`/scripts/array"

expandYaml () {
  dos2unix $DIR/$1
  for i in $( cat $DIR/$1 | grep -hv ^[[:space:]]*# | tr ' ' '\n' | grep .yaml ); do
    expandYaml $i
  done
  arr=$(array_append "$arr" "$DIR/$1" )
  return 0
}

usage() {
    echo "usage: yamlUnpack.sh -t <yaml.tar.gz> -d <destination>"
    echo "    -t <yaml.tar.gz>  : tar file with yaml defintions"
    echo "    -d <destination>  : path to destination folder"
    exit
}

ARGS=""

while [[ $# -gt 1 ]]
do
key="$1"
case $key in
    -t)
    TAR_FILE="$2"
    shift # past argument
    ;;
    -d)
    DEST_PATH="$2"
    shift # past argument
    ;;
    *)
            # unknown option
    ;;
esac
shift
done

if [ ! -f "$TAR_FILE" ]
then
    echo "Tar file not found!"
    usage
fi

if [ ! -d "$DEST_PATH" ] 
then 
    echo "Destination path not defined!"
    usage
fi

TEMP_DIR=/tmp/testing
rm -rf $TEMP_DIR
mkdir -p $TEMP_DIR

tar -zxf  $TAR_FILE -C $TEMP_DIR

#declare -a arr
PROJ_DIR=$(ls /tmp/testing)
DIR=$TEMP_DIR/$PROJ_DIR

echo $DIR

#rm -rf $DIR/merge.yaml
#rm -rf $DIR/system.yaml

expandYaml 000TopLevel.yaml
#sort for unique elements, don't change order
arr1=$(echo $arr | tr [:space:] '\n' | awk '!a[$0]++')


#echo "$arr" | while IFS= read element; do 
#  cat "$(echo "$element" | array_element_decode)" >> $DIR/merge.yaml 
#  printf '\n' >> $DIR/merge.yaml
#done
IFS_ORIG=$IFS
IFS=' '
echo $arr1 | while IFS= read element; do
  cat $element >> $DIR/merge.yaml;
  printf '\n' >> $DIR/merge.yaml
##  echo -e '\n' >> $DIR/merge.yaml
done
IFS=$IFS_ORIG
ret=$(python scripts/expand_yaml.py -f $DIR/merge.yaml -s $DIR/system.yaml)

printf "%s " $ret
printf "\n"

mv $DIR/system.yaml $DEST_PATH

printf "File system.yaml created on $DEST_PATH\n"
printf "Done!\n"

