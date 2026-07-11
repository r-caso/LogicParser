cmake_minimum_required(VERSION 3.24)
project(LogicParser
    VERSION 0.1.0
    DESCRIPTION "A minimal parser for propositional and first order languages"
    HOMEPAGE_URL "https://github.com/r-caso/LogicParser"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(LogicAST REQUIRED)

add_library(LogicParser
    "src/lexer.cpp"
    "src/parser.cpp"
)

target_link_libraries(LogicParser PUBLIC LogicAST::LogicAST)

target_include_directories(LogicParser PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Compiler Warnings
if(MSVC)
    target_compile_options(LogicParser PRIVATE /W4)
else()
    target_compile_options(LogicParser PRIVATE -Wall -Wextra)
endif()

# Install rules
include(GNUInstallDirs)

install(TARGETS LogicParser
    EXPORT LogicParserTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install headers matching namespace directory
install(DIRECTORY include/LogicParser/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/LogicParser
)

# Export targets to be used by CMake package config
install(EXPORT LogicParserTargets
    FILE LogicParserTargets.cmake
    NAMESPACE LogicParser::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LogicParser
)

# Write version file and configure config file for find_package(LogicParser)
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/LogicParserConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/LogicParserConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/LogicParserConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LogicParser
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/LogicParserConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/LogicParserConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LogicParser
)

if(PROJECT_IS_TOP_LEVEL)
    enable_testing()
    add_subdirectory(tests)
endif()
