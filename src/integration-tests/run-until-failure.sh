#!/bin/sh -x

rm aaa.log

count=0

while python3 -m pytest -m "fsm_mode and multi and eventual_consistency" test_sync_after_rollover.py::test_synch_after_missed_rollover -vvv >aaa.log;
do
    count=$((count + 1))
    rm aaa.log
    echo "Current Number of successful runs: $count"
done

echo "Number of successful runs: $count"
