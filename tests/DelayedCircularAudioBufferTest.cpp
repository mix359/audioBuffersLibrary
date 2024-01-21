// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "../buffers/DelayedCircularAudioBuffer.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_template_test_macros.hpp>

template<typename T>
struct DelayedCircularAudioBufferWrapper {
	abl::DelayedCircularAudioBuffer<T> audioBuffer;

	static DelayedCircularAudioBufferWrapper createWithIncrementalNumbers(size_t channels, size_t bufferSize, size_t singleBufferSize, size_t delayInSamples, size_t startOffset, const std::vector<size_t>& channelsMapping = {}, size_t startIndex = 0) {
		auto data = new T*[channels];
		size_t startValue = 1;
		for(size_t channel = 0; channel < channels; ++channel) {
			data[channel] = new T[bufferSize];
			std::iota(data[channel], data[channel] + bufferSize, startValue);
			startValue += bufferSize;
		}

		auto wrapper = DelayedCircularAudioBufferWrapper{data, channels, bufferSize, singleBufferSize, delayInSamples, startOffset, channelsMapping, startIndex};
		for(size_t channel = 0; channel < channels; ++channel) {
			delete[] data[channel];
		}
		delete[] data;

		return wrapper;
	}

	static DelayedCircularAudioBufferWrapper createWithFixedValue(size_t channels, size_t bufferSize, size_t singleBufferSize, size_t delayInSamples, size_t startOffset, T fixedValue, const std::vector<size_t>& channelsMapping = {}, size_t startIndex = 0) {
		auto data = new T*[channels];
		for(size_t channel = 0; channel < channels; ++channel) {
			data[channel] = new T[bufferSize];
			std::fill(data[channel], data[channel] + bufferSize, fixedValue);
		}

		auto wrapper = DelayedCircularAudioBufferWrapper{data, channels, bufferSize, singleBufferSize, delayInSamples, startOffset, channelsMapping, startIndex};
		for(size_t channel = 0; channel < channels; ++channel) {
			delete[] data[channel];
		}
		delete[] data;

		return wrapper;
	}

	DelayedCircularAudioBufferWrapper(T** newData, size_t channels, size_t bufferSize, size_t singleBufferSize, size_t delayInSamples, size_t bufferStartOffset, const std::vector<size_t>& channelsMapping = {}, size_t startIndex = 0) :
		audioBuffer{abl::DelayedCircularAudioBuffer {newData, channels, bufferSize, singleBufferSize, delayInSamples, bufferStartOffset, channelsMapping, startIndex}} {}

};

//@todo test other contructor?

TEST_CASE("[DelayedCircularAudioBuffer] View report correct empty state", "[DelayedCircularAudioBuffer]") {
	auto noValueWrapper = DelayedCircularAudioBufferWrapper<int>::createWithFixedValue(2, 16, 8, 0, 0, 0);
	auto noChannelsWrapper = DelayedCircularAudioBufferWrapper<int>::createWithFixedValue(0, 0, 0, 0, 0, 0);
	auto noBufferSizeWrapper = DelayedCircularAudioBufferWrapper<double>::createWithFixedValue(1, 0, 8, 0, 0, 0);
	auto noSingleBufferSizeWrapper = DelayedCircularAudioBufferWrapper<double>::createWithFixedValue(1, 16, 0, 0, 0, 0);

	REQUIRE_FALSE(noValueWrapper.audioBuffer.isEmpty());
	REQUIRE(noChannelsWrapper.audioBuffer.isEmpty());
	REQUIRE(noBufferSizeWrapper.audioBuffer.isEmpty());
	REQUIRE(noSingleBufferSizeWrapper.audioBuffer.isEmpty());
}


TEMPLATE_TEST_CASE("[DelayedCircularAudioBuffer] Buffer data is iterable", "[DelayedCircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 8;
	const size_t startOffset = 20;
	auto wrapper = DelayedCircularAudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, delayInSamples, startOffset);

	size_t i, j = 0;
	for(auto&& channelView : wrapper.audioBuffer) {
//		REQUIRE(dynamic_cast<abl::OffsettedReadCircularAudioBufferChannelView<TestType>*>(channelView.get()));
		i = startOffset;
		for(auto&& sample : channelView) {
			if(++i > bufferSize) i = 1;
			REQUIRE(sample == i + (j*bufferSize));
		}
		++j;
	}
	REQUIRE(j == channels);
}

TEMPLATE_TEST_CASE("[DelayedCircularAudioBuffer] CircularBufferChannelView is returned accessing a channel with the operator[] and getChannelView()", "[DelayedCircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 8;
	const size_t startOffset = 20;
	
	auto wrapper = DelayedCircularAudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, delayInSamples, startOffset);

	size_t i;
	for(size_t channel = 0; channel < channels; ++channel) {
		auto channelView = wrapper.audioBuffer[channel];
		REQUIRE(channelView.getBufferSize() == singleBufferSize);
//		REQUIRE(dynamic_cast<abl::OffsettedReadCircularAudioBufferChannelView<TestType>*>(channelView.get()));
		i = startOffset;
		for(auto&& sample : channelView) {
			if(++i > bufferSize) i = 1;
			REQUIRE(sample == i + (channel*bufferSize));
		}

		auto channelView2 = wrapper.audioBuffer.getChannelView(channel);
		REQUIRE(channelView2.getBufferSize() == singleBufferSize);
//		REQUIRE(dynamic_cast<abl::OffsettedReadCircularAudioBufferChannelView<TestType>*>(channelView2.get()));
		i = startOffset;
		for(auto&& sample : channelView2) {
			if(++i > bufferSize) i = 1;
			REQUIRE(sample == i + (channel*bufferSize));
		}
	}
}

TEMPLATE_TEST_CASE("[DelayedCircularAudioBuffer] Valid ranged CircularBufferChannelView is returned accessing a channel passing a sampleRange to getChannelView()", "[DelayedCircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 8;
	const size_t startOffset = 20;
	
	auto wrapper = DelayedCircularAudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, delayInSamples, startOffset);
	abl::SamplesRange samplesRange{2, 4};

	size_t i, j;
	for(size_t channel = 0; channel < channels; ++channel) {
		auto channelView = wrapper.audioBuffer.getChannelView(channel, samplesRange);
		REQUIRE(channelView.getBufferSize() == 4);
		j = 2;
		i = startOffset + 2;
		for(auto&& sample : channelView) {
			if(++i > bufferSize) i = 1;
			REQUIRE(sample == i + (channel*bufferSize));
			++j;
		}

		REQUIRE(j == 6);
	}
}

TEMPLATE_TEST_CASE("[DelayedCircularAudioBuffer] Buffer allocate the right memory block, can be copied and moved", "[DelayedCircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 8;
	const size_t singleBufferSize = 4;
	const size_t delayInSamples = 2;
	const size_t startOffset = 2;
	auto wrapper = DelayedCircularAudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, delayInSamples, startOffset, 1);

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == 1);
		}
	}

	abl::DelayedCircularAudioBuffer<TestType> copyBuffer{wrapper.audioBuffer};
	SECTION("copy constructor") {
		for (size_t channel = 0; channel < channels; ++channel) {
			for (size_t i = 0; i < singleBufferSize; ++i) {
				REQUIRE(copyBuffer.getSample(channel, i) == 1);
			}
		}
	}

	abl::DelayedCircularAudioBuffer<TestType> copyBuffer2{0, 0, 0};
	REQUIRE(copyBuffer2.isEmpty());
	copyBuffer2 = copyBuffer;

	SECTION("copy assignment") {
		REQUIRE(!copyBuffer2.isEmpty());

		for (size_t channel = 0; channel < channels; ++channel) {
			for (size_t i = 0; i < singleBufferSize; ++i) {
				REQUIRE(copyBuffer2.getSample(channel, i) == 1);
			}
		}
	}

	SECTION("move constructor") {
		REQUIRE(!copyBuffer.isEmpty());
		abl::DelayedCircularAudioBuffer<TestType> moveBuffer {std::move(copyBuffer)};

		for (size_t channel = 0; channel < channels; ++channel) {
			for (size_t i = 0; i < singleBufferSize; ++i) {
				REQUIRE(moveBuffer.getSample(channel, i) == 1);
			}
		}

		REQUIRE(copyBuffer.isEmpty());
	}

	SECTION("move assignment") {
		REQUIRE(!copyBuffer2.isEmpty());
		abl::DelayedCircularAudioBuffer<TestType> moveBuffer2 {0, 0, 0};

		REQUIRE(moveBuffer2.isEmpty());
		moveBuffer2 = std::move(copyBuffer2);

		REQUIRE(!moveBuffer2.isEmpty());

		for (size_t channel = 0; channel < channels; ++channel) {
			for (size_t i = 0; i < singleBufferSize; ++i) {
				REQUIRE(moveBuffer2.getSample(channel, i) == 1);
			}
		}

		REQUIRE(copyBuffer2.isEmpty());
	}

	//@todo find a way to test deallocation
}

TEMPLATE_TEST_CASE("[DelayedCircularAudioBuffer] Buffer can be resized", "[DelayedCircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 8;
	const size_t singleBufferSize = 4;
	const size_t delayInSamples = 2;
	const size_t startOffset = 2;
	auto wrapper = DelayedCircularAudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, delayInSamples, startOffset, 1);

	abl::DelayedCircularAudioBuffer<TestType>& audioBuffer = wrapper.audioBuffer;

	REQUIRE(audioBuffer.getChannelsCount() == channels);
	REQUIRE(audioBuffer.getBaseBufferSize() == bufferSize);
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(audioBuffer.getSample(channel, i) == 1);
		}
	}

	SECTION("buffer size grown clearing data") {
		auto testBuffer{audioBuffer};
		size_t testBufferSize = 16;
		testBuffer.resize(channels, testBufferSize);
		REQUIRE(testBuffer.getChannelsCount() == channels);
		REQUIRE(testBuffer.getBaseBufferSize() == testBufferSize);
		testBuffer.incrementIndex(10);
		for (size_t channel = 0; channel < channels; ++channel) {
			for (size_t i = 0; i < singleBufferSize; ++i) {
				REQUIRE(testBuffer.getSample(channel, i) == 0);
			}
		}
	}

	SECTION("buffer size grown keeping data") {
		auto testBuffer{audioBuffer};
		size_t testBufferSize = 16;
		testBuffer.resize(channels, testBufferSize, true);
		REQUIRE(testBuffer.getChannelsCount() == channels);
		REQUIRE(testBuffer.getBaseBufferSize() == testBufferSize);
		testBuffer.incrementIndex(12);
		for (size_t channel = 0; channel < channels; ++channel) {
			for (size_t i = 0; i < singleBufferSize; ++i) {
				REQUIRE(testBuffer.getSample(channel, i) == (i < 2 ? 0 : 1));
			}
		}
	}

	SECTION("buffer size grown keeping data not clearing extra space") {
		auto testBuffer{audioBuffer};
		size_t testBufferSize = 16;
		testBuffer.resize(channels, testBufferSize, true, false);
		REQUIRE(testBuffer.getChannelsCount() == channels);
		REQUIRE(testBuffer.getBaseBufferSize() == testBufferSize);
		for (size_t channel = 0; channel < channels; ++channel) {
			for (size_t i = 0; i < singleBufferSize; ++i) {
				REQUIRE(testBuffer.getSample(channel, i) == 1);
			}
		}
	}

	SECTION("channels count grown clearing data") {
		auto testBuffer{audioBuffer};
		size_t testChannelsCount = 4;
		testBuffer.resize(testChannelsCount, bufferSize);
		REQUIRE(testBuffer.getChannelsCount() == testChannelsCount);
		REQUIRE(testBuffer.getBaseBufferSize() == bufferSize);
		testBuffer.incrementIndex(12);
		for (size_t channel = 0; channel < testChannelsCount; ++channel) {
			for (size_t i = 0; i < singleBufferSize; ++i) {
				REQUIRE(testBuffer.getSample(channel, i) == 0);
			}
		}
	}

	SECTION("channels count grown keeping data") {
		auto testBuffer{audioBuffer};
		size_t testChannelsCount = 4;
		testBuffer.resize(testChannelsCount, bufferSize, true);
		REQUIRE(testBuffer.getChannelsCount() == testChannelsCount);
		REQUIRE(testBuffer.getBaseBufferSize() == bufferSize);
		testBuffer.incrementIndex(12);
		for (size_t channel = 0; channel < channels; ++channel) {
			for (size_t i = 0; i < singleBufferSize; ++i) {
				REQUIRE(testBuffer.getSample(channel, i) == 1);
			}
		}
		for (size_t channel = channels; channel < testChannelsCount; ++channel) {
			for (size_t i = 0; i < singleBufferSize; ++i) {
				REQUIRE(testBuffer.getSample(channel, i) == 0);
			}
		}
	}

	SECTION("channels count grown keeping data not clearing extra space") {
		auto testBuffer{audioBuffer};
		size_t testChannelsCount = 4;
		testBuffer.resize(testChannelsCount, bufferSize, true, false);
		REQUIRE(testBuffer.getChannelsCount() == testChannelsCount);
		REQUIRE(testBuffer.getBaseBufferSize() == bufferSize);
		testBuffer.incrementIndex(12);
		for (size_t channel = 0; channel < channels; ++channel) {
			for (size_t i = 0; i < singleBufferSize; ++i) {
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
		REQUIRE(testBuffer.getBaseBufferSize() == testBufferSize);
		testBuffer.incrementIndex(10);
		for (size_t channel = 0; channel < testChannelsCount; ++channel) {
			for (size_t i = singleBufferSize; i > 0; --i) {
				testBuffer.setSample(channel, i - 1, i - 1 + (channel * singleBufferSize));
			}
		}

		testBuffer.incrementIndex(2);
		for (size_t channel = 0; channel < testChannelsCount; ++channel) {
			for (size_t i = 0; i < singleBufferSize; ++i) {
				REQUIRE(testBuffer.getSample(channel, i) == i + (channel * singleBufferSize));
			}

			//@todo find a way to test pointers difference?
		}

		testBuffer.incrementIndex();
		for (size_t channel = 0; channel < testChannelsCount; ++channel) {
			for (size_t i = 0; i < singleBufferSize; ++i) {
				REQUIRE(testBuffer.getSample(channel, i) == 0);
			}
		}
	}

	SECTION("buffer size shrink clearing data") {
		auto testBuffer{audioBuffer};
		size_t testBufferSize = 4;
		testBuffer.resize(channels, testBufferSize);
		REQUIRE(testBuffer.getChannelsCount() == channels);
		REQUIRE(testBuffer.getBaseBufferSize() == testBufferSize);
		for (size_t channel = 0; channel < channels; ++channel) {
			for (size_t i = 0; i < singleBufferSize; ++i) {
				REQUIRE(testBuffer.getSample(channel, i) == 0);
			}
		}
	}

	SECTION("buffer size shrink keeping data") {
		auto testBuffer{audioBuffer};
		size_t testBufferSize = 4;
		testBuffer.resize(channels, testBufferSize, true);
		REQUIRE(testBuffer.getChannelsCount() == channels);
		REQUIRE(testBuffer.getBaseBufferSize() == testBufferSize);
		for (size_t channel = 0; channel < channels; ++channel) {
			for (size_t i = 0; i < singleBufferSize; ++i) {
				REQUIRE(testBuffer.getSample(channel, i) == 1);
			}
		}
	}

	SECTION("buffer size shrink keeping data and avoiding reallocation") {
		auto testBuffer{audioBuffer};
		size_t testBufferSize = 4;
		testBuffer.resize(channels, testBufferSize, true, true, true);
		REQUIRE(testBuffer.getChannelsCount() == channels);
		REQUIRE(testBuffer.getBaseBufferSize() == testBufferSize);
		for (size_t channel = 0; channel < channels; ++channel) {
			for (size_t i = 0; i < singleBufferSize; ++i) {
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
		REQUIRE(testBuffer.getBaseBufferSize() == bufferSize);
		for (size_t channel = 0; channel < testChannelsCount; ++channel) {
			for (size_t i = 0; i < singleBufferSize; ++i) {
				REQUIRE(testBuffer.getSample(channel, i) == 0);
			}
		}
	}

	SECTION("channels count shrink keeping data") {
		auto testBuffer{audioBuffer};
		size_t testChannelsCount = 1;
		testBuffer.resize(testChannelsCount, bufferSize, true);
		REQUIRE(testBuffer.getChannelsCount() == testChannelsCount);
		REQUIRE(testBuffer.getBaseBufferSize() == bufferSize);
		for (size_t channel = 0; channel < testChannelsCount; ++channel) {
			for (size_t i = 0; i < singleBufferSize; ++i) {
				REQUIRE(testBuffer.getSample(channel, i) == 1);
			}
		}
	}

	SECTION("channels count shrink keeping data") {
		auto testBuffer{audioBuffer};
		size_t testChannelsCount = 1;
		testBuffer.resize(testChannelsCount, bufferSize, true, true, true);
		REQUIRE(testBuffer.getChannelsCount() == testChannelsCount);
		REQUIRE(testBuffer.getBaseBufferSize() == bufferSize);
		for (size_t channel = 0; channel < testChannelsCount; ++channel) {
			for (size_t i = 0; i < singleBufferSize; ++i) {
				REQUIRE(testBuffer.getSample(channel, i) == 1);
			}
		}

		//@todo find a way to test pointers difference?
	}
}

TEMPLATE_TEST_CASE("[DelayedCircularAudioBuffer] Valid ranged CircularBufferView is returned from getRangedView()", "[DelayedCircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 8;
	const size_t startOffset = 20;
	
	auto wrapper = DelayedCircularAudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, delayInSamples, startOffset);
	auto rangedBufferView = wrapper.audioBuffer.getRangedView(abl::SamplesRange{1,5});
//	REQUIRE(dynamic_cast<abl::DelayedCircularAudioBufferView<TestType>*>(rangedBufferView.get()));

	size_t i, j;
	for(size_t channel = 0; channel < channels; ++channel) {
		auto channelView = rangedBufferView[channel];
		REQUIRE(channelView.getBufferSize() == 5);
		i = startOffset + 1;
		j = 1;
		for(auto&& sample : channelView) {
			if(++i > bufferSize) i = 1;
			REQUIRE(sample == i + (channel*bufferSize));
			++j;
		}

		REQUIRE(j == 6);
	}
}

TEMPLATE_TEST_CASE("[DelayedCircularAudioBuffer] Buffer data is accessible with getSample()", "[DelayedCircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 8;
	const size_t startOffset = 20;
	
	auto wrapper = DelayedCircularAudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, delayInSamples, startOffset);

	size_t j;
	for(size_t channel = 0; channel < channels; ++channel) {
		j = startOffset;
		for(size_t i = 0; i < singleBufferSize; ++i) {
			if(++j > bufferSize) j = 1;
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == j + (channel*bufferSize));
		}
	}
}

TEMPLATE_TEST_CASE("[DelayedCircularAudioBuffer] Buffer data can be set with setSample() and addSample()", "[DelayedCircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 8;
	const size_t startOffset = 20;
	
	auto wrapper = DelayedCircularAudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, delayInSamples, startOffset);

	const size_t index = 5;
	wrapper.audioBuffer.setSample(0, index, 20);
	wrapper.audioBuffer.incrementIndex();
	REQUIRE(wrapper.audioBuffer.getSample(0, index) == 20);

	wrapper.audioBuffer.resetIndex();
	wrapper.audioBuffer.addSample(1, index, 20);
	wrapper.audioBuffer.incrementIndex();
	REQUIRE(wrapper.audioBuffer.getSample(1, index) == 54); //32 (second channel) + 1 (index 5 + startOffset is 1) + 20 + 1 (the count start from 1)
}

TEMPLATE_TEST_CASE("[DelayedCircularAudioBuffer] Index and delayInSample work correctly", "[DelayedCircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 12;
	const size_t startOffset = 20;
	
	auto wrapper = DelayedCircularAudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, delayInSamples, startOffset, {}, 4);

	auto& bufferView = wrapper.audioBuffer;
	REQUIRE(bufferView.getIndex() == 4);
	REQUIRE(bufferView.getSample(0, 0) == 5 + startOffset);
	bufferView.setSample(0, 0, 10);
	bufferView.incrementIndex(delayInSamples);
	REQUIRE(bufferView.getSample(0, 0) == 10);

	bufferView.resetIndex();
	REQUIRE(bufferView.getDelayInSamples() == delayInSamples);
	bufferView.setDelayInSamples(16);
	REQUIRE(bufferView.getDelayInSamples() == 16);
	bufferView.setSample(0, 0, 20);
	bufferView.incrementIndex(16);
	REQUIRE(bufferView.getSample(0, 0) == 20);
}

TEMPLATE_TEST_CASE("[DelayedCircularAudioBuffer] Buffer data can be copied from another buffer", "[DelayedCircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 8;
	const size_t startOffset = 20;
	
	auto wrapper = DelayedCircularAudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, delayInSamples, startOffset, 0);
	auto wrapperCopy = DelayedCircularAudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, delayInSamples, startOffset);

	for(size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == 0);
		}
	}

	wrapper.audioBuffer.copyFrom(wrapperCopy.audioBuffer, abl::SamplesRange::allSamples());
	wrapper.audioBuffer.incrementIndex();
	for(size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			const auto val = i+1+startOffset;
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == (val > bufferSize ? val - bufferSize : val) + (channel * bufferSize));
		}
	}

	const size_t rangeFrom = 2;
	const size_t rangeCount = 4;
	wrapper.audioBuffer.resetIndex();
	wrapper.audioBuffer.copyFrom(wrapperCopy.audioBuffer, abl::SamplesRange(rangeFrom, rangeCount));
	wrapper.audioBuffer.incrementIndex();
	for(size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			auto val = i+1+startOffset;
			if(i >= rangeFrom && i < rangeFrom + rangeCount) {
				val -= rangeFrom;
			}
			if(val > bufferSize) val -= bufferSize;
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == val + (channel * bufferSize));
		}
	}

	wrapper.audioBuffer.resetIndex();
	wrapper.audioBuffer.copyFrom(wrapperCopy.audioBuffer, abl::SamplesRange::allSamples(), 0.5);
	wrapper.audioBuffer.incrementIndex();
	for(size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			const auto val = i+1+startOffset;
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == TestType((val > bufferSize ? val - bufferSize : val) + (channel * bufferSize)) / 2);
		}
	}
}

TEMPLATE_TEST_CASE("[DelayedCircularAudioBuffer] Channel buffer data can be copied from another channel buffer", "[DelayedCircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 8;
	const size_t startOffset = 20;
	
	auto wrapper = DelayedCircularAudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, delayInSamples, startOffset, 0);
	auto wrapperCopy = DelayedCircularAudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, 0, startOffset);

	for(size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == 0);
		}

		wrapper.audioBuffer.resetIndex();
		wrapper.audioBuffer.copyIntoChannelFrom(wrapperCopy.audioBuffer.getChannelView(channel), channel, abl::SamplesRange::allSamples());
		wrapper.audioBuffer.incrementIndex();
		for(size_t i = 0; i < singleBufferSize; ++i) {
			const auto val = i+1+startOffset;
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == (val > bufferSize ? val - bufferSize : val) + (channel * bufferSize));
		}

		const size_t rangeFrom = 2;
		const size_t rangeCount = 4;
		wrapper.audioBuffer.resetIndex();
		wrapper.audioBuffer.copyIntoChannelFrom(wrapperCopy.audioBuffer.getChannelView(channel), channel, abl::SamplesRange(rangeFrom, rangeCount));
		wrapper.audioBuffer.incrementIndex();
		for(size_t i = 0; i < singleBufferSize; ++i) {
			auto val = i+1+startOffset;
			if(i >= rangeFrom && i < rangeFrom + rangeCount) {
				val -= rangeFrom;
			}
			if(val > bufferSize) val -= bufferSize;
			REQUIRE(wrapper.audioBuffer[channel][i] == val + (channel * bufferSize));
		}

		wrapper.audioBuffer.resetIndex();
		wrapper.audioBuffer.copyIntoChannelFrom(wrapperCopy.audioBuffer.getChannelView(channel), channel, abl::SamplesRange::allSamples(), 0.5);
		wrapper.audioBuffer.incrementIndex();
		for(size_t i = 0; i < singleBufferSize; ++i) {
			const auto val = i+1+startOffset;
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == TestType((val > bufferSize ? val - bufferSize : val) + (channel * bufferSize)) / 2);
		}
	}
}

TEMPLATE_TEST_CASE("[DelayedCircularAudioBuffer] Buffer data can be copied from another buffer with ramp", "[DelayedCircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 8;
	const size_t startOffset = 20;

	auto wrapper = DelayedCircularAudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, delayInSamples, startOffset, 0);
	auto wrapperCopy = DelayedCircularAudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, delayInSamples, startOffset, 8);

	for(size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == 0);
		}
	}

	wrapper.audioBuffer.copyWithRampFrom(wrapperCopy.audioBuffer, 0.0, 1.0, abl::SamplesRange::allSamples());
	wrapper.audioBuffer.incrementIndex();
	for(size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == i);
		}
	}

	const size_t rangeFrom = 1;
	const size_t rangeCount = 4;
	wrapper.audioBuffer.resetIndex();
	wrapper.audioBuffer.copyWithRampFrom(wrapperCopy.audioBuffer, 0.0, 1.0, abl::SamplesRange(rangeFrom,rangeCount));
	wrapper.audioBuffer.incrementIndex();
	for(size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			unsigned val = i >= rangeFrom && i < rangeFrom + rangeCount ? (i - rangeFrom) * 2 : i;
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == val);
		}
	}

	wrapper.audioBuffer.resetIndex();
	wrapper.audioBuffer.copyWithRampFrom(wrapperCopy.audioBuffer, 0.0, 0.5, abl::SamplesRange::allSamples());
	wrapper.audioBuffer.incrementIndex();
	for(size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == TestType(i) / 2);
		}
	}
}

TEMPLATE_TEST_CASE("[DelayedCircularAudioBuffer] Channel buffer data can be copied from another buffer with ramp", "[DelayedCircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 8;
	const size_t startOffset = 20;
	
	auto wrapper = DelayedCircularAudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, delayInSamples, startOffset, 0);
	auto wrapperCopy = DelayedCircularAudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, delayInSamples, startOffset, 8);

	for(size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == 0);
		}

		wrapper.audioBuffer.resetIndex();
		wrapper.audioBuffer.copyIntoChannelWithRampFrom(wrapperCopy.audioBuffer.getChannelView(channel), channel, 0.0, 1.0, abl::SamplesRange::allSamples());
		wrapper.audioBuffer.incrementIndex();
		for(size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == i);
		}

		const size_t rangeFrom = 1;
		const size_t rangeCount = 4;
		wrapper.audioBuffer.resetIndex();
		wrapper.audioBuffer.copyIntoChannelWithRampFrom(wrapperCopy.audioBuffer.getChannelView(channel), channel, 0.0, 1.0, abl::SamplesRange(rangeFrom, rangeCount));
		wrapper.audioBuffer.incrementIndex();
		for(size_t i = 0; i < singleBufferSize; ++i) {
			auto val = i >= rangeFrom && i < rangeFrom + rangeCount ? (i - rangeFrom) * 2 : i;
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == val);
		}

		wrapper.audioBuffer.resetIndex();
		wrapper.audioBuffer.copyIntoChannelWithRampFrom(wrapperCopy.audioBuffer.getChannelView(channel), channel, 0.0, 0.5, abl::SamplesRange::allSamples());
		wrapper.audioBuffer.incrementIndex();
		for(size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == TestType(i)/2);
		}
	}
}

TEMPLATE_TEST_CASE("[DelayedCircularAudioBuffer] Buffer data can be added from another buffer", "[DelayedCircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 8;
	const size_t startOffset = 20;
	
	auto wrapper = DelayedCircularAudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, delayInSamples, startOffset, 2);
	auto wrapperCopy = DelayedCircularAudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, delayInSamples, startOffset);

	for(size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == 2);
		}
	}

	wrapper.audioBuffer.addFrom(wrapperCopy.audioBuffer, abl::SamplesRange::allSamples());
	wrapper.audioBuffer.incrementIndex();
	for(size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			const unsigned val = i+1+startOffset;
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == (val > bufferSize ? val - bufferSize : val) + (channel * bufferSize) + 2);
		}
	}

	wrapper.audioBuffer.resetIndex();
	wrapper.audioBuffer.addFrom(wrapperCopy.audioBuffer, abl::SamplesRange::allSamples(), 0.5);
	wrapper.audioBuffer.incrementIndex();
	for(size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			unsigned val = i+1+startOffset;
			if(val > bufferSize) val -= bufferSize;
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == (TestType(val) / 2) + val + 2 + (channel * bufferSize * 1.5));
		}
	}

	const size_t rangeFrom = 3;
	const size_t rangeCount = 4;
	auto rangeWrapper = DelayedCircularAudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, delayInSamples, startOffset, 2);
	rangeWrapper.audioBuffer.addFrom(wrapperCopy.audioBuffer, abl::SamplesRange(rangeFrom, rangeCount));
	rangeWrapper.audioBuffer.incrementIndex();
	for(size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			unsigned val = 0;
			if (i >= rangeFrom && i < rangeFrom + rangeCount) {
				val = i + 1 + startOffset - rangeFrom;
				if (val > bufferSize) val -= bufferSize;
				val += channel * bufferSize;
			}
			REQUIRE(rangeWrapper.audioBuffer.getSample(channel, i) == val + 2);
		}
	}
}

TEMPLATE_TEST_CASE("[DelayedCircularAudioBuffer] Channel buffer data can be added from another channel buffer", "[DelayedCircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 8;
	const size_t startOffset = 20;
	
	auto wrapper = DelayedCircularAudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, delayInSamples, startOffset, 2);
	auto wrapperCopy = DelayedCircularAudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, delayInSamples, startOffset);
	auto rangeWrapper = DelayedCircularAudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, delayInSamples, startOffset, 2);

	for(size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == 2);
		}

		wrapper.audioBuffer.resetIndex();
		wrapper.audioBuffer.addIntoChannelFrom(wrapperCopy.audioBuffer.getChannelView(channel), channel, abl::SamplesRange::allSamples());
		wrapper.audioBuffer.incrementIndex();
		for(size_t i = 0; i < singleBufferSize; ++i) {
			const unsigned val = i+1+startOffset;
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == (val > bufferSize ? val - bufferSize : val) + (channel * bufferSize) + 2);
		}

		wrapper.audioBuffer.resetIndex();
		wrapper.audioBuffer.addIntoChannelFrom(wrapperCopy.audioBuffer.getChannelView(channel), channel, abl::SamplesRange::allSamples(), 0.5);
		wrapper.audioBuffer.incrementIndex();
		for(size_t i = 0; i < singleBufferSize; ++i) {
			unsigned val = i+1+startOffset;
			if(val > bufferSize) val -= bufferSize;
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == (TestType(val) / 2) + val + 2 + (channel * bufferSize * 1.5));
		}

		const size_t rangeFrom = 3;
		const size_t rangeCount = 4;
		rangeWrapper.audioBuffer.resetIndex();
		rangeWrapper.audioBuffer.addIntoChannelFrom(wrapperCopy.audioBuffer.getChannelView(channel), channel, abl::SamplesRange(rangeFrom, rangeCount));
		rangeWrapper.audioBuffer.incrementIndex();
		for(size_t i = 0; i < singleBufferSize; ++i) {
			unsigned val = 0;
			if(i >= rangeFrom && i < rangeFrom + rangeCount) {
				val = i + 1 + startOffset - rangeFrom;
				if(val > bufferSize) val -= bufferSize;
				val += channel * bufferSize;
			}
			REQUIRE(rangeWrapper.audioBuffer.getSample(channel, i) == val + 2);
		}
	}
}

TEMPLATE_TEST_CASE("[DelayedCircularAudioBuffer] Buffer data can be added from another buffer with ramp", "[DelayedCircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 8;
	const size_t startOffset = 20;
	
	auto wrapper = DelayedCircularAudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, delayInSamples, startOffset, 3);
	auto wrapperCopy = DelayedCircularAudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, delayInSamples, startOffset, 8);

	for(size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == 3);
		}
	}

	wrapper.audioBuffer.addWithRampFrom(wrapperCopy.audioBuffer, 0.0, 1.0, abl::SamplesRange::allSamples());
	wrapper.audioBuffer.incrementIndex();
	for(size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == i + 3);
		}
	}

	wrapper.audioBuffer.resetIndex();
	wrapper.audioBuffer.addWithRampFrom(wrapperCopy.audioBuffer, 0.0, 0.5, abl::SamplesRange::allSamples());
	wrapper.audioBuffer.incrementIndex();
	for(size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == (TestType(i) / 2) + i + 3);
		}
	}

	const size_t rangeFrom = 2;
	const size_t rangeCount = 4;
	auto rangeWrapper = DelayedCircularAudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, delayInSamples, startOffset, 3);
	rangeWrapper.audioBuffer.addWithRampFrom(wrapperCopy.audioBuffer, 0.0, 0.5, abl::SamplesRange(rangeFrom, rangeCount));
	rangeWrapper.audioBuffer.incrementIndex();
	for(size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(rangeWrapper.audioBuffer.getSample(channel, i) == (i >= rangeFrom && i < rangeFrom + rangeCount ? i - rangeFrom : 0) + 3);
		}
	}
}

TEMPLATE_TEST_CASE("[DelayedCircularAudioBuffer] Channel buffer data can be added from another buffer with ramp", "[DelayedCircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 8;
	const size_t startOffset = 20;
	
	auto wrapper = DelayedCircularAudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, delayInSamples, startOffset, 3);
	auto wrapperCopy = DelayedCircularAudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, 0, startOffset, 8);
	auto rangeWrapper = DelayedCircularAudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, delayInSamples, startOffset, 3);

	for(size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == 3);
		}

		wrapper.audioBuffer.resetIndex();
		wrapper.audioBuffer.addIntoChannelWithRampFrom(wrapperCopy.audioBuffer.getChannelView(channel), channel, 0.0, 1.0, abl::SamplesRange::allSamples());
		wrapper.audioBuffer.incrementIndex();
		for(size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == i + 3);
		}

		wrapper.audioBuffer.resetIndex();
		wrapper.audioBuffer.addIntoChannelWithRampFrom(wrapperCopy.audioBuffer.getChannelView(channel), channel, 0.0, 0.5, abl::SamplesRange::allSamples());
		wrapper.audioBuffer.incrementIndex();
		for(size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == (TestType(i) / 2) + i + 3);
		}

		const size_t rangeFrom = 2;
		const size_t rangeCount = 4;
		rangeWrapper.audioBuffer.resetIndex();
		rangeWrapper.audioBuffer.addIntoChannelWithRampFrom(wrapperCopy.audioBuffer.getChannelView(channel), channel, 0.0, 0.5, abl::SamplesRange(rangeFrom, rangeCount));
		rangeWrapper.audioBuffer.incrementIndex();
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(rangeWrapper.audioBuffer.getSample(channel, i) == (i >= rangeFrom && i < rangeFrom + rangeCount ? i - rangeFrom : 0) + 3);
		}
	}
}

TEMPLATE_TEST_CASE("[DelayedCircularAudioBuffer] Buffer data is scaled with applyGain()", "[DelayedCircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 8;
	const size_t startOffset = 20;
	
	auto wrapper = DelayedCircularAudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, delayInSamples, startOffset);

	for(size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			unsigned val = i + 1 + startOffset;
			if(val > bufferSize) val -= bufferSize;
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == val + (channel * bufferSize));
		}
	}

	wrapper.audioBuffer.applyGain(0.5, abl::SamplesRange::allSamples());
	wrapper.audioBuffer.incrementIndex();
	for(size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			unsigned val = i + 1 + startOffset + delayInSamples;
			if(val > bufferSize) val -= bufferSize;
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == TestType(val + (channel * bufferSize)) / 2);
		}
	}

	wrapper.audioBuffer.resetIndex();
	wrapper.audioBuffer.applyGain(3.0, abl::SamplesRange::allSamples());
	wrapper.audioBuffer.incrementIndex();
	for(size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			unsigned val = i + 1 + startOffset + delayInSamples;
			if(val > bufferSize) val -= bufferSize;
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == (TestType(val + (channel * bufferSize)) / 2) * 3);
		}
	}

	const size_t rangeFrom = 5;
	const size_t rangeCount = 3;
	auto rangeWrapper = DelayedCircularAudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, delayInSamples, startOffset);
	rangeWrapper.audioBuffer.applyGain(2.0, abl::SamplesRange(rangeFrom, rangeCount));
	rangeWrapper.audioBuffer.incrementIndex();
	for(size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			unsigned val = i + 1 + startOffset + delayInSamples;
			if (val > bufferSize) val -= bufferSize;
			val += channel * bufferSize;
			REQUIRE(rangeWrapper.audioBuffer.getSample(channel, i) == (i >= rangeFrom && i < rangeFrom + rangeCount ? val * 2 : val));
		}
	}
}

TEMPLATE_TEST_CASE("[DelayedCircularAudioBuffer] Channel buffer data is scaled with applyGainToChannel()", "[DelayedCircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 8;
	const size_t startOffset = 20;
	
	auto wrapper = DelayedCircularAudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, delayInSamples, startOffset);
	auto rangeWrapper = DelayedCircularAudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, delayInSamples, startOffset);

	for(size_t channel = 0; channel < channels; ++channel) {
		wrapper.audioBuffer.resetIndex();
		for (size_t i = 0; i < singleBufferSize; ++i) {
			unsigned val = i + 1 + startOffset;
			if(val > bufferSize) val -= bufferSize;
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == val + (channel * bufferSize));
		}

		wrapper.audioBuffer.applyGainToChannel(0.5, channel, abl::SamplesRange::allSamples());
		wrapper.audioBuffer.incrementIndex();
		for (size_t i = 0; i < singleBufferSize; ++i) {
			unsigned val = i + 1 + startOffset + delayInSamples;
			if(val > bufferSize) val -= bufferSize;
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == TestType(val + (channel * bufferSize)) / 2);
		}

		wrapper.audioBuffer.resetIndex();
		wrapper.audioBuffer.applyGainToChannel(3.0, channel, abl::SamplesRange::allSamples());
		wrapper.audioBuffer.incrementIndex();
		for (size_t i = 0; i < singleBufferSize; ++i) {
			unsigned val = i + 1 + startOffset + delayInSamples;
			if(val > bufferSize) val -= bufferSize;
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == (TestType(val + (channel * bufferSize)) / 2) * 3);
		}

		const size_t rangeFrom = 5;
		const size_t rangeCount = 3;
		rangeWrapper.audioBuffer.resetIndex();
		rangeWrapper.audioBuffer.applyGainToChannel(2.0, channel, abl::SamplesRange(rangeFrom, rangeCount));
		rangeWrapper.audioBuffer.incrementIndex();
		for (size_t i = 0; i < singleBufferSize; ++i) {
			unsigned val = i + 1 + startOffset + delayInSamples;
			if (val > bufferSize) val -= bufferSize;
			val += channel * bufferSize;
			REQUIRE(rangeWrapper.audioBuffer.getSample(channel, i) == (i >= rangeFrom && i < rangeFrom + rangeCount ? val * 2 : val));
		}
	}
}

TEMPLATE_TEST_CASE("[DelayedCircularAudioBuffer] Buffer data is scaled with ramp using applyGainRamp()", "[DelayedCircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 8;
	const size_t startOffset = 20;
	
	auto wrapper = DelayedCircularAudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, delayInSamples, startOffset, 10);

	for(size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == 10);
		}
	}

	wrapper.audioBuffer.applyGainRamp(0.0, 1.0, abl::SamplesRange::allSamples());
	wrapper.audioBuffer.incrementIndex();
	for(size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == TestType(i) * 10 / singleBufferSize);
		}
	}

	wrapper.audioBuffer.resetIndex();
	wrapper.audioBuffer.applyGainRamp(0.5, 0.0, abl::SamplesRange::allSamples());
	wrapper.audioBuffer.incrementIndex();
	for(size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == (TestType(i) * 10 / singleBufferSize) * (singleBufferSize - i) / (singleBufferSize * 2));
		}
	}

	const size_t rangeFrom = 2;
	const size_t rangeCount = 4;
	auto rangeWrapper = DelayedCircularAudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, delayInSamples, startOffset, 10);
	rangeWrapper.audioBuffer.applyGainRamp(0.0, 1.0, abl::SamplesRange(rangeFrom, rangeCount));
	rangeWrapper.audioBuffer.incrementIndex();
	for(size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(rangeWrapper.audioBuffer.getSample(channel, i) == (i >= rangeFrom && i < rangeFrom + rangeCount ? TestType(i - rangeFrom) * 20 / singleBufferSize : 10));
		}
	}
}

TEMPLATE_TEST_CASE("[DelayedCircularAudioBuffer] Channel buffer data is scaled with applyGainRampToChannel()", "[DelayedCircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 8;
	const size_t startOffset = 20;
	
	auto wrapper = DelayedCircularAudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, delayInSamples, startOffset, 10);
	auto rangeWrapper = DelayedCircularAudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, delayInSamples, startOffset, 10);

	for(size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == 10);
		}

		wrapper.audioBuffer.resetIndex();
		wrapper.audioBuffer.applyGainRampToChannel(0.0, 1.0, channel, abl::SamplesRange::allSamples());
		wrapper.audioBuffer.incrementIndex();
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == TestType(i) * 10 / singleBufferSize);
		}

		wrapper.audioBuffer.resetIndex();
		wrapper.audioBuffer.applyGainRampToChannel(0.5, 0.0, channel, abl::SamplesRange::allSamples());
		wrapper.audioBuffer.incrementIndex();
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == (TestType(i) * 10 / singleBufferSize) * (singleBufferSize - i) / (singleBufferSize * 2));
		}

		const size_t rangeFrom = 2;
		const size_t rangeCount = 4;
		rangeWrapper.audioBuffer.resetIndex();
		rangeWrapper.audioBuffer.applyGainRampToChannel(0.0, 1.0, channel, abl::SamplesRange(rangeFrom, rangeCount));
		rangeWrapper.audioBuffer.incrementIndex();
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(rangeWrapper.audioBuffer.getSample(channel, i) == (i >= rangeFrom && i < rangeFrom + rangeCount ? TestType(i - rangeFrom) * 20 / singleBufferSize : 10));
		}
	}
}

TEMPLATE_TEST_CASE("[DelayedCircularAudioBuffer] Buffer data is cleared with clear()", "[DelayedCircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 8;
	const size_t startOffset = 20;
	
	auto wrapper = DelayedCircularAudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, delayInSamples, startOffset);

	wrapper.audioBuffer.clear(abl::SamplesRange::allSamples());

	wrapper.audioBuffer.incrementIndex();
	for(size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == 0);
		}
	}

	const size_t rangeFrom = 5;
	const size_t rangeCount = 3;
	auto rangeWrapper = DelayedCircularAudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, delayInSamples, startOffset, 4);
	rangeWrapper.audioBuffer.clear(abl::SamplesRange(rangeFrom, rangeCount));
	rangeWrapper.audioBuffer.incrementIndex();
	for(size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(rangeWrapper.audioBuffer.getSample(channel, i) == (i >= rangeFrom && i < rangeFrom + rangeCount ? 0 : 4));
		}
	}
}

TEMPLATE_TEST_CASE("[DelayedCircularAudioBuffer] Channel Buffer data is cleared with clearChannel()", "[DelayedCircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 8;
	const size_t startOffset = 20;
	
	auto wrapper = DelayedCircularAudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, delayInSamples, startOffset);
	auto rangeWrapper = DelayedCircularAudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, delayInSamples, startOffset);

	wrapper.audioBuffer.clearChannel(1, abl::SamplesRange::allSamples());
	wrapper.audioBuffer.incrementIndex();

	for(size_t i = 0; i < singleBufferSize; ++i) {
		const unsigned val = i+1+startOffset+delayInSamples;
		REQUIRE(wrapper.audioBuffer.getSample(0, i) == (val > bufferSize ? val - bufferSize : val));
	}

	for(size_t i = 0; i < singleBufferSize; ++i) {
		REQUIRE(wrapper.audioBuffer.getSample(1, i) == 0);
	}

	const size_t rangeFrom = 5;
	const size_t rangeCount = 3;
	rangeWrapper.audioBuffer.clearChannel(0, abl::SamplesRange(rangeFrom, rangeCount));
	rangeWrapper.audioBuffer.incrementIndex();

	for(size_t i = 0; i < singleBufferSize; ++i) {
		unsigned val = i+1+startOffset+delayInSamples;
		if(val > bufferSize) val -= bufferSize;
		REQUIRE(rangeWrapper.audioBuffer.getSample(0, i) == (i >= rangeFrom && i < rangeFrom + rangeCount ? 0 : val));
	}

	for(size_t i = 0; i < singleBufferSize; ++i) {
		unsigned val = i+1+startOffset+delayInSamples;
		if(val > bufferSize) val -= bufferSize;
		REQUIRE(rangeWrapper.audioBuffer.getSample(1, i) == val + bufferSize);
	}
}

TEMPLATE_TEST_CASE("[DelayedCircularAudioBuffer] Buffer data is reversed with reverse()", "[DelayedCircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 8;
	const size_t startOffset = 20;
	
	auto wrapper = DelayedCircularAudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, delayInSamples, startOffset);
	wrapper.audioBuffer.incrementIndex();

	const auto splitIndex = bufferSize - startOffset - delayInSamples;
	std::array<int, channels> valuesBeforeSplit{};
	std::array<int, channels> lastValues{};
	for(size_t channel = 0; channel < channels; ++channel) {
		valuesBeforeSplit[channel] = wrapper.audioBuffer.getSample(channel, splitIndex - 1);
		lastValues[channel] = wrapper.audioBuffer.getSample(channel, singleBufferSize - 1);
	}
	wrapper.audioBuffer.resetIndex();
	wrapper.audioBuffer.reverse(abl::SamplesRange::allSamples());

	wrapper.audioBuffer.incrementIndex();
	for(size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) ==  (i >= splitIndex ?  valuesBeforeSplit[channel] - i + splitIndex  : lastValues[channel] - i));
		}
	}

	const auto rangeFrom = 2;
	const auto rangeCount = 4;
	auto rangeWrapper = DelayedCircularAudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, delayInSamples, startOffset);
	rangeWrapper.audioBuffer.incrementIndex();
	std::array<int, channels> rangeLastValues{};
	for(size_t channel = 0; channel < channels; ++channel) {
		rangeLastValues[channel] = rangeWrapper.audioBuffer.getSample(channel, rangeFrom + rangeCount - 1);
	}
	rangeWrapper.audioBuffer.resetIndex();
	rangeWrapper.audioBuffer.reverse(abl::SamplesRange(rangeFrom, rangeCount));
	rangeWrapper.audioBuffer.incrementIndex();

	for(size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			if (i >= rangeFrom && i < rangeFrom + rangeCount) {
				REQUIRE(rangeWrapper.audioBuffer.getSample(channel, i) == (i >= splitIndex ? valuesBeforeSplit[channel] - i + splitIndex : rangeLastValues[channel] - i + rangeFrom));
			} else {
				unsigned val = i+1+startOffset+delayInSamples;
				REQUIRE(rangeWrapper.audioBuffer.getSample(channel, i) == (val > bufferSize ? val - bufferSize : val) + (bufferSize * channel));
			}
		}
	}
}

TEMPLATE_TEST_CASE("[DelayedCircularAudioBuffer] Channel Buffer data is reversed with reverse()", "[DelayedCircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 8;
	const size_t startOffset = 20;
	
	auto wrapper = DelayedCircularAudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, delayInSamples, startOffset);
	wrapper.audioBuffer.incrementIndex();

	auto splitIndex = bufferSize - startOffset - delayInSamples;
	std::array<int, channels> valuesBeforeSplit{};
	std::array<int, channels> lastValues{};
	for(size_t channel = 0; channel < channels; ++channel) {
		valuesBeforeSplit[channel] = wrapper.audioBuffer.getSample(channel, splitIndex - 1);
		lastValues[channel] = wrapper.audioBuffer.getSample(channel, singleBufferSize - 1);
	}

	wrapper.audioBuffer.resetIndex();
	wrapper.audioBuffer.reverseChannel(1, abl::SamplesRange::allSamples());

	wrapper.audioBuffer.incrementIndex();
	for(size_t i = 0; i < singleBufferSize; ++i) {
		const unsigned val = i+1+startOffset+delayInSamples;
		REQUIRE(wrapper.audioBuffer.getSample(0, i) == (val > bufferSize ? val - bufferSize : val));
	}

	for(size_t i = 0; i < singleBufferSize; ++i) {
		REQUIRE(wrapper.audioBuffer.getSample(1, i) == (i >= splitIndex ?  valuesBeforeSplit[1] - i + splitIndex  : lastValues[1] - i));
	}

	const auto rangeFrom = 2;
	const auto rangeCount = 4;
	auto rangeWrapper = DelayedCircularAudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, delayInSamples, startOffset);
	rangeWrapper.audioBuffer.incrementIndex();
	std::array<int, channels> rangeLastValues{};
	for(size_t channel = 0; channel < channels; ++channel) {
		rangeLastValues[channel] = rangeWrapper.audioBuffer.getSample(channel, rangeFrom + rangeCount - 1);
	}
	rangeWrapper.audioBuffer.resetIndex();
	rangeWrapper.audioBuffer.reverseChannel(0, abl::SamplesRange(rangeFrom, rangeCount));
	rangeWrapper.audioBuffer.incrementIndex();

	for (size_t i = 0; i < singleBufferSize; ++i) {
		if (i >= rangeFrom && i < rangeFrom + rangeCount) {
			REQUIRE(rangeWrapper.audioBuffer.getSample(0, i) == (i >= splitIndex ? valuesBeforeSplit[0] - i + splitIndex : rangeLastValues[0] - i + rangeFrom));
		} else {
			const unsigned val = i+1+startOffset+delayInSamples;
			REQUIRE(rangeWrapper.audioBuffer.getSample(0, i) == (val > bufferSize ? val - bufferSize : val) );
		}
	}

	for(size_t i = 0; i < singleBufferSize; ++i) {
		const unsigned val = i+1+startOffset+delayInSamples;
		REQUIRE(rangeWrapper.audioBuffer.getSample(1, i) == (val > bufferSize ? val - bufferSize : val) + bufferSize);
	}
}

TEMPLATE_TEST_CASE("[DelayedCircularAudioBuffer] View report correct higher peak", "[DelayedCircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 8;
	const size_t startOffset = 20;
	
	auto wrapper = DelayedCircularAudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, delayInSamples, startOffset);
	wrapper.audioBuffer.incrementIndex();

	REQUIRE(wrapper.audioBuffer.getHigherPeak(abl::SamplesRange::allSamples()) == 64);
	REQUIRE(wrapper.audioBuffer.getHigherPeak(abl::SamplesRange(3,3)) == 64);
	REQUIRE(wrapper.audioBuffer.getHigherPeak(abl::SamplesRange(5,2)) == 35);
}

TEMPLATE_TEST_CASE("[DelayedCircularAudioBuffer] View report correct higher peak for channels", "[DelayedCircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 8;
	const size_t startOffset = 20;
	
	auto wrapper = DelayedCircularAudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, delayInSamples, startOffset);
	wrapper.audioBuffer.incrementIndex();

	for(size_t channel = 0; channel < channels; ++channel) {
		REQUIRE(wrapper.audioBuffer.getHigherPeakForChannel(channel, abl::SamplesRange::allSamples()) == bufferSize * (channel + 1));
		REQUIRE(wrapper.audioBuffer.getHigherPeakForChannel(channel, abl::SamplesRange(3, 3)) == bufferSize * (channel + 1));
		REQUIRE(wrapper.audioBuffer.getHigherPeakForChannel(channel, abl::SamplesRange(5, 2)) == 3 + (bufferSize * channel));
	}
}

TEMPLATE_TEST_CASE("[DelayedCircularAudioBuffer] View report correct rms level for channels", "[DelayedCircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 8;
	const size_t startOffset = 20;
	
	auto wrapper = DelayedCircularAudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, delayInSamples, startOffset);
	wrapper.audioBuffer.incrementIndex();

	REQUIRE(wrapper.audioBuffer.getRMSLevelForChannel(0, abl::SamplesRange::allSamples()) == TestType(16.5));
	REQUIRE(wrapper.audioBuffer.getRMSLevelForChannel(1, abl::SamplesRange::allSamples()) == TestType(48.5));
	REQUIRE(wrapper.audioBuffer.getRMSLevelForChannel(0, abl::SamplesRange(3,4)) == TestType(9.5));
	REQUIRE(wrapper.audioBuffer.getRMSLevelForChannel(1, abl::SamplesRange(4,2)) == TestType(33.5)); //33 + 34 / 2
}

TEMPLATE_TEST_CASE("[DelayedCircularAudioBuffer] View report correct buffer size and channel count", "[DelayedCircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 8;
	const size_t startOffset = 20;
	
	auto wrapper = DelayedCircularAudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, delayInSamples, startOffset);
	wrapper.audioBuffer.incrementIndex();

	REQUIRE(wrapper.audioBuffer.getBufferSize() == singleBufferSize);
	REQUIRE(wrapper.audioBuffer.getChannelsCount() == channels);
}

TEMPLATE_TEST_CASE("[DelayedCircularAudioBuffer] Channels are mapped correctly", "[DelayedCircularAudioBuffer]", int, double) {
	const size_t channels = 4;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 8;
	const size_t startOffset = 20;
	
	std::vector<size_t> channelsMapping{3, 1, 2, 0, 1};
	auto wrapper = DelayedCircularAudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, delayInSamples, startOffset, channelsMapping);

	REQUIRE(wrapper.audioBuffer.getChannelsCount() == 5);
	size_t channel = 0;
	for(auto mappedChannel : channelsMapping) {
		for(size_t i = 0; i < singleBufferSize; ++i) {
			const auto val = i+1+startOffset;
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == (val > bufferSize ? val - bufferSize : val) + (mappedChannel*bufferSize));
		}
		++channel;
	}

	channelsMapping = {0, 2};
	wrapper.audioBuffer.setChannelsMapping(channelsMapping);
	REQUIRE(wrapper.audioBuffer.getChannelsCount() == 2);
	channel = 0;
	for(auto mappedChannel : channelsMapping) {
		for(size_t i = 0; i < singleBufferSize; ++i) {
			const auto val = i+1+startOffset;
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == (val > bufferSize ? val - bufferSize : val) + (mappedChannel*bufferSize));
		}
		++channel;
	}

	wrapper.audioBuffer.createSequentialChannelsMapping(1, 3);
	REQUIRE(wrapper.audioBuffer.getChannelsCount() == 3);
	auto sequentialChannelsMapping = wrapper.audioBuffer.getChannelsMapping();
	for(size_t i = 0; i < 3; ++i) {
		REQUIRE(sequentialChannelsMapping[i] == i + 1);
	}

	channel = 0;
	for(auto mappedChannel : sequentialChannelsMapping) {
		for(size_t i = 0; i < singleBufferSize; ++i) {
			const auto val = i+1+startOffset;
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == (val > bufferSize ? val - bufferSize : val) + (mappedChannel*bufferSize));
		}
		++channel;
	}
}


//test channel out-of-bound assert?