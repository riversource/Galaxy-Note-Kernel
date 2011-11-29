#!/sbin/ext/busybox sh

if [ ! -f /data/.FM/backupefs.tar.gz ];
then
  mkdir /data/.FM
  chmod 777 /data/.FM
  /sbin/ext/busybox tar zcvf /data/.FM/backupefs.tar.gz /efs
  /sbin/ext/busybox cat /dev/block/mmcblk0p1 > /data/.FM/efsdev-mmcblk0p1.img
  /sbin/ext/busybox gzip /data/.FM/efsdev-mmcblk0p1.img
  #make sure that sdcard is mounted, media scanned..etc
  (
    sleep 500
    /sbin/ext/busybox cp /data/.FM/efs* /sdcard
  ) &
fi

