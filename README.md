#libwebsockets
This is a fork repo of libwebsockets (https://github.com/warmcat/libwebsockets), used for Sakura (https://github.com/DaYeSquad/sakura).

##How to build:

##iOS 
(inspired by ofxLibwebsockets https://github.com/labatrockwell/ofxLibwebsockets):

1.Clone libwebsockets

2.Use resources in ofxLibwebsockets/extras/ios_libwebsockets

3.Change line 89 to the iOS version you like

4.Create Xcode project, please notice that you may failed for unkown times by executing below script, don't give up, just try again and again until you make it (I don't know why it fails :) )


```
cd /path/to/libwebsockets
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=/path/to/ofxLibwebsockets/extras/ios_libwebsockets/toolchain/iOS.cmake -GXcode ..
```

5.Replace lws_config.h in your build folder with the one at ofxLibwebsockets/extras/ios_libwebsockets/lws_config.h

6.Open the Xcode project in your build folder

7.Select the target 'websockets' (instead of ALL_BUILD) and select iOS Device as your target

8.Build the project for both simulator and real device

9.Build fat library

```
lipo -create libwebsockets_phone.a libwebsockets_simulator.a -output libwebsockets.a
```

##Android
(config.h和其他头文件已经被放在jni/，在不修改libwebsockets配置时无需修改)

1、修改jni/Android.mk第14行，将其修改为本地的openssl include路径

2、进入根目录，执行ndk-build

```
cd /path/to/libwebsockets
ndk-build
```

3、编译完成的静态链接库在obj/文件夹中
