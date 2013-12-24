#!/bin/bash

WORK_DIR="/home/oz9aec/funcube-data/"
PROG_DIR="/home/oz9aec/funcube/bin"
INPUT_DIR="$WORK_DIR/in"
RECYCLE_DIR="$WORK_DIR/recycle"
#OUTPUT_DIR containing time stamp is created set later

FILE_PREFIX="usrp_iq_sc_"
FILE_SUFFIX="_96_145900"

LOOP_DELAY=60
LOGFILE="$PROG_DIR/usrp_offline_sequencer.log"

# commands
CMD_CONVERT="$PROG_DIR/usrp_shift_and_resample.py"
CMD_FILTER="$PROG_DIR/filter"
CMD_DECODE="$PROG_DIR/decode"
CMD_SUBMIT="$PROG_DIR/submit.sh"


#while :
#do
	NOW="$(date --utc +%Y%m%d_%H%M%S)"
	
	# Check if there is a new file
	if [ "$(ls -A $INPUT_DIR)" ]; then
		echo "$NOW: New file in $INPUT_DIR " >> $LOGFILE
	else
		echo "$NOW: $INPUT_DIR is empty" >> $LOGFILE

		# wait for the next cycle
		sleep $LOOP_DELAY
		continue
	fi
	
	# we could have more than one file available
	INPUT_FILES="$(ls -A $INPUT_DIR)"

	for INPUT_FILE in ${INPUT_FILES[@]}; do

		# File name format:	IQ_1387217340+8_256000_c_145900000
		# Extract the unix timestamp and convert it to YYYMMDD_HHMMSS format

		
		# extract number between IQ_ and +8_256000_c_14590000
		TIMESTAMP="$(echo $INPUT_FILE | cut -d '_' -f 2 | cut -d '+' -f 1)"
		TIME_STRING="$(date --date=@$TIMESTAMP --utc +%Y%m%d_%H%M%S)"

		OUTPUT_DIR="$WORK_DIR/out/$TIME_STRING"
		mkdir -p $OUTPUT_DIR

		IQ_FILE="$FILE_PREFIX$TIME_STRING$FILE_SUFFIX.raw"
		echo "$NOW: Convert $INPUT_DIR/$INPUT_FILE => $OUTPUT_DIR/$IQ_FILE"  >> $LOGFILE
		
		# Convert to 96 ksps short-complex file
		$CMD_CONVERT --infile="$INPUT_DIR/$INPUT_FILE" --outfile="$OUTPUT_DIR/$IQ_FILE" --offset=-25k -i 96 -d 256  >  $OUTPUT_DIR/convert.log

		# perform filtering with gain 100 then decode
		NOW="$(date --utc +%Y%m%d_%H%M%S)"
		echo "$NOW: Filter and decode (G=100) $OUTPUT_DIR/$IQ_FILE"  >> $LOGFILE
		cat "$OUTPUT_DIR/$IQ_FILE" | $CMD_FILTER -g 100 2> $OUTPUT_DIR/filter100.log | $CMD_DECODE 1> $OUTPUT_DIR/data-100.txt 2>$OUTPUT_DIR/demod-100.log

		# perform filtering with gain 200 then decode
		NOW="$(date --utc +%Y%m%d_%H%M%S)"
		echo "$NOW: Filter and decode (G=200) $OUTPUT_DIR/$IQ_FILE"  >> $LOGFILE
		cat "$OUTPUT_DIR/$IQ_FILE" | $CMD_FILTER -g 200 2> $OUTPUT_DIR/filter200.log | $CMD_DECODE 1> $OUTPUT_DIR/data-200.txt 2>$OUTPUT_DIR/demod-200.log

		# check which decode got more packets
		NOW="$(date --utc +%Y%m%d_%H%M%S)"
		PKT100="$(cat $OUTPUT_DIR/data-100.txt | wc -l)"
		PKT200="$(cat $OUTPUT_DIR/data-200.txt | wc -l)"
		echo "$NOW: G=100 gave $PKT100 packets; G=200 gave $PKT200 packets" >> $LOGFILE
		
		if [ "$PKT100" -gt "$PKT200" ]; then
			echo "$NOW:  Submitting packets from $OUTPUT_DIR/data-100.txt" >> $LOGFILE
			$CMD_SUBMIT $OUTPUT_DIR/data-100.txt > $OUTPUT_DIR/submit.log  2>&1
		else
			echo "$NOW:  Submitting packets from $OUTPUT_DIR/data-200.txt" >> $LOGFILE
			$CMD_SUBMIT $OUTPUT_DIR/data-200.txt > $OUTPUT_DIR/submit.log  2>&1
		fi

		# Finally move original data file to trash
		NOW="$(date --utc +%Y%m%d_%H%M%S)"
		echo "$NOW: Move $INPUT_DIR/$INPUT_FILE to $RECYCLE_DIR/" >> $LOGFILE
		mv $INPUT_DIR/$INPUT_FILE $RECYCLE_DIR/

		NOW="$(date --utc +%Y%m%d_%H%M%S)"
		echo "$NOW: --------------------------------------------------------------" >> $LOGFILE

		# give system a break
		sleep 10
	done

#	sleep $LOOP_DELAY
#done

