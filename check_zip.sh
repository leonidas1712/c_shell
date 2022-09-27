#!/bin/bash

function check_files {
    for i do
        if [ ! -f $i ]; then
            return 1
        fi
    done
}

function check_compile {
    x=1
    make #> /dev/null
    x=$?
    [[ $x -ne 0 ]]
}

function transfer_files {
    files=(
        "Makefile"
        "myshell.h"
        "driver.c"
        "monitor.c"
        "fork-wrapper.c"
    )
    progfile=(
        "programs/goingtosleep.c"
        "programs/lazy.c"
        "programs/Makefile"
        "programs/result"
        "programs/showCmdArg"
    )
   for file in "${files[@]}"; do
        cp $1/$file $2
    done
    for file in "${progfile[@]}"; do
        cp $1/$file $2/$3
    done

}

function cleanup_and_exit {
    cd $1
    cd ../
    rm -rf $2
    exit $3
}

echo "Checking zip file...."

if [ "$#" -ne 1 ]; then
    echo "You should provide your zip file as a parameter and nothing else!"
    exit 1
fi

if ! [[ $1 =~ ^E[0-9]{7}.zip$ ]]; then
    echo "zip file is wrongly named: it should be <NUSNET ID>.zip"
    exit 1
fi

echo "Unzipping file: $1"

if [ ! -f $1 ]; then
    echo "Directory $1 does not exist."
    exit 1
fi

tmp_folder="test_grading_123456"
rm -rf $tmp_folder
mkdir $tmp_folder

cp $1 $tmp_folder

cd $tmp_folder
unzip $1 > /dev/null
rm $1


STUDENT_FN=${1%.*}
if [ ! -d $STUDENT_FN ]; then
    echo "Directory $STUDENT_FN does not exist."
    cleanup_and_exit "." $tmp_folder
    exit 1
fi


files=("myshell.c")

#pwd
cd $STUDENT_FN
#pwd

# Check if has bonus
has_bonus=false
if [ -d "bonus" ]; then
    has_bonus=true
fi

# Check if myshell.c is present

if ! check_files "${files[@]}"; then
    echo "Failed - myshell.c missing"
    cleanup_and_exit "../../" $tmp_folder 1
fi


if $has_bonus; then
    cd "bonus"
    if ! check_files "${files[@]}"; then
        echo "Bonus: Failed - bonus/myshell.c missing"
        cleanup_and_exit "../" $tmp_folder 1
    fi
    cd ../
fi

# Check if there are extra files
num_files=$(( $(find -type f -printf "\n" | wc -l) + 0 ))

if $has_bonus; then
    if [[ num_files -ne 2 ]]; then
        echo "Failed - extra files detected"
        cleanup_and_exit "../" $tmp_folder 1
    fi
else
    if [[ num_files -ne 1 ]]; then
        echo "Failed - extra files detected"
        cleanup_and_exit "../" $tmp_folder 1
    fi
fi

# Get skeleton files
echo "Transferring necessary skeleton files"
sk="skeleton"
programs="programs"
bonus="bonus"
SK_FILENAME="lab2.tar.gz"
mkdir $sk
cd $sk
wget --no-check-certificate 'https://docs.google.com/uc?export=download&id=1ywsVYgoO08XrOGzgxFepbfGxlH6zUObV' -O $SK_FILENAME  2> /dev/null
tar -zxf $SK_FILENAME > /dev/null

cd ../

LAB_FN=${SK_FILENAME%.*.*}
#echo "lab:" $LAB_FN
mkdir $programs
files=("myshell.c")
transfer_files $sk/$LAB_FN . $programs


# Check compilation
echo "Compiling your program...."

	
if check_compile; then
    echo "Failed - does not compile"
    cleanup_and_exit "../" $tmp_folder
fi



if $has_bonus; then
    echo "Compiling your bonus...."
    mkdir $bonus/$programs
    
    transfer_files $sk/$LAB_FN $bonus $programs	 
    cd "bonus"
    if check_compile; then
        echo "Bonus: Failed - does not compile"
        cleanup_and_exit "../../" $tmp_folder 1
    fi
    cd ../
fi



echo "**************************************************************"
echo "*            All checks have passed successfully             * "
echo "**************************************************************"

cleanup_and_exit "../" $tmp_folder 0
