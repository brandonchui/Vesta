cmake_minimum_required(VERSION 3.29)
project(EngineExample)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

set(THIRDPARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty")

# Experimental
option(ENABLE_SLANG_SUPPORT "Enables experimental Slang compiler support" OFF)


add_library(Engine SHARED
    "src/Common/OS/WindowBase.cpp"
    "src/Common/OS/WindowsWindow.cpp"
    "src/Common/OS/WindowsInput.cpp"
    "src/Common/Graphics/Direct3D12.cpp"
    "src/Common/Util/Time.cpp"
    "src/Common/Util/CameraController.cpp"
    "src/Common/Util/Logger.h"
    "src/Common/Resource/Resource.cpp"
    "src/VTMath.cpp"
    "src/Common/RingBuffer.hpp"
    "${THIRDPARTY_DIR}/D3D12MemoryAllocator/src/D3D12MemAlloc.cpp"
)

target_include_directories(Engine PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${THIRDPARTY_DIR}/DirectXTK12/Inc"
    "${THIRDPARTY_DIR}/DirectXTK12/Src"
    "${THIRDPARTY_DIR}/D3D12MemoryAllocator/include"
    "${THIRDPARTY_DIR}/imgui"
    "${THIRDPARTY_DIR}/spdlog/include"
)

if(ENABLE_SLANG_SUPPORT)
    message(STATUS "Experimental Slang support has been enabled.")

    target_compile_definitions(Engine PRIVATE WITH_SLANG_SUPPORT)

    target_sources(Engine PRIVATE "src/Common/SlangCompiler.cpp")

    target_include_directories(Engine PUBLIC "C:/slang/include")
    target_link_directories(Engine PUBLIC "C:/slang/lib")

endif()


target_compile_definitions(Engine PRIVATE RHI_BUILD_DLL -DUNICODE -D_UNICODE)
if (MSVC OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC"))
    target_compile_definitions(Engine PRIVATE _CRT_DECLARE_NONSTDC_NAMES=1 _USE_MATH_DEFINES)
endif()

target_compile_options(Engine PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)


file(GLOB EXAMPLE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/Examples/*")
set(SHADER_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/Shaders")

foreach(EXAMPLE_PATH ${EXAMPLE_DIRS})
    if(IS_DIRECTORY ${EXAMPLE_PATH})
        get_filename_component(EXAMPLE_NAME ${EXAMPLE_PATH} NAME)

        file(GLOB_RECURSE EXAMPLE_SOURCES "${EXAMPLE_PATH}/*.cpp")

        add_executable(${EXAMPLE_NAME} ${EXAMPLE_SOURCES})

        target_link_libraries(${EXAMPLE_NAME} PRIVATE Engine)

        target_compile_options(${EXAMPLE_NAME} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

        set_target_properties(${EXAMPLE_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

        add_custom_command(
            TARGET ${EXAMPLE_NAME}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${SHADER_SRC_DIR}"
                    "$<TARGET_FILE_DIR:${EXAMPLE_NAME}>/Shaders"
            COMMENT "Copying shaders for ${EXAMPLE_NAME}"
        )
    endif()
endforeach()
