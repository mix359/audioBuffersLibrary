// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "../buffers/DelayedCircularAudioBufferView.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_template_test_macros.hpp>

template<typename T>
struct DelayedCircularAudioBufferViewWrapper {
	T **data;
	size_t m_channels;
	abl::DelayedCircularAudioBufferView<T> audioBufferView;

	static DelayedCircularAudioBufferViewWrapper createWithIncrementalNumbers(size_t channels, size_t bufferSize, size_t singleBufferSize, size_t delayInSamples, size_t startOffset, const std::vector<size_t> &channelsMapping = {}, size_t startIndex = 0) {
		auto data = new T *[channels];
		size_t startValue = 1;
		for (size_t channel = 0; channel < channels; ++channel) {
			data[channel] = new T[bufferSize];
			std::iota(data[channel], data[channel] + bufferSize, startValue);
			startValue += bufferSize;
		}

		return DelayedCircularAudioBufferViewWrapper{data, channels, bufferSize, singleBufferSize, delayInSamples, startOffset, channelsMapping, startIndex};
	}

	static DelayedCircularAudioBufferViewWrapper createWithFixedValue(size_t channels, size_t bufferSize, size_t singleBufferSize, size_t delayInSamples, size_t startOffset, T fixedValue, const std::vector<size_t> &channelsMapping = {}, size_t startIndex = 0) {
		auto data = new T *[channels];
		for (size_t channel = 0; channel < channels; ++channel) {
			data[channel] = new T[bufferSize];
			std::fill(data[channel], data[channel] + bufferSize, fixedValue);
		}

		return DelayedCircularAudioBufferViewWrapper{data, channels, bufferSize, singleBufferSize, delayInSamples, startOffset, channelsMapping, startIndex};
	}

	DelayedCircularAudioBufferViewWrapper(T **newData, size_t channels, size_t bufferSize, size_t singleBufferSize, size_t delayInSamples, size_t bufferStartOffset, const std::vector<size_t> &channelsMapping = {}, size_t startIndex = 0) : data{newData}, m_channels{channels},
		audioBufferView{abl::DelayedCircularAudioBufferView{data, channels, bufferSize, singleBufferSize, delayInSamples, bufferStartOffset, channelsMapping, startIndex}} {}

	~DelayedCircularAudioBufferViewWrapper() {
		for (size_t channel = 0; channel < m_channels; ++channel) {
			delete[] data[channel];
		}
		delete[] data;
	}
};

TEST_CASE("[DelayedCircularAudioBufferView] View report correct empty state", "[DelayedCircularAudioBufferView]") {
	auto noValueWrapper = DelayedCircularAudioBufferViewWrapper<int>::createWithFixedValue(2, 16, 8, 0, 0, 0);
	auto noChannelsWrapper = DelayedCircularAudioBufferViewWrapper<int>::createWithFixedValue(0, 0, 0, 0, 0, 0);
	auto noBufferSizeWrapper = DelayedCircularAudioBufferViewWrapper<double>::createWithFixedValue(1, 0, 8, 0, 0, 0);
	auto noSingleBufferSizeWrapper = DelayedCircularAudioBufferViewWrapper<double>::createWithFixedValue(1, 16, 0, 0, 0, 0);

	REQUIRE_FALSE(noValueWrapper.audioBufferView.isEmpty());
	REQUIRE(noChannelsWrapper.audioBufferView.isEmpty());
	REQUIRE(noBufferSizeWrapper.audioBufferView.isEmpty());
	REQUIRE(noSingleBufferSizeWrapper.audioBufferView.isEmpty());
}


TEMPLATE_TEST_CASE("[DelayedCircularAudioBufferView] Buffer data is iterable", "[DelayedCircularAudioBufferView]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 8;
	const size_t startOffset = 20;
	auto wrapper = DelayedCircularAudioBufferViewWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, delayInSamples, startOffset);

	size_t i, j = 0;
	for (const auto& channelView: wrapper.audioBufferView) {
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

TEMPLATE_TEST_CASE("[DelayedCircularAudioBufferView] CircularBufferChannelView is returned accessing a channel with the operator[] and getChannelView()", "[DelayedCircularAudioBufferView]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 8;
	const size_t startOffset = 20;
	auto wrapper = DelayedCircularAudioBufferViewWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, delayInSamples, startOffset);

	size_t i;
	for (size_t channel = 0; channel < channels; ++channel) {
		auto channelView = wrapper.audioBufferView[channel];
		REQUIRE(channelView.getBufferSize() == singleBufferSize);
//		REQUIRE(dynamic_cast<abl::OffsettedReadCircularAudioBufferChannelView<TestType> *>(channelView.get()));
		i = startOffset;
		for (auto &&sample: channelView) {
			if (++i > bufferSize) i = 1;
			REQUIRE(sample == i + (channel * bufferSize));
		}

		auto channelView2 = wrapper.audioBufferView.getChannelView(channel);
		REQUIRE(channelView2.getBufferSize() == singleBufferSize);
//		REQUIRE(dynamic_cast<abl::OffsettedReadCircularAudioBufferChannelView<TestType> *>(channelView2.get()));
		i = startOffset;
		for (auto &&sample: channelView2) {
			if (++i > bufferSize) i = 1;
			REQUIRE(sample == i + (channel * bufferSize));
		}
	}
}

TEMPLATE_TEST_CASE("[DelayedCircularAudioBufferView] Valid ranged CircularBufferChannelView is returned accessing a channel passing a sampleRange to getChannelView()",
                   "[DelayedCircularAudioBufferView]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 8;
	const size_t startOffset = 20;
	auto wrapper = DelayedCircularAudioBufferViewWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, delayInSamples, startOffset);
	abl::SamplesRange samplesRange{2, 4};

	size_t i, j;
	for (size_t channel = 0; channel < channels; ++channel) {
		auto channelView = wrapper.audioBufferView.getChannelView(channel, samplesRange);
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

TEMPLATE_TEST_CASE("[DelayedCircularAudioBufferView] CircularBufferView can be copied and moved", "[DelayedCircularAudioBufferView]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 8;
	const size_t startOffset = 20;
	auto wrapper = DelayedCircularAudioBufferViewWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, delayInSamples, startOffset, 3);

	abl::DelayedCircularAudioBufferView<TestType> copyBuffer{wrapper.audioBufferView};

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(copyBuffer.getSample(channel, i) == 3);
		}
	}

	REQUIRE(!copyBuffer.isEmpty());
	abl::DelayedCircularAudioBufferView<TestType> moveBuffer{std::move(copyBuffer)};

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(moveBuffer.getSample(channel, i) == 3);
		}
	}

	REQUIRE(copyBuffer.isEmpty());
}

TEMPLATE_TEST_CASE("[DelayedCircularAudioBufferView] Valid ranged CircularBufferView is returned from getRangedView()", "[DelayedCircularAudioBufferView]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 8;
	const size_t startOffset = 20;
	auto wrapper = DelayedCircularAudioBufferViewWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, delayInSamples, startOffset);
	auto rangedBufferView = wrapper.audioBufferView.getRangedView(abl::SamplesRange{1, 5});
//	REQUIRE(dynamic_cast<abl::DelayedCircularAudioBufferView<TestType> *>(rangedBufferView.get()));

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

TEMPLATE_TEST_CASE("[DelayedCircularAudioBufferView] Buffer data is accessible with getSample()", "[DelayedCircularAudioBufferView]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 8;
	const size_t startOffset = 20;
	auto wrapper = DelayedCircularAudioBufferViewWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, delayInSamples, startOffset);

	size_t j;
	for (size_t channel = 0; channel < channels; ++channel) {
		j = startOffset;
		for (size_t i = 0; i < singleBufferSize; ++i) {
			if (++j > bufferSize) j = 1;
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == j + (channel * bufferSize));
		}
	}
}

TEMPLATE_TEST_CASE("[DelayedCircularAudioBufferView] Buffer data can be set with setSample() and addSample()", "[DelayedCircularAudioBufferView]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 8;
	const size_t startOffset = 20;
	auto wrapper = DelayedCircularAudioBufferViewWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, delayInSamples, startOffset);

	const size_t index = 5;
	wrapper.audioBufferView.setSample(0, index, 20);
	wrapper.audioBufferView.incrementIndex();
	REQUIRE(wrapper.audioBufferView.getSample(0, index) == 20);

	wrapper.audioBufferView.resetIndex();
	wrapper.audioBufferView.addSample(1, index, 20);
	wrapper.audioBufferView.incrementIndex();
	REQUIRE(wrapper.audioBufferView.getSample(1, index) == 54); //32 (second channel) + 1 (index 5 + startOffset is 1) + 20 + 1 (the count start from 1)
}

TEMPLATE_TEST_CASE("[DelayedCircularAudioBufferView] Index and delayInSample work correctly", "[DelayedCircularAudioBufferView]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 12;
	const size_t startOffset = 20;
	auto wrapper = DelayedCircularAudioBufferViewWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, delayInSamples, startOffset, {}, 4);

	auto &bufferView = wrapper.audioBufferView;
	REQUIRE(bufferView.getIndex() == 4);
	REQUIRE(bufferView.getSample(0, 0) == 5 + startOffset);
	bufferView.setSample(0, 0, 10);
	bufferView.incrementIndex(delayInSamples);
	REQUIRE(bufferView.getSample(0, 0) == 10);
	auto i = (startOffset + delayInSamples + 4) % bufferSize;
	REQUIRE(wrapper.data[0][i] == 10);

	bufferView.resetIndex();
	REQUIRE(bufferView.getDelayInSamples() == delayInSamples);
	bufferView.setDelayInSamples(16);
	REQUIRE(bufferView.getDelayInSamples() == 16);
	bufferView.setSample(0, 0, 20);
	bufferView.incrementIndex(16);
	REQUIRE(bufferView.getSample(0, 0) == 20);
	i = (startOffset + 16) % bufferSize;
	REQUIRE(wrapper.data[0][i] == 20);
}

TEMPLATE_TEST_CASE("[DelayedCircularAudioBufferView] Buffer data can be copied from another buffer", "[DelayedCircularAudioBufferView]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 8;
	const size_t startOffset = 20;
	auto wrapper = DelayedCircularAudioBufferViewWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, delayInSamples, startOffset, 0);
	auto wrapperCopy = DelayedCircularAudioBufferViewWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, delayInSamples, startOffset);

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == 0);
		}
	}

	wrapper.audioBufferView.copyFrom(wrapperCopy.audioBufferView, abl::SamplesRange::allSamples());
	wrapper.audioBufferView.incrementIndex();
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			const auto val = i + 1 + startOffset;
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == (val > bufferSize ? val - bufferSize : val) + (channel * bufferSize));
		}
	}

	const size_t rangeFrom = 2;
	const size_t rangeCount = 4;
	wrapper.audioBufferView.resetIndex();
	wrapper.audioBufferView.copyFrom(wrapperCopy.audioBufferView, abl::SamplesRange(rangeFrom, rangeCount));
	wrapper.audioBufferView.incrementIndex();
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			auto val = i + 1 + startOffset;
			if (i >= rangeFrom && i < rangeFrom + rangeCount) {
				val -= rangeFrom;
			}
			if (val > bufferSize) val -= bufferSize;
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == val + (channel * bufferSize));
		}
	}

	wrapper.audioBufferView.resetIndex();
	wrapper.audioBufferView.copyFrom(wrapperCopy.audioBufferView, abl::SamplesRange::allSamples(), 0.5);
	wrapper.audioBufferView.incrementIndex();
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			const auto val = i + 1 + startOffset;
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == TestType((val > bufferSize ? val - bufferSize : val) + (channel * bufferSize)) / 2);
		}
	}
}

TEMPLATE_TEST_CASE("[DelayedCircularAudioBufferView] Channel buffer data can be copied from another channel buffer", "[DelayedCircularAudioBufferView]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 8;
	const size_t startOffset = 20;
	auto wrapper = DelayedCircularAudioBufferViewWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, delayInSamples, startOffset, 0);
	auto wrapperCopy = DelayedCircularAudioBufferViewWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, 0, startOffset);

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == 0);
		}

		wrapper.audioBufferView.resetIndex();
		wrapper.audioBufferView.copyIntoChannelFrom(wrapperCopy.audioBufferView.getChannelView(channel), channel, abl::SamplesRange::allSamples());
		wrapper.audioBufferView.incrementIndex();
		for (size_t i = 0; i < singleBufferSize; ++i) {
			const auto val = i + 1 + startOffset;
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == (val > bufferSize ? val - bufferSize : val) + (channel * bufferSize));
		}

		const size_t rangeFrom = 2;
		const size_t rangeCount = 4;
		wrapper.audioBufferView.resetIndex();
		wrapper.audioBufferView.copyIntoChannelFrom(wrapperCopy.audioBufferView.getChannelView(channel), channel, abl::SamplesRange(rangeFrom, rangeCount));
		wrapper.audioBufferView.incrementIndex();
		for (size_t i = 0; i < singleBufferSize; ++i) {
			auto val = i + 1 + startOffset;
			if (i >= rangeFrom && i < rangeFrom + rangeCount) {
				val -= rangeFrom;
			}
			if (val > bufferSize) val -= bufferSize;
			REQUIRE(wrapper.audioBufferView[channel][i] == val + (channel * bufferSize));
		}

		wrapper.audioBufferView.resetIndex();
		wrapper.audioBufferView.copyIntoChannelFrom(wrapperCopy.audioBufferView.getChannelView(channel), channel, abl::SamplesRange::allSamples(), 0.5);
		wrapper.audioBufferView.incrementIndex();
		for (size_t i = 0; i < singleBufferSize; ++i) {
			const auto val = i + 1 + startOffset;
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == TestType((val > bufferSize ? val - bufferSize : val) + (channel * bufferSize)) / 2);
		}
	}
}

TEMPLATE_TEST_CASE("[DelayedCircularAudioBufferView] Buffer data can be copied from another buffer with ramp", "[DelayedCircularAudioBufferView]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 8;
	const size_t startOffset = 20;
	auto wrapper = DelayedCircularAudioBufferViewWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, delayInSamples, startOffset, 0);
	auto wrapperCopy = DelayedCircularAudioBufferViewWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, delayInSamples, startOffset, 8);

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == 0);
		}
	}

	wrapper.audioBufferView.copyWithRampFrom(wrapperCopy.audioBufferView, 0.0, 1.0, abl::SamplesRange::allSamples());
	wrapper.audioBufferView.incrementIndex();
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == i);
		}
	}

	const size_t rangeFrom = 1;
	const size_t rangeCount = 4;
	wrapper.audioBufferView.resetIndex();
	wrapper.audioBufferView.copyWithRampFrom(wrapperCopy.audioBufferView, 0.0, 1.0, abl::SamplesRange(rangeFrom, rangeCount));
	wrapper.audioBufferView.incrementIndex();
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			unsigned val = i >= rangeFrom && i < rangeFrom + rangeCount ? (i - rangeFrom) * 2 : i;
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == val);
		}
	}

	wrapper.audioBufferView.resetIndex();
	wrapper.audioBufferView.copyWithRampFrom(wrapperCopy.audioBufferView, 0.0, 0.5, abl::SamplesRange::allSamples());
	wrapper.audioBufferView.incrementIndex();
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == TestType(i) / 2);
		}
	}
}

TEMPLATE_TEST_CASE("[DelayedCircularAudioBufferView] Channel buffer data can be copied from another buffer with ramp", "[DelayedCircularAudioBufferView]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 8;
	const size_t startOffset = 20;
	auto wrapper = DelayedCircularAudioBufferViewWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, delayInSamples, startOffset, 0);
	auto wrapperCopy = DelayedCircularAudioBufferViewWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, delayInSamples, startOffset, 8);

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == 0);
		}

		wrapper.audioBufferView.resetIndex();
		wrapper.audioBufferView.copyIntoChannelWithRampFrom(wrapperCopy.audioBufferView.getChannelView(channel), channel, 0.0, 1.0, abl::SamplesRange::allSamples());
		wrapper.audioBufferView.incrementIndex();
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == i);
		}

		const size_t rangeFrom = 1;
		const size_t rangeCount = 4;
		wrapper.audioBufferView.resetIndex();
		wrapper.audioBufferView.copyIntoChannelWithRampFrom(wrapperCopy.audioBufferView.getChannelView(channel), channel, 0.0, 1.0, abl::SamplesRange(rangeFrom, rangeCount));
		wrapper.audioBufferView.incrementIndex();
		for (size_t i = 0; i < singleBufferSize; ++i) {
			auto val = i >= rangeFrom && i < rangeFrom + rangeCount ? (i - rangeFrom) * 2 : i;
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == val);
		}

		wrapper.audioBufferView.resetIndex();
		wrapper.audioBufferView.copyIntoChannelWithRampFrom(wrapperCopy.audioBufferView.getChannelView(channel), channel, 0.0, 0.5, abl::SamplesRange::allSamples());
		wrapper.audioBufferView.incrementIndex();
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == TestType(i) / 2);
		}
	}
}

TEMPLATE_TEST_CASE("[DelayedCircularAudioBufferView] Buffer data can be added from another buffer", "[DelayedCircularAudioBufferView]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 8;
	const size_t startOffset = 20;
	auto wrapper = DelayedCircularAudioBufferViewWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, delayInSamples, startOffset, 2);
	auto wrapperCopy = DelayedCircularAudioBufferViewWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, delayInSamples, startOffset);

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == 2);
		}
	}

	wrapper.audioBufferView.addFrom(wrapperCopy.audioBufferView, abl::SamplesRange::allSamples());
	wrapper.audioBufferView.incrementIndex();
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			const unsigned val = i + 1 + startOffset;
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == (val > bufferSize ? val - bufferSize : val) + (channel * bufferSize) + 2);
		}
	}

	wrapper.audioBufferView.resetIndex();
	wrapper.audioBufferView.addFrom(wrapperCopy.audioBufferView, abl::SamplesRange::allSamples(), 0.5);
	wrapper.audioBufferView.incrementIndex();
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			unsigned val = i + 1 + startOffset;
			if (val > bufferSize) val -= bufferSize;
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == (TestType(val) / 2) + val + 2 + (channel * bufferSize * 1.5));
		}
	}

	const size_t rangeFrom = 3;
	const size_t rangeCount = 4;
	auto rangeWrapper = DelayedCircularAudioBufferViewWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, delayInSamples, startOffset, 2);
	rangeWrapper.audioBufferView.addFrom(wrapperCopy.audioBufferView, abl::SamplesRange(rangeFrom, rangeCount));
	rangeWrapper.audioBufferView.incrementIndex();
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			unsigned val = 0;
			if (i >= rangeFrom && i < rangeFrom + rangeCount) {
				val = i + 1 + startOffset - rangeFrom;
				if (val > bufferSize) val -= bufferSize;
				val += channel * bufferSize;
			}
			REQUIRE(rangeWrapper.audioBufferView.getSample(channel, i) == val + 2);
		}
	}
}

TEMPLATE_TEST_CASE("[DelayedCircularAudioBufferView] Channel buffer data can be added from another channel buffer", "[DelayedCircularAudioBufferView]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 8;
	const size_t startOffset = 20;
	auto wrapper = DelayedCircularAudioBufferViewWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, delayInSamples, startOffset, 2);
	auto wrapperCopy = DelayedCircularAudioBufferViewWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, delayInSamples, startOffset);
	auto rangeWrapper = DelayedCircularAudioBufferViewWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, delayInSamples, startOffset, 2);

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == 2);
		}

		wrapper.audioBufferView.resetIndex();
		wrapper.audioBufferView.addIntoChannelFrom(wrapperCopy.audioBufferView.getChannelView(channel), channel, abl::SamplesRange::allSamples());
		wrapper.audioBufferView.incrementIndex();
		for (size_t i = 0; i < singleBufferSize; ++i) {
			const unsigned val = i + 1 + startOffset;
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == (val > bufferSize ? val - bufferSize : val) + (channel * bufferSize) + 2);
		}

		wrapper.audioBufferView.resetIndex();
		wrapper.audioBufferView.addIntoChannelFrom(wrapperCopy.audioBufferView.getChannelView(channel), channel, abl::SamplesRange::allSamples(), 0.5);
		wrapper.audioBufferView.incrementIndex();
		for (size_t i = 0; i < singleBufferSize; ++i) {
			unsigned val = i + 1 + startOffset;
			if (val > bufferSize) val -= bufferSize;
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == (TestType(val) / 2) + val + 2 + (channel * bufferSize * 1.5));
		}

		const size_t rangeFrom = 3;
		const size_t rangeCount = 4;
		rangeWrapper.audioBufferView.resetIndex();
		rangeWrapper.audioBufferView.addIntoChannelFrom(wrapperCopy.audioBufferView.getChannelView(channel), channel, abl::SamplesRange(rangeFrom, rangeCount));
		rangeWrapper.audioBufferView.incrementIndex();
		for (size_t i = 0; i < singleBufferSize; ++i) {
			unsigned val = 0;
			if (i >= rangeFrom && i < rangeFrom + rangeCount) {
				val = i + 1 + startOffset - rangeFrom;
				if (val > bufferSize) val -= bufferSize;
				val += channel * bufferSize;
			}
			REQUIRE(rangeWrapper.audioBufferView.getSample(channel, i) == val + 2);
		}
	}
}

TEMPLATE_TEST_CASE("[DelayedCircularAudioBufferView] Buffer data can be added from another buffer with ramp", "[DelayedCircularAudioBufferView]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 8;
	const size_t startOffset = 20;
	auto wrapper = DelayedCircularAudioBufferViewWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, delayInSamples, startOffset, 3);
	auto wrapperCopy = DelayedCircularAudioBufferViewWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, delayInSamples, startOffset, 8);

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == 3);
		}
	}

	wrapper.audioBufferView.addWithRampFrom(wrapperCopy.audioBufferView, 0.0, 1.0, abl::SamplesRange::allSamples());
	wrapper.audioBufferView.incrementIndex();
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == i + 3);
		}
	}

	wrapper.audioBufferView.resetIndex();
	wrapper.audioBufferView.addWithRampFrom(wrapperCopy.audioBufferView, 0.0, 0.5, abl::SamplesRange::allSamples());
	wrapper.audioBufferView.incrementIndex();
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == (TestType(i) / 2) + i + 3);
		}
	}

	const size_t rangeFrom = 2;
	const size_t rangeCount = 4;
	auto rangeWrapper = DelayedCircularAudioBufferViewWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, delayInSamples, startOffset, 3);
	rangeWrapper.audioBufferView.addWithRampFrom(wrapperCopy.audioBufferView, 0.0, 0.5, abl::SamplesRange(rangeFrom, rangeCount));
	rangeWrapper.audioBufferView.incrementIndex();
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(rangeWrapper.audioBufferView.getSample(channel, i) == (i >= rangeFrom && i < rangeFrom + rangeCount ? i - rangeFrom : 0) + 3);
		}
	}
}

TEMPLATE_TEST_CASE("[DelayedCircularAudioBufferView] Channel buffer data can be added from another buffer with ramp", "[DelayedCircularAudioBufferView]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 8;
	const size_t startOffset = 20;
	auto wrapper = DelayedCircularAudioBufferViewWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, delayInSamples, startOffset, 3);
	auto wrapperCopy = DelayedCircularAudioBufferViewWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, 0, startOffset, 8);
	auto rangeWrapper = DelayedCircularAudioBufferViewWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, delayInSamples, startOffset, 3);

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == 3);
		}

		wrapper.audioBufferView.resetIndex();
		wrapper.audioBufferView.addIntoChannelWithRampFrom(wrapperCopy.audioBufferView.getChannelView(channel), channel, 0.0, 1.0, abl::SamplesRange::allSamples());
		wrapper.audioBufferView.incrementIndex();
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == i + 3);
		}

		wrapper.audioBufferView.resetIndex();
		wrapper.audioBufferView.addIntoChannelWithRampFrom(wrapperCopy.audioBufferView.getChannelView(channel), channel, 0.0, 0.5, abl::SamplesRange::allSamples());
		wrapper.audioBufferView.incrementIndex();
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == (TestType(i) / 2) + i + 3);
		}

		const size_t rangeFrom = 2;
		const size_t rangeCount = 4;
		rangeWrapper.audioBufferView.resetIndex();
		rangeWrapper.audioBufferView.addIntoChannelWithRampFrom(wrapperCopy.audioBufferView.getChannelView(channel), channel, 0.0, 0.5, abl::SamplesRange(rangeFrom, rangeCount));
		rangeWrapper.audioBufferView.incrementIndex();
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(rangeWrapper.audioBufferView.getSample(channel, i) == (i >= rangeFrom && i < rangeFrom + rangeCount ? i - rangeFrom : 0) + 3);
		}
	}
}

TEMPLATE_TEST_CASE("[DelayedCircularAudioBufferView] Buffer data is scaled with applyGain()", "[DelayedCircularAudioBufferView]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 8;
	const size_t startOffset = 20;
	auto wrapper = DelayedCircularAudioBufferViewWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, delayInSamples, startOffset);

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			unsigned val = i + 1 + startOffset;
			if (val > bufferSize) val -= bufferSize;
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == val + (channel * bufferSize));
		}
	}

	wrapper.audioBufferView.applyGain(0.5, abl::SamplesRange::allSamples());
	wrapper.audioBufferView.incrementIndex();
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			unsigned val = i + 1 + startOffset + delayInSamples;
			if (val > bufferSize) val -= bufferSize;
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == TestType(val + (channel * bufferSize)) / 2);
		}
	}

	wrapper.audioBufferView.resetIndex();
	wrapper.audioBufferView.applyGain(3.0, abl::SamplesRange::allSamples());
	wrapper.audioBufferView.incrementIndex();
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			unsigned val = i + 1 + startOffset + delayInSamples;
			if (val > bufferSize) val -= bufferSize;
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == (TestType(val + (channel * bufferSize)) / 2) * 3);
		}
	}

	const size_t rangeFrom = 5;
	const size_t rangeCount = 3;
	auto rangeWrapper = DelayedCircularAudioBufferViewWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, delayInSamples, startOffset);
	rangeWrapper.audioBufferView.applyGain(2.0, abl::SamplesRange(rangeFrom, rangeCount));
	rangeWrapper.audioBufferView.incrementIndex();
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			unsigned val = i + 1 + startOffset + delayInSamples;
			if (val > bufferSize) val -= bufferSize;
			val += channel * bufferSize;
			REQUIRE(rangeWrapper.audioBufferView.getSample(channel, i) == (i >= rangeFrom && i < rangeFrom + rangeCount ? val * 2 : val));
		}
	}
}

TEMPLATE_TEST_CASE("[DelayedCircularAudioBufferView] Channel buffer data is scaled with applyGainToChannel()", "[DelayedCircularAudioBufferView]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 8;
	const size_t startOffset = 20;
	auto wrapper = DelayedCircularAudioBufferViewWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, delayInSamples, startOffset);
	auto rangeWrapper = DelayedCircularAudioBufferViewWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, delayInSamples, startOffset);

	for (size_t channel = 0; channel < channels; ++channel) {
		wrapper.audioBufferView.resetIndex();
		for (size_t i = 0; i < singleBufferSize; ++i) {
			unsigned val = i + 1 + startOffset;
			if (val > bufferSize) val -= bufferSize;
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == val + (channel * bufferSize));
		}

		wrapper.audioBufferView.applyGainToChannel(0.5, channel, abl::SamplesRange::allSamples());
		wrapper.audioBufferView.incrementIndex();
		for (size_t i = 0; i < singleBufferSize; ++i) {
			unsigned val = i + 1 + startOffset + delayInSamples;
			if (val > bufferSize) val -= bufferSize;
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == TestType(val + (channel * bufferSize)) / 2);
		}

		wrapper.audioBufferView.resetIndex();
		wrapper.audioBufferView.applyGainToChannel(3.0, channel, abl::SamplesRange::allSamples());
		wrapper.audioBufferView.incrementIndex();
		for (size_t i = 0; i < singleBufferSize; ++i) {
			unsigned val = i + 1 + startOffset + delayInSamples;
			if (val > bufferSize) val -= bufferSize;
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == (TestType(val + (channel * bufferSize)) / 2) * 3);
		}

		const size_t rangeFrom = 5;
		const size_t rangeCount = 3;
		rangeWrapper.audioBufferView.resetIndex();
		rangeWrapper.audioBufferView.applyGainToChannel(2.0, channel, abl::SamplesRange(rangeFrom, rangeCount));
		rangeWrapper.audioBufferView.incrementIndex();
		for (size_t i = 0; i < singleBufferSize; ++i) {
			unsigned val = i + 1 + startOffset + delayInSamples;
			if (val > bufferSize) val -= bufferSize;
			val += channel * bufferSize;
			REQUIRE(rangeWrapper.audioBufferView.getSample(channel, i) == (i >= rangeFrom && i < rangeFrom + rangeCount ? val * 2 : val));
		}
	}
}

TEMPLATE_TEST_CASE("[DelayedCircularAudioBufferView] Buffer data is scaled with ramp using applyGainRamp()", "[DelayedCircularAudioBufferView]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 8;
	const size_t startOffset = 20;
	auto wrapper = DelayedCircularAudioBufferViewWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, delayInSamples, startOffset, 10);

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == 10);
		}
	}

	wrapper.audioBufferView.applyGainRamp(0.0, 1.0, abl::SamplesRange::allSamples());
	wrapper.audioBufferView.incrementIndex();
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == TestType(i) * 10 / singleBufferSize);
		}
	}

	wrapper.audioBufferView.resetIndex();
	wrapper.audioBufferView.applyGainRamp(0.5, 0.0, abl::SamplesRange::allSamples());
	wrapper.audioBufferView.incrementIndex();
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == (TestType(i) * 10 / singleBufferSize) * (singleBufferSize - i) / (singleBufferSize * 2));
		}
	}

	const size_t rangeFrom = 2;
	const size_t rangeCount = 4;
	auto rangeWrapper = DelayedCircularAudioBufferViewWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, delayInSamples, startOffset, 10);
	rangeWrapper.audioBufferView.applyGainRamp(0.0, 1.0, abl::SamplesRange(rangeFrom, rangeCount));
	rangeWrapper.audioBufferView.incrementIndex();
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(rangeWrapper.audioBufferView.getSample(channel, i) == (i >= rangeFrom && i < rangeFrom + rangeCount ? TestType(i - rangeFrom) * 20 / singleBufferSize : 10));
		}
	}
}

TEMPLATE_TEST_CASE("[DelayedCircularAudioBufferView] Channel buffer data is scaled with applyGainRampToChannel()", "[DelayedCircularAudioBufferView]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 8;
	const size_t startOffset = 20;
	auto wrapper = DelayedCircularAudioBufferViewWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, delayInSamples, startOffset, 10);
	auto rangeWrapper = DelayedCircularAudioBufferViewWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, delayInSamples, startOffset, 10);

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == 10);
		}

		wrapper.audioBufferView.resetIndex();
		wrapper.audioBufferView.applyGainRampToChannel(0.0, 1.0, channel, abl::SamplesRange::allSamples());
		wrapper.audioBufferView.incrementIndex();
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == TestType(i) * 10 / singleBufferSize);
		}

		wrapper.audioBufferView.resetIndex();
		wrapper.audioBufferView.applyGainRampToChannel(0.5, 0.0, channel, abl::SamplesRange::allSamples());
		wrapper.audioBufferView.incrementIndex();
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == (TestType(i) * 10 / singleBufferSize) * (singleBufferSize - i) / (singleBufferSize * 2));
		}

		const size_t rangeFrom = 2;
		const size_t rangeCount = 4;
		rangeWrapper.audioBufferView.resetIndex();
		rangeWrapper.audioBufferView.applyGainRampToChannel(0.0, 1.0, channel, abl::SamplesRange(rangeFrom, rangeCount));
		rangeWrapper.audioBufferView.incrementIndex();
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(rangeWrapper.audioBufferView.getSample(channel, i) == (i >= rangeFrom && i < rangeFrom + rangeCount ? TestType(i - rangeFrom) * 20 / singleBufferSize : 10));
		}
	}
}

TEMPLATE_TEST_CASE("[DelayedCircularAudioBufferView] Buffer data is cleared with clear()", "[DelayedCircularAudioBufferView]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 8;
	const size_t startOffset = 20;
	auto wrapper = DelayedCircularAudioBufferViewWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, delayInSamples, startOffset);

	wrapper.audioBufferView.clear(abl::SamplesRange::allSamples());

	wrapper.audioBufferView.incrementIndex();
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == 0);
		}
	}

	const size_t rangeFrom = 5;
	const size_t rangeCount = 3;
	auto rangeWrapper = DelayedCircularAudioBufferViewWrapper<TestType>::createWithFixedValue(channels, bufferSize, singleBufferSize, delayInSamples, startOffset, 4);
	rangeWrapper.audioBufferView.clear(abl::SamplesRange(rangeFrom, rangeCount));
	rangeWrapper.audioBufferView.incrementIndex();
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(rangeWrapper.audioBufferView.getSample(channel, i) == (i >= rangeFrom && i < rangeFrom + rangeCount ? 0 : 4));
		}
	}
}

TEMPLATE_TEST_CASE("[DelayedCircularAudioBufferView] Channel Buffer data is cleared with clearChannel()", "[DelayedCircularAudioBufferView]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 8;
	const size_t startOffset = 20;
	auto wrapper = DelayedCircularAudioBufferViewWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, delayInSamples, startOffset);
	auto rangeWrapper = DelayedCircularAudioBufferViewWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, delayInSamples, startOffset);

	wrapper.audioBufferView.clearChannel(1, abl::SamplesRange::allSamples());
	wrapper.audioBufferView.incrementIndex();

	for (size_t i = 0; i < singleBufferSize; ++i) {
		const unsigned val = i + 1 + startOffset + delayInSamples;
		REQUIRE(wrapper.audioBufferView.getSample(0, i) == (val > bufferSize ? val - bufferSize : val));
	}

	for (size_t i = 0; i < singleBufferSize; ++i) {
		REQUIRE(wrapper.audioBufferView.getSample(1, i) == 0);
	}

	const size_t rangeFrom = 5;
	const size_t rangeCount = 3;
	rangeWrapper.audioBufferView.clearChannel(0, abl::SamplesRange(rangeFrom, rangeCount));
	rangeWrapper.audioBufferView.incrementIndex();

	for (size_t i = 0; i < singleBufferSize; ++i) {
		unsigned val = i + 1 + startOffset + delayInSamples;
		if (val > bufferSize) val -= bufferSize;
		REQUIRE(rangeWrapper.audioBufferView.getSample(0, i) == (i >= rangeFrom && i < rangeFrom + rangeCount ? 0 : val));
	}

	for (size_t i = 0; i < singleBufferSize; ++i) {
		unsigned val = i + 1 + startOffset + delayInSamples;
		if (val > bufferSize) val -= bufferSize;
		REQUIRE(rangeWrapper.audioBufferView.getSample(1, i) == val + bufferSize);
	}
}

TEMPLATE_TEST_CASE("[DelayedCircularAudioBufferView] Buffer data is reversed with reverse()", "[DelayedCircularAudioBufferView]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 8;
	const size_t startOffset = 20;
	auto wrapper = DelayedCircularAudioBufferViewWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, delayInSamples, startOffset);
	wrapper.audioBufferView.incrementIndex();

	const auto splitIndex = bufferSize - startOffset - delayInSamples;
	std::array<int, channels> valuesBeforeSplit{};
	std::array<int, channels> lastValues{};
	for (size_t channel = 0; channel < channels; ++channel) {
		valuesBeforeSplit[channel] = wrapper.audioBufferView.getSample(channel, splitIndex - 1);
		lastValues[channel] = wrapper.audioBufferView.getSample(channel, singleBufferSize - 1);
	}
	wrapper.audioBufferView.resetIndex();
	wrapper.audioBufferView.reverse(abl::SamplesRange::allSamples());

	wrapper.audioBufferView.incrementIndex();
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == (i >= splitIndex ? valuesBeforeSplit[channel] - i + splitIndex : lastValues[channel] - i));
		}
	}

	const auto rangeFrom = 2;
	const auto rangeCount = 4;
	auto rangeWrapper = DelayedCircularAudioBufferViewWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, delayInSamples, startOffset);
	rangeWrapper.audioBufferView.incrementIndex();
	std::array<int, channels> rangeLastValues{};
	for (size_t channel = 0; channel < channels; ++channel) {
		rangeLastValues[channel] = rangeWrapper.audioBufferView.getSample(channel, rangeFrom + rangeCount - 1);
	}
	rangeWrapper.audioBufferView.resetIndex();
	rangeWrapper.audioBufferView.reverse(abl::SamplesRange(rangeFrom, rangeCount));
	rangeWrapper.audioBufferView.incrementIndex();

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			if (i >= rangeFrom && i < rangeFrom + rangeCount) {
				REQUIRE(rangeWrapper.audioBufferView.getSample(channel, i) == (i >= splitIndex ? valuesBeforeSplit[channel] - i + splitIndex : rangeLastValues[channel] - i + rangeFrom));
			} else {
				unsigned val = i + 1 + startOffset + delayInSamples;
				REQUIRE(rangeWrapper.audioBufferView.getSample(channel, i) == (val > bufferSize ? val - bufferSize : val) + (bufferSize * channel));
			}
		}
	}
}

TEMPLATE_TEST_CASE("[DelayedCircularAudioBufferView] Channel Buffer data is reversed with reverse()", "[DelayedCircularAudioBufferView]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 8;
	const size_t startOffset = 20;
	auto wrapper = DelayedCircularAudioBufferViewWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, delayInSamples, startOffset);
	wrapper.audioBufferView.incrementIndex();

	auto splitIndex = bufferSize - startOffset - delayInSamples;
	std::array<int, channels> valuesBeforeSplit{};
	std::array<int, channels> lastValues{};
	for (size_t channel = 0; channel < channels; ++channel) {
		valuesBeforeSplit[channel] = wrapper.audioBufferView.getSample(channel, splitIndex - 1);
		lastValues[channel] = wrapper.audioBufferView.getSample(channel, singleBufferSize - 1);
	}

	wrapper.audioBufferView.resetIndex();
	wrapper.audioBufferView.reverseChannel(1, abl::SamplesRange::allSamples());

	wrapper.audioBufferView.incrementIndex();
	for (size_t i = 0; i < singleBufferSize; ++i) {
		const unsigned val = i + 1 + startOffset + delayInSamples;
		REQUIRE(wrapper.audioBufferView.getSample(0, i) == (val > bufferSize ? val - bufferSize : val));
	}

	for (size_t i = 0; i < singleBufferSize; ++i) {
		REQUIRE(wrapper.audioBufferView.getSample(1, i) == (i >= splitIndex ? valuesBeforeSplit[1] - i + splitIndex : lastValues[1] - i));
	}

	const auto rangeFrom = 2;
	const auto rangeCount = 4;
	auto rangeWrapper = DelayedCircularAudioBufferViewWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, delayInSamples, startOffset);
	rangeWrapper.audioBufferView.incrementIndex();
	std::array<int, channels> rangeLastValues{};
	for (size_t channel = 0; channel < channels; ++channel) {
		rangeLastValues[channel] = rangeWrapper.audioBufferView.getSample(channel, rangeFrom + rangeCount - 1);
	}
	rangeWrapper.audioBufferView.resetIndex();
	rangeWrapper.audioBufferView.reverseChannel(0, abl::SamplesRange(rangeFrom, rangeCount));
	rangeWrapper.audioBufferView.incrementIndex();

	for (size_t i = 0; i < singleBufferSize; ++i) {
		if (i >= rangeFrom && i < rangeFrom + rangeCount) {
			REQUIRE(rangeWrapper.audioBufferView.getSample(0, i) == (i >= splitIndex ? valuesBeforeSplit[0] - i + splitIndex : rangeLastValues[0] - i + rangeFrom));
		} else {
			const unsigned val = i + 1 + startOffset + delayInSamples;
			REQUIRE(rangeWrapper.audioBufferView.getSample(0, i) == (val > bufferSize ? val - bufferSize : val));
		}
	}

	for (size_t i = 0; i < singleBufferSize; ++i) {
		const unsigned val = i + 1 + startOffset + delayInSamples;
		REQUIRE(rangeWrapper.audioBufferView.getSample(1, i) == (val > bufferSize ? val - bufferSize : val) + bufferSize);
	}
}

TEMPLATE_TEST_CASE("[DelayedCircularAudioBufferView] View report correct higher peak", "[DelayedCircularAudioBufferView]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 8;
	const size_t startOffset = 20;
	auto wrapper = DelayedCircularAudioBufferViewWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, delayInSamples, startOffset);
	wrapper.audioBufferView.incrementIndex();

	REQUIRE(wrapper.audioBufferView.getHigherPeak(abl::SamplesRange::allSamples()) == 64);
	REQUIRE(wrapper.audioBufferView.getHigherPeak(abl::SamplesRange(3, 3)) == 64);
	REQUIRE(wrapper.audioBufferView.getHigherPeak(abl::SamplesRange(5, 2)) == 35);
}

TEMPLATE_TEST_CASE("[DelayedCircularAudioBufferView] View report correct higher peak for channels", "[DelayedCircularAudioBufferView]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 8;
	const size_t startOffset = 20;
	auto wrapper = DelayedCircularAudioBufferViewWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, delayInSamples, startOffset);
	wrapper.audioBufferView.incrementIndex();

	for (size_t channel = 0; channel < channels; ++channel) {
		REQUIRE(wrapper.audioBufferView.getHigherPeakForChannel(channel, abl::SamplesRange::allSamples()) == bufferSize * (channel + 1));
		REQUIRE(wrapper.audioBufferView.getHigherPeakForChannel(channel, abl::SamplesRange(3, 3)) == bufferSize * (channel + 1));
		REQUIRE(wrapper.audioBufferView.getHigherPeakForChannel(channel, abl::SamplesRange(5, 2)) == 3 + (bufferSize * channel));
	}
}

TEMPLATE_TEST_CASE("[DelayedCircularAudioBufferView] View report correct rms level for channels", "[DelayedCircularAudioBufferView]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 8;
	const size_t startOffset = 20;
	auto wrapper = DelayedCircularAudioBufferViewWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, delayInSamples, startOffset);
	wrapper.audioBufferView.incrementIndex();

	REQUIRE(wrapper.audioBufferView.getRMSLevelForChannel(0, abl::SamplesRange::allSamples()) == TestType(16.5));
	REQUIRE(wrapper.audioBufferView.getRMSLevelForChannel(1, abl::SamplesRange::allSamples()) == TestType(48.5));
	REQUIRE(wrapper.audioBufferView.getRMSLevelForChannel(0, abl::SamplesRange(3, 4)) == TestType(9.5));
	REQUIRE(wrapper.audioBufferView.getRMSLevelForChannel(1, abl::SamplesRange(4, 2)) == TestType(33.5)); //33 + 34 / 2
}

TEMPLATE_TEST_CASE("[DelayedCircularAudioBufferView] View report correct buffer size and channel count", "[DelayedCircularAudioBufferView]", int, double) {
	const size_t channels = 2;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 8;
	const size_t startOffset = 20;
	auto wrapper = DelayedCircularAudioBufferViewWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, delayInSamples, startOffset);
	wrapper.audioBufferView.incrementIndex();

	REQUIRE(wrapper.audioBufferView.getBufferSize() == singleBufferSize);
	REQUIRE(wrapper.audioBufferView.getChannelsCount() == channels);
}

TEMPLATE_TEST_CASE("[DelayedCircularAudioBufferView] Channels are mapped correctly", "[DelayedCircularAudioBufferView]", int, double) {
	const size_t channels = 4;
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t delayInSamples = 8;
	const size_t startOffset = 20;
	std::vector<size_t> channelsMapping{3, 1, 2, 0, 1};
	auto wrapper = DelayedCircularAudioBufferViewWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, singleBufferSize, delayInSamples, startOffset, channelsMapping);

	REQUIRE(wrapper.audioBufferView.getChannelsCount() == 5);
	size_t channel = 0;
	for (auto mappedChannel: channelsMapping) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			const auto val = i + 1 + startOffset;
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == (val > bufferSize ? val - bufferSize : val) + (mappedChannel * bufferSize));
		}
		++channel;
	}

	channelsMapping = {0, 2};
	wrapper.audioBufferView.setChannelsMapping(channelsMapping);
	REQUIRE(wrapper.audioBufferView.getChannelsCount() == 2);
	channel = 0;
	for (auto mappedChannel: channelsMapping) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			const auto val = i + 1 + startOffset;
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == (val > bufferSize ? val - bufferSize : val) + (mappedChannel * bufferSize));
		}
		++channel;
	}

	wrapper.audioBufferView.createSequentialChannelsMapping(1, 3);
	REQUIRE(wrapper.audioBufferView.getChannelsCount() == 3);
	auto sequentialChannelsMapping = wrapper.audioBufferView.getChannelsMapping();
	for (size_t i = 0; i < 3; ++i) {
		REQUIRE(sequentialChannelsMapping[i] == i + 1);
	}

	channel = 0;
	for (auto mappedChannel: sequentialChannelsMapping) {
		for (size_t i = 0; i < singleBufferSize; ++i) {
			const auto val = i + 1 + startOffset;
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == (val > bufferSize ? val - bufferSize : val) + (mappedChannel * bufferSize));
		}
		++channel;
	}
}


//test channel out-of-bound assert?