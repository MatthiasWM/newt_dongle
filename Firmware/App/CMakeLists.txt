# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ====================================================================================

include(FetchContent)

FetchContent_Declare(
  FLTK
  GIT_REPOSITORY  https://github.com/fltk/fltk
  GIT_TAG         release-1.4.3
  GIT_SHALLOW     TRUE
)

message(STATUS "Downloading and configuring FLTK if necessary, please wait...")
set( FLTK_USE_SYSTEM_LIBJPEG OFF )
set( FLTK_USE_SYSTEM_ZLIB OFF )
set( FLTK_USE_SYSTEM_LIBPNG OFF )
set( FLTK_BUILD_TEST OFF )
set( FLTK_BUILD_GL OFF )
set( FLTK_BUILD_FORMS OFF )
set( FLTK_BUILD_FLTK_OPTIONS OFF )
FetchContent_MakeAvailable(FLTK)
message(STATUS "Downloading and configuring FLTK - done.")

function ( build_with_fluid name dir )
	add_custom_command (
		WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/${dir}"
		COMMAND fltk::fluid -c ${name}.fl
		DEPENDS "${CMAKE_SOURCE_DIR}/${dir}/${name}.fl"
		DEPENDS "${ARGN}"
		OUTPUT "${CMAKE_SOURCE_DIR}/${dir}/${name}.cpp"
		OUTPUT "${CMAKE_SOURCE_DIR}/${dir}/${name}.h"
	)
	set_source_files_properties (
		${CMAKE_SOURCE_DIR}/${dir}/${name}.cpp
		PROPERTIES GENERATED TRUE
	)
	set_source_files_properties (
		${CMAKE_SOURCE_DIR}/${dir}/${name}.h
		PROPERTIES GENERATED TRUE
	)
endfunction ()

# ====================================================================================

project(NewtConnect C CXX ASM)

# Add executable. Default name is the project name, version 0.1

add_executable(NewtCU
        main.cpp 
        main.h
)

# Modify the below lines to enable/disable output over UART/USB
target_compile_definitions(NewtCU PRIVATE
        NEWT_CU_EXAMPLE=1
)

# Compiler options
#target_compile_options(NewtCU PRIVATE 
#        -Wno-psabi
#)

# Add the standard library to the build
target_link_libraries(NewtCU
	fltk::fltk
)

# Add the standard include files to the build
target_include_directories(NewtCU PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/Posix
)

set(APP NewtCU)
add_subdirectory(common)
add_subdirectory(Posix)


