#!/bin/sh

export PATH=$PATH:/home/frank_m/android-sdk-linux_x86/android_source/prebuilt/linux-x86/toolchain/arm-eabi-4.4.3/bin:/home/frank_m/android-sdk-linux_x86/linaro/android-toolchain-eabi/bin
export ARCH=arm
#export CROSS_COMPILE=/home/frank_m/android-sdk-linux_x86/android_source/prebuilt/linux-x86/toolchain/arm-eabi-4.4.3/bin/arm-eabi-
export CROSS_COMPILE=/home/frank_m/android-sdk-linux_x86/linaro/android-toolchain-eabi/bin/arm-eabi-

#make fm_defconfig
make -j8
find . -name *ko -exec arm-eabi-strip --strip-debug {} \;
find . -name *ko -exec cp -av {} /home/frank_m/android-sdk-linux_x86/Samsung/N7000/init/initramfs_own/lib/modules/ \;
make
