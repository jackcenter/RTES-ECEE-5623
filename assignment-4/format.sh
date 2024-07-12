#!/bin/bash

SEARCH_DIR=./src
FORMATTED_FILE=formatted_file.tmp
RESULT_FILE=output_file.tmp

for file in $(find $SEARCH_DIR -name '*.c' -o -name '*.cpp' -o -name '*.h')
do 
    clang-format -style=file $file > $FORMATTED_FILE
    diff -q $file $FORMATTED_FILE >> $RESULT_FILE
done

if [ -s $RESULT_FILE ]
then
    echo "clang-format differences for:"
    while IFS= read -r line;
    do
        echo "$line"
    done < $RESULT_FILE
    rm $FORMATTED_FILE
    rm $RESULT_FILE
    exit 1
else
    echo "Pass"
    rm $FORMATTED_FILE
    rm $RESULT_FILE 
    exit
fi