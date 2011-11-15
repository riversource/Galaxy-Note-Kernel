#!/bin/sh

export PATH=$PATH/home/frank_m/android-sdk-linux_x86/linaro/gcc-4.5/android-toolchain-eabi/bin:/home/frank_m/android-sdk-linux_x86/android_source/prebuilt/linux-x86/toolchain/arm-eabi-4.4.3/bin:
export ARCH=arm
#export CROSS_COMPILE=/home/frank_m/android-sdk-linux_x86/android_source/prebuilt/linux-x86/toolchain/arm-eabi-4.4.3/bin/arm-eabi-
export CROSS_COMPILE=/home/frank_m/android-sdk-linux_x86/linaro/gcc-4.5/android-toolchain-eabi/bin/arm-eabi-

make fm_defconfig
make -j8
find . -name *ko -exec arm-eabi-strip --strip-debug {} \;
find . -name *ko -exec cp -av {} ./initramfs_own/lib/modules/ \;
make
