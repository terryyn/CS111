#!/bin/bash
#
# sanity check script for Project 3B
#	extract tar file
#	required README fields (ID, EMAIL, NAME)
#	required Makefile targets (clean, dist, graphs, tests)
#	make default
#	make dist
#	make clean (returns directory to untared state)
#	make default, success, creates program
#	unrecognized parameters
#	error message for nonexistent input file
#	identical output for trivial test case
#
LAB="lab3b"
README="README"
MAKEFILE="Makefile"

EXPECTED=""
EXPECTEDS=""
PGM="lab3b"
PGMS="$PGM"

SUFFIXES=""

LIBRARY_URL="www.cs.ucla.edu/classes/cs111/Software"
PROJECT_URL="www.cs.ucla.edu/classes/cs111/Samples"

EXIT_OK=0
EXIT_ARG=1
EXIT_ERR=2

TIMEOUT=1
MIN_TESTS=20

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
	echo "... checking for source files"
	let found=0
	for s in $SUFFIXES
	do
		names=`echo *.$s`
		if [ "$names" != '*'.$s ]; then
			echo "    $names ... OK"
			let found=1
		fi
	done
	if [ $found -le 0 ]; then
		echo "no C/C++ source files found"
		let errors+=1
	fi
fi

echo "... checking for required Make targets"
checkTarget clean
let errors+=$?
checkTarget dist
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
		echo -n "    "
		cat STDERR
	fi
done

# make sure it detects a non-existent image
echo "... $PGM detects/reports non-existent input image"
rm -f STDERR
./$PGM NO_SUCH_INPUT.img > /dev/null 2>STDERR
if [ $? -eq 0 ]; then
	echo "ERROR: non-existent input file returns 0"
	let errors+=1
else
	echo "    RC!=0 ... OK"
fi

if [ ! -s STDERR ]
then
	echo "No error message to stderr for non-existent input file"
	let errors+=1
else
	echo -n "    "
	cat STDERR
fi

echo
let b=1
# run through all of the supplied test cases
while :
do
	# download the samples and error output
	pfx="P3B-test_"
	for s in csv err
	do
		f=$pfx$b.$s
		wget $PROJECT_URL/$f > /dev/null 2> /dev/null
		ret=$?
		if [ $b -lt $MIN_TESTS -a $ret -ne 0 ]; then
			echo "Unable to download testcase $f from $PROJECT_URL"
		fi
	done
	if [ $ret -ne 0 ]; then
		break;
	fi

	# run the program on the sample, and compare the output
	echo "... executing test case $pfx$b"
	timeout $TIMEOUT ./$PGM $pfx$b.csv | sort > TEST.out
	sort $pfx$b.err > GOLDEN.out
	cmp GOLDEN.out TEST.out
	if [ $? -ne 0 ]; then
		echo "        incorrect output for $b.csv"
		echo "YOUR OUTPUT ..."
		cat TEST.out
		echo "EXPECTED OUTPUT ..."
		cat GOLDEN.out
		echo "=========="
		let errors+=1
	else
		echo "        all" `wc -l < GOLDEN.out` "lines agree"
	fi
	let b+=1
done

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
