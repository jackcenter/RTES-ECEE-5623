#!/bin/bash

SEARCH_DIR=./
RESULT_FILE=output_file.tmp

cppcheck --enable=all --quiet --inline-suppr --suppressions-list=.suppress.cppcheck $SEARCH_DIR> $RESULT_FILE

if [ -s $RESULT_FILE ]
then
    echo "Pass"
    rm $RESULT_FILE
    exit
else
    rm $RESULT_FILE
    exit 1
fi