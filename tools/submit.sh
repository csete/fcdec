#!/bin/bash

SERVER=http://data.funcube.org.uk/api/data/hex/MYCALL/
AUTHKEY=0123456789abcdef

# Read from command line argument or stdin
while read -r DATA
do
    # skip blank lines
    [ -z "$DATA" ] && continue

    # Create string to encode (re-add trailing space)
    POSTDATA="$DATA "
    ENCSTR="$POSTDATA:$AUTHKEY"

    # calculate md5sum
    MD5STR=`echo -n $ENCSTR | md5sum -`

    # remove trailing " -"
    DIGEST=`echo $MD5STR | awk '{ print $1 }'`

    URL="$SERVER?digest=$DIGEST"
#    echo $URL

    RESULT=`curl --user-agent "fcdec for linux" -d "data=$POSTDATA" $URL`
    echo "Result: $RESULT"

    # Please don't reduce this delay. 5 seconds corresponds to
    # the rate used by the Dashboard software
    sleep 5
done < <(cat "$@")

