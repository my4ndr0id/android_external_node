#!/bin/bash
set -x

function kill_() {
  adb shell am start -a android.intent.action.VIEW -n com.android.browser/.BrowserActivity;
  sleep 2;
  pid=$(adb shell ps |grep browser |awk '{print $2}');
  adb shell kill -9 $pid;
}

# kill browser twice before running test to clear any peristance url cache
kill_
kill_

cd modules/proteus/proteusCamera/test/
for file in `ls *.html`; do
 adb shell am start -a android.intent.action.VIEW -n com.android.browser/.BrowserActivity -d file:///data/$file
 sleep 5
done
