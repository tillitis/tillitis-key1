#!/bin/bash

# SPDX-FileCopyrightText: 2026 Tillitis AB <tillitis.se>
# SPDX-License-Identifier: BSD-2-Clause

export PROJECT_COMMAND="make SCA=1 usb_device.ihx"
export PROJECT_CLEAN="clean"

export ARCH=""
export FREESTANDING=""

# Set report date
export REPORT_DATE=$(TZ="Europe/Stockholm" date +%Y-%m-%d_-_%H%M)

# Set number of threads to use
export JOBS=$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)

# Get current working directory
export CWD=$PWD

# Init checkers
export PROJECT_CHECKERS=""

# Set checkers
export PROJECT_CHECKERS="$PROJECT_CHECKERS --no-default-checkers"

export PROJECT_CHECKERS="$PROJECT_CHECKERS --biabduction"
export PROJECT_CHECKERS="$PROJECT_CHECKERS --enable-issue-type  ARRAY_OUT_OF_BOUNDS_L1"
export PROJECT_CHECKERS="$PROJECT_CHECKERS --disable-issue-type ARRAY_OUT_OF_BOUNDS_L2"
export PROJECT_CHECKERS="$PROJECT_CHECKERS --disable-issue-type ARRAY_OUT_OF_BOUNDS_L3"
export PROJECT_CHECKERS="$PROJECT_CHECKERS --enable-issue-type  Assert_failure"
export PROJECT_CHECKERS="$PROJECT_CHECKERS --disable-issue-type BIABDUCTION_MEMORY_LEAK"
export PROJECT_CHECKERS="$PROJECT_CHECKERS --enable-issue-type  DANGLING_POINTER_DEREFERENCE"
export PROJECT_CHECKERS="$PROJECT_CHECKERS --enable-issue-type  DIVIDE_BY_ZERO"
export PROJECT_CHECKERS="$PROJECT_CHECKERS --enable-issue-type  NULL_DEREFERENCE"
export PROJECT_CHECKERS="$PROJECT_CHECKERS --enable-issue-type  PRECONDITION_NOT_MET"
export PROJECT_CHECKERS="$PROJECT_CHECKERS --enable-issue-type  PREMATURE_NIL_TERMINATION_ARGUMENT"
export PROJECT_CHECKERS="$PROJECT_CHECKERS --enable-issue-type  RESOURCE_LEAK"

export PROJECT_CHECKERS="$PROJECT_CHECKERS --bufferoverrun"
export PROJECT_CHECKERS="$PROJECT_CHECKERS --enable-issue-type  BUFFER_OVERRUN_L1"
export PROJECT_CHECKERS="$PROJECT_CHECKERS --enable-issue-type  BUFFER_OVERRUN_L2"
export PROJECT_CHECKERS="$PROJECT_CHECKERS --disable-issue-type BUFFER_OVERRUN_L3"
export PROJECT_CHECKERS="$PROJECT_CHECKERS --disable-issue-type BUFFER_OVERRUN_L4"
export PROJECT_CHECKERS="$PROJECT_CHECKERS --disable-issue-type BUFFER_OVERRUN_L5"
export PROJECT_CHECKERS="$PROJECT_CHECKERS --enable-issue-type  BUFFER_OVERRUN_S2"
export PROJECT_CHECKERS="$PROJECT_CHECKERS --disable-issue-type BUFFER_OVERRUN_U5"
export PROJECT_CHECKERS="$PROJECT_CHECKERS --disable-issue-type CONDITION_ALWAYS_FALSE"
export PROJECT_CHECKERS="$PROJECT_CHECKERS --disable-issue-type CONDITION_ALWAYS_TRUE"
export PROJECT_CHECKERS="$PROJECT_CHECKERS --enable-issue-type  INFERBO_ALLOC_IS_BIG"
export PROJECT_CHECKERS="$PROJECT_CHECKERS --enable-issue-type  INFERBO_ALLOC_IS_NEGATIVE"
export PROJECT_CHECKERS="$PROJECT_CHECKERS --enable-issue-type  INFERBO_ALLOC_IS_ZERO"
export PROJECT_CHECKERS="$PROJECT_CHECKERS --enable-issue-type  INFERBO_ALLOC_MAY_BE_BIG"
export PROJECT_CHECKERS="$PROJECT_CHECKERS --enable-issue-type  INFERBO_ALLOC_MAY_BE_NEGATIVE"
export PROJECT_CHECKERS="$PROJECT_CHECKERS --enable-issue-type  INTEGER_OVERFLOW_L1"
export PROJECT_CHECKERS="$PROJECT_CHECKERS --disable-issue-type INTEGER_OVERFLOW_L2"
export PROJECT_CHECKERS="$PROJECT_CHECKERS --disable-issue-type INTEGER_OVERFLOW_L5"
export PROJECT_CHECKERS="$PROJECT_CHECKERS --disable-issue-type INTEGER_OVERFLOW_U5"

export PROJECT_CHECKERS="$PROJECT_CHECKERS --liveness"
export PROJECT_CHECKERS="$PROJECT_CHECKERS --enable-issue-type  DEAD_STORE"

export PROJECT_CHECKERS="$PROJECT_CHECKERS --pulse"
export PROJECT_CHECKERS="$PROJECT_CHECKERS --enable-issue-type  MEMORY_LEAK_C"
export PROJECT_CHECKERS="$PROJECT_CHECKERS --enable-issue-type  NULLPTR_DEREFERENCE"
#export PROJECT_CHECKERS="$PROJECT_CHECKERS --disable-issue-type PULSE_UNINITIALIZED_VALUE"
export PROJECT_CHECKERS="$PROJECT_CHECKERS --enable-issue-type  PULSE_UNINITIALIZED_VALUE"
export PROJECT_CHECKERS="$PROJECT_CHECKERS --disable-issue-type TAINT_ERROR"
export PROJECT_CHECKERS="$PROJECT_CHECKERS --enable-issue-type  USE_AFTER_FREE"

# Trap ctrl-c and call ctrl_c()
trap 'ctrl_c' SIGINT

cleanup() {
    mkdir -p "$CWD"/infer-out-$REPORT_DATE
    if [ -f "$CWD"/compile_commands.json ]
    then
        mv "$CWD"/compile_commands.json "$CWD"/infer-out-$REPORT_DATE/
    fi
}

ctrl_c() {
    echo ""
    cleanup
}

# Set exceptions
if [ -f "$CWD"/infer_exceptions.txt ]
then
    while read -r LINE;
    do
        EXCEPTION=$LINE

        # Filter exception line from potentially bad characters
        EXCEPTION=$(echo $EXCEPTION | sed 's/#//g')
        EXCEPTION=$(echo $EXCEPTION | sed 's/&//g')
        EXCEPTION=$(echo $EXCEPTION | sed 's/|//g')
        EXCEPTION=$(echo $EXCEPTION | sed 's/;//g')
        EXCEPTION=$(echo $EXCEPTION | sed 's/`//g')
        EXCEPTION=$(echo $EXCEPTION | sed 's/\$//g')
        EXCEPTION=$(echo $EXCEPTION | sed 's/(//g')
        EXCEPTION=$(echo $EXCEPTION | sed 's/)//g')

        # Add exception line to the project exceptions string
        PROJECT_EXCEPTIONS="$PROJECT_EXCEPTIONS # '$EXCEPTION'"
    done < "$CWD"/infer_exceptions.txt
fi

# Add the --censor-report flag for each exception
PROJECT_EXCEPTIONS=$(echo $PROJECT_EXCEPTIONS | sed 's/#/--censor-report/g')

function check_error()
{
    if [ $? -ne 0 ]; then
        echo "$1 failed!"
        exit 1
    else
        echo "$1 completed!"
    fi
}

# Clean
make $PROJECT_CLEAN
check_error "make clean"

# Run bear
bear --output "$CWD"/compile_commands.json -- $PROJECT_COMMAND -j$JOBS
check_error "bear"

# Filter compile_commands.json
if [[ -z "$ARCH" ]]; then # If $ARCH is empty, remove "-target" also.
sed -i 's/-target//g' "$CWD"/compile_commands.json
fi
sed -i 's/riscv32-unknown-none-elf/'"$ARCH"'/g' "$CWD"/compile_commands.json
sed -i 's/-march=rv32iczmmul/'"$FREESTANDING"'/g' "$CWD"/compile_commands.json
sed -i 's/-mabi=ilp32//g' "$CWD"/compile_commands.json
sed -i 's/-mcmodel=medany//g' "$CWD"/compile_commands.json

# Run infer capture
infer-capture \
    --compilation-database "$CWD"/compile_commands.json \
    --results-dir "$CWD"/infer-out-$REPORT_DATE
check_error "infer-capture"

# Move compile_commands.json to infer-out directory
mv -f "$CWD"/compile_commands.json "$CWD"/infer-out-$REPORT_DATE/

# Run infer analyze
infer-analyze \
    --results-dir "$CWD"/infer-out-$REPORT_DATE \
    --jobs $JOBS \
    --keep-going \
    --print-active-checkers \
    --no-report \
    $(echo $PROJECT_CHECKERS)
check_error "infer-analyze"

# Run infer report, need to use eval before infer-report to handle the single quotes inside PROJECT_EXCEPTIONS
eval infer-report \
    --results-dir "$CWD"/infer-out-$REPORT_DATE/ \
    --quiet \
    $PROJECT_EXCEPTIONS
check_error "infer-report"

# Run infer explore
infer-explore \
    --results-dir "$CWD"/infer-out-$REPORT_DATE/ \
    --select all \
    >& "$CWD"/infer-out-$REPORT_DATE/expanded-report.txt
check_error "infer-explore"

# Save checkers to get tracking of what checkers where used for the run
echo $PROJECT_CHECKERS | sed 's/--/\n--/g' > "$CWD"/infer-out-$REPORT_DATE/checkers_used.txt

echo "###### Begin infer issues summary ######"

# Show issues
if [ -s "$CWD"/infer-out-$REPORT_DATE/report.txt ]; then
    echo "Issues found!"
    cat "$CWD"/infer-out-$REPORT_DATE/report.txt
    EXIT_RESULT=1
else
    echo "No issues found!"
    EXIT_RESULT=0
fi

echo "###################################"

# Show censored issues
CENSORED_ISSUES=$(cat "$CWD"/infer-out-$REPORT_DATE/report.json | \
                      jq '.[] | if (.censored_reason) then (.bug_type + ":" + .file + ":" + (.line|tostring) + ":" + .procedure + ":" + .censored_reason) else empty end')

if [ -n "$CENSORED_ISSUES" ]; then
    echo "Censored issues:"
    echo $CENSORED_ISSUES | sed 's/\" \"/\"\n\"/g' | \
                            sort -n | \
                            tee "$CWD"/infer-out-$REPORT_DATE/censored-issues-report.txt
else
    echo "No censored issues."
fi

echo "###### End infer issues summary ######"

exit $EXIT_RESULT
