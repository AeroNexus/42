#!/usr/bin/env bash
# Place this file in the /docker-entrypoint.d folder so that it gets executed
# upon 42 container intialization.

# Path to current executing file:
#DIR="$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

# Env/arg checks

if [[ "$FORTYTWO_GRPC_CONFIG" == "" ]]; then
    echo "No FORTYTWO_GRPC_CONFIG found. Using search in current directory."
    FORTYTWO_GRPC_CONFIG=$(find . -name FortyTwo*.cfg | head -n 1)
fi

if [[ ! -f $FORTYTWO_GRPC_CONFIG ]]; then
    echo "Error: Could not find config $FORTYTWO_GRPC_CONFIG. Exiting."
    exit 1
fi

# Execute 42 GRPC server

echo "Starting 42 gRPC command/telemetry server..."
echo "> Config: ${FORTYTWO_GRPC_CONFIG}"

/grpc_server/FortyTwoGRPCServer -s -l trace -c $FORTYTWO_GRPC_CONFIG \
    > /42/logs/grpc_server.log 2>&1 &
