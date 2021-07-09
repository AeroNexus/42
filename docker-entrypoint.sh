#!/bin/bash
set -x

if [[ ! -d "$FORTYTWO_SCENARIO" ]]; then 
    echo "$0: No FORTYTWO_SCENARIO provided. Using defaults."
else
    echo "$0: Running with scenario $FORTYTWO_SCENARIO"
fi

cd /42 && ./42 $FORTYTWO_SCENARIO 2>&1
exec "$@"
