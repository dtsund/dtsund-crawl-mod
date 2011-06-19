
/* This file is for MSVC to generate a precompiled header */
#include "AppHdr.h"

/* Make setting up the link trivial on MSVC */
#ifdef TARGET_COMPILER_VC
    #pragma comment (lib, "pcre.lib")
    #pragma comment (lib, "lua.lib")
    #pragma comment (lib, "sqlite.lib")
        #ifdef USE_TILE
            #pragma comment (lib, "freetype.lib")
            #pragma comment (lib, "SDL.lib")
            #pragma comment (lib, "SDL_image.lib")
            #pragma comment (lib, "libpng.lib")
            #pragma comment (lib, "zlib.lib")
            #pragma comment (lib, "dxguid.lib")
            #pragma comment (lib, "winmm.lib")
        #endif
#endif
