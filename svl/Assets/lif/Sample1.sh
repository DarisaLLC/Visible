#!/bin/bash

#  If no args are passed to the script set directories-to-search 
#+ to current directory.
# The split file has the same name as the bash script but with extension .lif
######################

[ $# -eq 0 ] && directorys=`pwd` || directorys=$@
me=`basename "$0"`
file=${me//sh/lif}
echo $file
xa="xaa"
xb="xab"
xc="xac"

if [ ! -e "$file" ]       # Check if file exists.
then
    echo "file does not exist"
    cat $xa $xb $xc > $file
fi

