cmake_minimum_required(VERSION 3.10)
project(qdevcpp VERSION 0.2)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(MSVC)
	add_compile_options("/utf-8")
	add_compile_options("/DQSCINTILLA_DLL") # assume the qscintilla2 is a shared library
	if("${VCPKG_TARGET_TRIPLET}" STREQUAL "")
		set(VCPKG_TARGET_TRIPLET "x86-windows")
	endif()
	set(VCPKG_ROOT "C:\\vcpkg\\installed\\${VCPKG_TARGET_TRIPLET}")
	set(CMAKE_PREFIX_PATH ${VCPKG_ROOT})
	set(CMAKE_LIBRARY_PATH_RELWITHDEBINFO "${VCPKG_ROOT}\\lib")
	set(CMAKE_LIBRARY_PATH_DEBUG "${VCPKG_ROOT}\\debug\\lib")
endif()

find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)

set( HEADERS
	aboutqdevcpp.h
	compileconfig.h
	config.h
	confighelp.h
	debugger.h
	editorconfig.h
	editorinfo.h
	environmentconfig.h
	findreplace.h
	global.h
	language.h
	mainwindow.h
)

set( SOURCES
	aboutqdevcpp.cpp
	compileconfig.cpp
	debugger.cpp
	editorconfig.cpp
	editorinfo.cpp
	environmentconfig.cpp
	findreplace.cpp
	language.cpp
	main.cpp
	mainwindow.cpp
)

set( UIS
	aboutqdevcpp.ui
	compileconfig.ui
	editorconfig.ui
	environmentconfig.ui
	findreplace.ui
	mainwindow.ui
)

if (WIN32)

set( RESOURCES
	resource.qrc
	resource.rc
)

elseif (UNIX)

set( RESOURCES 
	resource.qrc
	desktop.qrc
)

endif()

find_path( QSCINTILLA_INCLUDE_DIR
	NAMES Qsci qsciglobal.h
	PATHS ${Qt5Core_INCLUDE_DIRS}
	PATH_SUFFIXES Qsci
)

find_library( QSCINTILLA_LIBRARY_RELWITHDEBINFO
	NAMES qscintilla2_qt5 qscintilla2
	PATHS ${CMAKE_LIBRARY_PATH_RELWITHDEBINFO}
)

find_library( QSCINTILLA_LIBRARY_DEBUG
	NAMES qscintilla2_qt5 qscintilla2
	PATHS ${CMAKE_LIBRARY_PATH_DEBUG}
)

if(QSCINTILLA_INCLUDE_DIR)
	add_library(QScintilla UNKNOWN IMPORTED)
	set_target_properties(QScintilla PROPERTIES
		IMPORTED_LOCATION_DEBUG ${QSCINTILLA_LIBRARY_DEBUG}
		IMPORTED_LOCATION_RELWITHDEBINFO ${QSCINTILLA_LIBRARY_RELWITHDEBINFO}
		INTERFACE_INCLUDE_DIRECTORIES ${QSCINTILLA_INCLUDE_DIR})
else()
	message(FATAL_ERROR "QScintilla Not Found")
endif()

add_executable(${PROJECT_NAME} ${HEADERS} ${SOURCES} ${RESOURCES} ${UIS})

if(MSVC)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:\"mainCRTStartup\"")
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC Qt5::Core Qt5::Gui Qt5::Widgets QScintilla)
