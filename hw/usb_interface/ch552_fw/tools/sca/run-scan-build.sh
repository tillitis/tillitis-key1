#!/bin/bash

# SPDX-FileCopyrightText: 2026 Tillitis AB <tillitis.se>
# SPDX-License-Identifier: BSD-2-Clause

export PROJECT_COMMAND="make SCA=1 usb_device.ihx"
export PROJECT_CLEAN="clean"

# Set report date
export REPORT_DATE=$(TZ="Europe/Stockholm" date +%Y-%m-%d_-_%H%M)

# Set number of threads to use
export JOBS=$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)

# Get current working directory
export CWD=$PWD

# Init checkers
export PROJECT_CHECKERS=""

# Set checkers
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker core.BitwiseShift"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker core.CallAndMessage"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker core.DivideZero"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker core.NonNullParamChecker"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker core.NullDereference"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker core.NullPointerArithm"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker core.StackAddressEscape"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker core.UndefinedBinaryOperatorResult"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker core.VLASize"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker core.uninitialized.ArraySubscript"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker core.uninitialized.Assign"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker core.uninitialized.Branch"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker core.uninitialized.CapturedBlockVariable"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker core.uninitialized.UndefReturn"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker core.uninitialized.NewArraySize"

export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker cplusplus.ArrayDelete"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker cplusplus.InnerPointer"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker cplusplus.Move"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker cplusplus.NewDelete"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker cplusplus.NewDeleteLeaks"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker cplusplus.PlacementNew"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker cplusplus.SelfAssignment"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker cplusplus.StringChecker"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker cplusplus.PureVirtualCall"

export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker deadcode.DeadStores"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker nullability.NullReturnedFromNonnull"

export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker optin.core.EnumCastOutOfRange"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker optin.cplusplus.UninitializedObject"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker optin.cplusplus.VirtualCall"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker optin.mpi.MPI-Checker"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker optin.performance.Padding"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker optin.portability.UnixAPI"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker optin.taint.GenericTaint"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker optin.taint.TaintedAlloc"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker optin.taint.TaintedDiv"

export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker security.ArrayBound"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker security.cert.env.InvalidPtr"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker security.FloatLoopCounter"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker security.insecureAPI.UncheckedReturn"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker security.insecureAPI.bcmp"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker security.insecureAPI.bcopy"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker security.insecureAPI.bzero"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker security.insecureAPI.decodeValueOfObjCType"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker security.insecureAPI.getpw"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker security.insecureAPI.gets"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker security.insecureAPI.mkstemp"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker security.insecureAPI.mktemp"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker security.insecureAPI.rand"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker security.insecureAPI.strcpy"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker security.insecureAPI.vfork"
#export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker security.insecureAPI.DeprecatedOrUnsafeBufferHandling"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker security.MmapWriteExec"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker security.PointerSub"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker security.PutenvStackArray"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker security.SetgidSetuidOrder"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker security.VAList"

export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker unix.API"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker unix.BlockInCriticalSection"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker unix.Chroot"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker unix.Errno"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker unix.Malloc"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker unix.MallocSizeof"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker unix.MismatchedDeallocator"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker unix.Vfork"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker unix.cstring.BadSizeArg"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker unix.cstring.NotNullTerminated"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker unix.cstring.NullArg"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker unix.StdCLibraryFunctions"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker unix.Stream"

# Extra C checkers (alpha state)
#export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker alpha.clone.CloneChecker"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker alpha.core.CastToStruct"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker alpha.core.Conversion"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker alpha.core.PointerArithm"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker alpha.core.StdVariant"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker alpha.core.TestAfterDivZero"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker alpha.core.StoreToImmutable"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker alpha.cplusplus.DeleteWithNonVirtualDtor"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker alpha.cplusplus.InvalidatedIterator"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker alpha.cplusplus.IteratorRange"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker alpha.cplusplus.MismatchedIterator"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker alpha.cplusplus.SmartPtr"
#export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker alpha.deadcode.UnreachableCode"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker alpha.llvm.Conventions"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker alpha.security.ReturnPtrRange"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker alpha.unix.PthreadLock"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker alpha.unix.SimpleStream"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker alpha.unix.cstring.BufferOverlap"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker alpha.unix.cstring.OutOfBounds"
export PROJECT_CHECKERS="$PROJECT_CHECKERS -enable-checker alpha.unix.cstring.UninitializedRead"

# Set exceptions
if [ -f "$CWD"/scan-build_exceptions.txt ]
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
    done < "$CWD"/scan-build_exceptions.txt
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

# Build, analyze and generate local output
scan-build $PROJECT_CHECKERS \
           -o "$CWD"/scan-build-out-$REPORT_DATE/ \
           -analyze-headers \
           -sarif \
           --keep-empty \
           --force-analyze-debug-code \
           --use-cc=clang \
           $PROJECT_COMMAND -j$JOBS
check_error "scan-build"

ls -1 "$CWD"/scan-build-out-$REPORT_DATE/*.sarif | sort -n > "$CWD"/scan-build-out-$REPORT_DATE/file1.txt
md5sum "$CWD"/scan-build-out-$REPORT_DATE/*.sarif | sort -n | uniq -w 16 | awk '{print $2}' | sort -n > "$CWD"/scan-build-out-$REPORT_DATE/file2.txt
comm -3 "$CWD"/scan-build-out-$REPORT_DATE/file1.txt "$CWD"/scan-build-out-$REPORT_DATE/file2.txt | xargs rm
rm "$CWD"/scan-build-out-$REPORT_DATE/file1.txt "$CWD"/scan-build-out-$REPORT_DATE/file2.txt
echo "File lists generated!"

sed -i "s|$(pwd)/||g" "$CWD"/scan-build-out-$REPORT_DATE/*.sarif
check_error "sed"

SARIF_FILES=$(ls -1 "$CWD"/scan-build-out-$REPORT_DATE/*.sarif)
for SARIF_FILE in $SARIF_FILES
do
    SARIF_FILE_MD5SUM=$(md5sum $SARIF_FILE | awk '{print $1}' | cut -b 1-8)
    mv $SARIF_FILE "$CWD"/scan-build-out-$REPORT_DATE/report-$SARIF_FILE_MD5SUM.sarif
done

# Run sarif_to_txt.py to get reports, need to use eval to handle the single quotes inside PROJECT_EXCEPTIONS
eval "$CWD"/sarif_to_txt.py \
                  --basic-report "$CWD"/scan-build-out-$REPORT_DATE/report.txt \
                  --expanded-report "$CWD"/scan-build-out-$REPORT_DATE/expanded-report.txt \
                  --censored-issues-report "$CWD"/scan-build-out-$REPORT_DATE/censored-issues-report.txt \
                  $PROJECT_EXCEPTIONS \
                  "$CWD"/scan-build-out-$REPORT_DATE/*.sarif
check_error "sarif_to_txt.py"

# Save checkers to get tracking of what checkers where used for the run
echo $PROJECT_CHECKERS | sed 's/--/\n--/g' > "$CWD"/scan-build-out-$REPORT_DATE/checkers_used.txt

echo "###### Begin scan-build issues summary ######"

# Show issues
if [ -s "$CWD"/scan-build-out-$REPORT_DATE/report.txt ]; then
    echo "Issues found!"
    cat "$CWD"/scan-build-out-$REPORT_DATE/report.txt
    EXIT_RESULT=1
else
    echo "No issues found!"
    EXIT_RESULT=0
fi

echo "###################################"

# Show censored issues
if [ -s "$CWD"/scan-build-out-$REPORT_DATE/censored-issues-report.txt ]
then
    echo "Censored issues:"
    cat "$CWD"/scan-build-out-$REPORT_DATE/censored-issues-report.txt | sort -n
else
    echo "No censored issues."
fi

echo "###### End scan-build issues summary ######"

exit $EXIT_RESULT
