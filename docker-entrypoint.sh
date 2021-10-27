#!/bin/bash
set -x
# By default, scenarios are stored under /42/scenarios
# When invoked below, 42 takes as argument a scenario directory relative to its root, e.g.
# ./42 my_scenarios

if [[ ! -d "$FORTYTWO_SCENARIO" ]]; then
    echo "$0: No FORTYTWO_SCENARIO provided. Using defaults."
else
    echo "$0: Running with scenario $FORTYTWO_SCENARIO"
fi

# Controls for headless mode
if [[ "$GRAPHICS_FRONT_END" == "false" ]]; then
    echo "$0: Setting graphics front end to FALSE (no x-windows)"
    sed -i 's/TRUE\(.*Graphics Front End?\)/FALSE\1\*\*/' /42/$FORTYTWO_SCENARIO/Inp_Sim.txt > /dev/null 2>&1
fi

# Allow for initial user script execution
DIR=/docker-entrypoint.d
if [[ -d "$DIR" ]]; then
  /bin/run-parts --verbose "$DIR"
fi

# Execute
cd /42 && ./42 $FORTYTWO_SCENARIO 2>&1
exec "$@"
