if /sbin/ext/busybox [ ! -f /system/cfroot/release-100-KJ4- ]; 
then
# Remount system RW
    /sbin/ext/busybox mount -o remount,rw /system

# ensure /system/xbin exists
    toolbox mkdir /system/xbin
    toolbox chmod 755 /system/xbin

# su
    toolbox rm /system/bin/su
    toolbox rm /system/xbin/su
    toolbox cat /res/misc/su > /system/xbin/su
    toolbox chown 0.0 /system/xbin/su
    toolbox chmod 6755 /system/xbin/su

# Superuser
    toolbox rm /system/app/Superuser.apk
    toolbox rm /data/app/Superuser.apk
    toolbox cat /res/misc/Superuser.apk > /system/app/Superuser.apk
#    /sbin/ext/busybox dd if=/dev/block/mmcblk0p5 of=/system/app/Superuser.apk skip=7000000 seek=0 bs=1 count=762010
    toolbox chown 0.0 /system/app/Superuser.apk
    toolbox chmod 644 /system/app/Superuser.apk

# CWM Manager

    toolbox rm /system/app/CWMManager.apk
    toolbox rm /data/dalvik-cache/*CWMManager.apk*
    toolbox rm /data/app/eu.chainfire.cfroot.cwmmanager*.apk

    toolbox cat /res/misc/CWMManager.apk > /system/app/CWMManager.apk
    toolbox chown 0.0 /system/app/CWMManager.apk
    toolbox chmod 644 /system/app/CWMManager.apk

# Once be enough
    toolbox mkdir /system/cfroot
    toolbox chmod 755 /system/cfroot
    toolbox rm /data/cfroot/*
    toolbox rmdir /data/cfroot
    toolbox rm /system/cfroot/*
    echo 1 > /system/cfroot/release-100-KJ4- 

# Remount system RO
    /sbin/ext/busybox mount -o remount,ro /system
fi;
