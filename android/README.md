# Android APKs with the commandline

## Install Java JDK 17

Download [Java Development Kit 17 - JDK](https://www.openlogic.com/openjdk-downloads) from the linked page.

**NOTE:** It's important to have version 17. I could not complete the APK build process with other (older and newer) versions.

Unzip java in a directory of your preference. Setting the `JAVA_HOME` environment var and adding the `bin` subdirectory to the `PATH` environment variable can come in handy, but it's not required for the process (plus you might not want to pollute the environment variables).

## Install Android SDK

### Bootstrap ANDROID_HOME

Download the [Android commandline tools](https://developer.android.com/studio) from the linked page (look for the "commandline tools" section).

Unzip the **Commandline tools** in a temporary directory (we will remove this directory later).

Open a command terminal and navigate to the the temporary directory. Go to subdirectory `cmdline-tools\bin`:

```
# This is where we will install the Android SDK
set ANDROID_HOME=C:\Users\jesus\Soft\android-sdk

android --sdk=%ANDROID_HOME% sdk install cmdline-tools/latest
```

The bootstrap at `ANDROID_HOME` is ready. You can safely remove the temporary directory containing the commandline tools.

### Populate ANDROID_HOME

Open a command terminal and navigate to the `ANDROID_HOME` directory. Go to subdirectory `cmdline-tools\latest\bin`:

```
# The android tool finds the SDK through ANDROID_HOME (or pass --sdk=<path>)
set ANDROID_HOME=C:\Users\jesus\Soft\android-sdk

# Install build tools: aapt, aapt2, d8, zipalign, apksigner...
android sdk install build-tools/36.0.0

# Install platform tools: adb...
android sdk install platform-tools

# NDK and SDK development kits
android sdk install ndk/30.0.16248370
android sdk install platforms/android-36

# Check what is installed / available
android sdk list
android sdk list --all "ndk*"
```

`android sdk` replaces the older `sdkmanager` tool, which is still shipped in `cmdline-tools\latest\bin`. Package names use `/` instead of the `;` used by `sdkmanager` (`build-tools/36.0.0` vs `"build-tools;36.0.0"`).

These are the package versions the build was last tested with:

| Package                | Version        |
|------------------------|----------------|
| `build-tools`          | 36.0.0         |
| `cmdline-tools/latest` | 23.0.0         |
| `ndk`                  | 30.0.16248370  |
| `platform-tools`       | 37.0.1         |
| `platforms/android-36` | 2.0.0          |

The build scripts don't detect these versions. If you install different ones, update the paths in `environment.bat` (`ANDROID_PLATFORM_DIR`, `NDK`, `BUILD_TOOLS`).

**NOTE 1:** The **build-tools** version must be at least as new as the **platform** (`android.jar`) version. Older `aapt`/`aapt2` cannot parse the resource table of newer `android.jar` files and spam alignment / out of bounds errors.

**NOTE 2:** Older versions of the **build-tools** had an issue in their **d8** script, which was passing the deprecated argument `-Djava.ext.dirs` to java (only accepted by previous versions of the JDK). This is fixed in recent versions.

**NOTE 3:** Once the commandline tools are installed inside `ANDROID_HOME`, `android` finds the SDK through the `ANDROID_HOME` variable, so `--sdk` is only needed for the bootstrap step.

Close the command line. We have everything we need to compile and deploy the Android app.


## Build and install

From the `android` directory:

```
build.bat          # compile the native libraries (libgame.so, libengine.so)
build.bat apk      # package, align and sign bin\NativeActivity.apk
build.bat install  # install the APK and push shaders.dat / assets.dat to the device
```

The application ID (`com.ilu.game`) is set by `PACKAGE_NAME` in `environment.bat` / `environment.sh`. The Java package of `MainActivity` is declared separately in `src/com/ilu/game/MainActivity.java` and in `AndroidManifest.xml`.


## Prepare your phone for debugging

### Enable debugging

- Make sure to have USB debugging enabled on your phone:
  - [Unlock the developer options](https://developer.android.com/studio/debug/dev-options).
- Enable the option *USB debugging*
- Enable the option *Wireless debugging* (for debugging over wireless connections)

### Play Protect

Recent Android versions may block the install with `INSTALL_FAILED_VERIFICATION_FAILURE` and a message saying the app was built for an older Android version. Keep `targetSdkVersion` in `AndroidManifest.xml` up to date (currently 36). If installs over USB are still blocked, disable *Verify apps over USB* in the developer options.

### Renderdoc captures

On some devices, the global `VK_LAYER_RENDERDOC_Capture` layer doesn't seem to be visible by Vulkan and it returns a `VK_ERROR_LAYER_NOT_PRESENT` when calling `vkCreateInstance`.

To fix this, open the developer options on your phone and:
- Enable the option *Disable permission monitoring*.

### Other useful options

- Enable the option *Keep screen on while charging* or *Stay awake*

## Debugging an APK

- Make sure your APK is debuggable:
  - Open your `AndroidManifest.xml` file.
  - The `application` tag needs the `android:debuggable="true"` attribute.
- Make the APK and install it in your device.
- Again, in the developer options on your phone:
  - Press *Select debug app* and select your app here.
  - Enable the *Wait for debugger* option.

## Debugging commands

```
adb shell pm dump com.ilu.game | grep versionCode

Server:
(pc) LLDB_SERVER_DIR=${TOOLCHAIN}/lib64/clang/14.0.7/lib/linux/aarch64
(pc) pushd ${LLDB_SERVER_DIR}
(pc) ${ADB} push lldb-server /data/local/tmp
(pc) popd
(pc) adb shell
(device) cd /data/local/tmp
(device) ./lldb-server platform --listen "*:9999" --server

Client:
(pc) adb devices // Note down the name of the device
(pc) adb shell pidof com.ilu.game // Note down the pid of the process
(pc) adb forward tcp:9999 tcp:9999
(pc) lldb.sh
(lldb) platform select remote-android
(lldb) platform connect connect://YTLNLJEYTGVGJBH6:9999
(lldb) platform process attach --pid <pid-number> // fails with "Operation not permitted"
```

### Server terminal
```
./debug.sh
```

### Client terminal 1
```
lldb
(lldb) platform select remote-android
(lldb) platform connect unix-abstract-connect:///com.ilu.game/debug.socket
(lldb) attach <pid>
```

### Client terminal 2
```
adb forward tcp:12345 jdwp:<pid>
jdb -attach localhost:12345
```

### Client terminal 1 (again)
```
(lldb) continue
// Then the application crashes
```

## Some reference links links

* [Android NDK](https://developer.android.com/ndk/downloads)
* [Android SDK Platform tools](https://developer.android.com/studio/releases/platform-tools)
* [System.loadLibrary() couldn't find native librery](https://stackoverflow.com/questions/27421134/system-loadlibrary-couldnt-find-native-library-in-my-case)
* [AAPT2](https://developer.android.com/studio/command-line/aapt2)
* [AAPT Issue](https://stackoverflow.com/questions/23522153/manually-aapt-add-native-library-so-to-apk)
* [Debugging C++ of an APK using lldb from the console](https://stackoverflow.com/questions/53448796/debugging-c-code-of-an-android-app-using-lldb-from-the-console)
* [Debugging C++ of an APK using lldb from the console 2](https://stackoverflow.com/questions/53733781/how-do-i-use-lldb-to-debug-c-code-on-android-on-command-line)
* [lldb-server manual page](https://lldb.llvm.org/man/lldb-server.html)
* [lldb manual page](https://lldb.llvm.org/man/lldb.html)
* [Debugging Android Native with LLDB](https://www.lili.kim/2019/01/28/android/Debug%20Android%20Native%20with%20LLDB/)
* [Useful thread where the last question is key](https://stackoverflow.com/questions/53733781/how-do-i-use-lldb-to-debug-c-code-on-android-on-command-line)
