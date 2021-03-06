#
# Crawl Light
# GNU Makefile
#
# largely written by Steven Noonan <steven@uplinklabs.net>
#    (if something breaks, blame him.)
#

# Typical targets:
#    make
#    make wizard
#    make debug
#    make install
#    make wizard install
#    make debug install
#    -- note, unlike most programs, you need to specify build type when
#       installing even if you just built it.
# Typical parameters:
#    TILES         -- set to anything to enable tiles build
#
#    CROSSHOST     -- target system, eg, i386-pc-msdosdjgpp or i586-mingw32msvc
#
#    prefix        -- installation base.  Specify eg. /usr/local on Unix systems.
#    DESTDIR       -- installation staging area (the dir you intend to pack)
#    DATADIR       -- place to hold immutable files.  Can be either relative to
#                     "prefix" or absolute.
#    SAVEDIR       -- place to hold writeable data (saves, database, morgue
#                     dumps).  Can be relative to "prefix", absolute or placed
#                     in the user's home dir (~).  Remember to protect the ~
#                     from your shell!
#                     Warning, shared directories are no longer supported.
#    SHAREDDIR     -- place to hold writeable data common to multiple
#                     players (scores, the logfile, ghosts).  Will be placed
#                     inside the SAVEDIR if not specified.
#       Layout examples:
#         prefix=~/crawl DATADIR=data/ SAVEDIR=saves/
#                  -- everything under ~/crawl
#         prefix=/usr/local
#                  -- system-wide installation
#
#    V             -- set to anything to enable verbose build
#
#    USE_ICC       -- set to use Intel's compiler
#    ASSERTS       -- set to enable assertion checks (implied in debug mode)
#
#
# Requirements:
#    For tile builds, you need pkg-config.
#    You also need libpng, sdl, sdl-image and libfreetype -- if you got your
#    source from git, you can 'git submodule update' to fetch them; you can also
#    ask for a package with convenience libraries instead -- we'll try to provide
#    them somewhere in the near future.

GAME = crawl

ASSERTS = yes

# Disable GNU Make implicit rules and variables. Leaving them enabled will slow
# down MinGW and Cygwin builds by a very VERY noticeable degree. Besides, we have
# _explicit_ rules defined for everything. So we don't need them.
MAKEFLAGS += -rR

#
# Compiler Flags
#
# The compiler flag variables are separated into their individual
# purposes, making it easier to deal with the various tools involved
# in a compile.
#
# These are also divided into global vs. local flags. So for instance,
# CFOPTIMIZE affects Crawl, Lua, and SQLite, while CFOPTIMIZE_L only
# affects Crawl.
#
# The variables are as follows:
# CFOPTIMIZE(_L) - Optimization flags
# CFWARN(_L) - Warning flags
# CFOTHERS(_L) - Anything else
#


ifdef USE_ICC

# If you have a Core 2 processor, this _really_ makes things fly:
#CFOPTIMIZE := -O2 -parallel -xT

# Optionally enable the 'ipo' feature, to facilitate inlining
# across object file bounds.
#CFOPTIMIZE_L := -ipo

# Some very good optimization flags.
CFOPTIMIZE := -O2 -parallel

else

CFOPTIMIZE := -O2

endif # USE_ICC

CFOTHERS := -pipe $(EXTERNAL_FLAGS) -fPIC
CFOTHERS_L := -fsigned-char
CFWARN := -Wall
CFWARN_L := -Wundef

DEFINES := $(EXTERNAL_DEFINES)

#
# The GCC and GXX variables are set later.
#
AR = ar
RANLIB = ranlib
CC = $(GCC)
CXX = $(GXX)
RM = rm -f
COPY = cp
COPY_R = cp -r
STRIP = strip -s
WINDRES = windres
CHMOD = chmod 2>/dev/null
CHOWN = chown 2>/dev/null
PNGCRUSH = $(COPY)
PNGCRUSH_LABEL = COPY
ADVPNG = advpng -z -4
DOXYGEN = doxygen
DOXYGEN_SIMPLE_CONF = crawl_simple.doxy
DOXYGEN_ALL_CONF = crawl_all.doxy
DOXYGEN_HTML_GEN = html/

export AR
export RANLIB
export RM
export CC
export CXX
export CFLAGS
export MAKEFLAGS
export CONFIGURE_FLAGS
export uname_S

LIBPCRE := contrib/install/lib/libpcre.a
LIBSDL := contrib/install/lib/libSDL.a
LIBPNG := contrib/install/lib/libpng.a
LIBSDLIMAGE := contrib/install/lib/libSDL_image.a
LIBFREETYPE := contrib/install/lib/libfreetype.a
LIBSQLITE := contrib/install/lib/libsqlite3.a
ifdef USE_LUAJIT
LIBLUA := contrib/install/lib/libluajit.a
else
LIBLUA := contrib/install/lib/liblua.a
endif
LIBZ := contrib/install/lib/libz.a

LUA_INCLUDE_DIR := /usr/include/lua5.1
LUA_LIB := -llua5.1
SQLITE_INCLUDE_DIR := /usr/include
SQLITE_LIB := -lsqlite3

#
# Platform Detection
#
uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')
uname_M := $(shell sh -c 'uname -m 2>/dev/null || echo not')
uname_O := $(shell sh -c 'uname -o 2>/dev/null || echo not')
uname_R := $(shell sh -c 'uname -r 2>/dev/null || echo not')
uname_P := $(shell sh -c 'uname -p 2>/dev/null || echo not')
uname_V := $(shell sh -c 'uname -v 2>/dev/null || echo not')

HOST = $(shell sh -c 'cc -dumpmachine || echo unknown')
ARCH = $(HOST)

ifdef CROSSHOST
	ARCH = $(CROSSHOST)
	NO_PKGCONFIG = YesPlease
	NO_AUTO_OPT = YesPlease
	NEED_STATIC = YesPlease
	CONFIGURE_FLAGS += --host=$(CROSSHOST)
	BUILD_LUA = yes
	BUILD_SQLITE = yes
	BUILD_ZLIB = YesPlease

	# If needed, override uname_S so we get the appropriate
	# things compiled.
	ifneq (,$(findstring mingw,$(CROSSHOST)))
		uname_S=MINGW32
	endif
	ifneq (,$(findstring djgpp,$(CROSSHOST)))
		uname_S=DOS
	endif

endif
ifneq (,$(findstring MINGW,$(uname_S)))
	GAME = crawl.exe
	bin_prefix = .
	WIN32 = Yes
	NO_RDYNAMIC = YesPlease
	NO_NCURSES = YesPlease
	NEED_LIBW32C = YesPlease
	BUILD_PCRE = YesPlease
	BUILD_ZLIB = YesPlease
	DEFINES_L += -DWINMM_PLAY_SOUNDS
	EXTRA_LIBS += -lwinmm
	ifdef TILES
		EXTRA_LIBS += -lmingw32 -lgdi32 -lwinmm contrib/install/lib/libSDLmain.a -mwindows
		BUILD_FREETYPE = YesPlease
		BUILD_SDL = YesPlease
		BUILD_SDLIMAGE = YesPlease
		BUILD_LIBPNG = YesPlease
	endif
endif
ifeq ($(uname_S),DOS)
	GAME = crawl.exe
	bin_prefix = .
	NO_NCURSES = yes
	NEED_LIBDOS = yes
	BUILD_PCRE = yes
	NO_RDYNAMIC = yes
endif
ifeq ($(uname_S),Darwin)
	STRIP := strip -x
	NEED_APPKIT = YesPlease
	LIBNCURSES_IS_UNICODE = Yes
	NO_PKGCONFIG = Yes
	BUILD_SQLITE = YesPlease
	BUILD_ZLIB = YesPlease
	ifdef TILES
		EXTRA_LIBS += -framework AppKit -framework AudioUnit -framework Carbon -framework IOKit -framework OpenGL contrib/install/lib/libSDLmain.a
		BUILD_FREETYPE = YesPlease
		BUILD_SDL = YesPlease
		BUILD_SDLIMAGE = YesPlease
		BUILD_LIBPNG = YesPlease
	endif
endif
ifdef USE_LUAJIT
	BUILD_LUA := YesPlease
endif
ifdef USE_ICC
	NO_RDYNAMIC := YesPlease
endif
ifneq (,$(findstring CYGWIN,$(uname_S)))
	GAME = crawl.exe
	NO_RDYNAMIC = YesPlease
	BUILD_PCRE = YesPlease
endif

ifdef BUILD_ALL
	BUILD_FREETYPE = YesPlease
	BUILD_PCRE = YesPlease
	BUILD_SDL = YesPlease
	BUILD_SDLIMAGE = YesPlease
	BUILD_SQLITE = YesPlease
	BUILD_LUA = YesPlease
	BUILD_LIBPNG = YesPlease
	BUILD_ZLIB = YesPlease
endif

#
# Set up the TILES variant
#
ifdef TILES

# For now, everything but the iPad uses the standard
# SDL/OpenGL/Freetype combination.
ifndef IPAD
	SDL = YesPlease
	GL = YesPlease
	FT = YesPlease
endif

endif

#
# Check for an Apple-released compiler.
#
ifeq ($(uname_S),Darwin)
ifneq ($(shell gcc -v 2>&1 | grep Apple),)
APPLE_GCC = YesPlease
endif
endif

#
# Set up object file dependencies for $(GAME) target.
#
include makefile.obj

# Just a quick hack to make it clean up
# tiles-specific object files too.
ifneq (,$(findstring clean,$(MAKECMDGOALS)))
TILES := YesPlease
endif

# Works for Mac OS X and Linux.
OBJECTS += crash-u.o

ifdef WIN32
EXTRA_OBJECTS += icon.o
endif

ifndef TILES
ifdef NEED_LIBW32C
OBJECTS += libw32c.o
else
ifdef NEED_LIBDOS
OBJECTS += libdos.o
else
OBJECTS += libunix.o
endif
endif
endif

# To get stack trace symbols.
# Note that MinGW doesn't support -rdynamic.
ifndef NO_RDYNAMIC
LDFLAGS := -rdynamic
endif
ifdef NEED_STATIC
LDFLAGS += -static
endif

ifdef USE_MERGE_BASE
MERGE_BASE := $(shell git merge-base HEAD $(USE_MERGE_BASE))
endif

ifdef USE_DGAMELAUNCH
# Permissions to set on the game executable.
MCHMOD := 755

# Permissions to set on the save directory.
MCHMOD_SAVEDIR := 775
MCHMOD_LOGS := 664

# The user:group to install the game as.
INSTALL_UGRP := games:games
endif

chroot_prefix :=
prefix        :=

ifeq ($(patsubst %/local,%,$(patsubst %/,%,$(prefix))),/usr)
FHS := yes
endif

ifeq (,$(bin_prefix))
ifneq ($(patsubst %/,%,$(prefix)),/usr)
bin_prefix    := bin
else
bin_prefix    := games
endif
endif

# If you're installing Crawl for multiple users, you *must* set this to a
# valid path before building Crawl. This is not necessary if you are building
# Crawl for a single user.
# If you're installing to /usr or /usr/local, we have sane defaults.

# SAVEDIR := saves/
# DATADIR := data/
ifneq (,$(FHS))
DATADIR       := share/crawl
SAVEDIR       := ~/.crawl
endif

ifneq ($(patsubst /opt%,%,$(prefix)),$(prefix))
DATADIR := data
SAVEDIR := ~/.crawl
endif


INCLUDES_L += -Icontrib/install/include
LIBS += -Lcontrib/install/lib

INCLUDES_L += -Iutil -I.

ifdef APPLE_GCC

ARCH := $(uname_M)
export ARCH

ifeq ($(ARCH),ppc)
	SDK_VER := 10.4
endif
ifeq ($(ARCH),i386)
	SDK_VER := 10.4
endif
ifeq ($(ARCH),x86_64)
ifdef TILES
	SDK_VER := 10.6
else
	SDK_VER := 10.5
endif
endif

# Mac OS X 10.4 adds a 'u' on the end of the SDK name. Everything
# else is much easier to predict the name of.
ifeq ($(SDK_VER),10.4)
GCC_VER := 4.0
SDKROOT := /Developer/SDKs/MacOSX$(SDK_VER)u.sdk
else
GCC_VER := 4.2
SDKROOT := /Developer/SDKs/MacOSX$(SDK_VER).sdk
endif

ifdef BUILD_UNIVERSAL
# [ds] 10.4 SDK g++-4.0 + x86_64 runs into SDL compile issues.
CFLAGS_ARCH := -arch i386 -arch ppc -faltivec
CFLAGS_DEPCC_ARCH := -arch i386
else
CFLAGS_ARCH := -arch $(ARCH)
endif

CC = $(GCC) $(CFLAGS_ARCH) -isysroot $(SDKROOT) -mmacosx-version-min=$(SDK_VER)
CXX = $(GXX) $(CFLAGS_ARCH) -isysroot $(SDKROOT) -mmacosx-version-min=$(SDK_VER)
DEPCC = $(GCC) $(or $(CFLAGS_DEPCC_ARCH),$(CFLAGS_ARCH)) -isysroot $(SDKROOT) -mmacosx-version-min=$(SDK_VER)
DEPCXX = $(GXX) $(or $(CFLAGS_DEPCC_ARCH),$(CFLAGS_ARCH)) -isysroot $(SDKROOT) -mmacosx-version-min=$(SDK_VER)

ifdef USE_ICC
CC += -gcc-name=gcc-$(GCC_VER) -gxx-name=g++-$(GCC_VER)
CXX += -gcc-name=gcc-$(GCC_VER) -gxx-name=g++-$(GCC_VER)
endif

endif # MacOS

ifndef CROSSHOST

ifneq ($(GCC_VER),)
# We do this in a separate variable because if we
# specify GCC_VER on the make command-line, the
# variable is immutable, and we can't add the dash.
GCC_VER_SUFFIX:=-$(GCC_VER)
endif

# Attempt to use a full compiler name, to make
# distcc builds work nicely.
LMACH := $(shell gcc -dumpmachine)-
ifeq ($(LMACH),-)
LMACH :=
endif
ifeq ($(shell which $(LMACH)gcc$(GCC_VER_SUFFIX) > /dev/null 2> /dev/null && echo "Yes"),)
LMACH :=
endif

GCC := $(LMACH)gcc$(GCC_VER_SUFFIX)
GXX := $(LMACH)g++$(GCC_VER_SUFFIX)

else

# Cross-compiling is a weird case.
GCC := $(CROSSHOST)-gcc
GXX := $(CROSSHOST)-g++
AR := $(CROSSHOST)-ar
RANLIB := $(CROSSHOST)-ranlib
STRIP := $(CROSSHOST)-strip
WINDRES := $(CROSSHOST)-windres

endif

# Define this to automatically generate code optimized for your machine
# (GCC only as of now).
#
# NOTE: Don't use this with a release build, since the generated code
# won't work for all machines.
ifdef HURRY
NO_AUTO_OPT = YesPlease
endif

ifdef AUTO_OPT
ifndef NO_AUTO_OPT
CFOPTIMIZE += -march=native
endif
endif

ifndef BUILD_LUA
  ifneq (,$(wildcard $(LUA_INCLUDE_DIR)/lua.h))
    INCLUDES_L += -I$(LUA_INCLUDE_DIR)
    LIBS += $(LUA_LIB)
  else
    ifneq (,$(wildcard /usr/include/lua.h))
      LIBS += -llua
    else
      BUILD_LUA = yes
    endif
  endif
endif

ifndef BUILD_SQLITE
  ifeq ($(shell grep -q sqlite3_prepare $(SQLITE_INCLUDE_DIR)/sqlite3.h 2>/dev/null && echo yes),yes)
    INCLUDES_L += -I$(SQLITE_INCLUDE_DIR)
    LIBS += $(SQLITE_LIB)
    ifneq ($(shell grep -q sqlite3_prepare_v2 $(SQLITE_INCLUDE_DIR)/sqlite3.h 2>/dev/null && echo yes),yes)
      DEFINES_L += -DANCIENT_SQLITE
    endif
  else
    BUILD_SQLITE = yes
  endif
endif

ifndef BUILD_ZLIB
  LIBS += -lz
else
  LIBS += $(LIBZ)
endif

RLTILES = rltiles

#
# Tiles build stuff
#
ifdef TILES

DEFINES_L += -DUSE_TILE
INCLUDES_L += -I$(RLTILES)

ifdef BUILD_SDL
INCLUDES_L += -Icontrib/install/include/SDL
endif
ifdef BUILD_FREETYPE
INCLUDES_L += -Icontrib/install/include/freetype2
endif

ifdef SDL
DEFINES_L += -DUSE_SDL
endif

ifdef GL
DEFINES_L += -DUSE_GL
endif

ifdef GLES
DEFINES_L += -DUSE_GLES
endif

ifdef FT
DEFINES_L += -DUSE_FT
endif

# Okay, we have to assume we're on something else that
# uses standard UNIX-like methods for finding libs.
#
# For instance, on Linux and most other UNIX-likes,
# the app pkg-config can provide the appropriate
# CFLAGS and LDFLAGS.
#

ifndef NO_PKGCONFIG
ifneq ($(shell which pkg-config 2> /dev/null),)
PKGCONFIG = YesPlease
endif
endif

ifdef PKGCONFIG

# If pkg-config is available, it's the surest way to find where
# the contributing libraries are located.
#

FREETYPE_INCLUDE := $(shell pkg-config freetype2 --cflags-only-I)
FREETYPE_CFLAGS  := $(shell pkg-config freetype2 --cflags-only-other)
FREETYPE_LDFLAGS := $(shell pkg-config freetype2 --libs-only-L) $(shell pkg-config freetype2 --libs-only-l)

SDL_INCLUDE := $(shell pkg-config sdl --cflags-only-I)
SDL_CFLAGS  := $(shell pkg-config sdl --cflags-only-other)
SDL_LDFLAGS := $(shell pkg-config sdl --libs-only-L) $(shell pkg-config sdl --libs-only-l)

LIBS += -pthread -lSDL_image $(SDL_LDFLAGS) $(FREETYPE_LDFLAGS)

endif # pkg-config

ifdef GLES
LIBS += -lSDL_gles -lEGL -lGLES_CM
else
ifneq ($(uname_S),Darwin)
ifeq (,$(findstring MINGW,$(uname_S)))
LIBS += -lGL -lGLU
else
LIBS += -lopengl32 -lglu32
endif
endif
endif

DEFINES_L += $(PNG_CFLAGS) $(FREETYPE_CFLAGS) $(SDL_CFLAGS)
INCLUDES_L += $(PNG_INCLUDE) $(FREETYPE_INCLUDE) $(SDL_INCLUDE)

ifdef PROPORTIONAL_FONT
DEFINES += -DPROPORTIONAL_FONT=\"$(PROPORTIONAL_FONT)\"
endif
ifdef MONOSPACED_FONT
DEFINES += -DMONOSPACED_FONT=\"$(MONOSPACED_FONT)\"
endif

endif # TILES

# On clang, unknown -Wfoo is merely a warning, thus -Werror.
CFWARN_L += $(shell w=-Wno-array-bounds;echo|$(GCC) -E - -Werror $$w >/dev/null 2>&1 && echo $$w)

CFWARN_L += -Wno-parentheses -Wwrite-strings -Wshadow -D_FORTIFY_SOURCE=0
CFOTHERS_L = $(EXTERNAL_FLAGS_L) $(EXTRA_FLAGS) $(DEFINES) $(SDL_CFLAGS)

ifndef NO_LUA_BINDINGS
CFOTHERS_L += -DCLUA_BINDINGS
endif

#
# Figure out the build settings for this type of build
#

# Debug
# No optimization, full debugging.
ifneq (,$(findstring debug,$(MAKECMDGOALS)))
	FULLDEBUG=YesPlease
	WIZARD=YesPlease
	DEBUG=YesPlease
	NO_OPTIMIZE=YesPlease
endif

# Wizard
# Optimized, with wizard mode.
ifneq (,$(findstring wizard,$(MAKECMDGOALS)))
	WIZARD=YesPlease
	DEBUG=YesPlease
endif

# Profile
# Optimized, with full debugging.
ifneq (,$(findstring profile,$(MAKECMDGOALS)))
	FULLDEBUG=YesPlease
	WIZARD=YesPlease
	DEBUG=YesPlease
endif

ifdef HURRY
	NO_OPTIMIZE=YesPlease
endif

ifdef FULLDEBUG
DEFINES += -DFULLDEBUG
endif
ifdef DEBUG
CFOTHERS := -ggdb $(CFOTHERS)
DEFINES += -DDEBUG
endif
ifdef WIZARD
DEFINES += -DWIZARD
endif
ifdef NO_OPTIMIZE
CFOPTIMIZE  := -O0
endif
ifdef PCH
CFWARN_L += -Winvalid-pch
endif

ifdef ASSERTS
DEFINES += -DASSERTS
endif

# Cygwin has a panic attack if we do this...
ifndef NO_OPTIMIZE
CFWARN_L += -Wuninitialized
endif

ifneq ($(strip $(chroot_prefix)),)
	USE_CHROOT=YesPlease
endif

ifdef USE_DGAMELAUNCH
	CFOTHERS_L += -DDGAMELAUNCH
endif

ifdef USE_CHROOT
	prefix_fp := $(abspath $(strip $(DESTDIR)$(chroot_prefix))/$(strip $(prefix)))
else
	prefix_fp := $(abspath $(strip $(DESTDIR)$(prefix)))
endif

ifneq ($(strip $(SAVEDIR)),)
  ifneq ($(filter ~%,$(SAVEDIR)),)
    CFOTHERS_L += -DSAVE_DIR_PATH=\"$(SAVEDIR)\"
    savedir_fp :=
    shareddir_fp :=
  else
    ifeq ($(filter /%,$(SAVEDIR)),)
      ifneq ($(prefix),)
	override SAVEDIR := $(strip $(prefix))/$(strip $(SAVEDIR))
      endif
    endif
    CFOTHERS_L += -DSAVE_DIR_PATH=\"$(abspath $(SAVEDIR))\"
    savedir_fp := $(abspath $(strip $(DESTDIR))$(strip $(SAVEDIR)))
    shareddir_fp := $(savedir_fp)/saves
  endif
endif

ifneq ($(strip $(SHAREDDIR)),)
  ifneq ($(filter ~%,$(SHAREDDIR)),)
    CFOTHERS_L += -DSHARED_DIR_PATH=\"$(SHAREDDIR)\"
    shareddir_fp :=
  else
    ifeq ($(filter /%,$(SHAREDDIR)),)
      ifneq ($(prefix),)
	override SHAREDDIR := $(strip $(prefix))/$(strip $(SHAREDDIR))
      endif
    endif
    CFOTHERS_L += -DSHARED_DIR_PATH=\"$(abspath $(SHAREDDIR))\"
    shareddir_fp := $(abspath $(strip $(DESTDIR))$(strip $(SHAREDDIR)))
  endif
endif

ifneq ($(strip $(DATADIR)),)
  ifeq ($(filter /%,$(DATADIR)),)
    #relative DATADIR
    ifneq ($(prefix),)
	override DATADIR := $(strip $(prefix))/$(strip $(DATADIR))
    endif
  endif
  CFOTHERS_L += -DDATA_DIR_PATH=\"$(abspath $(DATADIR))/\"
else
  ifneq ($(prefix),)
	DATADIR := $(strip $(prefix))/$(strip $(DATADIR))
  endif
endif
datadir_fp := $(abspath $(strip $(DESTDIR))$(strip $(DATADIR)))

ifndef NO_NCURSES

NC_PREFIX = /usr

# Include path for (n)curses with Unicode support.

# Your ncurses library may include Unicode support, and you may not have a
# separate libncursesw; this is the case on Mac OS/Darwin.
ifdef LIBNCURSES_IS_UNICODE
NC_LIB = ncurses
NC_INCLUDE = $(NC_PREFIX)/include/ncurses
else
NC_LIB = ncursesw
NC_INCLUDE = $(NC_PREFIX)/include/ncursesw
endif

INCLUDES_L += -I$(NC_INCLUDE)

ifndef TILES
LIBS += -L$(NC_PREFIX)/lib -l$(NC_LIB)
endif

endif

ifdef BUILD_PCRE
DEFINES += -DREGEX_PCRE
LIBS += -lpcre
endif

ifdef USE_ICC
NO_INLINE_DEPGEN := YesPlease
GCC := icc
GXX := icpc
AR  := xiar
RANLIB := true
LIBS += -lguide -lpthread
CFWARN := -wd383,810,869,981,1418 -we14,193,304
CFWARN_L :=
endif

LDFLAGS += $(CFOPTIMIZE) $(CFOPTIMIZE_L)

ifdef REPORT
CFOTHERS += -ftime-report
endif

CFLAGS   := $(CFOPTIMIZE) $(CFOTHERS) $(CFWARN)
CFLAGS_L := $(CFOPTIMIZE_L) $(DEFINES_L) $(CFWARN_L) $(INCLUDES_L) $(CFOTHERS_L)
ALL_CFLAGS := $(CFLAGS) $(CFLAGS_L)
YACC_CFLAGS  := $(ALL_CFLAGS) -Wno-unused-function -Wno-sign-compare -DYYENABLE_NLS=0 -DYYLTYPE_IS_TRIVIAL=0

UTIL = util/

EXTRA_OBJECTS += $(YACC_OBJECTS)

LEX := $(shell which flex 2> /dev/null)
YACC := $(shell which bison 2> /dev/null)

ifeq ($(strip $(LEX)),)
NO_YACC = YesPlease
endif
ifeq ($(strip $(YACC)),)
NO_YACC = YesPlease
endif

ifneq ($(findstring $(MAKEFLAGS),s),s)
ifndef V
        QUIET_CC       = @echo '   ' CC $@;
        QUIET_CXX      = @echo '   ' CXX $@;
        QUIET_PCH      = @echo '   ' PCH $@;
        QUIET_LINK     = @echo '   ' LINK $@;
        QUIET_GEN      = @echo '   ' GEN $@;
        QUIET_COPY     = @echo '   ' COPY $@;
        QUIET_DEPEND   = @echo '   ' DEPEND $@;
        QUIET_WINDRES  = @echo '   ' WINDRES $@;
        QUIET_PNGCRUSH = @echo '   ' $(PNGCRUSH_LABEL) $@;
        QUIET_ADVPNG   = @echo '   ' ADVPNG $@;
        export V
endif
endif

ifdef TILES
TILEIMAGEFILES := floor wall feat main player gui icons
TILEDEFS = $(TILEIMAGEFILES) dngn unrand
TILEDEFPRES = $(TILEDEFS:%=$(RLTILES)/tiledef-%)
TILEDEFOBJS = $(TILEDEFPRES:%=%.o)
TILEDEFSRCS = $(TILEDEFPRES:%=%.cc)
TILEDEFHDRS = $(TILEDEFPRES:%=%.h)

TILEFILES = $(TILEIMAGEFILES:%=%.png)
ORIGTILEFILES = $(TILEFILES:%=$(RLTILES)/%)
DESTTILEFILES = $(TILEFILES:%=dat/tiles/%)

OBJECTS += $(TILEDEFOBJS)

ifndef NO_OPTIMIZE
  ifneq (,$(shell which advpng))
    USE_ADVPNG = y
  else
    ifneq (,$(shell which pngcrush))
      PNGCRUSH = pngcrush -q -m 113
      PNGCRUSH_LABEL = PNGCRUSH
    endif
  endif
endif
endif

ifdef BUILD_PCRE
CONTRIBS += pcre
CONTRIB_LIBS += $(LIBPCRE)
endif
ifdef BUILD_FREETYPE
CONTRIBS += freetype
CONTRIB_LIBS += $(LIBFREETYPE)
endif
ifdef BUILD_SDLIMAGE
CONTRIBS += sdl-image
CONTRIB_LIBS += $(LIBSDLIMAGE)
endif
ifdef BUILD_SDL
CONTRIBS += sdl
CONTRIB_LIBS += $(LIBSDL)
endif
ifdef BUILD_LIBPNG
CONTRIBS += libpng
CONTRIB_LIBS += $(LIBPNG)
endif
ifdef BUILD_ZLIB
CONTRIBS += zlib
CONTRIB_LIBS += $(LIBZ)
endif
ifdef BUILD_LUA
ifdef USE_LUAJIT
CONTRIBS += luajit/src
CFOTHER_L += -DUSE_LUAJIT
else
CONTRIBS += lua/src
endif
CONTRIB_LIBS += $(LIBLUA)
endif
ifdef BUILD_SQLITE
CONTRIBS += sqlite
CONTRIB_LIBS += $(LIBSQLITE)
endif

EXTRA_OBJECTS += version.o

LIBS += $(CONTRIB_LIBS) $(EXTRA_LIBS)

DOC_BASE        := ../docs
DOC_TEMPLATES   := $(DOC_BASE)/template
GENERATED_DOCS  := $(DOC_BASE)/aptitudes.txt $(DOC_BASE)/FAQ.html
GENERATED_HEADERS := art-enum.h #the rest are private
GENERATED_FILES := $(GENERATED_HEADERS) art-data.h mon-mst.h $(RLTILES)/dc-unrand.txt compflag.h

GAME_DEPENDS  := $(DESTTILEFILES) $(OBJECTS) $(EXTRA_OBJECTS) $(CONTRIB_LIBS)
SRC_PKG_BASE  := crawl_light
SRC_VERSION   := $(shell git describe --tags --long $(MERGE_BASE) 2>/dev/null || cat util/release_ver)

# when making release builds, use just the bare tag
SRC_VERSION_SHORT := $(shell git describe --tags $(MERGE_BASE) 2>/dev/null || cat util/release_ver)
ifneq (,$(SRC_VERSION_SHORT))
  ifeq (,$(findstring -,$(SRC_VERSION_SHORT)))
    SRC_VERSION := $(SRC_VERSION_SHORT)
  endif
endif
export SRC_VERSION_SHORT

PKG_SRC_DIR   := $(SRC_PKG_BASE)-$(SRC_VERSION)
SRC_PKG_TAR   := $(PKG_SRC_DIR).tar.bz2
SRC_PKG_TAR_NODEPS := $(PKG_SRC_DIR)-nodeps.tar.bz2
SRC_PKG_ZIP   := $(PKG_SRC_DIR).zip

.PHONY: all test install clean clean-contrib clean-rltiles distclean debug \
        profile wizard package-source source build-windows package-windows

all: $(GAME) $(GENERATED_DOCS)

test:
	./$(GAME) -test > /dev/null


ifeq (,$(findstring clean,$(MAKECMDGOALS)))
ifeq (,$(findstring test,$(MAKECMDGOALS)))
ifeq (,$(filter %-windows,$(MAKECMDGOALS)))

#
# CFLAGS difference check
#
# Check for flag changes between the previous build and the current one,
# because any CFLAGS change could result in an inconsistent build if the
# person building it isn't careful.
#
# This should eliminate an annoying need to use 'make clean' every time.
#

TRACK_CFLAGS = $(subst ','\'',$(CC) $(CXX) $(ALL_CFLAGS))           # (stray ' for highlights)

.cflags: .force-cflags
	@FLAGS='$(TRACK_CFLAGS)'; \
    if test x"$$FLAGS" != x"`cat .cflags 2>/dev/null`" ; then \
        echo "    * rebuilding crawl: new build flags or prefix"; \
        echo "$$FLAGS" > .cflags; \
    fi

.PHONY: .force-cflags

##########################################################################
# Dependencies

DEPS := $(shell ls $(OBJECTS:.o=.d) $(YACC_OBJECTS:.o=.d) 2> /dev/null)

ifneq ($(DEPS),)
-include $(DEPS)
endif

endif
endif
endif

depend: $(OBJECTS:.o=.d) $(YACC_OBJECTS:.o=.d)

# This information is included in crash reports, and is printed with
# "crawl -version"
compflag.h: $(OBJECTS:.o=.cc) util/gen-cflg.pl
	$(QUIET_GEN)util/gen-cflg.pl compflag.h "$(ALL_CFLAGS)" "$(LDFLAGS)" "$(HOST)" "$(ARCH)"

# build.h is now produced using the mkrelease.sh script.  Run that manually.

version.o: build.h compflag.h

##########################################################################
# Documentation
#
$(DOC_BASE)/aptitudes.txt: $(DOC_TEMPLATES)/apt-tmpl.txt player.cc aptitudes.h \
						   util/gen-apt.pl
	$(QUIET_GEN)./util/gen-apt.pl $@ $^

$(DOC_BASE)/FAQ.html: dat/database/FAQ.txt util/FAQ2html.pl
	$(QUIET_GEN)./util/FAQ2html.pl $< $@

##########################################################################
# The level compiler
#

$(YACC_OBJECTS): $(CONTRIB_LIBS)

ifndef NO_YACC

prebuildyacc:	$(UTIL)levcomp.tab.cc $(UTIL)levcomp.tab.h $(UTIL)levcomp.lex.cc
		$(QUIET_COPY)$(COPY) $^ prebuilt/

$(UTIL)levcomp.tab.cc: $(UTIL)levcomp.ypp
		+@$(MAKE) -C $(UTIL) levcomp.tab.cc

$(UTIL)levcomp.lex.cc: $(UTIL)levcomp.lpp $(UTIL)levcomp.tab.cc
		+@$(MAKE) -C $(UTIL) levcomp.lex.cc

$(UTIL)levcomp.tab.h: $(UTIL)levcomp.tab.cc

else

prebuildyacc:
		@echo "**** yacc is not installed, aborting."; false

# Pull the level-compiler stuff up from prebuilt/

$(UTIL)levcomp.tab.cc: prebuilt/levcomp.tab.cc
		$(QUIET_COPY)$(COPY) prebuilt/*.h $(UTIL)
		$(QUIET_COPY)$(COPY) $< $@

$(UTIL)levcomp.lex.cc: prebuilt/levcomp.lex.cc
		$(QUIET_COPY)$(COPY) $< $@

endif

##########################################################################


##########################################################################
# The actual build targets
#
install: all
ifeq ($(DESTDIR)$(prefix),)
	@echo Neither "DESTDIR" nor "prefix" defined -- nowhere to install to, aborting.
	@exit 1
endif
	[ -d $(prefix_fp)/$(bin_prefix) ] || mkdir -p $(prefix_fp)/$(bin_prefix)
	$(COPY) $(GAME) $(prefix_fp)/$(bin_prefix)/
	$(STRIP) $(prefix_fp)/$(bin_prefix)/$(GAME)
	mkdir -p $(datadir_fp)/dat/des
	mkdir -p $(datadir_fp)/dat/clua
	mkdir -p $(datadir_fp)/dat/lua
	mkdir -p $(datadir_fp)/dat/database
	mkdir -p $(datadir_fp)/dat/descript
	mkdir -p $(datadir_fp)/docs/develop
	mkdir -p $(datadir_fp)/docs/develop/levels
	mkdir -p $(datadir_fp)/docs/license
	mkdir -p $(datadir_fp)/settings
	$(COPY_R) dat/des/* $(datadir_fp)/dat/des/
	$(COPY)   dat/clua/*.lua $(datadir_fp)/dat/clua/
	$(COPY)   dat/lua/*.lua $(datadir_fp)/dat/lua/
	$(COPY)   dat/database/*.txt $(datadir_fp)/dat/database/
	$(COPY)   dat/descript/*.txt $(datadir_fp)/dat/descript/
	$(COPY) ../docs/*.txt $(datadir_fp)/docs/
	$(COPY) ../docs/develop/*.txt $(datadir_fp)/docs/develop/
	$(COPY) ../docs/develop/levels/*.txt $(datadir_fp)/docs/develop/levels/
	$(COPY) ../docs/license/*.txt $(datadir_fp)/docs/license/
	$(COPY) ../settings/* $(datadir_fp)/settings/
ifeq ($(GAME),crawl.exe)
	sed -i 's/$$/\r/' `find $(datadir_fp) -iname '*.txt' -o -iname '*.des'`
endif
ifdef TILES
	mkdir -p $(datadir_fp)/dat/tiles
	$(COPY) dat/tiles/*.png $(datadir_fp)/dat/tiles/
ifndef PROPORTIONAL_FONT
	$(COPY) dat/tiles/Vera.ttf $(datadir_fp)/dat/tiles/
endif
ifndef MONOSPACED_FONT
	$(COPY) dat/tiles/VeraMono.ttf $(datadir_fp)/dat/tiles/
endif
endif
ifneq ($(savedir_fp),)
	mkdir -p $(savedir_fp)/saves
	mkdir -p $(savedir_fp)/morgue
ifeq ($(USE_DGAMELAUNCH),)
	$(CHOWN) $(INSTALL_UGRP) $(prefix_fp)/$(bin_prefix)/$(GAME) || true
	$(CHMOD) $(MCHMOD) $(prefix_fp)/$(bin_prefix)/$(GAME) || true
	$(CHOWN) -R $(INSTALL_UGRP) $(datadir_fp) || true
	$(CHOWN) -R $(INSTALL_UGRP) $(savedir_fp) || true
	$(CHMOD) $(MCHMOD_SAVEDIR) $(savedir_fp) || true
	$(CHMOD) $(MCHMOD_SAVEDIR) $(savedir_fp)/saves || true
	$(CHMOD) $(MCHMOD_SAVEDIR) $(savedir_fp)/morgue || true
endif
endif
ifneq ($(shareddir_fp),)
	mkdir -p $(shareddir_fp)
ifneq ($(patsubst /var/%,%,$(shareddir_fp)),$(shareddir_fp))
# Only if we're being installed for real.  Installations to a staging dir
# which are then packaged would trample existing files; these need to be
# handled by packagers themselves.
	touch $(shareddir_fp)/logfile
	touch $(shareddir_fp)/scores
endif
ifeq ($(USE_DGAMELAUNCH),)
	$(CHOWN) -R $(INSTALL_UGRP) $(shareddir_fp) || true
	$(CHMOD) $(MCHMOD_SAVEDIR) $(shareddir_fp) || true
	$(CHMOD) $(MCHMOD_LOGS) $(shareddir_fp)/logfile || true
	$(CHMOD) $(MCHMOD_LOGS) $(shareddir_fp)/scores || true
endif
endif

clean: clean-rltiles
	+$(MAKE) -C $(UTIL) clean
	$(RM) $(GAME) $(GAME).exe $(GENERATED_FILES) $(EXTRA_OBJECTS) libw32c.o\
	    libunix.o $(OBJECTS) $(OBJECTS:.o=.d) $(YACC_OBJECTS:.o=.d) *.ixx  \
	    .contrib-libs .cflags
	$(RM) -r build-win
	$(RM) -r build

clean-contrib:
	+$(MAKE) -C contrib clean

distclean: clean clean-contrib clean-rltiles
	$(RM) -r morgue saves
	$(RM) scores $(GAME) core $(DEPENDENCY_MKF)
	$(RM) -r mac-app-zips
	$(RM) -r $(DOXYGEN_HTML_GEN)

$(GAME): $(GAME_DEPENDS)
	+$(QUIET_LINK)$(CXX) $(LDFLAGS) $(EXTRA_OBJECTS) $(OBJECTS) -o $(GAME) $(LIBS)

debug: all
profile: all
wizard: all

doxy-simple: doxygen-simple
doxygen-simple:
	$(DOXYGEN) $(DOXYGEN_SIMPLE_CONF)

doxy-all: doxygen-all
doxygen-all:
	$(DOXYGEN) $(DOXYGEN_ALL_CONF)

$(UTIL)%.o: $(UTIL)%.cc .cflags
ifdef NO_INLINE_DEPGEN
	$(QUIET_CXX)$(CXX) $(YACC_CFLAGS) -c $< -o $(UTIL)$*.o
else
	$(QUIET_CXX)$(CXX) $(YACC_CFLAGS) -Wp,-MD,$(UTIL)$*.d,-MT,$(UTIL)$*.o -c $< -o $(UTIL)$*.o
endif

ifdef PCH
%.h.gch: %.h
	$(QUIET_PCH)$(CXX) $(ALL_CFLAGS) -c $< -o $@

CC_DEP := AppHdr.h.gch
endif

$(OBJECTS:%.o=%.cc): $(CC_DEP) $(TILEDEFHDRS) $(CONTRIB_LIBS)

ifdef NO_INLINE_DEPGEN
$(OBJECTS): $(OBJECTS:%.o=%.d) $(YACC_OBJECTS:%.o=%.d)
endif

%.d: %.cc .cflags $(GENERATED_HEADERS)
	$(QUIET_DEPEND)$(or $(DEPCXX),$(CXX)) -MM $(ALL_CFLAGS) -MT $*.o $< > $*.d

%.o: %.m .cflags $(GENERATED_HEADERS)
ifdef NO_INLINE_DEPGEN
	$(QUIET_CC)$(CC) $(ALL_CFLAGS) -c $< -o $@
else
	$(QUIET_CC)$(CC) $(ALL_CFLAGS) -Wp,-MD,$*.d,-MT,$@ -c $< -o $@
endif

%.o: %.cc .cflags $(GENERATED_HEADERS)
ifdef NO_INLINE_DEPGEN
	$(QUIET_CXX)$(CXX) $(ALL_CFLAGS) -c $< -o $@
else
	$(QUIET_CXX)$(CXX) $(ALL_CFLAGS) -Wp,-MD,$*.d,-MT,$@ -c $< -o $@
endif

icon.o: util/crawl.rc util/crawl.ico .cflags
	$(QUIET_WINDRES)$(WINDRES) util/crawl.rc icon.o

#
# Contribs
#

$(CONTRIB_LIBS): .contrib-libs
	@:

.contrib-libs: .cflags
ifneq (,$(CONTRIBS))
	  +@$(MAKE) -C contrib $(CONTRIBS)
endif
	@touch $@

$(foreach t,$(CONTRIB_LIBS),$(if $(wildcard $t),,$(shell rm -f .contrib-libs)))

#############################################################################
# Build unrandart data
art-data.h: art-data.txt util/art-data.pl art-func.h
	$(QUIET_GEN)util/art-data.pl
art-enum.h: art-data.h

mon-mst.h: mon-spll.h util/gen-mst.pl
	$(QUIET_GEN)util/gen-mst.pl

$(RLTILES)/dc-unrand.txt: art-data.h

artefact.o: art-data.h art-enum.h
mon-util.o: mon-mst.h
mon-util.d: mon-mst.h

#############################################################################
# RLTiles
#

.PHONY: rltile-build
rltile-build: .contrib-libs $(RLTILES)/dc-unrand.txt
	+$(MAKE) -C $(RLTILES) all

$(TILEDEFSRCS): rltile-build
$(TILEDEFHDRS): rltile-build
$(ORIGTILEFILES): rltile-build

dat/tiles/%.png: $(RLTILES)/%.png
	$(QUIET_PNGCRUSH)$(PNGCRUSH) $< $@
ifdef USE_ADVPNG
	$(QUIET_ADVPNG)$(ADVPNG) $@
endif

clean-rltiles:
	$(RM) $(DESTTILEFILES)
	+$(MAKE) -C $(RLTILES) distclean

#############################################################################
# Building the unified Windows package.
#

# You need to have NSIS installed.
package-windows:
ifneq (x$(SRC_VERSION_SHORT),x$(shell cat build-win/version.txt 2>/dev/null))
	+$(MAKE) build-windows
endif
	makensis -NOCD -DVERSION=$(SRC_VERSION_SHORT) util/crawl.nsi

build-windows:
ifneq ($(GAME),crawl.exe)
	@echo "This is only for Windows; please specify CROSSHOST.";false
endif
	+$(MAKE) clean
	+$(MAKE) TILES=y DESTDIR=build-win SAVEDIR='~/crawl' install
	mv build-win/crawl.exe build-win/crawl-tiles.exe
	+$(MAKE) TILES=  DESTDIR=build-win SAVEDIR='~/crawl' install
	mv build-win/crawl.exe build-win/crawl-console.exe
	echo $(SRC_VERSION_SHORT) >build-win/version.txt

#############################################################################
# Building Mac app bundles
#

mac-app-tiles: all
	+$(MAKE) -j1 -C mac -f Makefile.app-bundle tiles

mac-app-console: all
	+$(MAKE) -j1 -C mac -f Makefile.app-bundle
