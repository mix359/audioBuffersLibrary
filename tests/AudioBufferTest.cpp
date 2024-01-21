// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "../buffers/AudioBuffer.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_template_test_macros.hpp>

template<typename T>
struct AudioBufferWrapper {
	abl::AudioBuffer<T> audioBuffer;

	static AudioBufferWrapper createWithIncrementalNumbers(size_t channels, size_t bufferSize, const std::vector<size_t> &channelsMapping = {}) {
		auto data = new T *[channels];
		size_t startValue = 1;
		for (size_t channel = 0; channel < channels; ++channel) {
			data[channel] = new T[bufferSize];
			std::iota(data[channel], data[channel] + bufferSize, startValue);
			startValue += bufferSize;
		}

		auto wrapper = AudioBufferWrapper{data, channels, bufferSize, channelsMapping};
		for (size_t channel = 0; channel < channels; ++channel) {
			delete[] data[channel];
		}
		delete[] data;

		return std::move(wrapper);
	}

	static AudioBufferWrapper createWithFixedValue(size_t channels, size_t bufferSize, T fixedValue, const std::vector<size_t> &channelsMapping = {}) {
		auto data = new T *[channels];
		for (size_t channel = 0; channel < channels; ++channel) {
			data[channel] = new T[bufferSize];
			std::fill(data[channel], data[channel] + bufferSize, fixedValue);
		}

		auto wrapper = AudioBufferWrapper{data, channels, bufferSize, channelsMapping};
		for (size_t channel = 0; channel < channels; ++channel) {
			delete[] data[channel];
		}
		delete[] data;

		return std::move(wrapper);
	}

	AudioBufferWrapper(T **data, size_t channels, size_t bufferSize, const std::vector<size_t> &channelsMapping = {}) :
			audioBuffer{abl::AudioBuffer{data, channels, bufferSize, channelsMapping}} {}

	AudioBufferWrapper(AudioBufferWrapper &&oldWrapper) noexcept: audioBuffer(std::move(oldWrapper.audioBuffer)) {}
};

TEST_CASE("[AudioBuffer] View report correct empty state", "[AudioBuffer]") {
	auto wrapper = AudioBufferWrapper<int>::createWithFixedValue(2, 8, 0);
	auto emptyWrapper = AudioBufferWrapper<double>::createWithFixedValue(0, 0, 0);
	auto emptyWrapper2 = AudioBufferWrapper<int>::createWithFixedValue(1, 0, 0);

	REQUIRE_FALSE(wrapper.audioBuffer.isEmpty());
	REQUIRE(emptyWrapper.audioBuffer.isEmpty());
	REQUIRE(emptyWrapper2.audioBuffer.isEmpty());
}

TEMPLATE_TEST_CASE("[AudioBuffer] Buffer allocate the right memory block, can be copied and moved", "[AudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 8;
	auto wrapper = AudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, 1);

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == 1);
		}
	}

	abl::AudioBuffer<TestType> copyBuffer{wrapper.audioBuffer};
	SECTION("copy constructor") {
		for (size_t channel = 0; channel < channels; ++channel) {
			for (size_t i = 0; i < bufferSize; ++i) {
				REQUIRE(copyBuffer.getSample(channel, i) == 1);
			}
		}
	}

	abl::AudioBuffer<TestType> copyBuffer2{0, 0};
	REQUIRE(copyBuffer2.isEmpty());
	copyBuffer2 = copyBuffer;

	SECTION("copy assignment") {
		REQUIRE(!copyBuffer2.isEmpty());

		for (size_t channel = 0; channel < channels; ++channel) {
			for (size_t i = 0; i < bufferSize; ++i) {
				REQUIRE(copyBuffer2.getSample(channel, i) == 1);
			}
		}
	}

	SECTION("move constructor") {
		REQUIRE(!copyBuffer.isEmpty());
		abl::AudioBuffer<TestType> moveBuffer{std::move(copyBuffer)};

		for (size_t channel = 0; channel < channels; ++channel) {
			for (size_t i = 0; i < bufferSize; ++i) {
				REQUIRE(moveBuffer.getSample(channel, i) == 1);
			}
		}

		REQUIRE(copyBuffer.isEmpty());
	}

	SECTION("move assignment") {
		REQUIRE(!copyBuffer2.isEmpty());
		abl::AudioBuffer<TestType> moveBuffer2{0, 0};

		REQUIRE(moveBuffer2.isEmpty());
		moveBuffer2 = std::move(copyBuffer2);

		REQUIRE(!moveBuffer2.isEmpty());

		for (size_t channel = 0; channel < channels; ++channel) {
			for (size_t i = 0; i < bufferSize; ++i) {
				REQUIRE(moveBuffer2.getSample(channel, i) == 1);
			}
		}

		REQUIRE(copyBuffer2.isEmpty());
	}

	//@todo find a way to test deallocation
}

TEMPLATE_TEST_CASE("[AudioBuffer] Buffer can be resized", "[AudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 8;
	auto wrapper = AudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, 1);

	abl::AudioBuffer<TestType> &audioBuffer = wrapper.audioBuffer;

	REQUIRE(audioBuffer.getChannelsCount() == channels);
	REQUIRE(audioBuffer.getBufferSize() == bufferSize);
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(audioBuffer.getSample(channel, i) == 1);
		}
	}

	SECTION("buffer size grown clearing data") {
		auto testBuffer{audioBuffer};
		size_t testBufferSize = 16;
		testBuffer.resize(channels, testBufferSize);
		REQUIRE(testBuffer.getChannelsCount() == channels);
		REQUIRE(testBuffer.getBufferSize() == testBufferSize);
		for (size_t channel = 0; channel < channels; ++channel) {
			for (size_t i = 0; i < testBufferSize; ++i) {
				REQUIRE(testBuffer.getSample(channel, i) == 0);
			}
		}
	}

	SECTION("buffer size grown keeping data") {
		auto testBuffer{audioBuffer};
		size_t testBufferSize = 16;
		testBuffer.resize(channels, testBufferSize, true);
		REQUIRE(testBuffer.getChannelsCount() == channels);
		REQUIRE(testBuffer.getBufferSize() == testBufferSize);
		for (size_t channel = 0; channel < channels; ++channel) {
			for (size_t i = 0; i < bufferSize; ++i) {
				REQUIRE(testBuffer.getSample(channel, i) == 1);
			}
			for (size_t i = bufferSize; i < testBufferSize; ++i) {
				REQUIRE(testBuffer.getSample(channel, i) == 0);
			}
		}
	}

	SECTION("buffer size grown keeping data not clearing extra space") {
		auto testBuffer{audioBuffer};
		size_t testBufferSize = 16;
		testBuffer.resize(channels, testBufferSize, true, false);
		REQUIRE(testBuffer.getChannelsCount() == channels);
		REQUIRE(testBuffer.getBufferSize() == testBufferSize);
		for (size_t channel = 0; channel < channels; ++channel) {
			for (size_t i = 0; i < bufferSize; ++i) {
				REQUIRE(testBuffer.getSample(channel, i) == 1);
			}
		}
	}

	SECTION("channels count grown clearing data") {
		auto testBuffer{audioBuffer};
		size_t testChannelsCount = 4;
		testBuffer.resize(testChannelsCount, bufferSize);
		REQUIRE(testBuffer.getChannelsCount() == testChannelsCount);
		REQUIRE(testBuffer.getBufferSize() == bufferSize);
		for (size_t channel = 0; channel < testChannelsCount; ++channel) {
			for (size_t i = 0; i < bufferSize; ++i) {
				REQUIRE(testBuffer.getSample(channel, i) == 0);
			}
		}
	}

	SECTION("channels count grown keeping data") {
		auto testBuffer{audioBuffer};
		size_t testChannelsCount = 4;
		testBuffer.resize(testChannelsCount, bufferSize, true);
		REQUIRE(testBuffer.getChannelsCount() == testChannelsCount);
		REQUIRE(testBuffer.getBufferSize() == bufferSize);
		for (size_t channel = 0; channel < channels; ++channel) {
			for (size_t i = 0; i < bufferSize; ++i) {
				REQUIRE(testBuffer.getSample(channel, i) == 1);
			}
		}
		for (size_t channel = channels; channel < testChannelsCount; ++channel) {
			for (size_t i = 0; i < bufferSize; ++i) {
				REQUIRE(testBuffer.getSample(channel, i) == 0);
			}
		}
	}

	SECTION("channels count grown keeping data not clearing extra space") {
		auto testBuffer{audioBuffer};
		size_t testChannelsCount = 4;
		testBuffer.resize(testChannelsCount, bufferSize, true, false);
		REQUIRE(testBuffer.getChannelsCount() == testChannelsCount);
		REQUIRE(testBuffer.getBufferSize() == bufferSize);
		for (size_t channel = 0; channel < channels; ++channel) {
			for (size_t i = 0; i < bufferSize; ++i) {
				REQUIRE(testBuffer.getSample(channel, i) == 1);
			}
		}
	}

	SECTION("Avoid reallocation with channels count shrink and buffer size grow") {
		auto testBuffer{audioBuffer};
		size_t testChannelsCount = 1;
		size_t testBufferSize = 16;
		testBuffer.resize(testChannelsCount, testBufferSize, false, false, true);
		REQUIRE(testBuffer.getChannelsCount() == testChannelsCount);
		REQUIRE(testBuffer.getBufferSize() == testBufferSize);
		for (size_t channel = 0; channel < testChannelsCount; ++channel) {
			for (size_t i = testBufferSize; i > 0; --i) {
				testBuffer.setSample(channel, i - 1, i - 1 + (channel * testBufferSize));
			}
		}

		for (size_t channel = 0; channel < testChannelsCount; ++channel) {
			for (size_t i = 0; i < testBufferSize; ++i) {
				REQUIRE(testBuffer.getSample(channel, i) == i + (channel * testBufferSize));
			}

			//@todo find a way to test pointers difference?
		}
	}

	SECTION("buffer size shrink clearing data") {
		auto testBuffer{audioBuffer};
		size_t testBufferSize = 4;
		testBuffer.resize(channels, testBufferSize);
		REQUIRE(testBuffer.getChannelsCount() == channels);
		REQUIRE(testBuffer.getBufferSize() == testBufferSize);
		for (size_t channel = 0; channel < channels; ++channel) {
			for (size_t i = 0; i < testBufferSize; ++i) {
				REQUIRE(testBuffer.getSample(channel, i) == 0);
			}
		}
	}

	SECTION("buffer size shrink keeping data") {
		auto testBuffer{audioBuffer};
		size_t testBufferSize = 4;
		testBuffer.resize(channels, testBufferSize, true);
		REQUIRE(testBuffer.getChannelsCount() == channels);
		REQUIRE(testBuffer.getBufferSize() == testBufferSize);
		for (size_t channel = 0; channel < channels; ++channel) {
			for (size_t i = 0; i < testBufferSize; ++i) {
				REQUIRE(testBuffer.getSample(channel, i) == 1);
			}
		}
	}

	SECTION("buffer size shrink keeping data and avoiding reallocation") {
		auto testBuffer{audioBuffer};
		size_t testBufferSize = 4;
		testBuffer.resize(channels, testBufferSize, true, true, true);
		REQUIRE(testBuffer.getChannelsCount() == channels);
		REQUIRE(testBuffer.getBufferSize() == testBufferSize);
		for (size_t channel = 0; channel < channels; ++channel) {
			for (size_t i = 0; i < testBufferSize; ++i) {
				REQUIRE(testBuffer.getSample(channel, i) == 1);
			}

			//@todo find a way to test pointers difference?
		}
	}

	SECTION("channels count shrink clearing data") {
		auto testBuffer{audioBuffer};
		size_t testChannelsCount = 1;
		testBuffer.resize(testChannelsCount, bufferSize);
		REQUIRE(testBuffer.getChannelsCount() == testChannelsCount);
		REQUIRE(testBuffer.getBufferSize() == bufferSize);
		for (size_t channel = 0; channel < testChannelsCount; ++channel) {
			for (size_t i = 0; i < bufferSize; ++i) {
				REQUIRE(testBuffer.getSample(channel, i) == 0);
			}
		}
	}

	SECTION("channels count shrink keeping data") {
		auto testBuffer{audioBuffer};
		size_t testChannelsCount = 1;
		testBuffer.resize(testChannelsCount, bufferSize, true);
		REQUIRE(testBuffer.getChannelsCount() == testChannelsCount);
		REQUIRE(testBuffer.getBufferSize() == bufferSize);
		for (size_t channel = 0; channel < testChannelsCount; ++channel) {
			for (size_t i = 0; i < bufferSize; ++i) {
				REQUIRE(testBuffer.getSample(channel, i) == 1);
			}
		}
	}

	SECTION("channels count shrink keeping data") {
		auto testBuffer{audioBuffer};
		size_t testChannelsCount = 1;
		testBuffer.resize(testChannelsCount, bufferSize, true, true, true);
		REQUIRE(testBuffer.getChannelsCount() == testChannelsCount);
		REQUIRE(testBuffer.getBufferSize() == bufferSize);
		for (size_t channel = 0; channel < testChannelsCount; ++channel) {
			for (size_t i = 0; i < bufferSize; ++i) {
				REQUIRE(testBuffer.getSample(channel, i) == 1);
			}
		}

		//@todo find a way to test pointers difference?
	}

}


TEMPLATE_TEST_CASE("[AudioBuffer] Buffer data is iterable", "[AudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 8;
	auto wrapper = AudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize);

	size_t i, j = 0;
	for (auto&& channelView: wrapper.audioBuffer) {
		i = 0;
		for (auto &&sample: channelView) {
			REQUIRE(sample == ++i + (j * bufferSize));
		}
		++j;
	}
	REQUIRE(j == channels);
}

TEMPLATE_TEST_CASE("[AudioBuffer] BufferChannelView is returned accessing a channel with the operator[] and getChannelView()", "[AudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 8;
	auto wrapper = AudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize);

	size_t i;
	for (size_t channel = 0; channel < channels; ++channel) {
		auto channelView = wrapper.audioBuffer[channel];
		REQUIRE(channelView.getBufferSize() == bufferSize);
		i = 0;
		for (auto &&sample: channelView) {
			REQUIRE(sample == ++i + (channel * bufferSize));
		}

		auto channelView2 = wrapper.audioBuffer.getChannelView(channel);
		REQUIRE(channelView2.getBufferSize() == bufferSize);
		i = 0;
		for (auto &&sample: channelView2) {
			REQUIRE(sample == ++i + (channel * bufferSize));
		}
	}
}

TEMPLATE_TEST_CASE("[AudioBuffer] Valid ranged BufferChannelView is returned accessing a channel passing a sampleRange to getChannelView()", "[AudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 8;
	auto wrapper = AudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize);
	abl::SamplesRange samplesRange{2, 4};

	size_t i;
	for (size_t channel = 0; channel < channels; ++channel) {
		auto channelView = wrapper.audioBuffer.getChannelView(channel, samplesRange);
		REQUIRE(channelView.getBufferSize() == 4);
		i = 2;
		for (auto &&sample: channelView) {
			REQUIRE(sample == ++i + (channel * bufferSize));
		}

		REQUIRE(i == 6);
	}
}

TEMPLATE_TEST_CASE("[AudioBuffer] Valid ranged BufferView is returned from getRangedView()", "[AudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 8;
	auto wrapper = AudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize);
	auto rangedBufferView = wrapper.audioBuffer.getRangedView(abl::SamplesRange{1, 5});

	size_t i;
	for (size_t channel = 0; channel < channels; ++channel) {
		auto channelView = (rangedBufferView)[channel];
		REQUIRE(channelView.getBufferSize() == 5);
		i = 1;
		for (auto &&sample: channelView) {
			REQUIRE(sample == ++i + (channel * bufferSize));
		}

		REQUIRE(i == 6);
	}
}

TEMPLATE_TEST_CASE("[AudioBuffer] Buffer data is accessible with getSample()", "[AudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 8;
	auto wrapper = AudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize);

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == i + 1 + (channel * bufferSize));
		}
	}
}

TEMPLATE_TEST_CASE("[AudioBuffer] Buffer data can be set with setSample() and addSample()", "[AudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 8;
	auto wrapper = AudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize);

	wrapper.audioBuffer.setSample(0, 0, 20);
	REQUIRE(wrapper.audioBuffer.getSample(0, 0) == 20);

	wrapper.audioBuffer.addSample(1, 2, 20);
	REQUIRE(wrapper.audioBuffer.getSample(1, 2) == 31); //11 + 20
}

TEMPLATE_TEST_CASE("[AudioBuffer] Buffer data can be copied from another buffer", "[AudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 8;
	auto wrapper = AudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, 0);
	auto wrapperCopy = AudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize);

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == 0);
		}
	}

	wrapper.audioBuffer.copyFrom(wrapperCopy.audioBuffer, abl::SamplesRange::allSamples());
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == i + 1 + (channel * bufferSize));
		}
	}

	const size_t rangeFrom = 2;
	const size_t rangeCount = 4;
	wrapper.audioBuffer.copyFrom(wrapperCopy.audioBuffer, abl::SamplesRange(rangeFrom, rangeCount));
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == (i >= rangeFrom && i < rangeFrom + rangeCount ? 1 - rangeFrom : 1) + i + (channel * bufferSize));
		}
	}

	wrapper.audioBuffer.copyFrom(wrapperCopy.audioBuffer, abl::SamplesRange::allSamples(), 0.5);
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == TestType(i + 1 + (channel * bufferSize)) / 2);
		}
	}
}

TEMPLATE_TEST_CASE("[AudioBuffer] Channel buffer data can be copied from another channel buffer", "[AudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 8;
	auto wrapper = AudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, 0);
	auto wrapperCopy = AudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize);

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == 0);
		}

		wrapper.audioBuffer.copyIntoChannelFrom(wrapperCopy.audioBuffer.getChannelView(channel), channel, abl::SamplesRange::allSamples());
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == i + 1 + (channel * bufferSize));
		}

		const size_t rangeFrom = 2;
		const size_t rangeCount = 4;
		wrapper.audioBuffer.copyIntoChannelFrom(wrapperCopy.audioBuffer.getChannelView(channel), channel, abl::SamplesRange(rangeFrom, rangeCount));
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == (i >= rangeFrom && i < rangeFrom + rangeCount ? 1 - rangeFrom : 1) + i + (channel * bufferSize));
		}

		wrapper.audioBuffer.copyIntoChannelFrom(wrapperCopy.audioBuffer.getChannelView(channel), channel, abl::SamplesRange::allSamples(), 0.5);
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == TestType(i + 1 + (channel * bufferSize)) / 2);
		}
	}
}

TEMPLATE_TEST_CASE("[AudioBuffer] Buffer data can be copied from another buffer with ramp", "[AudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 8;
	auto wrapper = AudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, 0);
	auto wrapperCopy = AudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, 8);

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == 0);
		}
	}

	wrapper.audioBuffer.copyWithRampFrom(wrapperCopy.audioBuffer, 0.0, 1.0, abl::SamplesRange::allSamples());
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == i);
		}
	}

	const size_t rangeFrom = 1;
	const size_t rangeCount = 4;
	wrapper.audioBuffer.copyWithRampFrom(wrapperCopy.audioBuffer, 0.0, 1.0, abl::SamplesRange(rangeFrom, rangeCount));
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			unsigned val = i >= rangeFrom && i < rangeFrom + rangeCount ? (i - rangeFrom) * 2 : i;
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == val);
		}
	}

	wrapper.audioBuffer.copyWithRampFrom(wrapperCopy.audioBuffer, 0.0, 0.5, abl::SamplesRange::allSamples());
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == TestType(i) / 2);
		}
	}
}

TEMPLATE_TEST_CASE("[AudioBuffer] Channel buffer data can be copied from another buffer with ramp", "[AudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 8;
	auto wrapper = AudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, 0);
	auto wrapperCopy = AudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, 8);

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == 0);
		}

		wrapper.audioBuffer.copyIntoChannelWithRampFrom(wrapperCopy.audioBuffer.getChannelView(channel), channel, 0.0, 1.0, abl::SamplesRange::allSamples());
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == i);
		}

		const size_t rangeFrom = 1;
		const size_t rangeCount = 4;
		wrapper.audioBuffer.copyIntoChannelWithRampFrom(wrapperCopy.audioBuffer.getChannelView(channel), channel, 0.0, 1.0, abl::SamplesRange(rangeFrom, rangeCount));
		for (size_t i = 0; i < bufferSize; ++i) {
			auto val = i >= rangeFrom && i < rangeFrom + rangeCount ? (i - rangeFrom) * 2 : i;
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == val);
		}

		wrapper.audioBuffer.copyIntoChannelWithRampFrom(wrapperCopy.audioBuffer.getChannelView(channel), channel, 0.0, 0.5, abl::SamplesRange::allSamples());
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == TestType(i) / 2);
		}
	}
}

TEMPLATE_TEST_CASE("[AudioBuffer] Buffer data can be added from another buffer", "[AudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 8;
	auto wrapper = AudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, 2);
	auto wrapperCopy = AudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize);

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == 2);
		}
	}

	wrapper.audioBuffer.addFrom(wrapperCopy.audioBuffer, abl::SamplesRange::allSamples());
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == i + 1 + (channel * bufferSize) + 2);
		}
	}

	wrapper.audioBuffer.addFrom(wrapperCopy.audioBuffer, abl::SamplesRange::allSamples(), 0.5);
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == (TestType(i + 1 + (channel * bufferSize)) / 2) + (i + 1) + 2 + (channel * bufferSize));
		}
	}

	const size_t rangeFrom = 3;
	const size_t rangeCount = 4;
	auto rangeWrapper = AudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, 2);
	rangeWrapper.audioBuffer.addFrom(wrapperCopy.audioBuffer, abl::SamplesRange(rangeFrom, rangeCount));
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(rangeWrapper.audioBuffer.getSample(channel, i) == ((i >= rangeFrom && i < rangeFrom + rangeCount) ? i + 3 - rangeFrom + channel * bufferSize : 2));
		}
	}
}

TEMPLATE_TEST_CASE("[AudioBuffer] Channel buffer data can be added from another channel buffer", "[AudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 8;
	auto wrapper = AudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, 2);
	auto wrapperCopy = AudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize);
	auto rangeWrapper = AudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, 2);

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == 2);
		}

		wrapper.audioBuffer.addIntoChannelFrom(wrapperCopy.audioBuffer.getChannelView(channel), channel, abl::SamplesRange::allSamples());
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == i + 1 + (channel * bufferSize) + 2);
		}

		wrapper.audioBuffer.addIntoChannelFrom(wrapperCopy.audioBuffer.getChannelView(channel), channel, abl::SamplesRange::allSamples(), 0.5);
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == (TestType(i + 1 + (channel * bufferSize)) / 2) + (i + 1) + 2 + (channel * bufferSize));
		}

		const size_t rangeFrom = 3;
		const size_t rangeCount = 4;
		rangeWrapper.audioBuffer.addIntoChannelFrom(wrapperCopy.audioBuffer.getChannelView(channel), channel, abl::SamplesRange(rangeFrom, rangeCount));
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(rangeWrapper.audioBuffer.getSample(channel, i) == ((i >= rangeFrom && i < rangeFrom + rangeCount) ? i + 3 - rangeFrom + channel * bufferSize : 2));
		}
	}
}

TEMPLATE_TEST_CASE("[AudioBuffer] Buffer data can be added from another buffer with ramp", "[AudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 8;
	auto wrapper = AudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, 3);
	auto wrapperCopy = AudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, 8);

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == 3);
		}
	}

	wrapper.audioBuffer.addWithRampFrom(wrapperCopy.audioBuffer, 0.0, 1.0, abl::SamplesRange::allSamples());
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == i + 3);
		}
	}

	wrapper.audioBuffer.addWithRampFrom(wrapperCopy.audioBuffer, 0.0, 0.5, abl::SamplesRange::allSamples());
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == (TestType(i) / 2) + i + 3);
		}
	}

	const size_t rangeFrom = 2;
	const size_t rangeCount = 4;
	auto rangeWrapper = AudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, 3);
	rangeWrapper.audioBuffer.addWithRampFrom(wrapperCopy.audioBuffer, 0.0, 0.5, abl::SamplesRange(rangeFrom, rangeCount));
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(rangeWrapper.audioBuffer.getSample(channel, i) == (i >= rangeFrom && i < rangeFrom + rangeCount ? i - rangeFrom : 0) + 3);
		}
	}
}

TEMPLATE_TEST_CASE("[AudioBuffer] Channel buffer data can be added from another buffer with ramp", "[AudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 8;
	auto wrapper = AudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, 3);
	auto wrapperCopy = AudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, 8);
	auto rangeWrapper = AudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, 3);

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == 3);
		}

		wrapper.audioBuffer.addIntoChannelWithRampFrom(wrapperCopy.audioBuffer.getChannelView(channel), channel, 0.0, 1.0, abl::SamplesRange::allSamples());
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == i + 3);
		}

		wrapper.audioBuffer.addIntoChannelWithRampFrom(wrapperCopy.audioBuffer.getChannelView(channel), channel, 0.0, 0.5, abl::SamplesRange::allSamples());
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == (TestType(i) / 2) + i + 3);
		}

		const size_t rangeFrom = 2;
		const size_t rangeCount = 4;
		rangeWrapper.audioBuffer.addIntoChannelWithRampFrom(wrapperCopy.audioBuffer.getChannelView(channel), channel, 0.0, 0.5, abl::SamplesRange(rangeFrom, rangeCount));
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(rangeWrapper.audioBuffer.getSample(channel, i) == (i >= rangeFrom && i < rangeFrom + rangeCount ? i - rangeFrom : 0) + 3);
		}
	}
}

TEMPLATE_TEST_CASE("[AudioBuffer] Buffer data is scaled with applyGain()", "[AudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 8;
	auto wrapper = AudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize);

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == i + 1 + (channel * bufferSize));
		}
	}

	wrapper.audioBuffer.applyGain(0.5, abl::SamplesRange::allSamples());
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == TestType(i + 1 + (channel * bufferSize)) / 2);
		}
	}

	wrapper.audioBuffer.applyGain(3.0, abl::SamplesRange::allSamples());
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == (TestType(i + 1 + (channel * bufferSize)) / 2) * 3);
		}
	}

	const size_t rangeFrom = 5;
	const size_t rangeCount = 3;
	auto rangeWrapper = AudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize);
	rangeWrapper.audioBuffer.applyGain(2.0, abl::SamplesRange(rangeFrom, rangeCount));
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			unsigned val = i + 1 + channel * bufferSize;
			REQUIRE(rangeWrapper.audioBuffer.getSample(channel, i) == (i >= rangeFrom && i < rangeFrom + rangeCount ? val * 2 : val));
		}
	}
}

TEMPLATE_TEST_CASE("[AudioBuffer] Channel buffer data is scaled with applyGainToChannel()", "[AudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 8;
	auto wrapper = AudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize);
	auto rangeWrapper = AudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize);

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == i + 1 + (channel * bufferSize));
		}

		wrapper.audioBuffer.applyGainToChannel(0.5, channel, abl::SamplesRange::allSamples());
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == TestType(i + 1 + (channel * bufferSize)) / 2);
		}

		wrapper.audioBuffer.applyGainToChannel(3.0, channel, abl::SamplesRange::allSamples());
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == (TestType(i + 1 + (channel * bufferSize)) / 2) * 3);
		}

		const size_t rangeFrom = 5;
		const size_t rangeCount = 3;
		rangeWrapper.audioBuffer.applyGainToChannel(2.0, channel, abl::SamplesRange(rangeFrom, rangeCount));
		for (size_t i = 0; i < bufferSize; ++i) {
			unsigned val = i + 1 + channel * bufferSize;
			REQUIRE(rangeWrapper.audioBuffer.getSample(channel, i) == (i >= rangeFrom && i < rangeFrom + rangeCount ? val * 2 : val));
		}
	}
}

TEMPLATE_TEST_CASE("[AudioBuffer] Buffer data is scaled with ramp using applyGainRamp()", "[AudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 8;
	auto wrapper = AudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, 10);

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == 10);
		}
	}

	wrapper.audioBuffer.applyGainRamp(0.0, 1.0, abl::SamplesRange::allSamples());
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == TestType(i) * 10 / bufferSize);
		}
	}

	wrapper.audioBuffer.applyGainRamp(0.5, 0.0, abl::SamplesRange::allSamples());
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == (TestType(i) * 10 / bufferSize) * (bufferSize - i) / (bufferSize * 2));
		}
	}

	const size_t rangeFrom = 2;
	const size_t rangeCount = 4;
	auto rangeWrapper = AudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, 10);
	rangeWrapper.audioBuffer.applyGainRamp(0.0, 1.0, abl::SamplesRange(rangeFrom, rangeCount));
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(rangeWrapper.audioBuffer.getSample(channel, i) == (i >= rangeFrom && i < rangeFrom + rangeCount ? TestType(i - rangeFrom) * 20 / bufferSize : 10));
		}
	}
}

TEMPLATE_TEST_CASE("[AudioBuffer] Channel buffer data is scaled with applyGainRampToChannel()", "[AudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 8;
	auto wrapper = AudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, 10);
	auto rangeWrapper = AudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, 10);

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == 10);
		}

		wrapper.audioBuffer.applyGainRampToChannel(0.0, 1.0, channel, abl::SamplesRange::allSamples());
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == TestType(i) * 10 / bufferSize);
		}

		wrapper.audioBuffer.applyGainRampToChannel(0.5, 0.0, channel, abl::SamplesRange::allSamples());
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == (TestType(i) * 10 / bufferSize) * (bufferSize - i) / (bufferSize * 2));
		}

		const size_t rangeFrom = 2;
		const size_t rangeCount = 4;
		rangeWrapper.audioBuffer.applyGainRampToChannel(0.0, 1.0, channel, abl::SamplesRange(rangeFrom, rangeCount));
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(rangeWrapper.audioBuffer.getSample(channel, i) == (i >= rangeFrom && i < rangeFrom + rangeCount ? TestType(i - rangeFrom) * 20 / bufferSize : 10));
		}
	}
}

TEMPLATE_TEST_CASE("[AudioBuffer] Buffer data is cleared with clear()", "[AudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 8;
	auto wrapper = AudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize);

	wrapper.audioBuffer.clear(abl::SamplesRange::allSamples());

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == 0);
		}
	}

	const size_t rangeFrom = 5;
	const size_t rangeCount = 3;
	auto rangeWrapper = AudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, 4);
	rangeWrapper.audioBuffer.clear(abl::SamplesRange(rangeFrom, rangeCount));
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(rangeWrapper.audioBuffer.getSample(channel, i) == (i >= rangeFrom && i < rangeFrom + rangeCount ? 0 : 4));
		}
	}
}

TEMPLATE_TEST_CASE("[AudioBuffer] Channel Buffer data is cleared with clearChannel()", "[AudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 8;
	auto wrapper = AudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize);
	auto rangeWrapper = AudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize);

	wrapper.audioBuffer.clearChannel(1, abl::SamplesRange::allSamples());

	for (size_t i = 0; i < bufferSize; ++i) {
		REQUIRE(wrapper.audioBuffer.getSample(0, i) == i + 1);
	}

	for (size_t i = 0; i < bufferSize; ++i) {
		REQUIRE(wrapper.audioBuffer.getSample(1, i) == 0);
	}

	const size_t rangeFrom = 5;
	const size_t rangeCount = 3;
	rangeWrapper.audioBuffer.clearChannel(0, abl::SamplesRange(rangeFrom, rangeCount));

	for (size_t i = 0; i < bufferSize; ++i) {
		REQUIRE(rangeWrapper.audioBuffer.getSample(0, i) == (i >= rangeFrom && i < rangeFrom + rangeCount ? 0 : i + 1));
	}

	for (size_t i = 0; i < bufferSize; ++i) {
		REQUIRE(rangeWrapper.audioBuffer.getSample(1, i) == i + 1 + bufferSize);
	}
}

TEMPLATE_TEST_CASE("[AudioBuffer] Buffer data is reversed with reverse()", "[AudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 8;
	auto wrapper = AudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize);

	wrapper.audioBuffer.reverse(abl::SamplesRange::allSamples());

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == bufferSize * (channel + 1) - i);
		}
	}

	const auto rangeFrom = 2;
	const auto rangeCount = 4;
	auto rangeWrapper = AudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize);
	rangeWrapper.audioBuffer.reverse(abl::SamplesRange(rangeFrom, rangeCount));
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			if (i >= rangeFrom && i < rangeFrom + rangeCount) {
				REQUIRE(rangeWrapper.audioBuffer.getSample(channel, i) == bufferSize * (channel + 1) - i);
			} else {
				REQUIRE(rangeWrapper.audioBuffer.getSample(channel, i) == i + 1 + (bufferSize * channel));
			}
		}
	}
}

TEMPLATE_TEST_CASE("[AudioBuffer] Channel Buffer data is reversed with reverse()", "[AudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 8;
	auto wrapper = AudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize);

	wrapper.audioBuffer.reverseChannel(1, abl::SamplesRange::allSamples());

	for (size_t i = 0; i < bufferSize; ++i) {
		REQUIRE(wrapper.audioBuffer.getSample(0, i) == i + 1);
	}

	for (size_t i = 0; i < bufferSize; ++i) {
		REQUIRE(wrapper.audioBuffer.getSample(1, i) == 16 - i);
	}

	const auto rangeFrom = 2;
	const auto rangeCount = 4;
	auto rangeWrapper = AudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize);
	rangeWrapper.audioBuffer.reverseChannel(0, abl::SamplesRange(rangeFrom, rangeCount));

	for (size_t i = 0; i < bufferSize; ++i) {
		if (i >= rangeFrom && i < rangeFrom + rangeCount) {
			REQUIRE(rangeWrapper.audioBuffer.getSample(0, i) == bufferSize - i);
		} else {
			REQUIRE(rangeWrapper.audioBuffer.getSample(0, i) == i + 1);
		}
	}

	for (size_t i = 0; i < bufferSize; ++i) {
		REQUIRE(rangeWrapper.audioBuffer.getSample(1, i) == 9 + i);
	}
}

TEMPLATE_TEST_CASE("[AudioBuffer] View report correct higher peak", "[AudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 8;
	auto wrapper = AudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize);

	REQUIRE(wrapper.audioBuffer.getHigherPeak(abl::SamplesRange::allSamples()) == 16);
	REQUIRE(wrapper.audioBuffer.getHigherPeak(abl::SamplesRange(3, 3)) == 14);
	REQUIRE(wrapper.audioBuffer.getHigherPeak(abl::SamplesRange(5, 2)) == 15);
}

TEMPLATE_TEST_CASE("[AudioBuffer] View report correct higher peak for channels", "[AudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 8;
	auto wrapper = AudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize);

	for (size_t channel = 0; channel < channels; ++channel) {
		REQUIRE(wrapper.audioBuffer.getHigherPeakForChannel(channel, abl::SamplesRange::allSamples()) == bufferSize * (channel + 1));
		REQUIRE(wrapper.audioBuffer.getHigherPeakForChannel(channel, abl::SamplesRange(3, 3)) == 6 + (bufferSize * channel));
		REQUIRE(wrapper.audioBuffer.getHigherPeakForChannel(channel, abl::SamplesRange(5, 2)) == 7 + (bufferSize * channel));
	}
}

TEMPLATE_TEST_CASE("[AudioBuffer] View report correct rms level for channels", "[AudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 8;
	auto wrapper = AudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize);

	REQUIRE(wrapper.audioBuffer.getRMSLevelForChannel(0, abl::SamplesRange::allSamples()) == TestType(4.5));
	REQUIRE(wrapper.audioBuffer.getRMSLevelForChannel(1, abl::SamplesRange::allSamples()) == TestType(12.5));
	REQUIRE(wrapper.audioBuffer.getRMSLevelForChannel(0, abl::SamplesRange(3, 4)) == TestType(5.5));
	REQUIRE(wrapper.audioBuffer.getRMSLevelForChannel(1, abl::SamplesRange(4, 2)) == TestType(13.5));
}

TEMPLATE_TEST_CASE("[AudioBuffer] View report correct buffer size and channel count", "[AudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 8;
	auto wrapper = AudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize);

	REQUIRE(wrapper.audioBuffer.getBufferSize() == bufferSize);
	REQUIRE(wrapper.audioBuffer.getChannelsCount() == channels);
}

TEMPLATE_TEST_CASE("[AudioBuffer] Channels are mapped correctly", "[AudioBuffer]", int, double) {
	const size_t channels = 4;
	const size_t bufferSize = 8;
	std::vector<size_t> channelsMapping{3, 1, 2, 0, 1};
	auto wrapper = AudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, channelsMapping);

	REQUIRE(wrapper.audioBuffer.getChannelsCount() == 5);
	size_t channel = 0;
	for (auto mappedChannel: channelsMapping) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == i + 1 + (mappedChannel * bufferSize));
		}
		++channel;
	}

	channelsMapping = {0, 2};
	wrapper.audioBuffer.setChannelsMapping(channelsMapping);
	REQUIRE(wrapper.audioBuffer.getChannelsCount() == 2);
	channel = 0;
	for (auto mappedChannel: channelsMapping) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == i + 1 + (mappedChannel * bufferSize));
		}
		++channel;
	}

	wrapper.audioBuffer.createSequentialChannelsMapping(1, 3);
	REQUIRE(wrapper.audioBuffer.getChannelsCount() == 3);
	auto sequentialChannelsMapping = wrapper.audioBuffer.getChannelsMapping();
	for (size_t i = 0; i < 3; ++i) {
		REQUIRE(sequentialChannelsMapping[i] == i + 1);
	}

	channel = 0;
	for (auto mappedChannel: sequentialChannelsMapping) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == i + 1 + (mappedChannel * bufferSize));
		}
		++channel;
	}
}


//test channel out-of-bound assert?