cmake_minimum_required(VERSION 2.8.8)

project(qrpath)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

set(qrpath_SOURCES
	qrpath.c
)

set(testlib_SOURCES
)

set(qrpath_SOURCE_FILES)
set(qrpath_HEADER_FILES)
set(testlib_SOURCE_FILES)

foreach(x ${qrpath_SOURCES})
	list(APPEND qrpath_SOURCE_FILES "src/${x}")
endforeach()

foreach(x ${testlib_SOURCES})
	list(APPEND testlib_SOURCE_FILES "test/${x}")
endforeach()

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

#add_executable(alltest test/main.c ${qrpath_SOURCE_FILES} ${qrpath_HEADER_FILES})
#set_target_properties(alltest PROPERTIES COMPILE_FLAGS -DTESTING)
#target_link_libraries(alltest -lcunit -ldb)

add_library(qrpath SHARED ${qrpath_SOURCE_FILES})
install(TARGETS qrpath DESTINATION lib)
install(FILES include/qrpath/qrpath.h DESTINATION include/qrpath)

