#!/bin/bash

# SPDX-FileCopyrightText: 2025 Tillitis AB <tillitis.se>
# SPDX-License-Identifier: GPL-2.0-only

help() {
    echo "Usage: $(basename $0) [OPTION]"
    echo "Run multiple place and route threads with nextpnr-ice40"
    echo ""
    echo "  --debug                     Debugging mode, output all data from nextpnr-ice40"
    echo "  --help                      Help, display this help and exit"
    echo "  --freq <frequency>          Set target frequency for design in MHz (might be overridden from value in synthesis file)"
    echo "  --log-file <file>           Set log output filename (default: $LOG_FILE)"
    echo "  --out-file <file>           Set place and route output filename (default: $OUT_FILE)"
    echo "  --package <type>            Set package type (default: $PACKAGE)"
    echo "  --pin-file <file>           Set pin file to use (default: $PIN_FILE)"
    echo "  --synth-file <file>         Set synthesis file to use (default: $SYNTH_FILE)"
    echo "  --timeout '<time>[SUFFIX]'  Set time to run before script is killed"
    echo "  --threads <number>          Set number of threads to run (default: $THREADS, max: $(nproc))"
    echo "                                SUFFIX can be 's' for seconds, 'm' for minutes, 'h' for hours."
    echo "                                Given two or more arguments, time is sum of the values."
}

export TOP_DIR=$(pwd)
export LOCK_FILE=$(mktemp -u -p $TOP_DIR)
export LOCK_DIR=$(mktemp -u -p $TOP_DIR)

export LOG_FILE=../application_fpga_par.txt
export OUT_FILE=../application_fpga_par.json
export PACKAGE=sg48
export PIN_FILE=../data/application_fpga_tk1.pcf
export SYNTH_FILE=../synth.json

export PNR_DEBUG=/dev/null

# Get number of available CPU threads
THREADS=$(nproc --ignore=1)

# Array to store process IDs
pids=()

# Trap ctrl-c and call ctrl_c()
trap 'ctrl_c' SIGINT

cleanup() {
    echo "Running cleanup..."
    # Kill all child processes of the script
    pstree -p "$$" | grep -o '([0-9]\+)' | grep -o '[0-9]\+' | grep -v "$$" | xargs kill -9 >& /dev/null
    # Do directory cleanup
    rm -rf $TOP_DIR/tmp.*
    echo "Cleanup finished."
}

ctrl_c() {
    echo ""
    cleanup
}

terminate_script() {
  echo -n "Time limit reached!"
  kill -SIGINT $$
}

start_thread() {
    OUT_DIR="$(mktemp -d -p $TOP_DIR)" || { echo "Failed to create tempdir"; exit 1; }
    cd $OUT_DIR
    echo "Starting nextpnr run on PID = $BASHPID, tempdir = $OUT_DIR"
    while true; do
        nextpnr-ice40  \
                --log $LOG_FILE \
                --randomize-seed \
                --freq $FREQ \
                --exit-on-failed-target-frequency \
                --ignore-loops \
                --up5k \
                --package $PACKAGE \
                --json $SYNTH_FILE_PATH/$SYNTH_FILE \
                --pcf $PIN_FILE_PATH/$PIN_FILE \
                --write $OUT_FILE >& $PNR_DEBUG
        exit_code=$?

        if [ $exit_code -eq 0 ]; then
            mkdir $LOCK_DIR 2>/dev/null
            if [ $? -ne 0 ]; then
               return 1;
            fi

            (
                flock -x 200

                SEED=$(grep "Generated random seed" $LOG_FILE | awk '{print $5}' | awk --bignum '{printf "%20d\n", $0}')
                MAX_CLK_PLACE=$(grep "Max frequency for clock" $LOG_FILE | awk 'NR==1{print $7}')  # First occurence
                MAX_CLK_ROUTE=$(grep "Max frequency for clock" $LOG_FILE | awk 'NR==2{print $7}')  # Second occurence
                TARGET_CLK=$(grep "Max frequency for clock"    $LOG_FILE | awk 'NR==1{print $11}') # First occurence

                echo -n "PASS! Seed = $SEED, Max frequency for clock: place = $MAX_CLK_PLACE MHz, "
                echo "route = $MAX_CLK_ROUTE MHz, target = $TARGET_CLK MHz"
                cp -v $OUT_FILE $OUT_FILE_PATH/
                cp -v $LOG_FILE $LOG_FILE_PATH/

            ) 200>$LOCK_FILE

            return 0
        else

            (
                flock -x 200

                SEED=$(grep "Generated random seed" $LOG_FILE | awk '{print $5}' | awk --bignum '{printf "%20d\n", $0}')
                MAX_CLK_PLACE=$(grep "Max frequency for clock" $LOG_FILE | awk 'NR==1{print $7}')  # First occurence
                MAX_CLK_ROUTE=$(grep "Max frequency for clock" $LOG_FILE | awk 'NR==2{print $7}')  # Second occurence
                TARGET_CLK=$(grep "Max frequency for clock"    $LOG_FILE | awk 'NR==1{print $11}') # First occurence

                echo -n "FAIL! Seed = $SEED, Max frequency for clock: place = $MAX_CLK_PLACE MHz, "
                echo "route = $MAX_CLK_ROUTE MHz, target = $TARGET_CLK MHz, restarting..."
                rm -rf $OUT_DIR/*

            ) 200>$LOCK_FILE
        fi
    done
}

## Check flags
OPTS=$(getopt -o "" --long "\
                            debug,\
                            freq:,\
                            log-file:,\
                            out-file:,\
                            package:,\
                            pin-file:\
                            synth-file:,\
                            timeout:,\
                            threads:,\
                            help,\
                            verbose"  -n "$PROGNAME" -- "$@")

if [ $? != 0 ]
then
    echo "Error in command line arguments." >&2;
    help;
    exit 1;
fi

eval set -- "$OPTS"

while true; do
    case "$1" in
        --debug )                PNR_DEBUG=1;                        shift ;;
        --freq )                 FREQ="$2";                          shift 2 ;;
        --log-file )             LOG_FILE="$2";                      shift 2 ;;
        --out-file )             OUT_FILE="$2";                      shift 2 ;;
        --package )              PACKAGE="$2";                       shift 2 ;;
        --pin-file )             PIN_FILE="$2";                      shift 2 ;;
        --synth-file )           SYNTH_FILE="$2";                    shift 2 ;;
        --timeout )              TIMEOUT="$2";                       shift 2 ;;
        --threads )              THREADS="$2";                       shift 2 ;;
        --help )                 help;                               exit; ;;
        --verbose )              VERBOSE=$((VERBOSE + 1));           shift ;;
        -- )                     shift;                              break ;;
        * ) break ;;
    esac
done

export LOG_FILE_PATH=$(realpath $(dirname $LOG_FILE))
export LOG_FILE=$(basename $LOG_FILE)

export OUT_FILE_PATH=$(realpath $(dirname $OUT_FILE))
export OUT_FILE=$(basename $OUT_FILE)

export PIN_FILE_PATH=$(realpath $(dirname $PIN_FILE))
export PIN_FILE=$(basename $PIN_FILE)

export SYNTH_FILE_PATH=$(realpath $(dirname $SYNTH_FILE))
export SYNTH_FILE=$(basename $SYNTH_FILE)

if [ ! -f "$SYNTH_FILE_PATH/$SYNTH_FILE" ]; then
    echo "No $SYNTH_FILE file found. Did you run Yosys?"
    exit 1
fi

if [ -f "$OUT_FILE_PATH/$OUT_FILE" ]; then
    echo "Place and route output file $OUT_FILE_PATH/$OUT_FILE found. Remove before starting again!"
    exit 1
fi

if [ -f "$LOG_FILE_PATH/$LOG_FILE" ]; then
    echo "Log file $LOG_FILE_PATH/$LOG_FILE found. Remove before starting again!"
    exit 1
fi

if [ -z "$FREQ" ]; then
    echo "No target frequency set!"
    exit 1
else
    export FREQ
fi

if [ -n "$TIMEOUT" ]; then
    echo "Timeout:          $TIMEOUT"
    # Start background timer
    ( sleep ${TIMEOUT[@]} && terminate_script ) &
    if ! ps -p $! > /dev/null; then
        exit 1
    fi
fi

echo "Threads:          $THREADS"
echo "Target frequency: $FREQ MHz (might be overridden from value in synthesis file)"
echo "Synthesis file:   $SYNTH_FILE_PATH/$SYNTH_FILE"
echo "Pin file:         $PIN_FILE_PATH/$PIN_FILE"
echo "Package:          $PACKAGE"
echo "Out file:         $OUT_FILE_PATH/$OUT_FILE"
echo "Log file:         $LOG_FILE_PATH/$LOG_FILE"
echo ""
echo "Lock file:        $LOCK_FILE"
echo "Lock dir:         $LOCK_DIR"
echo ""

# Start threads equal to the number of CPU cores
for i in $(seq 1 $THREADS); do
    # Start the thread in the background
    start_thread &
    # Collect the PID
    pids+=($!)
done

wait_for_success() {
    while true; do
        # Check for exit 0 status of all collected PID processes
        if wait -n ${pids[@]}; then
            cleanup
            echo "Exiting..."
            exit 0
        else
            echo "Exiting..."
            exit 1
        fi
    done
}

# Wait for all threads
wait_for_success
