#ifndef __included_crawl_compile_flags_h
#define __included_crawl_compile_flags_h

#define CRAWL_CFLAGS "-O2 -pipe -Wall -DUSE_TILE -DUSE_SDL -DUSE_GL -DUSE_FT -D_GNU_SOURCE=1 -D_REENTRANT -Wundef -Wno-array-bounds -Wno-parentheses -Wwrite-strings -Wshadow -D_FORTIFY_SOURCE=0 -Wuninitialized -Icontrib/install/include -Iutil -I. -I/usr/include/lua5.1 -I/usr/include -Irltiles -I/usr/include/freetype2 -I/usr/include/SDL -I/usr/include/ncursesw -DWIZARD -DASSERTS -D_GNU_SOURCE=1 -D_REENTRANT -DCLUA_BINDINGS"
#define CRAWL_LDFLAGS "-rdynamic -O2"
#define CRAWL_HOST "x86_64-linux-gnu"
#define CRAWL_ARCH "x86_64-linux-gnu"

#endif

