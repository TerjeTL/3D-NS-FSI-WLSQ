# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.

cmake_minimum_required (VERSION 3.24)

#	CMAKE Defines
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})


#	CMAKE Project
project("application")

#	SUBDIRECTORIES
#add_subdirectory("ibm_application")

#set(libconfigdir_hint "C:/Development/C++/Libraries/libconfig-1.7.3/build/x64")

find_library(LIBCONFIG_DIR NAMES libconfig++ PATHS "C:/Development/C++/Libraries/libconfig-1.7.3/build/x64")
include_directories(C:/Development/C++/Libraries/libconfig-1.7.3/lib)

message (STATUS "Found LibConfig: " ${LIBCONFIG_DIR})

find_package(Eigen3)

# Add source to this project's executable.
add_executable(application "src/main.cpp")

target_sources(application PRIVATE
	"src/Boundary.cpp"
	"src/ConfigSettings.cpp"
	"src/Array3D.h"
	"src/CustomExceptions.h"
	"src/FlowVariableGroupStructs.h"
	"src/includes_and_names.h"
	"src/Mesh.cpp"
	"src/Node.h"
	"src/OutputManager.cpp"
	"src/Simulation.cpp"
	"src/SmallVectors.h"
	"src/Solver.cpp"
	"src/tests.h"
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET application PROPERTY CXX_STANDARD 17)
endif()

#target_compile_definitions(application PRIVATE _USE_MATH_DEFINES)

#target_include_directories(application
#	PUBLIC "${PROJECT_SOURCE_DIR}/ibm_application/include/"
#	PUBLIC "${CMAKE_BINARY_DIR}/_deps/hdf5-src/src/H5FDsubfiling"
	#PUBLIC libconfig++_DIR
#	PUBLIC "C:/Development/C++/Libraries/libconfig-1.7.3/lib"
#)

#list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/external/blaze/cmake")
#include(Blaze_Import)
#Blaze_Import(
#	QUIET
#	BLAS off
#	LAPACK off
#	THREADING off
#	CACHE_SIZE auto
#	VECTORIZATION off
#	STORAGE_ORDER rowMajor
#)

#set(SDL_LIBRARIES ${SDL_LIBRARIES} SDL2main SDL2 SDL2_ttf)

target_link_libraries(application
#	PUBLIC OpenMP::OpenMP_CXX
#	PRIVATE ${SDL_LIBRARIES}
#	PRIVATE Blaze
	PRIVATE Eigen3::Eigen
	PRIVATE ${LIBCONFIG_DIR}
#	PRIVATE hdf5_data_export
#	imgui
#	PRIVATE pybind11::embed
)

#print_all_variables()
# TODO: Add tests and install targets if needed.
