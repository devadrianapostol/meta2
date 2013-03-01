#!/bin/bash
# generate reproducible snapshot

# check for changes
if git diff --quiet; then
else
    echo "There are still uncommited changes in the working directory."
    echo "Please commit them first before generating a snapshot."
    exit 1
fi

# obtain HEAD id
fingerprint=$(git rev-parse HEAD)

# force rebuild
touch meta2.meta2
gmake -f Makefile META2=./meta2 all check

# generate bootstrap source
echo '#define FINGERPRINT \\' >bootstrap/meta2.c
./meta2 -g >>bootstrap/meta2.c
./meta2 -v >>bootstrap/meta2.c
echo '""' >>bootstrap/meta2.c

# commit bootstrap source
git commit -m "snapshot" bootstrap/meta2.c
