#!/bin/bash

# Skip these files in the git workspace since we rarely touch them and the script that
# generates them will always update a timestamp when we configure the project. If you
# do need to update them and you've run this script in your local checkout, change the
# flag to --no-skip-worktree and rerun this script.

declare -a TO_SKIP
TO_SKIP=(
        src/groups/bmq/bmqex/bmqex_bindutil.h
        src/groups/bmq/bmqex/bmqex_bindutil_cpp03.h
        src/groups/bmq/bmqex/bmqex_bindutil_cpp03.cpp
        src/groups/bmq/bmqu/bmqu_managedcallback.h
        src/groups/bmq/bmqu/bmqu_managedcallback_cpp03.cpp
        src/groups/bmq/bmqu/bmqu_managedcallback_cpp03.h
        src/groups/bmq/bmqu/bmqu_objectplaceholder.h
        src/groups/bmq/bmqu/bmqu_objectplaceholder_cpp03.cpp
        src/groups/bmq/bmqu/bmqu_objectplaceholder_cpp03.h
        src/groups/bmq/bmqu/bmqu_operationchain.h
        src/groups/bmq/bmqu/bmqu_operationchain_cpp03.cpp
        src/groups/bmq/bmqu/bmqu_operationchain_cpp03.h
)

git update-index --skip-worktree "${TO_SKIP[@]}"
