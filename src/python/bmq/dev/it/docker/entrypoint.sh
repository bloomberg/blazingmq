#!/opt/bb/bin/dumb-init /bin/bash

export PS1="[docker] \W $ "

cd ${BMQ_REPO}

if [[ "$@" == "" ]]; then
    /bin/bash
else
    exec "$@"
fi
