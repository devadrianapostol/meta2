#!/bin/bash
# usage: bash snapshot.sh [-n] 
#
# generate reproducible snapshot

set -e

# check for changes
if git diff --quiet; then :
else
    if test "$1" \!= "-n"; then
	echo "There are still uncommited changes in the working directory."
	echo "Please commit them first before generating a snapshot."
	exit 1
    fi
fi

# obtain HEAD id
fingerprint=$(git rev-parse HEAD)

# force rebuild
touch meta2.meta2
make -f Makefile all check

# generate bootstrap source
echo '#define FINGERPRINT "'$fingerprint'"' >bootstrap/meta2.c
echo '#define GENEALOGY \' >>bootstrap/meta2.c

for v in $(bootstrap/meta2 -g); do
    echo '"'$v'\n" \' >>bootstrap/meta2.c
done

parent=$(bootstrap/meta2 -v)
echo '"'$parent'\n"' >>bootstrap/meta2.c
cat meta2.h meta2.c >>bootstrap/meta2.c

# regenerate meta2.c
make -f Makefile bootstrap

# commit bootstrap source
if test "$1" \!= "-n"; then
    echo "committing snapshot"
    git commit -m "snapshot" bootstrap/meta2.c
fi
