#!/sbin/ext/busybox sh

/sbin/ext/busybox sh /sbin/boot/busybox.sh
#/sbin/ext/busybox sh /sbin/boot/cwm.sh
/sbin/ext/busybox mount rootfs -o remount,rw
/sbin/ext/busybox sh /sbin/boot/backupefs.sh
/sbin/ext/busybox sh /sbin/boot/calibrate.sh
/sbin/ext/busybox sh /sbin/boot/busybox.sh
/sbin/ext/busybox sh /sbin/boot/properties.sh
/sbin/ext/busybox sh /sbin/boot/install.sh
/sbin/ext/busybox sh /sbin/boot/scripts.sh
/sbin/ext/busybox mount rootfs -o remount,ro

read sync < /data/sync_fifo
rm /data/sync_fifo
