#!/usr/bin/env bash
# Place this file in the /docker-entrypoint.d folder so that it gets executed
# upon 42 container intialization.
#DIR="$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
LOGFILE=/42/logs/startup.log

log() {
    echo "$(basename $0): $1" | tee -a $LOGFILE
}

# Env/arg checks

if [[ "$FORTYTWO_GRPC_CONFIG" == "" ]]; then
    log "No FORTYTWO_GRPC_CONFIG found. Using search in current directory."
    FORTYTWO_GRPC_CONFIG=$(find . -name FortyTwo*.cfg | head -n 1)
fi

if [[ ! -f $FORTYTWO_GRPC_CONFIG ]]; then
    log "Error: Could not find config $FORTYTWO_GRPC_CONFIG. Exiting."
    exit 1
fi

# Execute 42 GRPC server

log "Starting 42 gRPC command/telemetry server..."
log "> Config: ${FORTYTWO_GRPC_CONFIG}"

/grpc_server/bin/FortyTwoGRPCServer -s -l trace -c $FORTYTWO_GRPC_CONFIG \
    > /42/logs/grpc_server.log 2>&1 &

log "> Finished!"
