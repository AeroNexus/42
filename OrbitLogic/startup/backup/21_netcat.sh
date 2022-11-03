#!/usr/bin/env bash
# Place this file in the /docker-entrypoint.d folder so that it gets executed
# upon 42 container intialization.

# Execute netcat

log "Starting netcat..."

nc -u -l 6790 \
    > /42/logs/netcat.log 2>&1 &

log "> Finished!"
