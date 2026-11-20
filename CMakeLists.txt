cmake_minimum_required(VERSION 3.15)

include_guard(GLOBAL)


set(CMAKE_PDB_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/pdbs")
#set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/externals/sanitizers-cmake/cmake" ${CMAKE_MODULE_PATH})
#find_package(Sanitizers)
#if(NOT "$ENV{CI}")
#include(cmake/locate_vcpkg.cmake)

#include(cmake/asan.cmake)
#setup_vcpkg_before_project()
#endif()
#TODO: stop if we use vs2015 or older

project(Vsif2Vcd VERSION 1.1.1.0
                  DESCRIPTION "Cross-platform decompiler for scenes.image file."
                  LANGUAGES CXX)




if(WIN32)
    set(VCPKG_APPLOCAL_DEPS ON)
    set(X_VCPKG_APPLOCAL_DEPS_INSTALL ON)
endif()



                  
configure_file (
    "${PROJECT_SOURCE_DIR}/include/version.hpp.in"
    "${PROJECT_BINARY_DIR}/include/version.hpp"
)

find_package(Boost REQUIRED
    COMPONENTS iostreams)




find_package(fmt CONFIG REQUIRED)


find_package (Bitsery CONFIG REQUIRED)


set(SPDLOG_FMT_EXTERNAL ON)
add_subdirectory(thirdparty/spdlog)
#target_link_libraries(main PRIVATE spdlog::spdlog spdlog::spdlog_header_only)




include(GNUInstallDirs)


set(PRIVATE_INCLUDES

    "include/VSIF.hpp"
    "include/map_bsp.hpp"
    "include/enum_bitmask.hpp"
    "include/gameinfoKV.hpp"
    "include/program.h"
    "include/BVCD.hpp"
    "include/gameinfo.hpp"
    "include/hardcoded_entries.h"
    "include/CRC.h"
    "include/pch.hpp"
    "include/response_system.h"
    "include/split_serialization.h"
    "include/helper.hpp"
    "include/VSIFStringPool.hpp"

    include/filesystem/ifile.h
    include/filesystem/imountpath.h
    )
set(SOURCES 
    "src/Vsif2vcd_c++.cpp"
    "src/BVCD.cpp"
    "src/map_bsp.cpp"
    "src/gameinfo.cpp"   "src/response_system.cpp"
      "src/VSIF.cpp"
      src/filesystem_internal/cvpkmountpath.h src/filesystem_internal/cvpkinfile.h src/filesystem_internal/cvpkinfile.cpp src/filesystem_internal/cvpkmountpath.cpp
      src/filesystem_internal/cloosemountpath.h src/filesystem_internal/cloosemountpath.cpp src/filesystem_internal/cloosefile.h src/filesystem_internal/cloosefile.cpp
        src/filesystem_internal/imountpath_internal.cpp
      )

if(WIN32)
    set(PRIVATE_INCLUDES ${PRIVATE_INCLUDES}
        "include/win/winver.h"
    )
endif()
# Dodaj źródło do pliku wykonywalnego tego projektu.
add_executable (Vsif2vcd ${SOURCES} ${PRIVATE_INCLUDES})
target_precompile_headers(Vsif2vcd PUBLIC "include/pch.hpp")

#add_subdirectory(thirdparty/hllib/HLLib)
#add_subdirectory(thirdparty/valve-bsp-parser)
add_subdirectory(thirdparty/sourcepp)
add_subdirectory(thirdparty/fcaseopen)
add_subdirectory(thirdparty/wildcard)
#add_subdirectory(thirdparty/lzma)


add_dependencies(Vsif2vcd sourcepp::bsppp sourcepp::vpkpp spdlog::spdlog)
set_property(TARGET Vsif2vcd PROPERTY CXX_STANDARD 20)
target_link_libraries(Vsif2vcd PUBLIC Boost::iostreams Bitsery::bitsery sourcepp::bsppp sourcepp::vpkpp sourcepp spdlog::spdlog fcaseopen wildest_card)

target_link_libraries(Vsif2vcd PRIVATE fmt::fmt)

find_path(BOOST_CRC_INCLUDE_DIRS "boost/crc.hpp")
target_include_directories(Vsif2vcd PRIVATE ${BOOST_CRC_INCLUDE_DIRS})

target_include_directories(Vsif2vcd PRIVATE fmt::fmt)
target_include_directories(Vsif2vcd PRIVATE fcaseopen)
target_include_directories(Vsif2vcd PRIVATE wildest_card)

find_path(BOOST_LEXICAL_CAST_INCLUDE_DIRS "boost/detail/basic_pointerbuf.hpp")
target_include_directories(Vsif2vcd PRIVATE ${BOOST_LEXICAL_CAST_INCLUDE_DIRS})

find_path(BOOST_TOKENIZER_INCLUDE_DIRS "boost/token_functions.hpp")
target_include_directories(Vsif2vcd PRIVATE ${BOOST_TOKENIZER_INCLUDE_DIRS})

find_path(BOOST_ALGORITHM_INCLUDE_DIRS "boost/algorithm/algorithm.hpp")
target_include_directories(Vsif2vcd PRIVATE ${BOOST_ALGORITHM_INCLUDE_DIRS})





target_include_directories(Vsif2vcd PUBLIC 
     ${Boost_INCLUDE_DIR} 
    "${PROJECT_SOURCE_DIR}/include/"
    "${PROJECT_SOURCE_DIR}/thirdparty/vdf_parser/include"
    "${PROJECT_SOURCE_DIR}/thirdparty/liblzma/include"

    "${PROJECT_SOURCE_DIR}/thirdparty/hllib/HLLib"
    "${PROJECT_SOURCE_DIR}/thirdparty/valve-bsp-parser/include"
    "${PROJECT_BINARY_DIR}/include/"
    ${Bitsery_INCLUDE_DIR})


  
install(TARGETS Vsif2vcd RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX} COMPONENT Vsif2VCD)
#install(TARGETS hllib RUNTIME COMPONENT Vsif2vcd DESTINATION ${CMAKE_INSTALL_PREFIX})
#Outside of Windows TARGET_RUNTIME_DLL expands to empty strings which will screw up the install script.
if(WIN32)
add_custom_command(TARGET Vsif2vcd POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:Vsif2vcd> $<TARGET_FILE_DIR:Vsif2vcd>
  COMMAND_EXPAND_LISTS
  )
install(FILES $<TARGET_RUNTIME_DLLS:Vsif2vcd> TYPE BIN COMPONENT Vsif2VCD)
endif()

include(cmake/CMakeTests.cmake)
include(cmake/CPackConfig.cmake)
