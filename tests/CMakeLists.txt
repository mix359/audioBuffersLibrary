find_package(Catch2 3 REQUIRED)

#add_subdirectory(Catch2 EXCLUDE_FROM_ALL)

add_executable(AudioBufferTests AudioBufferChannelViewTest.cpp
        AudioBufferViewTest.cpp
        CircularAudioBufferChannelViewTest.cpp
        CircularAudioBufferViewTest.cpp
        AudioBufferTest.cpp
        DelayedCircularAudioBufferViewTest.cpp
        CircularAudioBufferTest.cpp
        DelayedCircularAudioBufferTest.cpp
        OffsettedReadCircularAudioBufferChannelViewTest.cpp
)

target_compile_features(AudioBufferTests PRIVATE cxx_std_20)

target_link_libraries(AudioBufferTests PRIVATE audioBuffers)
target_link_libraries(AudioBufferTests PRIVATE Catch2::Catch2WithMain)


add_executable(AudioBufferBenchmark Benchmarks.cpp)

target_compile_features(AudioBufferBenchmark PRIVATE cxx_std_20)
#target_compile_options(AudioBufferBenchmark PRIVATE -O3)

target_link_libraries(AudioBufferBenchmark PRIVATE audioBuffers)
target_link_libraries(AudioBufferBenchmark PRIVATE Catch2::Catch2WithMain)

include(Catch)
catch_discover_tests(AudioBufferTests)
catch_discover_tests(AudioBufferBenchmark)
enable_testing()