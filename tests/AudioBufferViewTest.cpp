// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "../buffers/AudioBufferView.h"
#include "../buffers/AudioBufferChannelView.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_template_test_macros.hpp>

template<typename T>
struct AudioBufferViewWrapper {
	T **data;
	size_t m_channels;
	abl::AudioBufferView<T> audioBufferView;

	static AudioBufferViewWrapper createWithIncrementalNumbers(size_t channels, size_t bufferSize, const std::vector<size_t> &channelsMapping = {}) {
		auto data = new T *[channels];
		size_t startValue = 1;
		for (size_t channel = 0; channel < channels; ++channel) {
			data[channel] = new T[bufferSize];
			std::iota(data[channel], data[channel] + bufferSize, startValue);
			startValue += bufferSize;
		}

		return AudioBufferViewWrapper{data, channels, bufferSize, channelsMapping};
	}

	static AudioBufferViewWrapper createWithFixedValue(size_t channels, size_t bufferSize, T fixedValue, const std::vector<size_t> &channelsMapping = {}) {
		auto data = new T *[channels];
		for (size_t channel = 0; channel < channels; ++channel) {
			data[channel] = new T[bufferSize];
			std::fill(data[channel], data[channel] + bufferSize, fixedValue);
		}

		return AudioBufferViewWrapper{data, channels, bufferSize, channelsMapping};
	}

	AudioBufferViewWrapper(T **newData, size_t channels, size_t bufferSize, const std::vector<size_t> &channelsMapping = {}) : data{newData}, m_channels{channels},
		audioBufferView{abl::AudioBufferView{data, channels, bufferSize, channelsMapping}} {}

	~AudioBufferViewWrapper() {
		for (size_t channel = 0; channel < m_channels; ++channel) {
			delete[] data[channel];
		}
		delete[] data;
	}
};

TEST_CASE("[AudioBufferView] View report correct empty state", "[AudioBufferView]") {
	auto wrapper = AudioBufferViewWrapper<int>::createWithFixedValue(2, 8, 0);
	auto emptyWrapper = AudioBufferViewWrapper<double>::createWithFixedValue(0, 0, 0);
	auto emptyWrapper2 = AudioBufferViewWrapper<int>::createWithFixedValue(1, 0, 0);

	REQUIRE_FALSE(wrapper.audioBufferView.isEmpty());
	REQUIRE(emptyWrapper.audioBufferView.isEmpty());
	REQUIRE(emptyWrapper2.audioBufferView.isEmpty());
}

TEMPLATE_TEST_CASE("[AudioBufferView] Buffer data is iterable", "[AudioBufferView]", int, double) {
	size_t channels = 2;
	size_t bufferSize = 8;
	auto wrapper = AudioBufferViewWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize);

	size_t i, j = 0;
	for (auto&& channelView: wrapper.audioBufferView) {
		i = 0;
		for (auto &&sample: channelView) {
			REQUIRE(sample == ++i + (j * bufferSize));
		}
		++j;
	}
	REQUIRE(j == channels);
}

TEMPLATE_TEST_CASE("[AudioBufferView] BufferChannelView is returned accessing a channel with the operator[] and getChannelView()", "[AudioBufferView]", int, double) {
	size_t channels = 2;
	size_t bufferSize = 8;
	auto wrapper = AudioBufferViewWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize);

	size_t i;
	for (size_t channel = 0; channel < channels; ++channel) {
		auto channelView = wrapper.audioBufferView[channel];
		REQUIRE(channelView.getBufferSize() == bufferSize);
		i = 0;
		for (auto &&sample: channelView) {
			REQUIRE(sample == ++i + (channel * bufferSize));
		}

		auto channelView2 = wrapper.audioBufferView.getChannelView(channel);
		REQUIRE(channelView2.getBufferSize() == bufferSize);
		i = 0;
		for (auto &&sample: channelView2) {
			REQUIRE(sample == ++i + (channel * bufferSize));
		}
	}
}

TEMPLATE_TEST_CASE("[AudioBufferView] Valid ranged BufferChannelView is returned accessing a channel passing a sampleRange to getChannelView()", "[AudioBufferView]", int, double) {
	size_t channels = 2;
	size_t bufferSize = 8;
	auto wrapper = AudioBufferViewWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize);
	abl::SamplesRange samplesRange{2, 4};

	size_t i;
	for (size_t channel = 0; channel < channels; ++channel) {
		auto channelView = wrapper.audioBufferView.getChannelView(channel, samplesRange);
		REQUIRE(channelView.getBufferSize() == 4);
		i = 2;
		for (auto &&sample: channelView) {
			REQUIRE(sample == ++i + (channel * bufferSize));
		}

		REQUIRE(i == 6);
	}
}

TEMPLATE_TEST_CASE("[AudioBufferView] BufferView can be copied and moved", "[AudioBufferView]", int, double) {
	size_t channels = 2;
	size_t bufferSize = 8;
	auto wrapper = AudioBufferViewWrapper<TestType>::createWithFixedValue(channels, bufferSize, 3);

	abl::AudioBufferView<TestType> copyBuffer{wrapper.audioBufferView};

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(copyBuffer.getSample(channel, i) == 3);
		}
	}

	REQUIRE(!copyBuffer.isEmpty());
	abl::AudioBufferView<TestType> moveBuffer{std::move(copyBuffer)};

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(moveBuffer.getSample(channel, i) == 3);
		}
	}

	REQUIRE(copyBuffer.isEmpty());
}

TEMPLATE_TEST_CASE("[AudioBufferView] Valid ranged BufferView is returned from getRangedView()", "[AudioBufferView]", int, double) {
	size_t channels = 2;
	size_t bufferSize = 8;
	auto wrapper = AudioBufferViewWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize);
	auto rangedBufferView = wrapper.audioBufferView.getRangedView(abl::SamplesRange{1, 5});

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

TEMPLATE_TEST_CASE("[AudioBufferView] Buffer data is accessible with getSample()", "[AudioBufferView]", int, double) {
	size_t channels = 2;
	size_t bufferSize = 8;
	auto wrapper = AudioBufferViewWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize);

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == i + 1 + (channel * bufferSize));
		}
	}
}

TEMPLATE_TEST_CASE("[AudioBufferView] Buffer data can be set with setSample() and addSample()", "[AudioBufferView]", int, double) {
	size_t channels = 2;
	size_t bufferSize = 8;
	auto wrapper = AudioBufferViewWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize);

	wrapper.audioBufferView.setSample(0, 0, 20);
	REQUIRE(wrapper.audioBufferView.getSample(0, 0) == 20);

	wrapper.audioBufferView.addSample(1, 2, 20);
	REQUIRE(wrapper.audioBufferView.getSample(1, 2) == 31); //11 + 20
}

TEMPLATE_TEST_CASE("[AudioBufferView] Buffer data can be copied from another buffer", "[AudioBufferView]", int, double) {
	size_t channels = 2;
	size_t bufferSize = 8;
	auto wrapper = AudioBufferViewWrapper<TestType>::createWithFixedValue(channels, bufferSize, 0);
	auto wrapperCopy = AudioBufferViewWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize);

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == 0);
		}
	}

	wrapper.audioBufferView.copyFrom(wrapperCopy.audioBufferView, abl::SamplesRange::allSamples());
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == i + 1 + (channel * bufferSize));
		}
	}

	const size_t rangeFrom = 2;
	const size_t rangeCount = 4;
	wrapper.audioBufferView.copyFrom(wrapperCopy.audioBufferView, abl::SamplesRange(rangeFrom, rangeCount));
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == (i >= rangeFrom && i < rangeFrom + rangeCount ? 1 - rangeFrom : 1) + i + (channel * bufferSize));
		}
	}

	wrapper.audioBufferView.copyFrom(wrapperCopy.audioBufferView, abl::SamplesRange::allSamples(), 0.5);
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == TestType(i + 1 + (channel * bufferSize)) / 2);
		}
	}
}

TEMPLATE_TEST_CASE("[AudioBufferView] Channel buffer data can be copied from another channel buffer", "[AudioBufferView]", int, double) {
	size_t channels = 2;
	size_t bufferSize = 8;
	auto wrapper = AudioBufferViewWrapper<TestType>::createWithFixedValue(channels, bufferSize, 0);
	auto wrapperCopy = AudioBufferViewWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize);

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == 0);
		}

		wrapper.audioBufferView.copyIntoChannelFrom(wrapperCopy.audioBufferView.getChannelView(channel), channel, abl::SamplesRange::allSamples());
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == i + 1 + (channel * bufferSize));
		}

		const size_t rangeFrom = 2;
		const size_t rangeCount = 4;
		wrapper.audioBufferView.copyIntoChannelFrom(wrapperCopy.audioBufferView.getChannelView(channel), channel, abl::SamplesRange(rangeFrom, rangeCount));
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == (i >= rangeFrom && i < rangeFrom + rangeCount ? 1 - rangeFrom : 1) + i + (channel * bufferSize));
		}

		wrapper.audioBufferView.copyIntoChannelFrom(wrapperCopy.audioBufferView.getChannelView(channel), channel, abl::SamplesRange::allSamples(), 0.5);
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == TestType(i + 1 + (channel * bufferSize)) / 2);
		}
	}
}

TEMPLATE_TEST_CASE("[AudioBufferView] Buffer data can be copied from another buffer with ramp", "[AudioBufferView]", int, double) {
	size_t channels = 2;
	size_t bufferSize = 8;
	auto wrapper = AudioBufferViewWrapper<TestType>::createWithFixedValue(channels, bufferSize, 0);
	auto wrapperCopy = AudioBufferViewWrapper<TestType>::createWithFixedValue(channels, bufferSize, 8);

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == 0);
		}
	}

	wrapper.audioBufferView.copyWithRampFrom(wrapperCopy.audioBufferView, 0.0, 1.0, abl::SamplesRange::allSamples());
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == i);
		}
	}

	const size_t rangeFrom = 1;
	const size_t rangeCount = 4;
	wrapper.audioBufferView.copyWithRampFrom(wrapperCopy.audioBufferView, 0.0, 1.0, abl::SamplesRange(rangeFrom, rangeCount));
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			unsigned val = i >= rangeFrom && i < rangeFrom + rangeCount ? (i - rangeFrom) * 2 : i;
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == val);
		}
	}

	wrapper.audioBufferView.copyWithRampFrom(wrapperCopy.audioBufferView, 0.0, 0.5, abl::SamplesRange::allSamples());
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == TestType(i) / 2);
		}
	}
}

TEMPLATE_TEST_CASE("[AudioBufferView] Channel buffer data can be copied from another buffer with ramp", "[AudioBufferView]", int, double) {
	size_t channels = 2;
	size_t bufferSize = 8;
	auto wrapper = AudioBufferViewWrapper<TestType>::createWithFixedValue(channels, bufferSize, 0);
	auto wrapperCopy = AudioBufferViewWrapper<TestType>::createWithFixedValue(channels, bufferSize, 8);

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == 0);
		}

		wrapper.audioBufferView.copyIntoChannelWithRampFrom(wrapperCopy.audioBufferView.getChannelView(channel), channel, 0.0, 1.0, abl::SamplesRange::allSamples());
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == i);
		}

		const size_t rangeFrom = 1;
		const size_t rangeCount = 4;
		wrapper.audioBufferView.copyIntoChannelWithRampFrom(wrapperCopy.audioBufferView.getChannelView(channel), channel, 0.0, 1.0, abl::SamplesRange(rangeFrom, rangeCount));
		for (size_t i = 0; i < bufferSize; ++i) {
			auto val = i >= rangeFrom && i < rangeFrom + rangeCount ? (i - rangeFrom) * 2 : i;
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == val);
		}

		wrapper.audioBufferView.copyIntoChannelWithRampFrom(wrapperCopy.audioBufferView.getChannelView(channel), channel, 0.0, 0.5, abl::SamplesRange::allSamples());
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == TestType(i) / 2);
		}
	}
}

TEMPLATE_TEST_CASE("[AudioBufferView] Buffer data can be added from another buffer", "[AudioBufferView]", int, double) {
	size_t channels = 2;
	size_t bufferSize = 8;
	auto wrapper = AudioBufferViewWrapper<TestType>::createWithFixedValue(channels, bufferSize, 2);
	auto wrapperCopy = AudioBufferViewWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize);

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == 2);
		}
	}

	wrapper.audioBufferView.addFrom(wrapperCopy.audioBufferView, abl::SamplesRange::allSamples());
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == i + 1 + (channel * bufferSize) + 2);
		}
	}

	wrapper.audioBufferView.addFrom(wrapperCopy.audioBufferView, abl::SamplesRange::allSamples(), 0.5);
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == (TestType(i + 1 + (channel * bufferSize)) / 2) + (i + 1) + 2 + (channel * bufferSize));
		}
	}

	const size_t rangeFrom = 3;
	const size_t rangeCount = 4;
	auto rangeWrapper = AudioBufferViewWrapper<TestType>::createWithFixedValue(channels, bufferSize, 2);
	rangeWrapper.audioBufferView.addFrom(wrapperCopy.audioBufferView, abl::SamplesRange(rangeFrom, rangeCount));
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(rangeWrapper.audioBufferView.getSample(channel, i) == ((i >= rangeFrom && i < rangeFrom + rangeCount) ? i + 3 - rangeFrom + channel * bufferSize : 2));
		}
	}
}

TEMPLATE_TEST_CASE("[AudioBufferView] Channel buffer data can be added from another channel buffer", "[AudioBufferView]", int, double) {
	size_t channels = 2;
	size_t bufferSize = 8;
	auto wrapper = AudioBufferViewWrapper<TestType>::createWithFixedValue(channels, bufferSize, 2);
	auto wrapperCopy = AudioBufferViewWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize);
	auto rangeWrapper = AudioBufferViewWrapper<TestType>::createWithFixedValue(channels, bufferSize, 2);

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == 2);
		}

		wrapper.audioBufferView.addIntoChannelFrom(wrapperCopy.audioBufferView.getChannelView(channel), channel, abl::SamplesRange::allSamples());
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == i + 1 + (channel * bufferSize) + 2);
		}

		wrapper.audioBufferView.addIntoChannelFrom(wrapperCopy.audioBufferView.getChannelView(channel), channel, abl::SamplesRange::allSamples(), 0.5);
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == (TestType(i + 1 + (channel * bufferSize)) / 2) + (i + 1) + 2 + (channel * bufferSize));
		}

		const size_t rangeFrom = 3;
		const size_t rangeCount = 4;
		rangeWrapper.audioBufferView.addIntoChannelFrom(wrapperCopy.audioBufferView.getChannelView(channel), channel, abl::SamplesRange(rangeFrom, rangeCount));
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(rangeWrapper.audioBufferView.getSample(channel, i) == ((i >= rangeFrom && i < rangeFrom + rangeCount) ? i + 3 - rangeFrom + channel * bufferSize : 2));
		}
	}
}

TEMPLATE_TEST_CASE("[AudioBufferView] Buffer data can be added from another buffer with ramp", "[AudioBufferView]", int, double) {
	size_t channels = 2;
	size_t bufferSize = 8;
	auto wrapper = AudioBufferViewWrapper<TestType>::createWithFixedValue(channels, bufferSize, 3);
	auto wrapperCopy = AudioBufferViewWrapper<TestType>::createWithFixedValue(channels, bufferSize, 8);

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == 3);
		}
	}

	wrapper.audioBufferView.addWithRampFrom(wrapperCopy.audioBufferView, 0.0, 1.0, abl::SamplesRange::allSamples());
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == i + 3);
		}
	}

	wrapper.audioBufferView.addWithRampFrom(wrapperCopy.audioBufferView, 0.0, 0.5, abl::SamplesRange::allSamples());
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == (TestType(i) / 2) + i + 3);
		}
	}

	const size_t rangeFrom = 2;
	const size_t rangeCount = 4;
	auto rangeWrapper = AudioBufferViewWrapper<TestType>::createWithFixedValue(channels, bufferSize, 3);
	rangeWrapper.audioBufferView.addWithRampFrom(wrapperCopy.audioBufferView, 0.0, 0.5, abl::SamplesRange(rangeFrom, rangeCount));
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(rangeWrapper.audioBufferView.getSample(channel, i) == (i >= rangeFrom && i < rangeFrom + rangeCount ? i - rangeFrom : 0) + 3);
		}
	}
}

TEMPLATE_TEST_CASE("[AudioBufferView] Channel buffer data can be added from another buffer with ramp", "[AudioBufferView]", int, double) {
	size_t channels = 2;
	size_t bufferSize = 8;
	auto wrapper = AudioBufferViewWrapper<TestType>::createWithFixedValue(channels, bufferSize, 3);
	auto wrapperCopy = AudioBufferViewWrapper<TestType>::createWithFixedValue(channels, bufferSize, 8);
	auto rangeWrapper = AudioBufferViewWrapper<TestType>::createWithFixedValue(channels, bufferSize, 3);

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == 3);
		}

		wrapper.audioBufferView.addIntoChannelWithRampFrom(wrapperCopy.audioBufferView.getChannelView(channel), channel, 0.0, 1.0, abl::SamplesRange::allSamples());
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == i + 3);
		}

		wrapper.audioBufferView.addIntoChannelWithRampFrom(wrapperCopy.audioBufferView.getChannelView(channel), channel, 0.0, 0.5, abl::SamplesRange::allSamples());
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == (TestType(i) / 2) + i + 3);
		}

		const size_t rangeFrom = 2;
		const size_t rangeCount = 4;
		rangeWrapper.audioBufferView.addIntoChannelWithRampFrom(wrapperCopy.audioBufferView.getChannelView(channel), channel, 0.0, 0.5, abl::SamplesRange(rangeFrom, rangeCount));
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(rangeWrapper.audioBufferView.getSample(channel, i) == (i >= rangeFrom && i < rangeFrom + rangeCount ? i - rangeFrom : 0) + 3);
		}
	}
}

TEMPLATE_TEST_CASE("[AudioBufferView] Buffer data is scaled with applyGain()", "[AudioBufferView]", int, double) {
	size_t channels = 2;
	size_t bufferSize = 8;
	auto wrapper = AudioBufferViewWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize);

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == i + 1 + (channel * bufferSize));
		}
	}

	wrapper.audioBufferView.applyGain(0.5, abl::SamplesRange::allSamples());
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == TestType(i + 1 + (channel * bufferSize)) / 2);
		}
	}

	wrapper.audioBufferView.applyGain(3.0, abl::SamplesRange::allSamples());
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == (TestType(i + 1 + (channel * bufferSize)) / 2) * 3);
		}
	}

	const size_t rangeFrom = 5;
	const size_t rangeCount = 3;
	auto rangeWrapper = AudioBufferViewWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize);
	rangeWrapper.audioBufferView.applyGain(2.0, abl::SamplesRange(rangeFrom, rangeCount));
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			unsigned val = i + 1 + channel * bufferSize;
			REQUIRE(rangeWrapper.audioBufferView.getSample(channel, i) == (i >= rangeFrom && i < rangeFrom + rangeCount ? val * 2 : val));
		}
	}
}

TEMPLATE_TEST_CASE("[AudioBufferView] Channel buffer data is scaled with applyGainToChannel()", "[AudioBufferView]", int, double) {
	size_t channels = 2;
	size_t bufferSize = 8;
	auto wrapper = AudioBufferViewWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize);
	auto rangeWrapper = AudioBufferViewWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize);

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == i + 1 + (channel * bufferSize));
		}

		wrapper.audioBufferView.applyGainToChannel(0.5, channel, abl::SamplesRange::allSamples());
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == TestType(i + 1 + (channel * bufferSize)) / 2);
		}

		wrapper.audioBufferView.applyGainToChannel(3.0, channel, abl::SamplesRange::allSamples());
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == (TestType(i + 1 + (channel * bufferSize)) / 2) * 3);
		}

		const size_t rangeFrom = 5;
		const size_t rangeCount = 3;
		rangeWrapper.audioBufferView.applyGainToChannel(2.0, channel, abl::SamplesRange(rangeFrom, rangeCount));
		for (size_t i = 0; i < bufferSize; ++i) {
			unsigned val = i + 1 + channel * bufferSize;
			REQUIRE(rangeWrapper.audioBufferView.getSample(channel, i) == (i >= rangeFrom && i < rangeFrom + rangeCount ? val * 2 : val));
		}
	}
}

TEMPLATE_TEST_CASE("[AudioBufferView] Buffer data is scaled with ramp using applyGainRamp()", "[AudioBufferView]", int, double) {
	size_t channels = 2;
	size_t bufferSize = 8;
	auto wrapper = AudioBufferViewWrapper<TestType>::createWithFixedValue(channels, bufferSize, 10);

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == 10);
		}
	}

	wrapper.audioBufferView.applyGainRamp(0.0, 1.0, abl::SamplesRange::allSamples());
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == TestType(i) * 10 / bufferSize);
		}
	}

	wrapper.audioBufferView.applyGainRamp(0.5, 0.0, abl::SamplesRange::allSamples());
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == (TestType(i) * 10 / bufferSize) * (bufferSize - i) / (bufferSize * 2));
		}
	}

	const size_t rangeFrom = 2;
	const size_t rangeCount = 4;
	auto rangeWrapper = AudioBufferViewWrapper<TestType>::createWithFixedValue(channels, bufferSize, 10);
	rangeWrapper.audioBufferView.applyGainRamp(0.0, 1.0, abl::SamplesRange(rangeFrom, rangeCount));
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(rangeWrapper.audioBufferView.getSample(channel, i) == (i >= rangeFrom && i < rangeFrom + rangeCount ? TestType(i - rangeFrom) * 20 / bufferSize : 10));
		}
	}
}

TEMPLATE_TEST_CASE("[AudioBufferView] Channel buffer data is scaled with applyGainRampToChannel()", "[AudioBufferView]", int, double) {
	size_t channels = 2;
	size_t bufferSize = 8;
	auto wrapper = AudioBufferViewWrapper<TestType>::createWithFixedValue(channels, bufferSize, 10);
	auto rangeWrapper = AudioBufferViewWrapper<TestType>::createWithFixedValue(channels, bufferSize, 10);

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == 10);
		}

		wrapper.audioBufferView.applyGainRampToChannel(0.0, 1.0, channel, abl::SamplesRange::allSamples());
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == TestType(i) * 10 / bufferSize);
		}

		wrapper.audioBufferView.applyGainRampToChannel(0.5, 0.0, channel, abl::SamplesRange::allSamples());
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == (TestType(i) * 10 / bufferSize) * (bufferSize - i) / (bufferSize * 2));
		}

		const size_t rangeFrom = 2;
		const size_t rangeCount = 4;
		rangeWrapper.audioBufferView.applyGainRampToChannel(0.0, 1.0, channel, abl::SamplesRange(rangeFrom, rangeCount));
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(rangeWrapper.audioBufferView.getSample(channel, i) == (i >= rangeFrom && i < rangeFrom + rangeCount ? TestType(i - rangeFrom) * 20 / bufferSize : 10));
		}
	}
}

TEMPLATE_TEST_CASE("[AudioBufferView] Buffer data is cleared with clear()", "[AudioBufferView]", int, double) {
	size_t channels = 2;
	size_t bufferSize = 8;
	auto wrapper = AudioBufferViewWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize);

	wrapper.audioBufferView.clear(abl::SamplesRange::allSamples());

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == 0);
		}
	}

	const size_t rangeFrom = 5;
	const size_t rangeCount = 3;
	auto rangeWrapper = AudioBufferViewWrapper<TestType>::createWithFixedValue(channels, bufferSize, 4);
	rangeWrapper.audioBufferView.clear(abl::SamplesRange(rangeFrom, rangeCount));
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(rangeWrapper.audioBufferView.getSample(channel, i) == (i >= rangeFrom && i < rangeFrom + rangeCount ? 0 : 4));
		}
	}
}

TEMPLATE_TEST_CASE("[AudioBufferView] Channel Buffer data is cleared with clearChannel()", "[AudioBufferView]", int, double) {
	size_t channels = 2;
	size_t bufferSize = 8;
	auto wrapper = AudioBufferViewWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize);
	auto rangeWrapper = AudioBufferViewWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize);

	wrapper.audioBufferView.clearChannel(1, abl::SamplesRange::allSamples());

	for (size_t i = 0; i < bufferSize; ++i) {
		REQUIRE(wrapper.audioBufferView.getSample(0, i) == i + 1);
	}

	for (size_t i = 0; i < bufferSize; ++i) {
		REQUIRE(wrapper.audioBufferView.getSample(1, i) == 0);
	}

	const size_t rangeFrom = 5;
	const size_t rangeCount = 3;
	rangeWrapper.audioBufferView.clearChannel(0, abl::SamplesRange(rangeFrom, rangeCount));

	for (size_t i = 0; i < bufferSize; ++i) {
		REQUIRE(rangeWrapper.audioBufferView.getSample(0, i) == (i >= rangeFrom && i < rangeFrom + rangeCount ? 0 : i + 1));
	}

	for (size_t i = 0; i < bufferSize; ++i) {
		REQUIRE(rangeWrapper.audioBufferView.getSample(1, i) == i + 1 + bufferSize);
	}
}

TEMPLATE_TEST_CASE("[AudioBufferView] Buffer data is reversed with reverse()", "[AudioBufferView]", int, double) {
	size_t channels = 2;
	size_t bufferSize = 8;
	auto wrapper = AudioBufferViewWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize);

	wrapper.audioBufferView.reverse(abl::SamplesRange::allSamples());

	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == bufferSize * (channel + 1) - i);
		}
	}

	const auto rangeFrom = 2;
	const auto rangeCount = 4;
	auto rangeWrapper = AudioBufferViewWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize);
	rangeWrapper.audioBufferView.reverse(abl::SamplesRange(rangeFrom, rangeCount));
	for (size_t channel = 0; channel < channels; ++channel) {
		for (size_t i = 0; i < bufferSize; ++i) {
			if (i >= rangeFrom && i < rangeFrom + rangeCount) {
				REQUIRE(rangeWrapper.audioBufferView.getSample(channel, i) == bufferSize * (channel + 1) - i);
			} else {
				REQUIRE(rangeWrapper.audioBufferView.getSample(channel, i) == i + 1 + (bufferSize * channel));
			}
		}
	}
}

TEMPLATE_TEST_CASE("[AudioBufferView] Channel Buffer data is reversed with reverse()", "[AudioBufferView]", int, double) {
	size_t channels = 2;
	size_t bufferSize = 8;
	auto wrapper = AudioBufferViewWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize);

	wrapper.audioBufferView.reverseChannel(1, abl::SamplesRange::allSamples());

	for (size_t i = 0; i < bufferSize; ++i) {
		REQUIRE(wrapper.audioBufferView.getSample(0, i) == i + 1);
	}

	for (size_t i = 0; i < bufferSize; ++i) {
		REQUIRE(wrapper.audioBufferView.getSample(1, i) == 16 - i);
	}

	const auto rangeFrom = 2;
	const auto rangeCount = 4;
	auto rangeWrapper = AudioBufferViewWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize);
	rangeWrapper.audioBufferView.reverseChannel(0, abl::SamplesRange(rangeFrom, rangeCount));

	for (size_t i = 0; i < bufferSize; ++i) {
		if (i >= rangeFrom && i < rangeFrom + rangeCount) {
			REQUIRE(rangeWrapper.audioBufferView.getSample(0, i) == bufferSize - i);
		} else {
			REQUIRE(rangeWrapper.audioBufferView.getSample(0, i) == i + 1);
		}
	}

	for (size_t i = 0; i < bufferSize; ++i) {
		REQUIRE(rangeWrapper.audioBufferView.getSample(1, i) == 9 + i);
	}
}

TEMPLATE_TEST_CASE("[AudioBufferView] View report correct higher peak", "[AudioBufferView]", int, double) {
	size_t channels = 2;
	size_t bufferSize = 8;
	auto wrapper = AudioBufferViewWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize);

	REQUIRE(wrapper.audioBufferView.getHigherPeak(abl::SamplesRange::allSamples()) == 16);
	REQUIRE(wrapper.audioBufferView.getHigherPeak(abl::SamplesRange(3, 3)) == 14);
	REQUIRE(wrapper.audioBufferView.getHigherPeak(abl::SamplesRange(5, 2)) == 15);
}

TEMPLATE_TEST_CASE("[AudioBufferView] View report correct higher peak for channels", "[AudioBufferView]", int, double) {
	size_t channels = 2;
	size_t bufferSize = 8;
	auto wrapper = AudioBufferViewWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize);

	for (size_t channel = 0; channel < channels; ++channel) {
		REQUIRE(wrapper.audioBufferView.getHigherPeakForChannel(channel, abl::SamplesRange::allSamples()) == bufferSize * (channel + 1));
		REQUIRE(wrapper.audioBufferView.getHigherPeakForChannel(channel, abl::SamplesRange(3, 3)) == 6 + (bufferSize * channel));
		REQUIRE(wrapper.audioBufferView.getHigherPeakForChannel(channel, abl::SamplesRange(5, 2)) == 7 + (bufferSize * channel));
	}
}

TEMPLATE_TEST_CASE("[AudioBufferView] View report correct rms level for channels", "[AudioBufferView]", int, double) {
	size_t channels = 2;
	size_t bufferSize = 8;
	auto wrapper = AudioBufferViewWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize);

	REQUIRE(wrapper.audioBufferView.getRMSLevelForChannel(0, abl::SamplesRange::allSamples()) == TestType(4.5));
	REQUIRE(wrapper.audioBufferView.getRMSLevelForChannel(1, abl::SamplesRange::allSamples()) == TestType(12.5));
	REQUIRE(wrapper.audioBufferView.getRMSLevelForChannel(0, abl::SamplesRange(3, 4)) == TestType(5.5));
	REQUIRE(wrapper.audioBufferView.getRMSLevelForChannel(1, abl::SamplesRange(4, 2)) == TestType(13.5));
}

TEMPLATE_TEST_CASE("[AudioBufferView] View report correct buffer size and channel count", "[AudioBufferView]", int, double) {
	size_t channels = 2;
	size_t bufferSize = 8;
	auto wrapper = AudioBufferViewWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize);

	REQUIRE(wrapper.audioBufferView.getBufferSize() == bufferSize);
	REQUIRE(wrapper.audioBufferView.getChannelsCount() == channels);
}

TEMPLATE_TEST_CASE("[AudioBufferView] Channels are mapped correctly", "[AudioBufferView]", int, double) {
	size_t channels = 4;
	size_t bufferSize = 8;
	std::vector<size_t> channelsMapping{3, 1, 2, 0, 1};
	auto wrapper = AudioBufferViewWrapper<TestType>::createWithIncrementalNumbers(channels, bufferSize, channelsMapping);

	REQUIRE(wrapper.audioBufferView.getChannelsCount() == 5);
	size_t channel = 0;
	for (auto mappedChannel: channelsMapping) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == i + 1 + (mappedChannel * bufferSize));
		}
		++channel;
	}

	channelsMapping = {0, 2};
	wrapper.audioBufferView.setChannelsMapping(channelsMapping);
	REQUIRE(wrapper.audioBufferView.getChannelsCount() == 2);
	channel = 0;
	for (auto mappedChannel: channelsMapping) {
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == i + 1 + (mappedChannel * bufferSize));
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
		for (size_t i = 0; i < bufferSize; ++i) {
			REQUIRE(wrapper.audioBufferView.getSample(channel, i) == i + 1 + (mappedChannel * bufferSize));
		}
		++channel;
	}
}


//test channel out-of-bound assert?