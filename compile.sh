cd src/

javac -cp ~/adt-bundle-linux-x86_64/sdk/platforms/android-18/android.jar  com/android/inject/Hooker*.java
~/adt-bundle-linux-x86_64/sdk/build-tools/17.0.0/dx --dex --output=hook.jar com/android/inject/Hooker*.class

mv hook.jar ../assets/

cd ../jni
ndk-build
mv ../libs/armeabi/libhook.so ../assets/
mv ../libs/armeabi/inj ../assets/
cd ..
