#!/sbin/ext/busybox sh

if [ ! -f /data/.Abyss/backupefs.tar.gz ];
then
  mkdir /data/.Abyss
  chmod 777 /data/.Abyss
  /sbin/ext/busybox tar zcvf /data/.Abyss/backupefs.tar.gz /efs
  /sbin/ext/busybox cat /dev/block/mmcblk0p1 > /data/.Abyss/efsdev-mmcblk0p1.img
  /sbin/ext/busybox gzip /data/.Abyss/efsdev-mmcblk0p1.img
  #make sure that sdcard is mounted, media scanned..etc
  (
    sleep 500
    /sbin/ext/busybox cp /data/.Abyss/efs* /sdcard
  ) &
fi

