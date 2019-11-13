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
	if(NOT DEFINED ${CMAKE_PREFIX_PATH})
		set(CMAKE_PREFIX_PATH "C:\\vcpkg\\installed\\x86-windows")
	endif()
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

find_library( QSCINTILLA_LIBRARY
	NAMES qscintilla2_qt5 qscintilla2
	PATHS ${Qt5Core_LIBRARIES}
)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(
	QScintilla
	FOUND_VAR QSCINTILLA_FOUND
	REQUIRED_VARS QSCINTILLA_INCLUDE_DIR QSCINTILLA_LIBRARY
	VERSION_VAR QSCINTILLA_VERSION
)

if(QSCINTILLA_LIBRARY AND QSCINTILLA_INCLUDE_DIR)
	set(QSCINTILLA_LIBRARIES ${QSCINTILLA_LIBRARY})
	set(QSCINTILLA_INCLUDE_DIRS ${QSCINTILLA_INCLUDE_DIR})
endif()

mark_as_advanced(QSCINTILLA_INCLUDE_DIR QSCINTILLA_LIBRARY)

if(QSCINTILLA_LIBRARY AND QSCINTILLA_INCLUDE_DIR)
	set(QSCINTILLA_LIBRARIES ${QSCINTILLA_LIBRARY})
	set(QSCINTILLA_INCLUDE_DIRS ${QSCINTILLA_INCLUDE_DIR})
	add_library(QScintilla UNKNOWN IMPORTED)
	set_target_properties(QScintilla PROPERTIES
		IMPORTED_LOCATION ${QSCINTILLA_LIBRARY}
		INTERFACE_INCLUDE_DIRECTORIES ${QSCINTILLA_INCLUDE_DIR})
endif()

add_executable(${PROJECT_NAME} ${HEADERS} ${SOURCES} ${RESOURCES} ${UIS})

if(MSVC)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:\"mainCRTStartup\"")
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC Qt5::Core Qt5::Gui Qt5::Widgets QScintilla)
