#!/system/bin/sh
SLEEP=500
 
if [ -e /data/.FM/calibrata ] ; then
        exit 0
fi
 
(
while : ; do
	#in case we might want to cancel the calibration by creating the file
	if [ -e /data/.FM/calibrata ] ; then
        	exit 0
	fi
        LEVEL=$(cat /sys/class/power_supply/battery/capacity)
        CURR_ADC=$(cat /sys/class/power_supply/battery/batt_current_adc)
        STATUS=$(cat /sys/class/power_supply/battery/status)
        BATTFULL=$(cat /sys/class/power_supply/battery/batt_full_check)
        if [ "$LEVEL" == "100" ] && [ "$BATTFULL" == "1" ]; then
                log -p i -t battery-calibration "*** LEVEL: $LEVEL CUR: $CURR_ADC***: calibrando..."
                rm -f /data/system/batterystats.bin
		mkdir /data/.FM
		chmod 777 /data/.FM
                touch /data/.FM/calibrata
                exit 0
        fi
        # log -p i -t battery-calibration "*** LEVEL: $LEVEL CUR: $CUR ***: sleeping for $SLEEP s..."
        sleep $SLEEP
done
) &
