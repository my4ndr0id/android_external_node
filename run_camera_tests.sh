##
 # Copyright (c) 2011-2012, Code Aurora Forum. All rights reserved.
 #
 # Redistribution and use in source and binary forms, with or without
 # modification, are permitted provided that the following conditions are
 # met:
 #     * Redistributions of source code must retain the above copyright
 #       notice, this list of conditions and the following disclaimer.
 #     * Redistributions in binary form must reproduce the above
 #       copyright notice, this list of conditions and the following
 #       disclaimer in the documentation and/or other materials provided
 #       with the distribution.
 #     * Neither the name of Code Aurora Forum, Inc. nor the names of its
 #       contributors may be used to endorse or promote products derived
 #       from this software without specific prior written permission.
 #
 # THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 # WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 # MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 # ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 # BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 # CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 # SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 # BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 # WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 # OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 # IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
##

#!/bin/bash

function kill_() {
  adb shell am start -a android.intent.action.VIEW -n com.android.browser/.BrowserActivity;
  sleep 2;
  pid=$(adb shell ps |grep browser |awk '{print $2}');
  adb shell kill -9 $pid;
}

# kill browser twice before running test to clear any peristance url cache
# kill_
# kill_

for i in $*
do
   case $i in
       -md=*|--module-dir=*)
               MODULE_DIR=`echo $i | sed 's/[-a-zA-Z0-9]*=//'`
               ;;
   esac
done

if [ "$MODULE_DIR" == "" ]; then
  MODULE_DIR=../../dapi/modules
fi

if [ ! -d $MODULE_DIR ]; then
  echo "module directory \"$MODULE_DIR\" not present, specify it with --module-dir="
  exit
fi


cd $MODULE_DIR/camera/test/
for file in `ls *.html`; do
 adb shell am start -a android.intent.action.VIEW -n com.android.browser/.BrowserActivity -d file:///data/$file
 sleep 5
done
