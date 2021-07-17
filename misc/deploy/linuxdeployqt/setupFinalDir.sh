#!/bin/bash

# Execute from build dir ../misc/deploy/linuxdeployqt/setupFinalDir.sh fileName
#
# This script makes a standalone ZIP package for distribution on Linux.
#
# Tested on Ubuntu 18 x64

name=$1
if [ "$name" == "" ]
then
  name=Alenka-Linux-Appimage
fi

folder=`mktemp -d -p .`

echo -e Deploying to $name'\n'

mkdir -p $folder/$name/log
mkdir -p $folder/$name/montageTemplates

cp -v Alenka-x86_64.AppImage $folder/$name &&
  alenka=OK || alenka=fail

echo '#!/bin/sh

DIR=`dirname $0`
export LD_LIBRARY_PATH=$DIR
' > $folder/$name/Alenka

echo '#!/bin/sh

DIR=`dirname $0`
export LD_LIBRARY_PATH=$DIR:$AMDAPPSDKROOT/lib/x86_64/sdk
' > $folder/$name/Alenka-AMD

echo '$DIR/Alenka-x86_64.AppImage "$@"' | tee -a $folder/$name/Alenka >> \
  $folder/$name/Alenka-AMD
chmod u+x $folder/$name/Alenka*

cp -v "`dirname $0`/README" $folder/$name/README
cp -v "`dirname $0`/../options.ini" $folder/$name
cp -v "`dirname $0`/../montageHeader.cl" $folder/$name
cp -v "`dirname $0`/../montageTemplates"/* $folder/$name/montageTemplates
cp -v "`dirname $0`/../../../LICENSE.txt" $folder/$name

cd $folder &&
  zip -r $name.zip $name &&
  cd - &&
  mv $folder/$name.zip . &&
  zip=OK || zip=fail

rm -r $folder

echo
echo ========= Deployment summary =========
echo "Files                   Status"
echo ======================================
echo "Alenka                  $alenka"
echo "zip                     $zip"
