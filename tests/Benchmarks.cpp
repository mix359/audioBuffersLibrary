// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "../buffers/AudioBufferChannelView.h"
#include "../buffers/AudioBufferView.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#ifndef AUDIOBUFFERS_BENCHMARKS_H
#define AUDIOBUFFERS_BENCHMARKS_H

TEMPLATE_TEST_CASE("[AudioBufferChannelView] Benchmark normal access vs buffer view", "[AudioBufferChannelView]", int, double) {
	size_t bufferSize = 16;

	auto data = new TestType[bufferSize];
	abl::AudioBufferChannelView<TestType> audioBufferChannelView{data, bufferSize};
	std::iota(data, data + bufferSize, 1);

	BENCHMARK("Normal Iterator") {
         auto sum = 0;
         for(size_t i = 0; i < bufferSize; ++i) {
             sum += data[i];
         }
         return sum;
	};

	BENCHMARK("AudioBufferChannelView Iterator") {
	     auto sum = 0;
	     for (auto &&sample: audioBufferChannelView) {
	         sum += sample;
	     }
	     return sum;
	};

	BENCHMARK("AudioBufferChannelView Iterator no variant") {
        auto sum = 0;
        for (auto iter = audioBufferChannelView.unwrapper_begin(); iter != audioBufferChannelView.unwrapped_end(); ++iter) {
            sum += *iter;
        }
        return sum;
	};

	BENCHMARK("AudioBufferChannelView operator[]") {
       auto sum = 0;
       for(size_t i = 0; i < bufferSize; ++i) {
           sum += audioBufferChannelView[i];
       }
       return sum;
	};

	delete[] data;
}

TEMPLATE_TEST_CASE("[AudioBufferView] Benchmark normal access vs buffer view", "[AudioBufferView]", int, double) {
	size_t channels = 2;
	size_t bufferSize = 16;

	auto data = new TestType*[channels];
	size_t startValue = 1;
	for (size_t channel = 0; channel < channels; ++channel) {
		data[channel] = new TestType[bufferSize];
		std::iota(data[channel], data[channel] + bufferSize, startValue);
		startValue += bufferSize;
	}

	abl::AudioBufferView<TestType> audioBufferView{data, channels, bufferSize};

	BENCHMARK("Normal Iterator") {
         auto sum = 0;
         for(size_t channel = 0; channel < channels; ++channel) {
             for(size_t i = 0; i < bufferSize; ++i) {
                 sum += data[channel][i];
             }
         }
         return sum;
	};

	BENCHMARK("AudioBufferView Iterator") {
	      auto sum = 0;
	      for (auto&& channelView: audioBufferView) {
	          for (auto &&sample: channelView) {
	              sum += sample;
	          }
	      }
	      return sum;
	};

//	BENCHMARK("AudioBufferView Iterator no variant") {
//         auto sum = 0;
//         for (auto&& channelView: audioBufferView) {
//			 if(auto view = boost::variant2::get_if<abl::AudioBufferChannelView<TestType>>(&channelView)) {
//				 for (auto iter = view->unwrapper_begin(); iter != view->unwrapped_end(); ++iter) {
//					 sum += *iter;
//				 }
//			 }
//         }
//         return sum;
//	};

	BENCHMARK("AudioBufferView operator[]") {
        auto sum = 0;
        for(size_t channel = 0; channel < channels; ++channel) {
            auto view = audioBufferView[channel];
            for(size_t i = 0; i < bufferSize; ++i) {
                sum += (view)[i];
            }
        }
        return sum;
	};

	for(size_t channel = 0; channel < channels; ++channel) {
		delete[] data[channel];
	}
	delete[] data;
}

#endif //AUDIOBUFFERS_BENCHMARKS_H
