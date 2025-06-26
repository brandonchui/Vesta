cmake_minimum_required(VERSION 3.29)
project(EngineExample)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

set(THIRDPARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty")



add_library(Engine SHARED
    "src/Common/OS/WindowBase.cpp"
    "src/Common/OS/WindowsWindow.cpp"
    "src/Common/OS/WindowsInput.cpp"
    "src/Common/Graphics/Direct3D12.cpp"
    "src/Common/Util/Time.cpp"
    "src/Common/Util/CameraController.cpp"
    "src/Common/Util/Logger.h"
    "src/Common/Resource/Resource.cpp"
    "${THIRDPARTY_DIR}/D3D12MemoryAllocator/src/D3D12MemAlloc.cpp"
)

target_compile_definitions(Engine PRIVATE RHI_BUILD_DLL -DUNICODE -D_UNICODE)


target_include_directories(Engine PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${THIRDPARTY_DIR}/DirectXTK12/Inc"
    "${THIRDPARTY_DIR}/DirectXTK12/Src"
    "${THIRDPARTY_DIR}/D3D12MemoryAllocator/include"
    "${THIRDPARTY_DIR}/imgui"
    "${THIRDPARTY_DIR}/spdlog/include"
)
target_compile_options(Engine PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)


# Builds all the examples
file(GLOB EXAMPLE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/Examples/*")

foreach(EXAMPLE_PATH ${EXAMPLE_DIRS})
    if(IS_DIRECTORY ${EXAMPLE_PATH})
        get_filename_component(EXAMPLE_NAME ${EXAMPLE_PATH} NAME)

        file(GLOB_RECURSE EXAMPLE_SOURCES "${EXAMPLE_PATH}/*.cpp")

        add_executable(${EXAMPLE_NAME} ${EXAMPLE_SOURCES})

        target_link_libraries(${EXAMPLE_NAME} PRIVATE Engine)

        target_compile_options(${EXAMPLE_NAME} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

        set_target_properties(${EXAMPLE_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    endif()
endforeach()
