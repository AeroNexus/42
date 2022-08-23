#!/bin/bash
set -x
# By default, scenarios are stored under /42/scenarios
# When invoked below, 42 takes as argument a scenario directory relative to its root, e.g.
# ./42 my_scenarios

if [[ "$FORTYTWO_LOG_DIR" == "" ]]; then
    export FORTYTWO_LOG_DIR=/42/logs
fi

LOGFILE="${FORTYTWO_LOG_DIR}/42.log"

hr() { printf -- '-%.0s' {1..80} && echo ; }

startup_report() {
    printf "$(date)\n"
    printf "Assigned IP Address = $(hostname -i)...\n"
    hr && env | egrep 'FORTYTWO|SCENARIO|GRAPHICS' && hr
}

# Check for passed-in variable settings

if [[ ! -d "$FORTYTWO_SCENARIO" ]]; then
    echo "$0: No FORTYTWO_SCENARIO provided. Using defaults."
else
    echo "$0: Running with scenario $FORTYTWO_SCENARIO"
fi

# Initialize new logs
rm -f ${FORTYTWO_LOG_DIR}/*.log
startup_report | tee -a ${LOGFILE}

# Execute all scripts in startup folder
DIR=/startup
if [[ -d "$DIR" ]]; then
  cd $DIR && /bin/run-parts . -v --regex '.*.sh' >> ${LOGFILE}
fi

# Allow for additional user script execution at startup
DIR=/docker-entrypoint.d
if [[ -d "$DIR" ]]; then
  cd $DIR && /bin/run-parts . -v --regex '.*.sh' >> ${LOGFILE}
fi

# Report on any outputs from the above /startup scripts
cat /42/logs/startup.log

# Controls for headless mode
if [[ "$GRAPHICS_FRONT_END" == "false" ]]; then
    echo "$0: Setting graphics front end to FALSE (no x-windows)"
    sed -i '6s/.*/FALSE ! Graphics OFF, Set by docker-entrypoint/' /42/$FORTYTWO_SCENARIO/Inp_Sim.txt > /dev/null 2>&1
else
    sed -i '6s/.*/TRUE ! Graphics ON, Set by docker-entrypoint/' /42/$FORTYTWO_SCENARIO/Inp_Sim.txt > /dev/null 2>&1
fi

hr | tee -a ${LOGFILE}
printf "Starting 42 sim...\n" | tee -a ${LOGFILE}
hr | tee -a ${LOGFILE}

# Execute
cd /42 && ./42 $FORTYTWO_SCENARIO | tee -a ${LOGFILE}
exec "$@"
