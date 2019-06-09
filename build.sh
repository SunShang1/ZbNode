#!/bin/bash

usage()
{
    echo usage: build.sh project_name
    exit
}

# 
if [ $# != 1 ]; then
    usage
fi

ROOT_DIR=$PWD
OTAHEAD_DIR=Tools/otahead

PROD=$1
releaseFile=Proj/$PROD/App/release.h
projFile=Proj/$PROD/ide/$PROD.ewp

if [ ! -f $releaseFile ] || [ ! -f $projFile ]; then
    echo project $PROD is not found!
    usage
fi
SW_VER_MAJOR=`cat $releaseFile | grep SW_VER_MAJOR | awk '{print $3}'`
SW_VER_MINOR=`cat $releaseFile | grep SW_VER_MINOR | awk '{print $3}'`

GITSHA=`./getGitSHA.sh ./`

GITSHA_HEX0=`echo $GITSHA | cut -c1-2`
GITSHA_HEX1=`echo $GITSHA | cut -c3-4`
GITSHA_HEX2=`echo $GITSHA | cut -c5-6`
GITSHA_HEX3=`echo $GITSHA | cut -c7-8`
GITSHA_HEX=0x$GITSHA_HEX0,0x$GITSHA_HEX1,0x$GITSHA_HEX2,0x$GITSHA_HEX3

BUILD_DATA=`date +'%y%m%d'`

SW_VER_STR=$SW_VER_MAJOR.$SW_VER_MINOR-g$GITSHA

OUT_FILE_HEX=$PROD-v$SW_VER_STR-$BUILD_DATA.hex
OUT_FILE_SIM=$PROD-v$SW_VER_STR-$BUILD_DATA.sim


#set out put file name
lineOfName=`sed -n '/<name>OutputFile/=' $projFile | sed -n 6p`
lineOfName=`expr $lineOfName + 1`
sed -i -e "$lineOfName s/\(<state>\).*\(<\/state>\)/\1$OUT_FILE_HEX\2/" $projFile

#set extra out put file name
lineOfName=`sed -n '/<name>ExtraOutputFile/=' $projFile | sed -n 2p`
lineOfName=`expr $lineOfName + 1`
sed -i -e "$lineOfName s/\(<state>\).*\(<\/state>\)/\1$OUT_FILE_SIM\2/" $projFile

#set out put file format
lineOfName=`sed -n '/<name>DebugInformation/=' $projFile | sed -n 2p`
lineOfName=`expr $lineOfName + 1`
sed -i -e "$lineOfName s/\(<state>\).*\(<\/state>\)/\11\2/" $projFile

# if the project is ZbNode
if [ "$PROD" == "ZbNode" ]; then
    cd $OTAHEAD_DIR;
    make clean
    make
    cd $ROOT_DIR
fi

AUTO_GEN_FILE=Proj/$PROD/App/autogen.h
echo "/* This file is auto generated */" > $AUTO_GEN_FILE
echo >> $AUTO_GEN_FILE
echo "#ifndef __AUTOGEN_H__" >> $AUTO_GEN_FILE
echo "#define __AUTOGEN_H__" >> $AUTO_GEN_FILE
echo >> $AUTO_GEN_FILE

echo "#define SOFT_SHA  \"$GITSHA\"" >> $AUTO_GEN_FILE
echo "#define SOFT_SHA_HEX  $GITSHA_HEX" >> $AUTO_GEN_FILE
echo >> $AUTO_GEN_FILE
echo "#endif" >> $AUTO_GEN_FILE

exit 0
