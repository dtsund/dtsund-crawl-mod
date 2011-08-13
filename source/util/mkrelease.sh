#!/bin/sh
# mkrelease.sh: Creates a release suitable for distfiles.atheme.org.
#
# Copyright (c) 2007, 2011 atheme.org
#
# Permission to use, copy, modify, and/or distribute this software for
# any purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

# The project name is unlikely to change; variant maintainers will want to
# change this line, though.
PROJECT=CrawlLight

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
# a hash code.  GITDIR is the *absolute* pathway to the project's base directory.
VERSION=`git describe --tags --always`
RELEASENAME="$PROJECT-$VERSION"
TIP=`git describe --tags --always --long`
GITDIR=`git rev-parse --show-cdup`
GITDIR=`realpath $GITDIR`

WRKDIR=`pwd`

# UGLY UGLY UGLY
# When making this on MinGW, the 'scripts' (not really scripts at all, just C++
# programs that *should* rightly be scripts) that generate the .cc and .h files
# for tile definitons just don't work.  Therefore, these files need to be
# packaged in the tarball.  Since make clean nukes them, we have to regenerate
# them before packaging.
# TODO: replace the tilegen program with an honest-to-gog script.
cd $GITDIR/source/rltiles/
make all
cd tool/
rm *.o *.d *.elf
cd $WRKDIR

# Delete possible old residual crud.
if [ -d $RELEASENAME ]; then
	echo "Deleting previous release named $RELEASENAME."
	rm -rf $WRKDIR/$RELEASENAME/
fi

echo "Making release named $RELEASENAME (tip $TIP)"

echo "Making build.h..."
printf "#define CRAWL_VERSION_FINAL %s\n#define CRAWL_VERSION_SHORT \"%s\"\n#define CRAWL_VERSION_LONG \"%s\"\n" "$FINAL" "$VERSION" "$TIP" > $GITDIR/source/build.h

echo "Building root: $RELEASENAME/"
cd $GITDIR || exit 1
git archive --format=tar --prefix=$RELEASENAME/ HEAD | gzip >$WRKDIR/$RELEASENAME-working.tar.gz || exit 1
cd $WRKDIR || exit 1

# git archive can't just make directories for some mysterious reason, so
# we have it make a tarball and immediately untar it.
tar -xzvf $RELEASENAME-working.tar.gz || exit 1
cd $RELEASENAME || exit 1
rm -rf .gitignore
rm -rf .indent.pro source/util/makerelease.sh

# We can't tell git about build.h because the act of committing a new version
# of build.h actually changes what should be in the thing.  Therefore, we
# generate the thing above and manually move it here, instead of relying on
# git archive to put it in the tarball.
cp $GITDIR/source/build.h $WRKDIR/$RELEASENAME/source/build.h

# Run application specific instructions here.
if [ -x "$GITDIR/scripts/application.sh" ]; then
	source $GITDIR/scripts/application.sh
fi

cd ..

echo "Building $RELEASENAME.tar.gz from $RELEASENAME/"
tar zcf $RELEASENAME.tar.gz $RELEASENAME/

echo "Building $RELEASENAME.tar.bz2 from $RELEASENAME/"
tar jcf $RELEASENAME.tar.bz2 $RELEASENAME/

rm $RELEASENAME-working.tar.gz

ok="0"

echo
echo "Done. If you have any bugs to report, report them against"
echo "the distfiles.atheme.org component at http://jira.atheme.org"
echo "Thanks!"
echo
