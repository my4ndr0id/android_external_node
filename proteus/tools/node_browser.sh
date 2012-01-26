#!/bin/bash
set -x

# FIXME: $# doesnt work in this case since the input is passed as one string
args=0
for i in $*
do
args=`expr $args + 1`
done

# kill if browser is running
pid=$(adb shell ps |grep browser |awk '{print $2}')
if [ "$pid" ]; then
echo "** Killing browser"
adb shell kill -9 $pid
adb shell am start -a android.intent.action.VIEW -n com.android.browser/.BrowserActivity
sleep 1
pid=$(adb shell ps |grep browser |awk '{print $2}')
adb shell kill -9 $pid
fi

mkdir .tmp

# ics browser seems to not reload if opening the same page, so we need to create different html
# if there are >1 arguments
for i in $*
do

if [ $args -gt 1 ]; then
file=`expr $i : '.*/\([a-zA-Z0-9_\.\-]*$\)'`.html
else
file=proteus.html
fi

echo "
<script>
function onLoad() {
  navigator.loadModule('test', function(module) {
      module.runTest('$i');
      });
}
</script>
<body onload='onLoad()'>
</body>
" |tee .tmp/$file

adb push .tmp/$file /data/$file
adb shell am start -a android.intent.action.VIEW -n com.android.browser/.BrowserActivity -d file:///data/$file

# This allows for the tab creation
sleep 1

done

sleep 3

echo "** Killing browser"
pid=$(adb shell ps |grep browser |awk '{print $2}')
adb shell kill -9 $pid

if [ $args -gt 1 ]; then
# launch and kill again to get around the issue of ics caching the previous state of browser
# it doesnt cache the second crash :)
adb shell am start -a android.intent.action.VIEW -n com.android.browser/.BrowserActivity
sleep 1
pid=$(adb shell ps |grep browser |awk '{print $2}')
adb shell kill -9 $pid
fi
