
cmake_minimum_required(VERSION 2.8)

project(Bombermaaan-R)

set(VERSION 1.4.0 CACHE STRING "Version of Bombermaaan-R")

set(BUILD_SHARED_LIBS ON CACHE BOOL "build the data library as shared library")

# needed packages
find_package(SDL REQUIRED)
find_package(SDL_mixer REQUIRED)

# includes
include_directories(src
    ${SDL_INCLUDE_DIR}
    ${SDL_MIXER_INCLUDE_DIRS}
)

# sources
set(BOMBERMAAAN_R_SRCFILES
    STDAFX.CPP
    CAiArena.cpp
    CAiBomber.cpp
    CAiManager.cpp
    CArenaCloser.cpp
    CArena.cpp
    CArenaSnapshot.cpp
    CBoard.cpp
    CBomb.cpp
    CBomber.cpp
    CBomberMove.cpp
    CClock.cpp
    CCloudManager.cpp
    CCommandChunk.cpp
    CConsole.cpp
    CControls.cpp
    CDebug.cpp
    CDemo.cpp
    CDisplay.cpp
    CDrawGame.cpp
    CElement.cpp
    CExplosion.cpp
    CFloor.cpp
    CFont.cpp
    CGame.cpp
    CHurryMessage.cpp
    CInput.cpp
    CItem.cpp
    CLevel.cpp
    CLog.cpp
    CMainInput.cpp
    CMatch.cpp
    CMenuBase.cpp
    CMenuBomber.cpp
    CMenu.cpp
    CMenuInput.cpp
    CMenuLevel.cpp
    CMenuMatch.cpp
    CMenuYesNo.cpp
    CModeScreen.cpp
    CMosaic.cpp
    COptions.cpp
    CPauseMessage.cpp
    CPlayerInput.cpp
    CRandomMosaic.cpp
    CScores.cpp
    CScroller.cpp
    CSDLInput.cpp
    CSDLVideo.cpp
    CSound.cpp
    CTitle.cpp
    CVictory.cpp
    CWall.cpp
    CWindow.cpp
    CWinner.cpp
    WinMain.cpp
    winreplace.cpp
)

# add source directory
foreach(f ${BOMBERMAAAN_R_SRCFILES})
    list(APPEND BOMBERMAAAN_R_SOURCES src/${f})
endforeach()

# defines


# third-party libraries
include_directories(third-party/simpleini)

include_directories(third-party/tinyxml)
set(TINYXML_SRCFILES
    tinystr.cpp
    tinyxml.cpp
    tinyxmlerror.cpp
    tinyxmlparser.cpp
)
# add source directory
foreach(f ${TINYXML_SRCFILES})
    list(APPEND TINYXML_SOURCES third-party/tinyxml/${f})
endforeach()
add_library(TinyXML STATIC ${TINYXML_SOURCES})

# data library
include_directories(RESGEN)
add_executable(ResGen RESGEN/ResGen.cpp)
add_custom_command(OUTPUT RESGEN/ResData.c
    COMMAND ResGen
    DEPENDS ResGen
    WORKING_DIRECTORY RESGEN
)
add_library(${PROJECT_NAME}-data RESGEN/ResData.c)

# executable
add_executable(${PROJECT_NAME} ${BOMBERMAAAN_R_SOURCES})
target_link_libraries(${PROJECT_NAME}
    ${PROJECT_NAME}-data
    TinyXML
    ${SDL_MIXER_LIBRARIES}
    ${SDL_LIBRARY}
)
add_dependencies(${PROJECT_NAME}
    ${PROJECT_NAME}-data
    TinyXML
)

# installation
install(TARGETS ${PROJECT_NAME} ${PROJECT_NAME}-data
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
)
if(UNIX)
    install(FILES assets/linux/Bombermaaan.desktop
        DESTINATION share/applications)
    install(FILES assets/linux/Bombermaaan.png
        DESTINATION share/pixmaps)
endif()

# packaging
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION ${VERSION})
set(CPACK_GENERATOR "TGZ;DEB")

set(CPACK_DEBIAN_PACKAGE_DEPENDS
    "libc6 (>= 2.7-1),"
    "libgcc1 (>= 1:4.1.1-21),"
    "libsdl1.2 (>= 1.2.10-1),"
    "libsdl-mixer1.2 (>= 1.2.7-1),"
    "libstdc++6 (>= 4.1.1-21)"
)
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Markus Drescher <markus.drescher@ira.uka.de>")
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "A Bomberman clone.")
set(CPACK_DEBIAN_PACKAGE_SECTION "games")
include(CPack)
