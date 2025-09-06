# Simple C++ CMakeLists.txt Template - Explicit Files
cmake_minimum_required(VERSION 3.16)

project(monte-carlo-pi-estimation VERSION 1.0.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Export compile commands for IDEs and tools
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Compiler warnings
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Add vendor libraries
add_subdirectory(vendor)
add_subdirectory(vendor/sdl)
add_subdirectory(vendor/glm)

# Find packages BEFORE defining targets
# find_package(fmt REQUIRED)
# find_package(spdlog REQUIRED)

# Recursively find all source files in src/
file(GLOB_RECURSE SOURCES
    src/*.cpp
)

# Recursively find all header files in src/ (for IDE support)
file(GLOB_RECURSE HEADERS
    src/*.h
)

# Define targets AFTER finding packages
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Configure target properties AFTER defining target
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link libraries AFTER target configuration
target_link_libraries(${PROJECT_NAME} PRIVATE
    imgui
	glm
)

# Copy SDL3 DLL to output directory on Windows
if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SDL3::SDL3>
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMENT "Copying SDL3 DLL to output directory"
    )
endif()

# Determine OS and architecture strings
if(WIN32)
    set(OS_NAME "windows")
elseif(APPLE)
    set(OS_NAME "macos")
elseif(UNIX)
    set(OS_NAME "linux")
else()
    set(OS_NAME "unknown")
endif()

# Determine architecture
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)")
        set(ARCH_NAME "arm64")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|AMD64)")
        set(ARCH_NAME "x64")
    else()
        set(ARCH_NAME "64bit")
    endif()
else()
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm|armv7)")
        set(ARCH_NAME "arm32")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(i386|i686|x86)")
        set(ARCH_NAME "x86")
    else()
        set(ARCH_NAME "32bit")
    endif()
endif()

# Set output directory: build/bin/build-type/os/arch/
set_target_properties(${PROJECT_NAME} PROPERTIES
    # RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin/${OS_NAME}/${ARCH_NAME}
    # RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE}/${CMAKE_SYSTEM_NAME}/${CMAKE_SYSTEM_PROCESSOR}
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin/$<CONFIG>/${CMAKE_SYSTEM_NAME}/${CMAKE_SYSTEM_PROCESSOR}
    # Set working directory for IDEs (Visual Studio, etc.)
    VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

# Custom target to run the executable from project root
add_custom_target(run
    COMMAND $<TARGET_FILE:${PROJECT_NAME}>
    DEPENDS ${PROJECT_NAME}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running ${PROJECT_NAME} from project root"
)