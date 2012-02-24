#!/bin/bash
set -x

DIR=""
OPTIONS=""
JSBEAUTIFY=""
INDENT=2

help() {
  echo "beautify.sh -b=<jsbeautifier python script> -d=<dir_with_js_files_to_beautify> <js-beautify-options>"
  echo "e.g beautify.sh -b=/local/mnt/workspace -d=modules/proteus/proteusCamera/lib -d"
  exit
}

for i in $*
do
   case $i in
        -d=*|--dir=*)
		DIR=`echo $i | sed 's/[-a-zA-Z0-9]*=//'`
		;;
        -b=*|--jsbeautifier=*)
		JSBEAUTIFY=`echo $i | sed 's/[-a-zA-Z0-9]*=//'`
		;;
         -h|--help)
                help
		;;
         *)
                OPTIONS="$OPTIONS $i"
                ;;
   esac
done

echo $DIR
echo $JSBEAUTIFY

if [ $DIR == "" ] || [ $JSBEAUTIFY == "" ]; then
help
fi

echo "Options passed to js-beautify at " $JSBEAUTIFY " with options: " $OPTIONS

cd $DIR
for file in `ls *.js`; do
  echo "Beautifying: " $file
  cp $file .$file
  $JSBEAUTIFY -s2 $OPTIONS $file > tmpfile
  if [ -s tmpfile ]; then
    cp tmpfile $file
    rm tmpfile
    echo "Beautified: " $file
  else
    echo "FILE empty something wrong"
  fi
done
