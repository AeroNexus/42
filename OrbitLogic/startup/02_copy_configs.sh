#!/usr/bin/env bash

LOGFILE=/42/logs/startup.log

log() {
    echo "$(basename $0): $1" | tee -a $LOGFILE
}

if [[ ! -d "/entrypoint-config.d" ]]; then
    exit 0
fi

log "Copying configs from entrypoint-config.d to /config..."

for file in $(ls /entrypoint-config.d); do
    cp -vf /entrypoint-config.d/${file} /config/${file}
done

log "> Finished!"
