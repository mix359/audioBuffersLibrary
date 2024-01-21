// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "../buffers/CircularAudioBuffer.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_template_test_macros.hpp>

template<typename T>
struct CircularAudioBufferWrapper {
	abl::CircularAudioBuffer<T> audioBuffer;

	static CircularAudioBufferWrapper createWithIncrementalNumbers(size_t channels, size_t bufferSize, size_t singleBufferSize, size_t startOffset, const std::vector<size_t> &channelsMapping = {}, size_t startReadIndex = 0, size_t startWriteIndex = 0) {
		auto data = new T *[channels];
		size_t startValue = 1;
		for (size_t channel = 0; channel < channels; ++channel) {
			data[channel] = new T[bufferSize];
			std::iota(data[channel], data[channel] + bufferSize, startValue);
			startValue += bufferSize;
		}

		auto wrapper = CircularAudioBufferWrapper{data, channels, bufferSize, singleBufferSize, startOffset, channelsMapping, startReadIndex, startWriteIndex};
		for (size_t channel = 0; channel < channels; ++channel) {
			delete[] data[channel];
		}
		delete[] data;

		return wrapper;
	}

	static CircularAudioBufferWrapper createWithFixedValue(size_t channels, size_t bufferSize, size_t singleBufferSize, size_t startOffset, T fixedValue, const std::vector<size_t> &channelsMapping = {}, size_t startReadIndex = 0, size_t startWriteIndex = 0) {
		auto data = new T *[channels];
		for (size_t channel = 0; channel < channels; ++channel) {
			data[channel] = new T[bufferSize];
			std::fill(data[channel], data[channel] + bufferSize, fixedValue);
		}

		auto wrapper = CircularAudioBufferWrapper{data, channels, bufferSize, singleBufferSize, startOffset, channelsMapping, startReadIndex, startWriteIndex};
		for (size_t channel = 0; channel < channels; ++channel) {
			delete[] data[channel];
		}
		delete[] data;

		return wrapper;
	}

	CircularAudioBufferWrapper(T **newData, size_t channels, size_t bufferSize, size_t singleBufferSize, size_t bufferStartOffset, const std::vector<size_t> &channelsMapping = {}, size_t startReadIndex = 0, size_t startWriteIndex = 0)
		: audioBuffer{abl::CircularAudioBuffer{newData, channels, bufferSize, singleBufferSize, bufferStartOffset, channelsMapping, startReadIndex, startWriteIndex}} {}

};

//@todo test other contructor?

TEST_CASE("[CircularAudioBuffer] View report correct empty state", "[CircularAudioBuffer]") {
	auto noValueWrapper = CircularAudioBufferWrapper<int>::createWithFixedValue(2, 16, 8, 0, 0);
	auto noChannelsWrapper = CircularAudioBufferWrapper<int>::createWithFixedValue(0, 0, 0, 0, 0);
	auto noBufferSizeWrapper = CircularAudioBufferWrapper<double>::createWithFixedValue(1, 0, 8, 0, 0);
	auto noSingleBufferSizeWrapper = CircularAudioBufferWrapper<double>::createWithFixedValue(1, 16, 0, 0, 0);

	REQUIRE_FALSE(noValueWrapper.audioBuffer.isEmpty());
	REQUIRE(noChannelsWrapper.audioBuffer.isEmpty());
	REQUIRE(noBufferSizeWrapper.audioBuffer.isEmpty());
	REQUIRE(noSingleBufferSizeWrapper.audioBuffer.isEmpty());
}


TEMPLATE_TEST_CASE("[CircularAudioBuffer] Buffer data is iterable", "[CircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t startOffset = 28;
	auto wrapper = CircularAudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, startOffset);

	size_t i, j = 0;
	for (auto &&channelView: wrapper.audioBuffer) {
//		REQUIRE(dynamic_cast<abl::OffsettedReadCircularAudioBufferChannelView<TestType> *>(channelView.get()));
		i = startOffset;
		for (auto &&sample: channelView) {
			if (++i > bufferSize) i = 1;
			REQUIRE(sample == i + (j * bufferSize));
		}
		++j;
	}
	REQUIRE(j == channels);
}

TEMPLATE_TEST_CASE("[CircularAudioBuffer] CircularBufferChannelView is returned accessing a channel with the operator[] and getChannelView()", "[CircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t startOffset = 28;
	auto wrapper = CircularAudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, startOffset);

	size_t i;
	for (size_t channel = 0; channel < channels; ++channel) {
		auto channelView = wrapper.audioBuffer[channel];
		REQUIRE(channelView.getBufferSize() == singleBufferSize);
//		REQUIRE(dynamic_cast<abl::OffsettedReadCircularAudioBufferChannelView<TestType> *>(channelView.get()));
		i = startOffset;
		for (auto &&sample: channelView) {
			if (++i > bufferSize) i = 1;
			REQUIRE(sample == i + (channel * bufferSize));
		}

		auto channelView2 = wrapper.audioBuffer.getChannelView(channel);
		REQUIRE(channelView2.getBufferSize() == singleBufferSize);
//		REQUIRE(dynamic_cast<abl::OffsettedReadCircularAudioBufferChannelView<TestType> *>(channelView2.get()));
		i = startOffset;
		for (auto &&sample: channelView2) {
			if (++i > bufferSize) i = 1;
			REQUIRE(sample == i + (channel * bufferSize));
		}
	}
}

TEMPLATE_TEST_CASE("[CircularAudioBuffer] Valid ranged CircularBufferChannelView is returned accessing a channel passing a sampleRange to getChannelView()", "[CircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t startOffset = 28;
	auto wrapper = CircularAudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, startOffset);
	abl::SamplesRange samplesRange{2, 4};

	size_t i, j;
	for (size_t channel = 0; channel < channels; ++channel) {
		auto channelView = wrapper.audioBuffer.getChannelView(channel, samplesRange);
		REQUIRE(channelView.getBufferSize() == 4);
		j = 2;
		i = startOffset + 2;
		for (auto &&sample: channelView) {
			if (++i > bufferSize) i = 1;
			REQUIRE(sample == i + (channel * bufferSize));
			++j;
		}

		REQUIRE(j == 6);
	}
}

TEMPLATE_TEST_CASE("[CircularAudioBuffer] Buffer allocate the right memory block, can be copied and moved", "[CircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 8;
	const size_t singleBufferSize = 4;
	const size_t startOffset = 2;
	auto wrapper = CircularAudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, startOffset, 1);

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == 1);
		}
	}

	abl::CircularAudioBuffer<TestType> copyBuffer{wrapper.audioBuffer};
	SECTION("copy constructor") {
		for (size_t channel = 0; channel < channels; ++channel) {
			for (size_t i = 0; i < singleBufferSize; ++i) {
				REQUIRE(copyBuffer.getSample(channel, i) == 1);
			}
		}
	}

	abl::CircularAudioBuffer<TestType> copyBuffer2{0, 0};
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
		abl::CircularAudioBuffer<TestType> moveBuffer{std::move(copyBuffer)};

		for (size_t channel = 0; channel < channels; ++channel) {
			for (size_t i = 0; i < singleBufferSize; ++i) {
				REQUIRE(moveBuffer.getSample(channel, i) == 1);
			}
		}

		REQUIRE(copyBuffer.isEmpty());
	}

	SECTION("move assignment") {
		REQUIRE(!copyBuffer2.isEmpty());
		abl::CircularAudioBuffer<TestType> moveBuffer2{0, 0};

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

TEMPLATE_TEST_CASE("[CircularAudioBuffer] Buffer can be resized", "[CircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 8;
	const size_t singleBufferSize = 4;
	const size_t startOffset = 4;
	auto wrapper = CircularAudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, startOffset, 1);

	abl::CircularAudioBuffer<TestType> &audioBuffer = wrapper.audioBuffer;

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
		testBuffer.incrementWriteIndex(10);
		testBuffer.incrementReadIndex(10);
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
		testBuffer.incrementWriteIndex(10);
		testBuffer.incrementReadIndex(10);
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
		testBuffer.incrementWriteIndex(10);
		testBuffer.incrementReadIndex(10);
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
		testBuffer.incrementWriteIndex(10);
		testBuffer.incrementReadIndex(10);
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
		testBuffer.incrementWriteIndex(10);
		testBuffer.incrementReadIndex(10);
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
		testBuffer.incrementWriteIndex(10);
		testBuffer.incrementReadIndex(10);
		for (size_t channel = 0; channel < testChannelsCount; ++channel) {
			for (size_t i = singleBufferSize; i > 0; --i) {
				testBuffer.setSample(channel, i - 1, i - 1 + (channel * singleBufferSize));
			}
		}

		for (size_t channel = 0; channel < testChannelsCount; ++channel) {
			for (size_t i = 0; i < singleBufferSize; ++i) {
				REQUIRE(testBuffer.getSample(channel, i) == i + (channel * singleBufferSize));
			}

			//@todo find a way to test pointers difference?
		}

		testBuffer.incrementWriteIndex();
		testBuffer.incrementReadIndex();
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

TEMPLATE_TEST_CASE("[CircularAudioBuffer] Valid ranged CircularBufferView is returned from getRangedView()", "[CircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t startOffset = 28;
	auto wrapper = CircularAudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, startOffset);
	auto rangedBufferView = wrapper.audioBuffer.getRangedView(abl::SamplesRange{1, 5});
//	REQUIRE(dynamic_cast<abl::CircularAudioBufferView<TestType> *>(rangedBufferView.get()));

	size_t i, j;
	for (size_t channel = 0; channel < channels; ++channel) {
		auto channelView = rangedBufferView[channel];
		REQUIRE(channelView.getBufferSize() == 5);
		i = startOffset + 1;
		j = 1;
		for (auto &&sample: channelView) {
			if (++i > bufferSize) i = 1;
			REQUIRE(sample == i + (channel * bufferSize));
			++j;
		}

		REQUIRE(j == 6);
	}
}

TEMPLATE_TEST_CASE("[CircularAudioBuffer] Buffer data is accessible with getSample()", "[CircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t startOffset = 28;
	auto wrapper = CircularAudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, startOffset);

	size_t j;
	for (size_t channel = 0; channel < channels; ++channel) {
		j = startOffset;
		for (size_t i = 0; i < singleBufferSize; ++i) {
			if (++j > bufferSize) j = 1;
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == j + (channel * bufferSize));
		}
	}
}

TEMPLATE_TEST_CASE("[CircularAudioBuffer] Buffer data can be set with setSample() and addSample()", "[CircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t startOffset = 28;
	auto wrapper = CircularAudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, startOffset);

	const size_t index = 5;
	wrapper.audioBuffer.setSample(0, index, 20);
	REQUIRE(wrapper.audioBuffer.getSample(0, index) == 20);

	wrapper.audioBuffer.addSample(1, index, 20);
	REQUIRE(wrapper.audioBuffer.getSample(1, index) == 54); //32 (second channel) + 1 (index 5 + startOffset is 1) + 20 + 1 (the count start from 1)
}

TEMPLATE_TEST_CASE("[CircularAudioBuffer] Read and write indices work correctly", "[CircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t startOffset = 28;
	auto wrapper = CircularAudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, startOffset);

	auto &bufferView = wrapper.audioBuffer;

	REQUIRE_FALSE(bufferView.isDataAvailable());
	bufferView.incrementWriteIndex();
	REQUIRE(bufferView.getWriteIndex() == singleBufferSize);
	bufferView.incrementWriteIndex(2);
	REQUIRE(bufferView.getWriteIndex() == singleBufferSize + 2);
	REQUIRE(bufferView.isDataAvailable());

	bufferView.incrementReadIndex();
	REQUIRE(bufferView.getReadIndex() == singleBufferSize);
	bufferView.incrementReadIndex(2);
	REQUIRE(bufferView.getReadIndex() == singleBufferSize + 2);
	REQUIRE_FALSE(bufferView.isDataAvailable());
	bufferView.incrementWriteIndex();
	REQUIRE(bufferView.isDataAvailable());

	bufferView.incrementWriteIndex();
	REQUIRE_FALSE(bufferView.getReadIndex() == bufferView.getWriteIndex());
	bufferView.resetWriteIndexToReadIndexPosition();
	REQUIRE(bufferView.getReadIndex() == bufferView.getWriteIndex());

	bufferView.resetIndexes();
	REQUIRE(bufferView.getReadIndex() == 0);
	REQUIRE(bufferView.getWriteIndex() == 0);

}

TEMPLATE_TEST_CASE("[CircularAudioBuffer] Data are red correctly moving the read internal index", "[CircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t startOffset = 28;
	auto wrapper = CircularAudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, startOffset);

	size_t j;
	for (size_t channel = 0; channel < channels; ++channel) {
		j = startOffset;
		for (size_t i = 0; i < singleBufferSize; ++i) {
			if (++j > bufferSize) j = 1;
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == j + (channel * bufferSize));
		}
	}

	wrapper.audioBuffer.incrementWriteIndex();
	wrapper.audioBuffer.incrementReadIndex();

	for (size_t channel = 0; channel < channels; ++channel) {
		j = (startOffset + singleBufferSize) % bufferSize;
		for (size_t i = 0; i < singleBufferSize; ++i) {
			if (++j > bufferSize) j = 1;
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == j + (channel * bufferSize));
		}
	}

	wrapper.audioBuffer.incrementWriteIndex(bufferSize);
	wrapper.audioBuffer.incrementReadIndex(bufferSize);

	for (size_t channel = 0; channel < channels; ++channel) {
		j = (startOffset + singleBufferSize) % bufferSize;
		for (size_t i = 0; i < singleBufferSize; ++i) {
			if (++j > bufferSize) j = 1;
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == j + (channel * bufferSize));
		}
	}
}

TEMPLATE_TEST_CASE("[CircularAudioBuffer] Data are wrote correctly moving the write internal index", "[CircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t startOffset = 28;
	auto wrapper = CircularAudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, startOffset);

	wrapper.audioBuffer.incrementWriteIndex();
	wrapper.audioBuffer.incrementReadIndex();
	REQUIRE(wrapper.audioBuffer.getSample(0, 0) == (1 + singleBufferSize + startOffset) % bufferSize);

	wrapper.audioBuffer.setSample(0, 0, 20);
	REQUIRE(wrapper.audioBuffer.getSample(0, 0) == 20);
}

TEMPLATE_TEST_CASE("[CircularAudioBuffer] Buffer data can be copied from another buffer", "[CircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t startOffset = 28;
	auto wrapper = CircularAudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, startOffset, 0);
	auto wrapperCopy = CircularAudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, startOffset);

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == 0);
		}
	}

	wrapper.audioBuffer.copyFrom(wrapperCopy.audioBuffer, abl::SamplesRange::allSamples());
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			const auto val = i + 1 + startOffset;
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == (val > bufferSize ? val - bufferSize : val) + (channel * bufferSize));
		}
	}

	const size_t rangeFrom = 2;
	const size_t rangeCount = 4;
	wrapper.audioBuffer.copyFrom(wrapperCopy.audioBuffer, abl::SamplesRange(rangeFrom, rangeCount));
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			auto val = i + 1 + startOffset;
			if (i >= rangeFrom && i < rangeFrom + rangeCount) {
				val -= rangeFrom;
			}
			if (val > bufferSize) val -= bufferSize;
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == val + (channel * bufferSize));
		}
	}

	wrapper.audioBuffer.copyFrom(wrapperCopy.audioBuffer, abl::SamplesRange::allSamples(), 0.5);
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			const auto val = i + 1 + startOffset;
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == TestType((val > bufferSize ? val - bufferSize : val) + (channel * bufferSize)) / 2);
		}
	}
}

TEMPLATE_TEST_CASE("[CircularAudioBuffer] Channel buffer data can be copied from another channel buffer", "[CircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t startOffset = 28;
	auto wrapper = CircularAudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, startOffset, 0);
	auto wrapperCopy = CircularAudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, startOffset);

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == 0);
		}

		wrapper.audioBuffer.copyIntoChannelFrom(wrapperCopy.audioBuffer.getChannelView(channel), channel, abl::SamplesRange::allSamples());
		for (size_t i = 0; i < singleBufferSize; ++i) {
			const auto val = i + 1 + startOffset;
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == (val > bufferSize ? val - bufferSize : val) + (channel * bufferSize));
		}

		const size_t rangeFrom = 2;
		const size_t rangeCount = 4;
		wrapper.audioBuffer.copyIntoChannelFrom(wrapperCopy.audioBuffer.getChannelView(channel), channel, abl::SamplesRange(rangeFrom, rangeCount));
		for (size_t i = 0; i < singleBufferSize; ++i) {
			auto val = i + 1 + startOffset;
			if (i >= rangeFrom && i < rangeFrom + rangeCount) {
				val -= rangeFrom;
			}
			if (val > bufferSize) val -= bufferSize;
			REQUIRE(wrapper.audioBuffer[channel][i] == val + (channel * bufferSize));
		}

		wrapper.audioBuffer.copyIntoChannelFrom(wrapperCopy.audioBuffer.getChannelView(channel), channel, abl::SamplesRange::allSamples(), 0.5);
		for (size_t i = 0; i < singleBufferSize; ++i) {
			const auto val = i + 1 + startOffset;
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == TestType((val > bufferSize ? val - bufferSize : val) + (channel * bufferSize)) / 2);
		}
	}
}

TEMPLATE_TEST_CASE("[CircularAudioBuffer] Buffer data can be copied from another buffer with ramp", "[CircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t startOffset = 28;
	auto wrapper = CircularAudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, startOffset, 0);
	auto wrapperCopy = CircularAudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, startOffset, 8);

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == 0);
		}
	}

	wrapper.audioBuffer.copyWithRampFrom(wrapperCopy.audioBuffer, 0.0, 1.0, abl::SamplesRange::allSamples());
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == i);
		}
	}

	const size_t rangeFrom = 1;
	const size_t rangeCount = 4;
	wrapper.audioBuffer.copyWithRampFrom(wrapperCopy.audioBuffer, 0.0, 1.0, abl::SamplesRange(rangeFrom, rangeCount));
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			unsigned val = i >= rangeFrom && i < rangeFrom + rangeCount ? (i - rangeFrom) * 2 : i;
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == val);
		}
	}

	wrapper.audioBuffer.copyWithRampFrom(wrapperCopy.audioBuffer, 0.0, 0.5, abl::SamplesRange::allSamples());
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == TestType(i) / 2);
		}
	}
}

TEMPLATE_TEST_CASE("[CircularAudioBuffer] Channel buffer data can be copied from another buffer with ramp", "[CircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t startOffset = 28;
	auto wrapper = CircularAudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, startOffset, 0);
	auto wrapperCopy = CircularAudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, startOffset, 8);

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == 0);
		}

		wrapper.audioBuffer.copyIntoChannelWithRampFrom(wrapperCopy.audioBuffer.getChannelView(channel), channel, 0.0, 1.0, abl::SamplesRange::allSamples());
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == i);
		}

		const size_t rangeFrom = 1;
		const size_t rangeCount = 4;
		wrapper.audioBuffer.copyIntoChannelWithRampFrom(wrapperCopy.audioBuffer.getChannelView(channel), channel, 0.0, 1.0, abl::SamplesRange(rangeFrom, rangeCount));
		for (size_t i = 0; i < singleBufferSize; ++i) {
			auto val = i >= rangeFrom && i < rangeFrom + rangeCount ? (i - rangeFrom) * 2 : i;
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == val);
		}

		wrapper.audioBuffer.copyIntoChannelWithRampFrom(wrapperCopy.audioBuffer.getChannelView(channel), channel, 0.0, 0.5, abl::SamplesRange::allSamples());
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == TestType(i) / 2);
		}
	}
}

TEMPLATE_TEST_CASE("[CircularAudioBuffer] Buffer data can be added from another buffer", "[CircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t startOffset = 28;
	auto wrapper = CircularAudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, startOffset, 2);
	auto wrapperCopy = CircularAudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, startOffset);

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == 2);
		}
	}

	wrapper.audioBuffer.addFrom(wrapperCopy.audioBuffer, abl::SamplesRange::allSamples());
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			const unsigned val = i + 1 + startOffset;
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == (val > bufferSize ? val - bufferSize : val) + (channel * bufferSize) + 2);
		}
	}

	wrapper.audioBuffer.addFrom(wrapperCopy.audioBuffer, abl::SamplesRange::allSamples(), 0.5);
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			unsigned val = i + 1 + startOffset;
			if (val > bufferSize) val -= bufferSize;
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == (TestType(val) / 2) + val + 2 + (channel * bufferSize * 1.5));
		}
	}

	const size_t rangeFrom = 3;
	const size_t rangeCount = 4;
	auto rangeWrapper = CircularAudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, startOffset, 2);
	rangeWrapper.audioBuffer.addFrom(wrapperCopy.audioBuffer, abl::SamplesRange(rangeFrom, rangeCount));
	for (size_t channel = 0; channel < channels; ++channel) {
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

TEMPLATE_TEST_CASE("[CircularAudioBuffer] Channel buffer data can be added from another channel buffer", "[CircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t startOffset = 28;
	auto wrapper = CircularAudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, startOffset, 2);
	auto wrapperCopy = CircularAudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, startOffset);
	auto rangeWrapper = CircularAudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, startOffset, 2);

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == 2);
		}

		wrapper.audioBuffer.addIntoChannelFrom(wrapperCopy.audioBuffer.getChannelView(channel), channel, abl::SamplesRange::allSamples());
		for (size_t i = 0; i < singleBufferSize; ++i) {
			const unsigned val = i + 1 + startOffset;
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == (val > bufferSize ? val - bufferSize : val) + (channel * bufferSize) + 2);
		}

		wrapper.audioBuffer.addIntoChannelFrom(wrapperCopy.audioBuffer.getChannelView(channel), channel, abl::SamplesRange::allSamples(), 0.5);
		for (size_t i = 0; i < singleBufferSize; ++i) {
			unsigned val = i + 1 + startOffset;
			if (val > bufferSize) val -= bufferSize;
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == (TestType(val) / 2) + val + 2 + (channel * bufferSize * 1.5));
		}

		const size_t rangeFrom = 3;
		const size_t rangeCount = 4;
		rangeWrapper.audioBuffer.addIntoChannelFrom(wrapperCopy.audioBuffer.getChannelView(channel), channel, abl::SamplesRange(rangeFrom, rangeCount));
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

TEMPLATE_TEST_CASE("[CircularAudioBuffer] Buffer data can be added from another buffer with ramp", "[CircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t startOffset = 28;
	auto wrapper = CircularAudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, startOffset, 3);
	auto wrapperCopy = CircularAudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, startOffset, 8);

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == 3);
		}
	}

	wrapper.audioBuffer.addWithRampFrom(wrapperCopy.audioBuffer, 0.0, 1.0, abl::SamplesRange::allSamples());
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == i + 3);
		}
	}

	wrapper.audioBuffer.addWithRampFrom(wrapperCopy.audioBuffer, 0.0, 0.5, abl::SamplesRange::allSamples());
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == (TestType(i) / 2) + i + 3);
		}
	}

	const size_t rangeFrom = 2;
	const size_t rangeCount = 4;
	auto rangeWrapper = CircularAudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, startOffset, 3);
	rangeWrapper.audioBuffer.addWithRampFrom(wrapperCopy.audioBuffer, 0.0, 0.5, abl::SamplesRange(rangeFrom, rangeCount));
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(rangeWrapper.audioBuffer.getSample(channel, i) == (i >= rangeFrom && i < rangeFrom + rangeCount ? i - rangeFrom : 0) + 3);
		}
	}
}

TEMPLATE_TEST_CASE("[CircularAudioBuffer] Channel buffer data can be added from another buffer with ramp", "[CircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t startOffset = 28;
	auto wrapper = CircularAudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, startOffset, 3);
	auto wrapperCopy = CircularAudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, startOffset, 8);
	auto rangeWrapper = CircularAudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, startOffset, 3);

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == 3);
		}

		wrapper.audioBuffer.addIntoChannelWithRampFrom(wrapperCopy.audioBuffer.getChannelView(channel), channel, 0.0, 1.0, abl::SamplesRange::allSamples());
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == i + 3);
		}

		wrapper.audioBuffer.addIntoChannelWithRampFrom(wrapperCopy.audioBuffer.getChannelView(channel), channel, 0.0, 0.5, abl::SamplesRange::allSamples());
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == (TestType(i) / 2) + i + 3);
		}

		const size_t rangeFrom = 2;
		const size_t rangeCount = 4;
		rangeWrapper.audioBuffer.addIntoChannelWithRampFrom(wrapperCopy.audioBuffer.getChannelView(channel), channel, 0.0, 0.5, abl::SamplesRange(rangeFrom, rangeCount));
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(rangeWrapper.audioBuffer.getSample(channel, i) == (i >= rangeFrom && i < rangeFrom + rangeCount ? i - rangeFrom : 0) + 3);
		}
	}
}

TEMPLATE_TEST_CASE("[CircularAudioBuffer] Buffer data is scaled with applyGain()", "[CircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t startOffset = 28;
	auto wrapper = CircularAudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, startOffset);

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			unsigned val = i + 1 + startOffset;
			if (val > bufferSize) val -= bufferSize;
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == val + (channel * bufferSize));
		}
	}

	wrapper.audioBuffer.applyGain(0.5, abl::SamplesRange::allSamples());
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			unsigned val = i + 1 + startOffset;
			if (val > bufferSize) val -= bufferSize;
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == TestType(val + (channel * bufferSize)) / 2);
		}
	}

	wrapper.audioBuffer.applyGain(3.0, abl::SamplesRange::allSamples());
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			unsigned val = i + 1 + startOffset;
			if (val > bufferSize) val -= bufferSize;
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == (TestType(val + (channel * bufferSize)) / 2) * 3);
		}
	}

	const size_t rangeFrom = 5;
	const size_t rangeCount = 3;
	auto rangeWrapper = CircularAudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, startOffset);
	rangeWrapper.audioBuffer.applyGain(2.0, abl::SamplesRange(rangeFrom, rangeCount));
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			unsigned val = i + 1 + startOffset;
			if (val > bufferSize) val -= bufferSize;
			val += channel * bufferSize;
			REQUIRE(rangeWrapper.audioBuffer.getSample(channel, i) == (i >= rangeFrom && i < rangeFrom + rangeCount ? val * 2 : val));
		}
	}
}

TEMPLATE_TEST_CASE("[CircularAudioBuffer] Channel buffer data is scaled with applyGainToChannel()", "[CircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t startOffset = 28;
	auto wrapper = CircularAudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, startOffset);
	auto rangeWrapper = CircularAudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, startOffset);

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			unsigned val = i + 1 + startOffset;
			if (val > bufferSize) val -= bufferSize;
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == val + (channel * bufferSize));
		}

		wrapper.audioBuffer.applyGainToChannel(0.5, channel, abl::SamplesRange::allSamples());
		for (size_t i = 0; i < singleBufferSize; ++i) {
			unsigned val = i + 1 + startOffset;
			if (val > bufferSize) val -= bufferSize;
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == TestType(val + (channel * bufferSize)) / 2);
		}

		wrapper.audioBuffer.applyGainToChannel(3.0, channel, abl::SamplesRange::allSamples());
		for (size_t i = 0; i < singleBufferSize; ++i) {
			unsigned val = i + 1 + startOffset;
			if (val > bufferSize) val -= bufferSize;
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == (TestType(val + (channel * bufferSize)) / 2) * 3);
		}

		const size_t rangeFrom = 5;
		const size_t rangeCount = 3;
		rangeWrapper.audioBuffer.applyGainToChannel(2.0, channel, abl::SamplesRange(rangeFrom, rangeCount));
		for (size_t i = 0; i < singleBufferSize; ++i) {
			unsigned val = i + 1 + startOffset;
			if (val > bufferSize) val -= bufferSize;
			val += channel * bufferSize;
			REQUIRE(rangeWrapper.audioBuffer.getSample(channel, i) == (i >= rangeFrom && i < rangeFrom + rangeCount ? val * 2 : val));
		}
	}
}

TEMPLATE_TEST_CASE("[CircularAudioBuffer] Buffer data is scaled with ramp using applyGainRamp()", "[CircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t startOffset = 28;
	auto wrapper = CircularAudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, startOffset, 10);

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == 10);
		}
	}

	wrapper.audioBuffer.applyGainRamp(0.0, 1.0, abl::SamplesRange::allSamples());
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == TestType(i) * 10 / singleBufferSize);
		}
	}

	wrapper.audioBuffer.applyGainRamp(0.5, 0.0, abl::SamplesRange::allSamples());
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == (TestType(i) * 10 / singleBufferSize) * (singleBufferSize - i) / (singleBufferSize * 2));
		}
	}

	const size_t rangeFrom = 2;
	const size_t rangeCount = 4;
	auto rangeWrapper = CircularAudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, startOffset, 10);
	rangeWrapper.audioBuffer.applyGainRamp(0.0, 1.0, abl::SamplesRange(rangeFrom, rangeCount));
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(rangeWrapper.audioBuffer.getSample(channel, i) == (i >= rangeFrom && i < rangeFrom + rangeCount ? TestType(i - rangeFrom) * 20 / singleBufferSize : 10));
		}
	}
}

TEMPLATE_TEST_CASE("[CircularAudioBuffer] Channel buffer data is scaled with applyGainRampToChannel()", "[CircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t startOffset = 28;
	auto wrapper = CircularAudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, startOffset, 10);
	auto rangeWrapper = CircularAudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, startOffset, 10);

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == 10);
		}

		wrapper.audioBuffer.applyGainRampToChannel(0.0, 1.0, channel, abl::SamplesRange::allSamples());
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == TestType(i) * 10 / singleBufferSize);
		}

		wrapper.audioBuffer.applyGainRampToChannel(0.5, 0.0, channel, abl::SamplesRange::allSamples());
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == (TestType(i) * 10 / singleBufferSize) * (singleBufferSize - i) / (singleBufferSize * 2));
		}

		const size_t rangeFrom = 2;
		const size_t rangeCount = 4;
		rangeWrapper.audioBuffer.applyGainRampToChannel(0.0, 1.0, channel, abl::SamplesRange(rangeFrom, rangeCount));
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(rangeWrapper.audioBuffer.getSample(channel, i) == (i >= rangeFrom && i < rangeFrom + rangeCount ? TestType(i - rangeFrom) * 20 / singleBufferSize : 10));
		}
	}
}

TEMPLATE_TEST_CASE("[CircularAudioBuffer] Buffer data is cleared with clear()", "[CircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t startOffset = 28;
	auto wrapper = CircularAudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, startOffset);

	wrapper.audioBuffer.clear(abl::SamplesRange::allSamples());

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == 0);
		}
	}

	const size_t rangeFrom = 5;
	const size_t rangeCount = 3;
	auto rangeWrapper = CircularAudioBufferWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, startOffset, 4);
	rangeWrapper.audioBuffer.clear(abl::SamplesRange(rangeFrom, rangeCount));
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(rangeWrapper.audioBuffer.getSample(channel, i) == (i >= rangeFrom && i < rangeFrom + rangeCount ? 0 : 4));
		}
	}
}

TEMPLATE_TEST_CASE("[CircularAudioBuffer] Channel Buffer data is cleared with clearChannel()", "[CircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t startOffset = 28;
	auto wrapper = CircularAudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, startOffset);
	auto rangeWrapper = CircularAudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, startOffset);

	wrapper.audioBuffer.clearChannel(1, abl::SamplesRange::allSamples());

	for (size_t i = 0; i < singleBufferSize; ++i) {
		const unsigned val = i + 1 + startOffset;
		REQUIRE(wrapper.audioBuffer.getSample(0, i) == (val > bufferSize ? val - bufferSize : val));
	}

	for (size_t i = 0; i < singleBufferSize; ++i) {
		REQUIRE(wrapper.audioBuffer.getSample(1, i) == 0);
	}

	const size_t rangeFrom = 5;
	const size_t rangeCount = 3;
	rangeWrapper.audioBuffer.clearChannel(0, abl::SamplesRange(rangeFrom, rangeCount));

	for (size_t i = 0; i < singleBufferSize; ++i) {
		unsigned val = i + 1 + startOffset;
		if (val > bufferSize) val -= bufferSize;
		REQUIRE(rangeWrapper.audioBuffer.getSample(0, i) == (i >= rangeFrom && i < rangeFrom + rangeCount ? 0 : val));
	}

	for (size_t i = 0; i < singleBufferSize; ++i) {
		unsigned val = i + 1 + startOffset;
		if (val > bufferSize) val -= bufferSize;
		REQUIRE(rangeWrapper.audioBuffer.getSample(1, i) == val + bufferSize);
	}
}

TEMPLATE_TEST_CASE("[CircularAudioBuffer] Buffer data is reversed with reverse()", "[CircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t startOffset = 28;
	auto wrapper = CircularAudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, startOffset);

	const auto splitIndex = bufferSize - startOffset;
	std::array<int, channels> valuesBeforeSplit{};
	std::array<int, channels> lastValues{};
	for (size_t channel = 0; channel < channels; ++channel) {
		valuesBeforeSplit[channel] = wrapper.audioBuffer.getSample(channel, splitIndex - 1);
		lastValues[channel] = wrapper.audioBuffer.getSample(channel, singleBufferSize - 1);
	}
	wrapper.audioBuffer.reverse(abl::SamplesRange::allSamples());

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == (i >= splitIndex ? valuesBeforeSplit[channel] - i + splitIndex : lastValues[channel] - i));
		}
	}

	const auto rangeFrom = 2;
	const auto rangeCount = 4;
	auto rangeWrapper = CircularAudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, startOffset);
	std::array<int, channels> rangeLastValues{};
	for (size_t channel = 0; channel < channels; ++channel) {
		rangeLastValues[channel] = rangeWrapper.audioBuffer.getSample(channel, rangeFrom + rangeCount - 1);
	}
	rangeWrapper.audioBuffer.reverse(abl::SamplesRange(rangeFrom, rangeCount));

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			if (i >= rangeFrom && i < rangeFrom + rangeCount) {
				REQUIRE(rangeWrapper.audioBuffer.getSample(channel, i) == (i >= splitIndex ? valuesBeforeSplit[channel] - i + splitIndex : rangeLastValues[channel] - i + rangeFrom));
			} else {
				REQUIRE(rangeWrapper.audioBuffer.getSample(channel, i) == (i + startOffset + 1 > bufferSize ? i + startOffset + 1 - bufferSize : i + startOffset + 1) + (bufferSize * channel));
			}
		}
	}
}

TEMPLATE_TEST_CASE("[CircularAudioBuffer] Channel Buffer data is reversed with reverse()", "[CircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t startOffset = 28;
	auto wrapper = CircularAudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, startOffset);

	const auto splitIndex = bufferSize - startOffset;
	std::array<int, channels> valuesBeforeSplit{};
	std::array<int, channels> lastValues{};
	for (size_t channel = 0; channel < channels; ++channel) {
		valuesBeforeSplit[channel] = wrapper.audioBuffer.getSample(channel, splitIndex - 1);
		lastValues[channel] = wrapper.audioBuffer.getSample(channel, singleBufferSize - 1);
	}

	wrapper.audioBuffer.reverseChannel(1, abl::SamplesRange::allSamples());

	for (size_t i = 0; i < singleBufferSize; ++i) {
		const unsigned val = i + 1 + startOffset;
		REQUIRE(wrapper.audioBuffer.getSample(0, i) == (val > bufferSize ? val - bufferSize : val));
	}

	for (size_t i = 0; i < singleBufferSize; ++i) {
		REQUIRE(wrapper.audioBuffer.getSample(1, i) == (i >= splitIndex ? valuesBeforeSplit[1] - i + splitIndex : lastValues[1] - i));
	}

	const auto rangeFrom = 2;
	const auto rangeCount = 4;
	auto rangeWrapper = CircularAudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, startOffset);
	std::array<int, channels> rangeLastValues{};
	for (size_t channel = 0; channel < channels; ++channel) {
		rangeLastValues[channel] = rangeWrapper.audioBuffer.getSample(channel, rangeFrom + rangeCount - 1);
	}
	rangeWrapper.audioBuffer.reverseChannel(0, abl::SamplesRange(rangeFrom, rangeCount));

	for (size_t i = 0; i < singleBufferSize; ++i) {
		if (i >= rangeFrom && i < rangeFrom + rangeCount) {
			REQUIRE(rangeWrapper.audioBuffer.getSample(0, i) == (i >= splitIndex ? valuesBeforeSplit[0] - i + splitIndex : rangeLastValues[0] - i + rangeFrom));
		} else {
			REQUIRE(rangeWrapper.audioBuffer.getSample(0, i) == (i + startOffset + 1 > bufferSize ? i + startOffset + 1 - bufferSize : i + startOffset + 1));
		}
	}

	for (size_t i = 0; i < singleBufferSize; ++i) {
		const unsigned val = i + 1 + startOffset;
		REQUIRE(rangeWrapper.audioBuffer.getSample(1, i) == (val > bufferSize ? val - bufferSize : val) + bufferSize);
	}
}

TEMPLATE_TEST_CASE("[CircularAudioBuffer] View report correct higher peak", "[CircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t startOffset = 28;
	auto wrapper = CircularAudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, startOffset);

	REQUIRE(wrapper.audioBuffer.getHigherPeak(abl::SamplesRange::allSamples()) == 64);
	REQUIRE(wrapper.audioBuffer.getHigherPeak(abl::SamplesRange(3, 3)) == 64);
	REQUIRE(wrapper.audioBuffer.getHigherPeak(abl::SamplesRange(5, 2)) == 35);
}

TEMPLATE_TEST_CASE("[CircularAudioBuffer] View report correct higher peak for channels", "[CircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t startOffset = 28;
	auto wrapper = CircularAudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, startOffset);

	for (size_t channel = 0; channel < channels; ++channel) {
		REQUIRE(wrapper.audioBuffer.getHigherPeakForChannel(channel, abl::SamplesRange::allSamples()) == bufferSize * (channel + 1));
		REQUIRE(wrapper.audioBuffer.getHigherPeakForChannel(channel, abl::SamplesRange(3, 3)) == bufferSize * (channel + 1));
		REQUIRE(wrapper.audioBuffer.getHigherPeakForChannel(channel, abl::SamplesRange(5, 2)) == 3 + (bufferSize * channel));
	}
}

TEMPLATE_TEST_CASE("[CircularAudioBuffer] View report correct rms level for channels", "[CircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t startOffset = 28;
	auto wrapper = CircularAudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, startOffset);

	REQUIRE(wrapper.audioBuffer.getRMSLevelForChannel(0, abl::SamplesRange::allSamples()) == TestType(16.5));
	REQUIRE(wrapper.audioBuffer.getRMSLevelForChannel(1, abl::SamplesRange::allSamples()) == TestType(48.5));
	REQUIRE(wrapper.audioBuffer.getRMSLevelForChannel(0, abl::SamplesRange(3, 4)) == TestType(9.5));
	REQUIRE(wrapper.audioBuffer.getRMSLevelForChannel(1, abl::SamplesRange(4, 2)) == TestType(33.5)); //33 + 34 / 2
}

TEMPLATE_TEST_CASE("[CircularAudioBuffer] View report correct buffer size and channel count", "[CircularAudioBuffer]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t startOffset = 28;
	auto wrapper = CircularAudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, startOffset);

	REQUIRE(wrapper.audioBuffer.getBufferSize() == singleBufferSize);
	REQUIRE(wrapper.audioBuffer.getChannelsCount() == channels);
}

TEMPLATE_TEST_CASE("[CircularAudioBuffer] Channels are mapped correctly", "[CircularAudioBuffer]", int, double) {
	const size_t channels = 4;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t startOffset = 28;
	std::vector<size_t> channelsMapping{3, 1, 2, 0, 1};
	auto wrapper = CircularAudioBufferWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, startOffset, channelsMapping);

	REQUIRE(wrapper.audioBuffer.getChannelsCount() == 5);
	size_t channel = 0;
	for (auto mappedChannel: channelsMapping) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			const auto val = i + 1 + startOffset;
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == (val > bufferSize ? val - bufferSize : val) + (mappedChannel * bufferSize));
		}
		++channel;
	}

	channelsMapping = {0, 2};
	wrapper.audioBuffer.setChannelsMapping(channelsMapping);
	REQUIRE(wrapper.audioBuffer.getChannelsCount() == 2);
	channel = 0;
	for (auto mappedChannel: channelsMapping) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			const auto val = i + 1 + startOffset;
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == (val > bufferSize ? val - bufferSize : val) + (mappedChannel * bufferSize));
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
		for (size_t i = 0; i < singleBufferSize; ++i) {
			const auto val = i + 1 + startOffset;
			REQUIRE(wrapper.audioBuffer.getSample(channel, i) == (val > bufferSize ? val - bufferSize : val) + (mappedChannel * bufferSize));
		}
		++channel;
	}
}


//test channel out-of-bound assert?