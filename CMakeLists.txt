cmake_minimum_required(VERSION 3.25)

set(projname "Network_simulator")
project(${projname}
        VERSION 2.0
        DESCRIPTION "Simple network simulator using pipes and sockets"
        LANGUAGES C )
message( STATUS "CMAKE_SOURCE_DIR = [${CMAKE_SOURCE_DIR}]" )

set(CMAKE_C_STANDARD 99)
set(CMAKE_VERBOSE_MAKEFILE off)

set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -Wall -Wextra")
set(CMAKE_C_FLAGS_DEBUG  "${CMAKE_C_FLAGS_DEBUG} -Debug ")

find_package(Doxygen
        REQUIRED dot
        OPTIONAL_COMPONENTS mscgen dia)

if(DOXYGEN_FOUND)
    message(STATUS "Generating documentation using Doxygen")

    set(DOXYGEN_PROJECT_NUMBER ${CMAKE_PROJECT_VERSION})
    set(DOXYGEN_PROJECT_BRIEF ${PROJECT_DESCRIPTION})
    set(DOXYGEN_GENERATE_HTML YES)
    set(DOXYGEN_PROJECT_LOGO "images/network_example.png")
    set(DOXYGEN_GENERATE_TREEVIEW YES)
    set(DOXYGEN_DISABLE_INDEX NO)
    set(DOXYGEN_USE_MDFILE_AS_MAINPAGE README.md)
    set(DOXYGEN_MULTILINE_CPP_IS_BRIEF YES)
    set(DOXYGEN_TAB_SIZE 3)
    set(DOXYGEN_JAVADOC_AUTOBRIEF YES)
    set(DOXYGEN_ALIASES "pattern=@par Design Pattern:^^")
    set(DOXYGEN_SORT_MEMBERS_CTORS_1ST YES)
    set(DOXYGEN_IMAGE_PATH images/)
    set(DOXYGEN_OPTIMIZE_OUTPUT_FOR_C YES)
    set(DOXYGEN_CLANG_ASSISTED_PARSING NO)
    set(DOXYGEN_EXTRACT_PRIVATE YES)
    set(DOXYGEN_EXTRACT_PRIV_VIRTUAL YES)
    set(DOXYGEN_EXTRACT_PACKAGE YES)
    set(DOXYGEN_EXTRACT_STATIC YES)
    set(DOXYGEN_EXTRACT_LOCAL_METHODS YES)
    set(DOXYGEN_INTERNAL_DOCS YES)
    set(DOXYGEN_QUIET YES)
    set(DOXYGEN_HTML_TIMESTAMP YES)
    set(DOXYGEN_PAPER_TYPE letter)
    set(DOXYGEN_BUILTIN_STL_SUPPORT YES)
    set(DOXYGEN_HAVE_DOT YES)
    set(DOXYGEN_DOT_NUM_THREADS 1)
    set(DOXYGEN_UML_LOOK YES)
    set(DOXYGEN_UML_LIMIT_NUM_FIELDS 100)
    set(DOXYGEN_DOT_UML_DETAILS YES)
    set(DOXYGEN_DOT_WRAP_THRESHOLD 80)
    set(DOXYGEN_TEMPLATE_RELATIONS YES)
    set(DOXYGEN_DOT_GRAPH_MAX_NODES 100)
    set(DOXYGEN_DOT_TRANSPARENT YES)
    set(DOXYGEN_CALL_GRAPH YES)
    set(DOXYGEN_CALLER_GRAPH YES)
    set(DOXYGEN_DOT_IMAGE_FORMAT svg)
    set(DOXYGEN_INTERACTIVE_SVG YES)
    # set(DOXYGEN_WARN_AS_ERROR FAIL_ON_WARNINGS)
    set(DOXYGEN_STRIP_CODE_COMMENTS NO)
    set(DOXYGEN_SOURCE_BROWSER NO)
    set(DOXYGEN_VERBATIM_HEADERS YES)
    set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/generated_docs)

    message(STATUS "DOXYGEN_OUTPUT_DIRECTORY = [${DOXYGEN_OUTPUT_DIRECTORY}]"
            )

    doxygen_add_docs(doxygen
            ${CMAKE_SOURCE_DIR}/src
            ${CMAKE_SOURCE_DIR}/README.md
            ${CMAKE_SOURCE_DIR}/CONTRIBUTE.md
            ALL
            COMMENT "Generating doxygen documentation for ${PROJECT_NAME} to ${DOXYGEN_OUTPUT_DIRECTORY}")

    add_custom_command(TARGET doxygen
            PRE_BUILD
            COMMAND rsync --recursive --mkpath --checksum --delete ${CMAKE_SOURCE_DIR}/images ${DOXYGEN_OUTPUT_DIRECTORY}/html
            COMMENT "Updating images directory"
            )

    add_custom_command(TARGET doxygen
            POST_BUILD
            COMMAND rsync --recursive --checksum --delete --compress --stats ${DOXYGEN_OUTPUT_DIRECTORY}/html/ brewerj3@uhunix.hawaii.edu:~/public_html/ee205/${CMAKE_PROJECT_NAME}
            COMMENT "Pushing files to UHUnix"
            )

else (DOXYGEN_FOUND)
    message("Doxygen needs to be installed to generate the doxygen documentation")
endif (DOXYGEN_FOUND)

message(STATUS "PROJECT_BINARY_DIR = [${PROJECT_BINARY_DIR}]")

add_executable(net367
        src/main.c src/main.h
        src/man.c src/man.h
        src/packet.c src/packet.h
        src/host.c src/host.h
        src/net.c src/net.h
        src/switch2.c src/switch2.h
        src/server.c src/server.h)

file(COPY p2p.config DESTINATION ${CMAKE_BINARY_DIR})
file(COPY p2ps1.config DESTINATION ${CMAKE_BINARY_DIR})
file(COPY p2ps2.config DESTINATION ${CMAKE_BINARY_DIR})
file(COPY psp.config DESTINATION ${CMAKE_BINARY_DIR})
file(COPY pDNSp.config DESTINATION ${CMAKE_BINARY_DIR})
file(COPY LabSwitch DESTINATION ${CMAKE_BINARY_DIR})
file(COPY test1.config DESTINATION ${CMAKE_BINARY_DIR})
file(COPY test2.config DESTINATION ${CMAKE_BINARY_DIR})
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")