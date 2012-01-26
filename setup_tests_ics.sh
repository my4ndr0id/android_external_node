UT=false
CAMERA=false
MODLOADER=false
UNZIP=false
PERMISSION=false
FILESYSTEM=false
DEVICEINFO=false
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
PPATH=/data/data/com.android.browser/.proteus
DPATH=/data/data/com.android.browser/.proteus/downloads
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
  adb push test/proteus/permissions/FeatureWebapp.html /data/
  adb push test/proteus/permissions/Navigator.html /data/
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

