#!/usr/bin/env bash
# Generate the 42 scenario using the Orbit Logic Scenario Generator
# SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

LOGFILE=/42/logs/startup.log

log() {
    echo "$(basename $0): $1" | tee -a $LOGFILE
}

# Execute the generator only if a config is provided
if [[ "$SCENARIO_GENERATOR_CONFIG" == "" ]]; then
    log "No SCENARIO_GENERATOR_CONFIG specified; will not generate anything"
    exit 0
fi

# Generator configuration yaml is required
if [[ ! -f $SCENARIO_GENERATOR_CONFIG ]]; then
    log "Error: unable to find config: ${SCENARIO_GENERATOR_CONFIG}!"
    exit 1
fi

# Provide option for COE input file
if [[ "$SCENARIO_GENERATOR_COE_INPUT" == "" ]]; then
    log "Warning: no SCENARIO_GENERATOR_COE_INPUT specified. Using /config location."
    SCENARIO_GENERATOR_COE_INPUT=/config/coe_inputs.csv
fi

if [[ -f $SCENARIO_GENERATOR_COE_INPUT ]]; then
    COE_OUTPUT=/config/coe.csv

    # If provided, reduce the supplied constellation COE file to a select satellite
    if [[ "$SCENARIO_GENERATOR_COE_SELECTOR" != "" ]]; then
        log "Selecting $SCENARIO_GENERATOR_COE_SELECTOR from constellation in $COE_INPUT"
        egrep "SatName|$SCENARIO_GENERATOR_COE_SELECTOR" ${SCENARIO_GENERATOR_COE_INPUT} > ${COE_OUTPUT}
    else
        # Else use the full constellation COE file
        log "Using all satellites in $SCENARIO_GENERATOR_COE_INPUT"
        cat ${SCENARIO_GENERATOR_COE_INPUT} > ${COE_OUTPUT}
    fi
else
    log "No SCENARIO_GENERATOR_COE_INPUT file provided."
fi

# Find the newest scenario generator jarfile e.g. JastroScenarioGenerator-6.1.5.jar
cd ${SCENARIO_GENERATOR_DIR}/bin
JARFILE=$(ls JastroScenarioGenerator*.jar | tail -n 1)

log "Generating scenario with $JARFILE"
log "SCENARIO_GENERATOR_CONFIG = $SCENARIO_GENERATOR_CONFIG"

# Generate the scenario
java -jar $JARFILE -f $SCENARIO_GENERATOR_CONFIG >> $LOGFILE 2>&1

if [[ "$?" != "0" ]]; then
    log "Error generating scenario!"
    exit $?
fi

# Note that the yaml must use name="Scenario" to match this directory
#log "Copying output to FORTYTWO_SCENARIO=${FORTYTWO_SCENARIO}"
#cp -vf ${SCENARIO_GENERATOR_DIR}/Scenario/* /42/${FORTYTWO_SCENARIO}/

log "Finished!"
