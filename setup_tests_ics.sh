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
UT=false
CAMERA=false
MODLOADER=false
UNZIP=false
PERMISSION=false
FILESYSTEM=false
DEVICEINFO=false
PROTEUS_TESTS=false
for i in $*
do
   case $i in
       -u|--unit-tests)
               UT=true
               ;;
       -c|--camera)
               CAMERA=true
              ;;
       -ml|--mod-loader)
              MODLOADER=true
               ;;
       -p|--perm)
               PERMISSION=true
               ;;
       -uz|--unzip)
               UNZIP=true
               ;;
       -di|--device-info)
               DEVICEINFO=true
               ;;
       -f|--filesystem)
               FILESYSTEM=true
               ;;
       -a|--all)
               UT=true
               UNZIP=true
               CAMERA=true
               MODLOADER=true
               PERMISSION=true
               FILESYSTEM=true
               DEVICEINFO=true
               ;;
       *)
                echo "Invalid option: " $i
                exit
                ;;
   esac
done

set -x
adb shell am start -a android.intent.action.VIEW -n com.android.browser/.BrowserActivity

# create directories
PPATH=/data/data/com.android.browser/.dapi
DPATH=/data/data/com.android.browser/.dapi/downloads
adb shell mkdir $PPATH
adb shell mkdir $DPATH

# change permissions to the app
APPID=`adb shell ps |grep browser |awk '{print $1}'`
if [ "$APPID" != "" ]; then
adb shell chown $APPID $PPATH
adb shell chown $APPID $DPATH
fi

adb shell mkdir $DPATH/public-test
adb push public-test $DPATH/public-test

if $UT; then
  adb shell mkdir $DPATH/test
  adb shell mkdir $DPATH/test/simple
  adb push test/simple $DPATH/test/simple
  adb push test/common.js $DPATH/test

  adb shell mkdir $DPATH/test/fixtures
  adb push test/fixtures $DPATH/test/fixtures

  adb shell mkdir $DPATH/test/tmp
  adb shell chown $APPID $DPATH/test/tmp

  # push proteus tests..
  adb shell mkdir $DPATH/test/proteus
  adb push test/proteus $DPATH/test/proteus

  adb shell mkdir $DPATH/public-test
  adb push public-test $DPATH/public-test

  adb push core_tests.html /data/
fi

if $MODLOADER; then
  adb push modules/proteus/proteusModLoader/test/proteusModLoaderTest.js $DPATH
  adb push modules/proteus/proteusModLoader/test/proteusModLoaderTest.html /data/
fi

# unzip
if $UNZIP; then
  UNZIPPATH=$DPATH/proteusUnzip/test
  adb shell mkdir $DPATH/proteusUnzip
  adb shell mkdir $DPATH/proteusUnzip/test
  adb push modules/proteus/proteusUnzip/test $UNZIPPATH
  adb push modules/proteus/proteusUnzip/test/unzip.html /data/
fi

if $DEVICEINFO; then
  DEVICEINFOPATH=$DPATH/proteusDeviceInfo/test
  adb shell mkdir $DPATH/proteusDeviceInfo
  adb shell mkdir $DPATH/proteusDeviceInfo/test
  adb push modules/proteus/proteusDeviceInfo/test $DEVICEINFOPATH
  adb push modules/proteus/proteusDeviceInfo/test/proteusDeviceInfoTest.html /data/
fi

# permissions
if $PERMISSION; then
  PPATH=$DPATH/public-permission
  adb shell rm -r $PPATH
  adb shell mkdir $PPATH
  adb shell mkdir $PPATH/bin
  adb shell mkdir $PPATH/lib
  adb push modules/proteus/permission/package.json $PPATH
  adb push modules/proteus/permission/lib/permission.js $PPATH/lib/
  adb push ../../out/target/product/msm8660_surf/symbols/system/lib/permission.so $PPATH/bin/
  adb push modules/proteus/permission/test/FeatureWebapp.html /data/
  adb push modules/proteus/permission/test/Navigator.html /data/
  adb push modules/proteus/permissionUI/test/pemissionUITest.html /data/
  adb push modules/proteus/testPermissions/test/testpermissions.html /data/
fi

# To run modloader test
# adb shell am start -a android.intent.action.VIEW -n com.android.browser/.BrowserActivity -d file:///data/proteusModLoaderTest.html

# To run unzip test
# adb shell am start -a android.intent.action.VIEW -n com.android.browser/.BrowserActivity -d file:///data/unzip.html

# run permissions
# adb shell am start -a android.intent.action.VIEW -n com.android.browser/.BrowserActivity -d file:///data/FeatureWebapp.html
# adb shell am start -a android.intent.action.VIEW -n com.android.browser/.BrowserActivity -d file:///data/Navigator.html

if $CAMERA; then
  adb shell mkdir $DPATH/public-camera
  adb shell mkdir $DPATH/public-camera/lib
  adb shell mkdir $DPATH/public-camera/bin
  adb push ../../out/target/product/msm8660_surf/system/lib/camera_bindings.so $DPATH/public-camera/bin
  adb push modules/proteus/proteusCamera/package.json $DPATH/public-camera/
  adb push modules/proteus/proteusCamera/lib/camera-api.js $DPATH/public-camera/lib
  adb push modules/proteus/proteusCamera/test /data/
fi
#adb shell am start -a android.intent.action.VIEW -n com.android.browser/.BrowserActivity -d file:///data/camera-vnode.html

if $FILESYSTEM; then
  FSPATH=$DPATH/proteusFS
  adb shell mkdir $FSPATH
  adb shell mkdir $FSPATH/lib
  adb push modules/proteus/proteusFS/package.json $FSPATH
fi

# kill the browser
pid=$(adb shell ps |grep browser |awk '{print $2}')
adb shell kill -9 $pid

