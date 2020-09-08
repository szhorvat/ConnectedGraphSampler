cmake_minimum_required(VERSION 3.2)
project(ConnectedGraphSampler)

set(CMAKE_CXX_STANDARD 14)

find_package( Boost 1.57 COMPONENTS program_options REQUIRED )

include_directories( 
    ../src
    ${Boost_INCLUDE_DIR} 
)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_executable( cdsample cdsample.cpp )

target_link_libraries( cdsample LINK_PUBLIC ${Boost_LIBRARIES} )
