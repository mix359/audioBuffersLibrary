// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "../buffers/AudioBufferChannelView.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_template_test_macros.hpp>

template<typename T>
struct AudioBufferChannelViewWrapper {
	T *data;
	abl::AudioBufferChannelView<T> audioBufferChannelView;

	static AudioBufferChannelViewWrapper createWithIncrementalNumbers(size_t size) {
		auto data = new T[size];
		AudioBufferChannelViewWrapper<T> wrapper{data, size};
		std::iota(data, data + size, 1);
		return wrapper;
	}

	static AudioBufferChannelViewWrapper createWithFixedValue(size_t size, T fixedValue) {
		auto data = new T[size];
		AudioBufferChannelViewWrapper<T> wrapper{data, size};
		std::fill(data, data + size, fixedValue);
		return wrapper;
	}

	AudioBufferChannelViewWrapper(T *newData, size_t size) : data{newData}, audioBufferChannelView{abl::AudioBufferChannelView{data, size}} {}

	~AudioBufferChannelViewWrapper() { delete[] data; }
};

TEST_CASE("[AudioBufferChannelView] View report correct empty state", "[AudioBufferChannelView]") {
	auto wrapper = AudioBufferChannelViewWrapper<int>::createWithIncrementalNumbers(8);
	auto emptyWrapper = AudioBufferChannelViewWrapper<int>::createWithIncrementalNumbers(0);

	REQUIRE_FALSE(wrapper.audioBufferChannelView.isEmpty());
	REQUIRE(emptyWrapper.audioBufferChannelView.isEmpty());
}

TEMPLATE_TEST_CASE("[AudioBufferChannelView] Buffer data is iterable", "[AudioBufferChannelView]", int, double) {
	const size_t bufferSize = 8;
	auto wrapper = AudioBufferChannelViewWrapper<TestType>::createWithIncrementalNumbers(bufferSize);

	size_t i = 0;
	for (auto &&data: wrapper.audioBufferChannelView) {
		REQUIRE(data == ++i);
	}
}

TEMPLATE_TEST_CASE("[AudioBufferChannelView] Buffer data is accessible with operator[] and getSample()", "[AudioBufferChannelView]", int, double) {
	const size_t bufferSize = 8;
	auto wrapper = AudioBufferChannelViewWrapper<TestType>::createWithIncrementalNumbers(bufferSize);

	for (size_t i = 0; i < bufferSize; ++i) {
		REQUIRE(wrapper.audioBufferChannelView[i] == i + 1);
		REQUIRE(wrapper.audioBufferChannelView.getSample(i) == i + 1);
	}
}

TEMPLATE_TEST_CASE("[AudioBufferChannelView] Buffer data can be set with setSample() and addSample()", "[AudioBufferChannelView]", int, double) {
	auto wrapper = AudioBufferChannelViewWrapper<TestType>::createWithIncrementalNumbers(1);
	wrapper.audioBufferChannelView.setSample(0, 20);
	REQUIRE(wrapper.audioBufferChannelView[0] == 20);

	wrapper.audioBufferChannelView.addSample(0, 20);
	REQUIRE(wrapper.audioBufferChannelView[0] == 40);
}

TEMPLATE_TEST_CASE("[AudioBufferChannelView] Buffer can be copied and moved", "[AudioBufferChannelView]", int, double) {
	const size_t bufferSize = 8;
	auto wrapper = AudioBufferChannelViewWrapper<TestType>::createWithFixedValue(bufferSize, 3);

	abl::AudioBufferChannelView<TestType> copyBuffer{wrapper.audioBufferChannelView};

	for (size_t i = 0; i < bufferSize; ++i) {
		REQUIRE(copyBuffer.getSample(i) == 3);
	}

	REQUIRE(!copyBuffer.isEmpty());
	abl::AudioBufferChannelView<TestType> moveBuffer{std::move(copyBuffer)};

	for (size_t i = 0; i < bufferSize; ++i) {
		REQUIRE(moveBuffer.getSample(i) == 3);
	}

	REQUIRE(copyBuffer.isEmpty());
}

TEMPLATE_TEST_CASE("[AudioBufferChannelView] Buffer data can be copied from another buffer", "[AudioBufferChannelView]", int, double) {
	const size_t bufferSize = 8;
	auto wrapper = AudioBufferChannelViewWrapper<TestType>::createWithFixedValue(bufferSize, 0);
	auto wrapperCopy = AudioBufferChannelViewWrapper<TestType>::createWithIncrementalNumbers(bufferSize);

	for (size_t i = 0; i < bufferSize; ++i) {
		REQUIRE(wrapper.audioBufferChannelView[i] == 0);
	}

	wrapper.audioBufferChannelView.copyFrom(wrapperCopy.audioBufferChannelView, abl::SamplesRange::allSamples());
	for (size_t i = 0; i < bufferSize; ++i) {
		REQUIRE(wrapper.audioBufferChannelView[i] == i + 1);
	}

	wrapper.audioBufferChannelView.copyFrom(wrapperCopy.audioBufferChannelView, abl::SamplesRange::allSamples(), 0.5);
	for (size_t i = 0; i < bufferSize; ++i) {
		REQUIRE(wrapper.audioBufferChannelView[i] == (i + TestType(1)) / 2);
	}

	size_t rangeFrom = 2;
	size_t rangeCount = 4;
	auto rangeWrapper = AudioBufferChannelViewWrapper<TestType>::createWithFixedValue(bufferSize, 0);
	rangeWrapper.audioBufferChannelView.copyFrom(wrapperCopy.audioBufferChannelView, abl::SamplesRange(rangeFrom, rangeCount));
	for (size_t i = 0; i < bufferSize; ++i) {
		REQUIRE(rangeWrapper.audioBufferChannelView[i] == (i >= rangeFrom && i < rangeFrom + rangeCount ? i + 1 - rangeFrom : 0));
	}
}

TEMPLATE_TEST_CASE("[AudioBufferChannelView] Buffer data can be copied from another buffer with ramp", "[AudioBufferChannelView]", int, double) {
	const size_t bufferSize = 8;
	auto wrapper = AudioBufferChannelViewWrapper<TestType>::createWithFixedValue(bufferSize, 0);
	auto wrapperCopy = AudioBufferChannelViewWrapper<TestType>::createWithFixedValue(bufferSize, 8);

	for (size_t i = 0; i < bufferSize; ++i) {
		REQUIRE(wrapper.audioBufferChannelView[i] == 0);
	}

	wrapper.audioBufferChannelView.copyWithRampFrom(wrapperCopy.audioBufferChannelView, 0.0, 1.0, abl::SamplesRange::allSamples());
	for (size_t i = 0; i < bufferSize; ++i) {
		REQUIRE(wrapper.audioBufferChannelView[i] == i);
	}

	wrapper.audioBufferChannelView.copyWithRampFrom(wrapperCopy.audioBufferChannelView, 0.0, 0.5, abl::SamplesRange::allSamples());
	for (size_t i = 0; i < bufferSize; ++i) {
		REQUIRE(wrapper.audioBufferChannelView[i] == TestType(i) / 2);
	}

	size_t rangeFrom = 2;
	size_t rangeCount = 4;
	auto rangeWrapper = AudioBufferChannelViewWrapper<TestType>::createWithFixedValue(bufferSize, 0);
	rangeWrapper.audioBufferChannelView.copyWithRampFrom(wrapperCopy.audioBufferChannelView, 0.0, 1.0, abl::SamplesRange(rangeFrom, rangeCount));
	for (size_t i = 0; i < bufferSize; ++i) {
		REQUIRE(rangeWrapper.audioBufferChannelView[i] == (i >= rangeFrom && i < rangeFrom + rangeCount ? (i - rangeFrom) * 2 : 0));
	}
}

TEMPLATE_TEST_CASE("[AudioBufferChannelView] Buffer data can be added from another buffer", "[AudioBufferChannelView]", int, double) {
	const size_t bufferSize = 8;
	auto wrapper = AudioBufferChannelViewWrapper<TestType>::createWithFixedValue(bufferSize, 2);
	auto wrapperCopy = AudioBufferChannelViewWrapper<TestType>::createWithIncrementalNumbers(bufferSize);

	for (size_t i = 0; i < bufferSize; ++i) {
		REQUIRE(wrapper.audioBufferChannelView[i] == 2);
	}

	wrapper.audioBufferChannelView.addFrom(wrapperCopy.audioBufferChannelView, abl::SamplesRange::allSamples());
	for (size_t i = 0; i < bufferSize; ++i) {
		REQUIRE(wrapper.audioBufferChannelView[i] == (i + 1) + 2);
	}

	wrapper.audioBufferChannelView.addFrom(wrapperCopy.audioBufferChannelView, abl::SamplesRange::allSamples(), 0.5);
	for (size_t i = 0; i < bufferSize; ++i) {
		REQUIRE(wrapper.audioBufferChannelView[i] == (TestType(i + 1) / 2) + (i + 1) + 2);
	}

	size_t rangeFrom = 1;
	size_t rangeCount = 5;
	auto rangeWrapper = AudioBufferChannelViewWrapper<TestType>::createWithFixedValue(bufferSize, 2);
	rangeWrapper.audioBufferChannelView.addFrom(wrapperCopy.audioBufferChannelView, abl::SamplesRange(rangeFrom, rangeCount));
	for (size_t i = 0; i < bufferSize; ++i) {
		REQUIRE(rangeWrapper.audioBufferChannelView[i] == (i >= rangeFrom && i < rangeFrom + rangeCount ? (i + 1 - rangeFrom) + 2 : 2));
	}
}

TEMPLATE_TEST_CASE("[AudioBufferChannelView] Buffer data can be added from another buffer with ramp", "[AudioBufferChannelView]", int, double) {
	const size_t bufferSize = 8;
	auto wrapper = AudioBufferChannelViewWrapper<TestType>::createWithFixedValue(bufferSize, 3);
	auto wrapperCopy = AudioBufferChannelViewWrapper<TestType>::createWithFixedValue(bufferSize, 8);

	for (size_t i = 0; i < bufferSize; ++i) {
		REQUIRE(wrapper.audioBufferChannelView[i] == 3);
	}

	wrapper.audioBufferChannelView.addWithRampFrom(wrapperCopy.audioBufferChannelView, 0.0, 1.0, abl::SamplesRange::allSamples());
	for (size_t i = 0; i < bufferSize; ++i) {
		REQUIRE(wrapper.audioBufferChannelView[i] == i + 3);
	}

	wrapper.audioBufferChannelView.addWithRampFrom(wrapperCopy.audioBufferChannelView, 0.0, 0.5, abl::SamplesRange::allSamples());
	for (size_t i = 0; i < bufferSize; ++i) {
		REQUIRE(wrapper.audioBufferChannelView[i] == (TestType(i) / 2) + i + 3);
	}

	size_t rangeFrom = 3;
	size_t rangeCount = 4;
	auto rangeWrapper = AudioBufferChannelViewWrapper<TestType>::createWithFixedValue(bufferSize, 3);
	rangeWrapper.audioBufferChannelView.addWithRampFrom(wrapperCopy.audioBufferChannelView, 0.0, 1.0, abl::SamplesRange(rangeFrom, rangeCount));
	for (size_t i = 0; i < bufferSize; ++i) {
		REQUIRE(rangeWrapper.audioBufferChannelView[i] == (i >= rangeFrom && i < rangeFrom + rangeCount ? ((i - rangeFrom) * 2) + 3 : 3));
	}
}

TEMPLATE_TEST_CASE("[AudioBufferChannelView] Buffer data is scaled with applyGain()", "[AudioBufferChannelView]", int, double) {
	const size_t bufferSize = 8;
	auto wrapper = AudioBufferChannelViewWrapper<TestType>::createWithIncrementalNumbers(bufferSize);

	for (size_t i = 0; i < bufferSize; ++i) {
		REQUIRE(wrapper.audioBufferChannelView[i] == i + 1);
	}

	wrapper.audioBufferChannelView.applyGain(0.5, abl::SamplesRange::allSamples());
	for (size_t i = 0; i < bufferSize; ++i) {
		REQUIRE(wrapper.audioBufferChannelView[i] == TestType(i + 1) / 2);
	}

	wrapper.audioBufferChannelView.applyGain(3.0, abl::SamplesRange::allSamples());
	for (size_t i = 0; i < bufferSize; ++i) {
		REQUIRE(wrapper.audioBufferChannelView[i] == ((TestType(i) + 1) / 2) * 3);
	}

	size_t rangeFrom = 1;
	size_t rangeCount = 5;
	auto rangeWrapper = AudioBufferChannelViewWrapper<TestType>::createWithIncrementalNumbers(bufferSize);
	rangeWrapper.audioBufferChannelView.applyGain(2.0, abl::SamplesRange(rangeFrom, rangeCount));
	for (size_t i = 0; i < bufferSize; ++i) {
		REQUIRE(rangeWrapper.audioBufferChannelView[i] == (i >= rangeFrom && i < rangeFrom + rangeCount ? (i + 1) * 2 : i + 1));
	}
}

TEMPLATE_TEST_CASE("[AudioBufferChannelView] Buffer data is scaled with ramp using applyGainRamp()", "[AudioBufferChannelView]", int, double) {
	const size_t bufferSize = 8;
	auto wrapper = AudioBufferChannelViewWrapper<TestType>::createWithFixedValue(bufferSize, 10);

	for (size_t i = 0; i < bufferSize; ++i) {
		REQUIRE(wrapper.audioBufferChannelView[i] == 10);
	}

	wrapper.audioBufferChannelView.applyGainRamp(0.0, 1.0, abl::SamplesRange::allSamples());
	for (size_t i = 0; i < bufferSize; ++i) {
		REQUIRE(wrapper.audioBufferChannelView[i] == TestType(i) * 10 / bufferSize);
	}

	wrapper.audioBufferChannelView.applyGainRamp(0.5, 0.0, abl::SamplesRange::allSamples());
	for (size_t i = 0; i < bufferSize; ++i) {
		REQUIRE(wrapper.audioBufferChannelView[i] == (TestType(i) * 10 / bufferSize) * (bufferSize - i) / (bufferSize * 2));
	}

	size_t rangeFrom = 2;
	size_t rangeCount = 4;
	auto rangeWrapper = AudioBufferChannelViewWrapper<TestType>::createWithFixedValue(bufferSize, 10);
	rangeWrapper.audioBufferChannelView.applyGainRamp(0.0, 1.0, abl::SamplesRange(rangeFrom, rangeCount));
	for (size_t i = 0; i < bufferSize; ++i) {
		REQUIRE(rangeWrapper.audioBufferChannelView[i] == (i >= rangeFrom && i < rangeFrom + rangeCount ? TestType(i - rangeFrom) * 20 / bufferSize : 10));
	}
}

TEMPLATE_TEST_CASE("[AudioBufferChannelView] Buffer data is cleared with clear()", "[AudioBufferChannelView]", int, double) {
	const size_t bufferSize = 8;
	auto wrapper = AudioBufferChannelViewWrapper<TestType>::createWithIncrementalNumbers(bufferSize);

	wrapper.audioBufferChannelView.clear(abl::SamplesRange::allSamples());

	for (size_t i = 0; i < bufferSize; ++i) {
		REQUIRE(wrapper.audioBufferChannelView[i] == 0);
	}

	size_t rangeFrom = 5;
	size_t rangeCount = 3;
	auto rangeWrapper = AudioBufferChannelViewWrapper<TestType>::createWithFixedValue(bufferSize, 4);
	rangeWrapper.audioBufferChannelView.clear(abl::SamplesRange(rangeFrom, rangeCount));
	for (size_t i = 0; i < bufferSize; ++i) {
		REQUIRE(rangeWrapper.audioBufferChannelView[i] == (i >= rangeFrom && i < rangeFrom + rangeCount ? 0 : 4));
	}
}

TEMPLATE_TEST_CASE("[AudioBufferChannelView] Buffer data is reversed with reverse()", "[AudioBufferChannelView]", int, double) {
	const size_t bufferSize = 8;
	auto wrapper = AudioBufferChannelViewWrapper<TestType>::createWithIncrementalNumbers(bufferSize);

	wrapper.audioBufferChannelView.reverse(abl::SamplesRange::allSamples());

	for (size_t i = 0; i < bufferSize; ++i) {
		REQUIRE(wrapper.audioBufferChannelView[i] == 8 - i);
	}

	size_t rangeFrom = 2;
	size_t rangeCount = 4;
	wrapper.audioBufferChannelView.reverse(abl::SamplesRange(rangeFrom, rangeCount));
	for (size_t i = 0; i < bufferSize; ++i) {
		REQUIRE(wrapper.audioBufferChannelView[i] == (i >= rangeFrom && i < rangeFrom + rangeCount ? i + 1 : 8 - i));
	}
}

TEMPLATE_TEST_CASE("[AudioBufferChannelView] View report correct higher peak", "[AudioBufferChannelView]", int, double) {
	const size_t bufferSize = 8;
	auto wrapper = AudioBufferChannelViewWrapper<TestType>::createWithIncrementalNumbers(bufferSize);

	REQUIRE(wrapper.audioBufferChannelView.getHigherPeak(abl::SamplesRange::allSamples()) == 8);
	REQUIRE(wrapper.audioBufferChannelView.getHigherPeak(abl::SamplesRange(1, 3)) == 4);
}

TEMPLATE_TEST_CASE("[AudioBufferChannelView] View report correct rms level", "[AudioBufferChannelView]", int, double) {
	const size_t bufferSize = 8;
	auto wrapper = AudioBufferChannelViewWrapper<TestType>::createWithIncrementalNumbers(bufferSize);

	REQUIRE(wrapper.audioBufferChannelView.getRMSLevel(abl::SamplesRange::allSamples()) == TestType(4.5));
	REQUIRE(wrapper.audioBufferChannelView.getRMSLevel(abl::SamplesRange(3, 3)) == 5);
}

TEMPLATE_TEST_CASE("[AudioBufferChannelView] View report correct buffer size", "[AudioBufferChannelView]", int, double) {
	const size_t bufferSize = 8;
	auto wrapper = AudioBufferChannelViewWrapper<TestType>::createWithIncrementalNumbers(bufferSize);

	REQUIRE(wrapper.audioBufferChannelView.getBufferSize() == bufferSize);
}

//@todo test out of range assert in all call? (catch2 can't)