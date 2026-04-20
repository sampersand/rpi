#!/bin/sh

set -euC

if [ $METHOD != GET ]; then
	status 405 true
	exit
fi

readonly UNAME=`uname`
readonly TEMPERATURE_FILE=/sys/class/thermal/thermal_zone0/temp

h DOCTYPE html head [ title -t status ] body
h h1 -T "Status of the PI"

h table -a border=1 -a cellpadding=10 -a cellspacing=0 \
	thead [ tr [ th -t{Metric,Value,Status} ] ] \
	tbody

fp_lt () { test "$(echo "$*" | bc)" -eq 1; }
setstatus () {
	if fp_lt "$1 < $2"; then
		echo Normal
	elif fp_lt "$1 < $3"; then
		echo Moderate
	else
		echo High
	fi
}

NumProcs=`ps -ax | wc -l`
h tr [ td -t "Number of procs" -t $NumProcs -t IDK ]

DiskSpace=$(df -h / | awk 'NR==2{printf "%s / %s (%s used)", $3, $4, $5}')
h tr [ td -t "Disk Space" -t "$DiskSpace" -t  ]

h /tbody /table
h /body /html
exit
echo Number of procs:
ps -ax | wc -l
h br/

echo Temperature:
if ( -e $TEMPERATURE_FILE:q ) then
	set Raw_Temp_C = `cat $TEMPERATURE_FILE`
	set Temp_F = `echo "scale=1; ($Raw_Temp_C / 1000 * 1.8) + 32" | bc`
	echo $Temp_fºF
	if ( $Raw_Temp_C > 8000 ) echo '(temp quite hot!)'
else
	echo '&lt;not available&gt;'
endif
h br/

echo Disk Space:
df -h / | awk 'NR==2{printf "%s / %s (%s used)", $3, $4, $5}'
h br/

echo Uptime:
uptime | cut -f1 -d, | sed 's/.*up/up/'
h br/

echo Current Time:
date
h br/

echo Memory Usage:
if ( $UNAME == Darwin ) then
	echo -n "(macos): "
	{ vm_stat; sysctl -n hw.memsize; } | awk '/page size/{p=$8} /Pages (active|wired down):|occupied by compressor/{gsub(/\./,"",$NF); u+=$NF} /^[0-9]+$/{t=$1} END{printf "%.1f%% used\n", (u*p/t)*100}'
endif
h br/

echo CPU Usage:
if ( $UNAME == Darwin ) then
	top -l 1 | awk '/CPU usage/{print}'
else
	top -bn1 | awk '/^%Cpu/{printf "%.1f%% used\n", 100-$8}'
endif
h br/


# Ram

# Load average

# Throttling / Under-voltage (Pi-specific)
# bashvcgencmd get_throttled
# # returns hex flags, 0x0 means all good
# # 0x50005 means currently throttled + under-voltage

# Bandwidth / Network
# bashcat /proc/net/dev   # cumulative bytes per interface
# # to get a rate, you read it twice with a sleep in between


h /body /html

exit

if ( $?HEADER_Accept ) then
	if ( $HEADER_Accept:q == application/json ) then
		header Content-Type application/json

		printf '{"num_procs":%d' $Num_Procs
		if ( $?Temp_f ) then
			printf ',"temp_f":%d' $Num_Procs
			printf  -n ',"temp":'$Temp_f
		exit
	endif
endif

h DOCTYPE html [ \
	head [] \
	body [ \
		table -aborder=1 -acellpadding=10 -acellspacing=0 [ \
			thead [ th -t Metric -t Value ] \
			tbody [ \
				tr [ td -t "Num Procs" -t "$Num_Procs" ] \
				tr [ td -t "another field" -t "foobar" ] \
			] \
		] \
	] ]
