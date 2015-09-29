#!/bin/bash

CMD_ADD="clang4vim add"
CMD_RM="clang4vim rm"
CMD_CLEAR="clang4vim clear"

# Specify files to index here
SOURCE_PATH=`cd $1; pwd` # convert $1 to an absolute path
find $SOURCE_PATH\
    -name "*.cpp" -or\
    -name "*.hpp" -or\
    -name "*.cxx" -or\
    -name "*.hxx" -or\
    -name "*.cc" -or\
    -name "*.c" -or\
    -name "*.h"\
    | sort > files2.txt

add_to_index() {
    INDEX_FILE=`echo ${1}.i.gz | tr "/" "%"`
    echo $CMD_ADD index.db $INDEX_FILE `cat ${SOURCE_PATH}/.clang_complete` $1
    $CMD_ADD index.db $INDEX_FILE `cat ${SOURCE_PATH}/.clang_complete` $1 || exit 1
}

remove_from_index() {
    INDEX_FILE=`echo ${1}.i.gz | tr "/" "%"`
    echo CMD_RM index.db $INDEX_FILE
    $CMD_RM index.db $INDEX_FILE || exit 1
    echo rm $INDEX_FILE
    rm $INDEX_FILE
}

if [ ! -f index.db -o ! -f files.txt ]; then
    echo "Creating database"
    CMD_CLEAR index.db
    for i in `cat files2.txt`; do
        add_to_index $i
    done
    mv files2.txt files.txt
    exit
fi

echo "Updating database"
#Generate the list of files added since last time
comm -23 files2.txt files.txt > filesadded.txt

for i in `cat filesadded.txt`; do
    add_to_index $i
done

# Remove removed files
for i in `comm -23 files.txt files2.txt`; do
    remove_from_index $i
done

# Update modified files
for i in `cat files.txt`; do
    if [ -f $i -a $i -nt files.txt ]; then
        remove_from_index $i
        add_to_index $i
    fi
done

rm filesadded.txt
mv files2.txt files.txt
