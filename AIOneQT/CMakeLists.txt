cmake_minimum_required(VERSION 3.16)

project(AIOneQT VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

get_filename_component(LLAMA_CPP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/llama.cpp" ABSOLUTE)

# set(LLAMA_BUILD_TESTS OFF CACHE BOOL "Disable tests" FORCE)
# set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "Disable examples" FORCE)
# set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static lib" FORCE)

set(GGML_NATIVE OFF CACHE BOOL "Disable platform-specific optimizations" FORCE)
set(LLAMA_NATIVE OFF CACHE BOOL "Disable native CPU optimizations" FORCE)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_WIN32_WINNT=0x0A00")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_WIN32_WINNT=0x0A00")

add_compile_definitions(_WIN32_WINNT=0x0A00)
add_compile_definitions(GGML_WIN_VER=0x0A00)
add_compile_definitions(NTDDI_VERSION=0x0A000000)

add_subdirectory(${LLAMA_CPP_DIR} ${CMAKE_BINARY_DIR}/llama.cpp_build)

find_package(Qt6 REQUIRED COMPONENTS Quick)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(appAIOneQT
    main.cpp
)

qt_add_qml_module(appAIOneQT
    URI AIOneQT
    QML_FILES
        Main.qml
        SOURCES inputhandler.h inputhandler.cpp
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appAIOneQT PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appAIOneQT
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appAIOneQT
    PRIVATE Qt6::Quick
    PRIVATE llama
)

# target_include_directories(appAIOneQT PRIVATE
#     ${LLAMA_CPP_DIR}
#     ${CMAKE_CURRENT_SOURCE_DIR}/../llama.cpp
# )



include(GNUInstallDirs)
install(TARGETS appAIOneQT
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
