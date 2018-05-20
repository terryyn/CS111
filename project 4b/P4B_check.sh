#!/bin/bash
#
# sanity check script for Project 4B
#	extract tar file
#	required README fields (ID, EMAIL, NAME)
#	required Makefile targets (clean, dist, graphs, tests)
#	make default
#	make dist
#	make clean (returns directory to untared state)
#	make default, success, creates program
#	unrecognized parameters
#	recognizes standard parameters
#	report generation
#	command processing (direct input)
#	command processing (pipe_test input)
#	log generation
#	use of expected functions

# Note: if you dummy up the sensor sampling
#	this test script can be run on any
#	Linux system.
#
LAB="lab4b"
README="README"
MAKEFILE="Makefile"

SOURCES=""
EXPECTED=""
EXPECTEDS=".c"
PGM="lab4b"
PGMS=$PGM
TEST_PGM=pipe_test

SUFFIXES=""

LIBRARY_URL="www.cs.ucla.edu/classes/cs111/Software"

EXIT_OK=0
EXIT_ARG=1
EXIT_FAIL=1

TIMEOUT=1

let errors=0

if [ -z "$1" ]
then
	echo usage: $0 your-student-id
	exit 1
else
	student=$1
fi

# make sure the tarball has the right name
tarball="$LAB-$student.tar.gz"
if [ ! -s $tarball ]
then
	echo "ERROR: Unable to find submission tarball:" $tarball
	exit 1
fi

# get copy of our grading/checking functions
if [ -s functions.sh ]; then
	source functions.sh
else
	wget $LIBRARY_URL/functions.sh 2> /dev/null
	if [ $? -eq 0 ]; then
		>&2 echo "Downloading functions.sh from $LIBRARY_URL"
		source functions.sh
	else
		>&2 echo "FATAL: unable to pull test functions from $LIBRARY_URL"
		exit -1
	fi
fi

# get a copy of the input genartion programs
if [ ! -s $TEST_PGM ]; then
	downLoad $TEST_PGM $LIBRARY_URL "c" "-lpthread"
fi

# read the tarball into a test directory
TEMP=`pwd`/"CS111_test.$LOGNAME"
if [ -d $TEMP ]
then
	echo Deleting old $TEMP
	rm -rf $TEMP
fi
mkdir $TEMP
unTar $LAB $student $TEMP
cd $TEMP

# note the initial contents
dirSnap $TEMP $$

echo "... checking for README file"
checkFiles $README
let errors+=$?

echo "... checking for submitter ID in $README"
ID=`getIDs $README $student`
let errors+=$?

echo "... checking for submitter email in $README"
EMAIL=`getEmail $README`
let errors+=$?

echo "... checking for submitter name in $README"
NAME=`getName $README`
let errors+=$?

echo "... checking slip-day use in $README"
SLIPDAYS=0
slips=`grep "SLIPDAYS:" $README`
if [ $? -eq 0 ]
then
	slips=`echo $slips | cut -d: -f2 | tr -d \[:space:\]`
	if [ -n "$slips" ]
	then
		if [[ $slips == ?([0-9]) ]]
		then
			SLIPDAYS=$slips
			echo "    $SLIPDAYS days"
		else
			echo "    INVALID SLIPDAYS: $slips"
			let errors+=1
		fi
	else
		echo "    EMPTY SLIPDAYS ENTRY"
		let errors+=1
	fi
else
	echo "    no SLIPDAYS: entry"
fi

echo "... checking for other expected files"
checkFiles $MAKEFILE $EXPECTED
let errors+=$?

# make sure we find files with all the expected suffixes
if [ -n "$SUFFIXES" ]; then
	echo "... checking for other files of expected types"
	checkSuffixes $SUFFIXES
	let errors+=$?
fi

echo "... checking for required Make targets"
checkTarget clean
let errors+=$?
checkTarget dist
let errors+=$?

echo "... checking for required compillation options"
checkMakefile Wall
let errors+=$?
checkMakefile Wextra
let errors+=$?

# make sure we can build the expected program
echo "... building default target(s)"
make 2> STDERR
testRC $? 0
let errors+=$?
noOutput STDERR
let errors+=$?

echo "... deleting programs and data to force rebuild"
rm -f $PGMS

echo "... checking make dist"
make dist 2> STDERR
testRC $? 0
let errors+=$?

checkFiles $TARBALL
if [ $? -ne 0 ]; then
	echo "ERROR: make dist did not produce $tarball"
	let errors+=1
fi

echo " ... checking make clean"
rm -f STDERR
make clean
testRC $? 0
let errors+=$?
dirCheck $TEMP $$
let errors+=$?

#
# now redo the default make and start testing functionality
#
echo "... redo default make"
make 2> STDERR
testRC $? 0
let errors+=$?
noOutput STDERR
let errors+=$?

echo "... checking for expected products"
checkPrograms $PGMS
let errors+=$?

# see if they detect and report invalid arguments
for p in $PGMS
do
	echo "... $p detects/reports bogus arguments"
	./$p --bogus < /dev/tty > /dev/null 2>STDERR
	testRC $? $EXIT_ARG
	if [ ! -s STDERR ]
	then
		echo "No Usage message to stderr for --bogus"
		let errors+=1
	else
		echo -n "        "
		cat STDERR
	fi
done


# test the standard arguments
echo
p=2
s="C"
echo "... $PGM supports --scale, --period, --log"
./$PGM --period=$p --scale=$s --log="LOGFILE" <<-EOF
SCALE=F
PERIOD=1
START
STOP
LOG test
OFF
EOF
ret=$?
if [ $ret -ne 0 ]
then
	echo "RETURNS RC=$ret"
	let errors+=1
fi

if [ ! -s LOGFILE ]
then
	echo "did not create a log file"
	let errors+=1
else
	echo "... $PGM supports and logs all sensor commands"
	for c in SCALE=F PERIOD=1 START STOP OFF SHUTDOWN "LOG test"
	do
		grep "$c" LOGFILE > /dev/null
		if [ $? -ne 0 ]
		then
			echo "DID NOT LOG $c command"
			let errors+=1
		else
			echo "    $c ... RECOGNIZED AND LOGGED"
		fi
	done

	if [ $errors -gt 0 ]
	then
		echo "   LOG FILE DUMP FOLLOWS   "
		cat LOGFILE
	fi
fi

# test input from pipe_test
echo
echo "... $PGM correctly processes input from $TEST_PGM"
../$TEST_PGM ./$PGM --period=$p --scale=$s --log="LOGFILE" > STDOUT 2> STDERR <<-EOF
	PAUSE 2
	SEND "SCALE=F\n"
	SEND "PERIOD=1\n"
	SEND "START\n"
	PAUSE 2
	SEND "STOP\n"
	SEND "LOG test\n"
	SEND "OFF\n"
	CLOSE
EOF

if [ ! -s LOGFILE ]
then
	echo "did not create a log file"
	let errors+=1
else
	for c in SCALE=F PERIOD=1 START STOP OFF SHUTDOWN "LOG test"
	do
		grep "$c" LOGFILE > /dev/null
		if [ $? -ne 0 ]
		then
			echo "DID NOT LOG $c command"
			let errors+=1
		else
			echo "    $c ... RECOGNIZED AND LOGGED"
		fi
	done

	if [ $errors -gt 0 ]
	then
		echo "   LOG FILE DUMP FOLLOWS   "
		cat LOGFILE
	fi
fi

echo
echo "... correct reporting format"
egrep '[0-9][0-9]:[0-9][0-9]:[0-9][0-9] [0-9]+\.[0-9]\>' LOGFILE > FOUND
if [ $? -eq 0 ] 
then
	echo "   " `cat FOUND` "... OK"
else
	echo NO VALID REPORTS IN LOGFILE:
	let errors+=1
	cat LOGFILE
fi

#echo "... usage of expected library functions"
#for r in sched_yield pthread_mutex_lock pthread_mutex_unlock __sync_lock_test_and_set __sync_lock_release
#do
#	grep $r *.c > /dev/null
#	if [ $? -ne 0 ] 
#	then
#		echo "No calls to $r"
#		let errors+=1
#	else
#		echo "    ... $r ... OK"
#	fi
#done

echo
if [ $SLIPDAYS -eq 0 ]
then
	echo "THIS SUBMISSION WILL USE NO SLIP-DAYS"
else
	echo "THIS SUBMISSION WILL USE $SLIPDAYS SLIP-DAYS"
fi

echo
echo "THE ONLY STUDENTS WHO WILL RECEIVE CREDIT FOR THIS SUBMISSION ARE:"
commas=`echo $ID | tr -c -d "," | wc -c`
let submitters=commas+1
let f=1
while [ $f -le $submitters ]
do
	id=`echo $ID | cut -d, -f$f`
	mail=`echo $EMAIL | cut -d, -f$f`
	echo "    $id    $mail"
	let f+=1
done
echo

# delete temp files, report errors, and exit
cleanup $$ $errors
