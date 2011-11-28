#!/bin/sh

export PATH=$PATH:/home/frank_m/android-sdk-linux_x86/linaro/gcc-4.5/android-toolchain-eabi/bin
#export PATH=$PATH:/home/frank_m/android-sdk-linux_x86/android_source/prebuilt/linux-x86/toolchain/arm-eabi-4.4.0/bin
#export PATH=$PATH:/home/frank_m/android-sdk-linux_x86/codesourcery/arm-2009q3/bin
#export PATH=$PATH:/home/frank_m/android-sdk-linux_x86/codesourcery/arm-2011.03/bin

export ARCH=arm
export CROSS_COMPILE=/home/frank_m/android-sdk-linux_x86/linaro/gcc-4.5/android-toolchain-eabi/bin/arm-eabi-
#export CROSS_COMPILE=/home/frank_m/android-sdk-linux_x86/android_source/prebuilt/linux-x86/toolchain/arm-eabi-4.4.0/bin/arm-eabi-
#export CROSS_COMPILE=/home/frank_m/android-sdk-linux_x86/codesourcery/arm-2009q3/bin/arm-none-linux-gnueabi-
#export CROSS_COMPILE=/home/frank_m/android-sdk-linux_x86/codesourcery/arm-2011.03/bin/arm-none-linux-gnueabi-

make fm_defconfig
make -j8
find . -name *ko -exec ${CROSS_COMPILE}strip --strip-debug {} \;
find . -name *ko -exec cp -av {} ./initramfs_own/lib/modules/ \;
make
