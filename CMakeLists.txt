cmake_minimum_required(VERSION 3.27)
project(audioBuffers)

set(CMAKE_CXX_STANDARD 20)

set(ABL_MAIN_PROJECT OFF)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(ABL_MAIN_PROJECT ON)
endif()

option(ABL_BUILD_TESTS "Build the unit tests" ${ABL_MAIN_PROJECT})

find_package(Boost 1.83.0)
include_directories(${Boost_INCLUDE_DIRS})

add_library(audioBuffers SHARED
        buffers/AudioBuffer.h
        buffers/AudioBufferChannelView.h
        buffers/AudioBufferView.h
        buffers/AudioBufferWithMemoryManagement.h
        buffers/BasicCircularAudioBufferView.h
        buffers/CircularAudioBuffer.h
        buffers/CircularAudioBufferChannelView.h
        buffers/CircularAudioBufferView.h
        buffers/DelayedCircularAudioBuffer.h
        buffers/DelayedCircularAudioBufferView.h
        buffers/OffsettedReadCircularAudioBufferChannelView.h
        buffers/AudioBufferChannelViewWrapper.h
        buffers/AudioBufferChannelViewConcepts.h
        buffers/AudioBufferViewConcepts.h
        buffers/AudioBufferViewWrapper.h
)

set_target_properties(audioBuffers PROPERTIES LINKER_LANGUAGE CXX)

if (ABL_BUILD_TESTS)
    add_subdirectory(tests)
endif()