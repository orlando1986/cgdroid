cd src/

javac -cp ~/adt-bundle-linux-x86_64/sdk/platforms/android-18/android.jar  com/assquad/inject/Root*.java
~/adt-bundle-linux-x86_64/sdk/build-tools/17.0.0/dx --dex --output=root.jar com/assquad/inject/Root*.class

mv root.jar ../assets/

cd ../jni
ndk-build
#mv ../libs/armeabi/libhook.so ../assets/
#mv ../libs/armeabi/inj ../assets/
cd ..
