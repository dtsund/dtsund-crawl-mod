#!/bin/sh

# The script takes one argument: true if it's a release version, false otherwise.
# It does *not* check whether you typo'd; if you do so, Crawl Light won't
# compile.
if [ "x$1" = "x" ]; then
    echo "Usage: $0 version <true|false>"
    exit
else
    FINAL="$1"
fi

# VERSION will be the short version number, TIP will be the version number with
# a hash code.
VERSION=`git describe --tags --always`
TIP=`git describe --tags --always --long`

printf "#define CRAWL_VERSION_FINAL %s\n#define CRAWL_VERSION_SHORT \"%s\"\n#define CRAWL_VERSION_LONG \"%s\"\n" "$FINAL" "$VERSION" "$TIP" > build.h

