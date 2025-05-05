cmake_minimum_required (VERSION 2.6)
project (wradio)
set(CMAKE_BUILD_TYPE Debug)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

#set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set( CMAKE_EXPORT_COMPILE_COMMANDS ON )

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
if(COMPILER_SUPPORTS_CXX11)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
else()
	message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

find_package(PkgConfig)
pkg_check_modules(LIBUSB libusb-1.0)

# dabensemble.cpp
# dabmotuserapplicationdecoder.cpp
# dabplusservicecomponentdecoder.cpp
# dabservicecomponent.cpp
# dabservicecomponentdecoder.cpp
# dabservicecomponentmscpacketdata.cpp
# dabservicecomponentmscstreamaudio.cpp
# dabservicecomponentmscstream.cpp
# dabservicecomponentmscstreamdata.cpp
# dabuserapplication.cpp
# dabuserapplicationdecoder.cpp
# dynamiclabeldecoder.cpp
# jdabservice.cpp
# motdecoder.cpp
# paddecoder.cpp
# raontunerinput.cpp
# slideshowdecoder.cpp
# wradio.cpp

add_executable(wradio wradio.cpp raontunerinput.cpp dabensemble.cpp ficparser.cpp dabservice.cpp 
	thirdparty/fec/init_rs_char.c
	thirdparty/fec/decode_rs_char.c
	thirdparty/fec/encode_rs_char.c
	dabservicecomponent.cpp
	dabservicecomponentdecoder.cpp
	dabmotuserapplicationdecoder.cpp
	dabmpegservicecomponentdecoder.cpp
	dabplusservicecomponentdecoder.cpp
	dabservicecomponentmscpacketdata.cpp
	dabservicecomponentmscstream.cpp
	dabservicecomponentmscstreamaudio.cpp
	dabservicecomponentmscstreamdata.cpp
	dabservicecomponentmscstreamdata.cpp
	dabuserapplication.cpp
	dabuserapplicationdecoder.cpp
	paddecoder.cpp
	jdabservice.cpp
	motdecoder.cpp
	dynamiclabeldecoder.cpp
	slideshowdecoder.cpp
	fig_00_ext_00.cpp
	fig_00_ext_01.cpp
	fig_00_ext_02.cpp
	fig_00_ext_03.cpp 
	fig_00_ext_04.cpp
	fig_00_ext_05.cpp
	fig_00_ext_06.cpp
	fig_00_ext_07.cpp
	fig_00_ext_08.cpp
	fig_00_ext_09.cpp
	fig_00_ext_10.cpp
	fig_00_ext_13.cpp
	fig_00_ext_14.cpp
	fig_00_ext_17.cpp
	fig_00_ext_18.cpp
	fig_00_ext_19.cpp
	fig_00_ext_20.cpp
	fig_00_ext_21.cpp
	fig_00_ext_24.cpp
	fig_00_ext_25.cpp
	fig_00_ext_26.cpp
	fig_01_ext_00.cpp
	fig_01_ext_01.cpp
	fig_01_ext_04.cpp
	fig_01_ext_05.cpp
	fig_01_ext_06.cpp
	jdabppprtkservice.cpp
	dabdatapkt.cpp
	dabdatafec.cpp
	)
include_directories(${LIBUSB_INCLUDE_DIRS})
target_link_libraries(wradio ${LIBUSB_LIBRARIES})
