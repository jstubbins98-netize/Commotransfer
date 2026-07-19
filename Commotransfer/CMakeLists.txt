cmake_minimum_required(VERSION 3.16)
project(Commotransfer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ── Find Qt (prefer Qt6, fall back to Qt5) ──────────────────────────────────
find_package(Qt6 COMPONENTS Widgets Core QUIET)
if(Qt6_FOUND)
    set(QT_VERSION_MAJOR 6)
    message(STATUS "Using Qt6")
else()
    find_package(Qt5 5.15 REQUIRED COMPONENTS Widgets Core)
    set(QT_VERSION_MAJOR 5)
    message(STATUS "Using Qt5")
endif()

# ── Sources ──────────────────────────────────────────────────────────────────
set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
    src/SetupWizard.cpp
    src/SetupWizard.h
    src/OpenCBMWrapper.cpp
    src/OpenCBMWrapper.h
    src/TransferWorker.cpp
    src/TransferWorker.h
    src/DropZone.cpp
    src/DropZone.h
)

# ── macOS app bundle ─────────────────────────────────────────────────────────
if(APPLE)
    set(MACOSX_BUNDLE_ICON_FILE "")
    add_executable(Commotransfer MACOSX_BUNDLE ${SOURCES})
    set_target_properties(Commotransfer PROPERTIES
        MACOSX_BUNDLE_BUNDLE_NAME        "Commotransfer"
        MACOSX_BUNDLE_BUNDLE_VERSION     "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_GUI_IDENTIFIER     "com.commotransfer.app"
        MACOSX_BUNDLE_INFO_STRING        "Commotransfer — Write .prg files to real floppy disks"
    )
else()
    add_executable(Commotransfer ${SOURCES})
endif()

# ── Link Qt ──────────────────────────────────────────────────────────────────
if(Qt6_FOUND)
    target_link_libraries(Commotransfer Qt6::Widgets Qt6::Core)
else()
    target_link_libraries(Commotransfer Qt5::Widgets Qt5::Core)
endif()

# ── Install ──────────────────────────────────────────────────────────────────
install(TARGETS Commotransfer
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
