find_package(PkgConfig REQUIRED)

add_executable(RetroEngine ${RETRO_FILES})

set(DEP_PATH psp)

pkg_check_modules(OGG ogg)

if(NOT OGG_FOUND)
    set(COMPILE_OGG TRUE)
    message(NOTICE "libogg not found, attempting to build from source")
else()
    message("found libogg")
    target_link_libraries(RetroEngine ${OGG_STATIC_LIBRARIES})
    target_link_options(RetroEngine PRIVATE ${OGG_STATIC_LDLIBS_OTHER})
    target_compile_options(RetroEngine PRIVATE ${OGG_STATIC_CFLAGS})
endif()

pkg_check_modules(VORBIS vorbis vorbisfile)

if(NOT VORBIS_FOUND)
    set(COMPILE_VORBIS TRUE)
    message(NOTICE "libvorbis not found, attempting to build from source")
else()
    message("found libvorbis")
    target_link_libraries(RetroEngine ${VORBIS_STATIC_LIBRARIES})
    target_link_options(RetroEngine PRIVATE ${VORBIS_STATIC_LDLIBS_OTHER})
    target_compile_options(RetroEngine PRIVATE ${VORBIS_STATIC_CFLAGS})
endif()

option(RETRO_USE_PSP_NATIVE "Use native PSP APIs instead of SDL" ON)

if(RETRO_USE_PSP_NATIVE)
    message("Using native PSP APIs (no SDL)")
    target_compile_definitions(RetroEngine PRIVATE RETRO_USE_PSP_NATIVE=1)
    target_link_libraries(RetroEngine
        pspaudiolib
        pspaudio
        pspgu
        pspgum
        pspge
        pspdisplay
        psprtc
        pspctrl
        psppower
        pspdebug
    )
else()
pkg_check_modules(THEORA theora theoradec)

if(NOT THEORA_FOUND)
    set(COMPILE_THEORA TRUE)
    message(NOTICE "libtheora not found, attempting to build from source")
else()
    message("found libtheora")
    target_link_libraries(RetroEngine ${THEORA_STATIC_LIBRARIES})
    target_link_options(RetroEngine PRIVATE ${THEORA_STATIC_LDLIBS_OTHER})
    target_compile_options(RetroEngine PRIVATE ${THEORA_STATIC_CFLAGS})
endif()

if(RETRO_SDL_VERSION STREQUAL "2")
    pkg_check_modules(SDL2 sdl2 REQUIRED)
    target_link_libraries(RetroEngine ${SDL2_STATIC_LIBRARIES})
    target_link_options(RetroEngine PRIVATE ${SDL2_STATIC_LDLIBS_OTHER})
    target_compile_options(RetroEngine PRIVATE ${SDL2_STATIC_CFLAGS})
elseif(RETRO_SDL_VERSION STREQUAL "1")
    pkg_check_modules(SDL1 sdl1 REQUIRED)
    target_link_libraries(RetroEngine ${SDL1_STATIC_LIBRARIES})
    target_link_options(RetroEngine PRIVATE ${SDL1_STATIC_LDLIBS_OTHER})
    target_compile_options(RetroEngine PRIVATE ${SDL1_STATIC_CFLAGS})
    endif()
endif()

if(NOT GAME_STATIC)
    message(FATAL_ERROR "GAME_STATIC must be on")
endif()

set(RETRO_MOD_LOADER ON CACHE BOOL "Enable the mod loader" FORCE)
set(RETRO_USE_HW_RENDER OFF CACHE BOOL "Disable hardware rendering on PSP" FORCE)

if(RETRO_MOD_LOADER)
    set_target_properties(RetroEngine PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
    )
endif()

target_compile_options(RetroEngine PRIVATE
    -O3
    -ffast-math
    -fomit-frame-pointer
    -finline-functions
    -funroll-loops
    -march=allegrex
    -mtune=allegrex
    -mno-check-zero-division
    -fsingle-precision-constant
)

target_compile_definitions(RetroEngine PRIVATE RETRO_DISABLE_LOG=1)
target_link_libraries(RetroEngine m)

create_pbp_file(TARGET RetroEngine
    TITLE "Sonic CD"
    ICON_PATH ../psp/ICON0.png
    BACKGROUND_PATH ../psp/PIC1.png)
