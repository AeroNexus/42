#!/usr/bin/env bash
DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
DIRECTORY=/config # required/default configs location
LOGFILE=/42/logs/startup.log

log() {
    echo "$(basename $0): $1" | tee -a $LOGFILE
}

log "Updating config files for environment..."
log "> DIRECTORY=$DIRECTORY"

templates=$(find $DIRECTORY -type f -name "*.template.*" 2>/dev/null)

# Replace any instance of $VAR with the current environment variable value
for filename in ${templates[@]}; do
    newfile=$(echo $filename | sed 's/.template//g')
    ${DIR}/env_replace.pl $filename > $newfile
done

log "> Finished!"
