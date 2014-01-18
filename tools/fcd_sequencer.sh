#!/bin/bash

WORK_DIR="$HOME/funcube/data/"
PROG_DIR="$HOME/funcube/bin"
RECYCLE_DIR="$WORK_DIR/recycle"
#OUTPUT_DIR containing time stamp is created and set later

FILE_PREFIX="fcd_iq_sc_"
FILE_SUFFIX="_96_145924"
AUDIO_DEV=alsa_input.usb-Hanlincrest_Ltd._FUNcube_Dongle_V1.0-00-V10.analog-stereo
#AUDIO_DEV=hw:1

LOOP_DELAY=1
LOGFILE="$WORK_DIR/fcd_sequencer.log"

# commands
CMD_FCDCTL="$PROG_DIR/fcdctl"
CMD_FILTER="$PROG_DIR/filter"
CMD_DECODE="$PROG_DIR/decode"
CMD_SUBMIT="$PROG_DIR/submit.sh"

# command port
PORT=12345

# default duration of recording
DURATION=780


while :
do
    # wait for external trigger
    NOW="$(date --utc +%Y%m%d_%H%M%S)"
    echo "$NOW:" >> $LOGFILE
    echo "$NOW: Waiting for new command on port $PORT" >> $LOGFILE

    command="$(nc -l $PORT)"
    NOW="$(date --utc +%Y%m%d_%H%M%S)"
    case "$command" in
        exit)
            echo "$NOW: Received exit command" >> $LOGFILE
            exit
            ;;
        start*)
            echo "$NOW: Received start command: $command" >> $LOGFILE
            ;;
        *)
            echo "$NOW: Received unknown command: $command" >> $LOGFILE
            continue
            ;;
    esac

    # Comamnd is "start +780" where 780 is the pass duration in seconds
#    IFS='+' read -ra ARR <<< "$command"
    arr=(${command//+/ })
    tmp=${arr[1]}

    # check if tmp is an integer
    if [ "$tmp" -eq "$tmp" ] 2>/dev/null; then
        DURATION=$tmp
    fi
    echo "$NOW: Will capture IQ for $DURATION seconds" >> $LOGFILE

    # ensure FCD is configured
    $CMD_FCDCTL -f 145.924 -g 10 -c 0 >> $LOGFILE
    $CMD_FCDCTL -l >> $LOGFILE

    NOW="$(date --utc +%Y%m%d_%H%M%S)"

    TIME_STRING=$NOW
    OUTPUT_DIR="$WORK_DIR/$TIME_STRING"
    mkdir -p $OUTPUT_DIR

    IQ_FILE="$FILE_PREFIX$TIME_STRING$FILE_SUFFIX.raw"
    echo "$NOW: Using audio input: $AUDIO_DEV" >> $LOGFILE
    echo "$NOW: Start capture to $OUTPUT_DIR/$IQ_FILE"  >> $LOGFILE

    # Record IQ
#    sox -q -r 96k -e signed-integer -b 16 --endian little -c 2 -t alsa $AUDIO_DEV $OUTPUT_DIR/$IQ_FILE trim 0 $DURATION >> $LOGFILE 2>&1 
    sox -q -r 96k -e signed-integer -b 16 --endian little -c 2 -t pulseaudio $AUDIO_DEV $OUTPUT_DIR/$IQ_FILE trim 0 $DURATION >> $LOGFILE 2>&1 

    # perform filtering with gain 100 then decode
    NOW="$(date --utc +%Y%m%d_%H%M%S)"
    echo -n "$NOW: Filter and decode (G=100) $OUTPUT_DIR/$IQ_FILE:"  >> $LOGFILE
    cat "$OUTPUT_DIR/$IQ_FILE" | $CMD_FILTER -g 100 2> $OUTPUT_DIR/filter100.log | $CMD_DECODE 1> $OUTPUT_DIR/data-100.txt 2>$OUTPUT_DIR/demod-100.log
    PKT100="$(cat $OUTPUT_DIR/data-100.txt | wc -l)"
    echo "$PKT100" >> $LOGFILE
 
    # perform filtering with gain 200 then decode
    NOW="$(date --utc +%Y%m%d_%H%M%S)"
    echo -n "$NOW: Filter and decode (G=200) $OUTPUT_DIR/$IQ_FILE:"  >> $LOGFILE
    cat "$OUTPUT_DIR/$IQ_FILE" | $CMD_FILTER -g 200 2> $OUTPUT_DIR/filter200.log | $CMD_DECODE 1> $OUTPUT_DIR/data-200.txt 2>$OUTPUT_DIR/demod-200.log
    PKT200="$(cat $OUTPUT_DIR/data-200.txt | wc -l)"
    echo "$PKT200" >> $LOGFILE
 
    # Use "sort -u" to find the union of the two decode sequences and submit packet
    NOW="$(date --utc +%Y%m%d_%H%M%S)"
    echo "$NOW: G=100 gave $PKT100 packets; G=200 gave $PKT200 packets" >> $LOGFILE
    sort -u $OUTPUT_DIR/data-100.txt $OUTPUT_DIR/data-200.txt  1> $OUTPUT_DIR/data-sorted.txt 2>$OUTPUT_DIR/sort.log
    PKTSORT="$(cat $OUTPUT_DIR/data-sorted.txt | wc -l)"
    echo "$NOW:   *** Unique packets: $PKTSORT" >> $LOGFILE

    echo "$NOW:  Submitting packets from $OUTPUT_DIR/data-sorted.txt" >> $LOGFILE
    $CMD_SUBMIT $OUTPUT_DIR/data-sorted.txt > $OUTPUT_DIR/submit.log  2>&1

    NOW="$(date --utc +%Y%m%d_%H%M%S)"
    echo "$NOW:" >> $LOGFILE
    echo "$NOW: --------------------------------------------------------------" >> $LOGFILE

    # FIXME: we can remove this
    sleep $LOOP_DELAY
done

