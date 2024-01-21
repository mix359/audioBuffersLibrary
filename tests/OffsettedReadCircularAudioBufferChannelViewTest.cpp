// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "../buffers/OffsettedReadCircularAudioBufferChannelView.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_template_test_macros.hpp>

template<typename T>
struct OffsettedReadCircularAudioBufferChannelViewWrapper {
	T *data;
	abl::OffsettedReadCircularAudioBufferChannelView<T> audioBufferChannelView;

	static OffsettedReadCircularAudioBufferChannelViewWrapper createWithIncrementalNumbers(size_t bufferSize, size_t singleBufferSize, size_t readStartOffset, std::optional<size_t> writeStartOffset = {}) {
		auto data = new T[bufferSize];
		OffsettedReadCircularAudioBufferChannelViewWrapper wrapper{data, bufferSize, singleBufferSize, readStartOffset, writeStartOffset.has_value() ? writeStartOffset.value() : readStartOffset};
		std::iota(data, data + bufferSize, 1);
		return wrapper;
	}

	static OffsettedReadCircularAudioBufferChannelViewWrapper createWithFixedValue(size_t bufferSize, size_t singleBufferSize, size_t readStartOffset, T fixedValue, std::optional<size_t> writeStartOffset = {}) {
		auto data = new T[bufferSize];
		OffsettedReadCircularAudioBufferChannelViewWrapper wrapper{data, bufferSize, singleBufferSize, readStartOffset, writeStartOffset.has_value() ? writeStartOffset.value() : readStartOffset};
		std::fill(data, data + bufferSize, fixedValue);
		return wrapper;
	}

	OffsettedReadCircularAudioBufferChannelViewWrapper(T *newData, size_t bufferSize, size_t singleBufferSize, size_t readStartOffset, size_t writeStartOffset) : data{newData}, audioBufferChannelView{
			abl::OffsettedReadCircularAudioBufferChannelView{data, bufferSize, singleBufferSize, readStartOffset, writeStartOffset}} {}

	~OffsettedReadCircularAudioBufferChannelViewWrapper() { delete[] data; }
};

TEST_CASE("[OffsettedReadCircularAudioBufferChannelView] View report correct empty state", "[OffsettedReadCircularAudioBufferChannelView]") {
	auto wrapper = OffsettedReadCircularAudioBufferChannelViewWrapper<int>::createWithFixedValue(16, 8, 0, 0);
	auto inSizeZeroWrapper = OffsettedReadCircularAudioBufferChannelViewWrapper<double>::createWithIncrementalNumbers(16, 0, 0);
	auto outSizeZeroWrapper = OffsettedReadCircularAudioBufferChannelViewWrapper<int>::createWithIncrementalNumbers(0, 8, 0);

	REQUIRE_FALSE(wrapper.audioBufferChannelView.isEmpty());
	REQUIRE(inSizeZeroWrapper.audioBufferChannelView.isEmpty());
	REQUIRE(outSizeZeroWrapper.audioBufferChannelView.isEmpty());
}

TEMPLATE_TEST_CASE("[OffsettedReadCircularAudioBufferChannelView] Buffer data is iterable", "[OffsettedReadCircularAudioBufferChannelView]", int, double) {
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t startOffset = 28;
	auto wrapper = OffsettedReadCircularAudioBufferChannelViewWrapper<TestType>::createWithIncrementalNumbers(bufferSize, singleBufferSize, startOffset);

	size_t i = startOffset;
	for (auto &&data: wrapper.audioBufferChannelView) {
		if (++i > bufferSize) i = 1;
		REQUIRE(data == i);
	}
}

TEMPLATE_TEST_CASE("[OffsettedReadCircularAudioBufferChannelView] Buffer data is accessible with operator[] and getSample()", "[OffsettedReadCircularAudioBufferChannelView]", int, double) {
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t startOffset = 28;
	auto wrapper = OffsettedReadCircularAudioBufferChannelViewWrapper<TestType>::createWithIncrementalNumbers(bufferSize, singleBufferSize, startOffset);

	size_t val = startOffset;
	for (size_t i = 0; i < singleBufferSize; ++i) {
		if (++val > bufferSize) val = 1;
		REQUIRE(wrapper.audioBufferChannelView[i] == val);
		REQUIRE(wrapper.audioBufferChannelView.getSample(i) == val);
	}
}

TEMPLATE_TEST_CASE("[OffsettedReadCircularAudioBufferChannelView] Buffer data can be set with setSample() and addSample()", "[OffsettedReadCircularAudioBufferChannelView]", int, double) {
	auto wrapper = OffsettedReadCircularAudioBufferChannelViewWrapper<TestType>::createWithIncrementalNumbers(32, 8, 4);

	const size_t index = 5;
	wrapper.audioBufferChannelView.setSample(index, 20);
	REQUIRE(wrapper.audioBufferChannelView[index] == 20);

	wrapper.audioBufferChannelView.addSample(index, 20);
	REQUIRE(wrapper.audioBufferChannelView[index] == 40);

	REQUIRE(wrapper.data[index + 4] == 40);
}

TEMPLATE_TEST_CASE("[OffsettedReadCircularAudioBufferChannelView] Buffer is correctly offsetted for read and write operations", "[OffsettedReadCircularAudioBufferChannelView]", int, double) {
	size_t writeDelta = 2;
	size_t baseOffset = 4;
	auto wrapper = OffsettedReadCircularAudioBufferChannelViewWrapper<TestType>::createWithIncrementalNumbers(32, 8, baseOffset, baseOffset + writeDelta);

	const size_t index = 3;
	wrapper.audioBufferChannelView.setSample(index, 20);
	REQUIRE(wrapper.audioBufferChannelView[index + writeDelta] == 20);

	REQUIRE(wrapper.data[index + baseOffset + writeDelta] == 20);
}

TEMPLATE_TEST_CASE("[OffsettedReadCircularAudioBufferChannelView] Buffer can be copied and moved", "[OffsettedReadCircularAudioBufferChannelView]", int, double) {
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t startOffset = 28;
	auto wrapper = OffsettedReadCircularAudioBufferChannelViewWrapper<TestType>::createWithFixedValue(bufferSize, singleBufferSize, startOffset, 3);

	abl::OffsettedReadCircularAudioBufferChannelView<TestType> copyBuffer{wrapper.audioBufferChannelView};

	for (size_t i = 0; i < singleBufferSize; ++i) {
		REQUIRE(copyBuffer.getSample(i) == 3);
	}

	REQUIRE(!copyBuffer.isEmpty());
	abl::OffsettedReadCircularAudioBufferChannelView<TestType> moveBuffer{std::move(copyBuffer)};

	for (size_t i = 0; i < singleBufferSize; ++i) {
		REQUIRE(moveBuffer.getSample(i) == 3);
	}

	REQUIRE(copyBuffer.isEmpty());
}

TEMPLATE_TEST_CASE("[OffsettedReadCircularAudioBufferChannelView] Buffer data can be copied from another buffer", "[OffsettedReadCircularAudioBufferChannelView]", int, double) {
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t startOffset = 28;
	auto wrapper = OffsettedReadCircularAudioBufferChannelViewWrapper<TestType>::createWithFixedValue(bufferSize, singleBufferSize, startOffset, 0);
	auto wrapperCopy = OffsettedReadCircularAudioBufferChannelViewWrapper<TestType>::createWithIncrementalNumbers(bufferSize, singleBufferSize, startOffset);

	for (size_t i = 0; i < singleBufferSize; ++i) {
		REQUIRE(wrapper.audioBufferChannelView[i] == 0);
	}

	wrapper.audioBufferChannelView.copyFrom(wrapperCopy.audioBufferChannelView, abl::SamplesRange::allSamples());
	for (size_t i = 0; i < singleBufferSize; ++i) {
		const unsigned val = i + 1 + startOffset;
		REQUIRE(wrapper.audioBufferChannelView[i] == (val > bufferSize ? val - bufferSize : val));
	}

	const size_t rangeFrom = 2;
	const size_t rangeCount = 4;
	wrapper.audioBufferChannelView.copyFrom(wrapperCopy.audioBufferChannelView, abl::SamplesRange(2, 4));
	for (size_t i = 0; i < singleBufferSize; ++i) {
		unsigned val = i + 1 + startOffset;
		if (i >= rangeFrom && i < rangeFrom + rangeCount) {
			val -= rangeFrom;
		}
		if (val > bufferSize) val -= bufferSize;
		REQUIRE(wrapper.audioBufferChannelView[i] == val);
	}

	wrapper.audioBufferChannelView.copyFrom(wrapperCopy.audioBufferChannelView, abl::SamplesRange::allSamples(), 0.5);
	for (size_t i = 0; i < singleBufferSize; ++i) {
		const unsigned val = i + 1 + startOffset;
		REQUIRE(wrapper.audioBufferChannelView[i] == (val > bufferSize ? TestType(val - bufferSize) / 2 : TestType(val) / 2));
	}
}

TEMPLATE_TEST_CASE("[OffsettedReadCircularAudioBufferChannelView] Buffer data can be copied from another buffer with ramp", "[OffsettedReadCircularAudioBufferChannelView]", int, double) {
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t startOffset = 28;
	auto wrapper = OffsettedReadCircularAudioBufferChannelViewWrapper<TestType>::createWithFixedValue(bufferSize, singleBufferSize, startOffset, 0);
	auto wrapperCopy = OffsettedReadCircularAudioBufferChannelViewWrapper<TestType>::createWithFixedValue(bufferSize, singleBufferSize, startOffset, 8);

	for (size_t i = 0; i < singleBufferSize; ++i) {
		REQUIRE(wrapper.audioBufferChannelView[i] == 0);
	}

	wrapper.audioBufferChannelView.copyWithRampFrom(wrapperCopy.audioBufferChannelView, 0.0, 1.0, abl::SamplesRange::allSamples());
	for (size_t i = 0; i < singleBufferSize; ++i) {
		REQUIRE(wrapper.audioBufferChannelView[i] == i);
	}

	const size_t rangeFrom = 1;
	const size_t rangeCount = 4;
	wrapper.audioBufferChannelView.copyWithRampFrom(wrapperCopy.audioBufferChannelView, 0.0, 1.0, abl::SamplesRange(rangeFrom, rangeCount));
	for (size_t i = 0; i < singleBufferSize; ++i) {
		unsigned val = i >= rangeFrom && i < rangeFrom + rangeCount ? (i - rangeFrom) * 2 : i;
		REQUIRE(wrapper.audioBufferChannelView[i] == val);
	}

	wrapper.audioBufferChannelView.copyWithRampFrom(wrapperCopy.audioBufferChannelView, 0.0, 0.5, abl::SamplesRange::allSamples());
	for (size_t i = 0; i < singleBufferSize; ++i) {
		REQUIRE(wrapper.audioBufferChannelView[i] == TestType(i) / 2);
	}
}

TEMPLATE_TEST_CASE("[OffsettedReadCircularAudioBufferChannelView] Buffer data can be added from another buffer", "[OffsettedReadCircularAudioBufferChannelView]", int, double) {
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t startOffset = 28;
	auto wrapper = OffsettedReadCircularAudioBufferChannelViewWrapper<TestType>::createWithFixedValue(bufferSize, singleBufferSize, startOffset, 2);
	auto wrapperCopy = OffsettedReadCircularAudioBufferChannelViewWrapper<TestType>::createWithIncrementalNumbers(bufferSize, singleBufferSize, startOffset);

	for (size_t i = 0; i < singleBufferSize; ++i) {
		REQUIRE(wrapper.audioBufferChannelView[i] == 2);
	}

	wrapper.audioBufferChannelView.addFrom(wrapperCopy.audioBufferChannelView, abl::SamplesRange::allSamples());
	for (size_t i = 0; i < singleBufferSize; ++i) {
		const unsigned val = i + 1 + startOffset;
		REQUIRE(wrapper.audioBufferChannelView[i] == (val > bufferSize ? val - bufferSize : val) + 2);
	}

	wrapper.audioBufferChannelView.addFrom(wrapperCopy.audioBufferChannelView, abl::SamplesRange::allSamples(), 0.5);
	for (size_t i = 0; i < singleBufferSize; ++i) {
		unsigned val = i + 1 + startOffset;
		if (val > bufferSize) val -= bufferSize;
		REQUIRE(wrapper.audioBufferChannelView[i] == (TestType(val) / 2) + val + 2);
	}

	const size_t rangeFrom = 3;
	const size_t rangeCount = 4;
	auto rangeWrapper = OffsettedReadCircularAudioBufferChannelViewWrapper<TestType>::createWithFixedValue(bufferSize, singleBufferSize, startOffset, 2);
	rangeWrapper.audioBufferChannelView.addFrom(wrapperCopy.audioBufferChannelView, abl::SamplesRange(rangeFrom, rangeCount));
	for (size_t i = 0; i < singleBufferSize; ++i) {
		unsigned val = 0;
		if (i >= rangeFrom && i < rangeFrom + rangeCount) {
			val = i + 1 + startOffset - rangeFrom;
			if (val > bufferSize) val -= bufferSize;
		}
		REQUIRE(rangeWrapper.audioBufferChannelView[i] == val + 2);
	}
}

TEMPLATE_TEST_CASE("[OffsettedReadCircularAudioBufferChannelView] Buffer data can be added from another buffer with ramp", "[OffsettedReadCircularAudioBufferChannelView]", int, double) {
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t startOffset = 28;
	auto wrapper = OffsettedReadCircularAudioBufferChannelViewWrapper<TestType>::createWithFixedValue(bufferSize, singleBufferSize, startOffset, 3);
	auto wrapperCopy = OffsettedReadCircularAudioBufferChannelViewWrapper<TestType>::createWithFixedValue(bufferSize, singleBufferSize, startOffset, 8);

	for (size_t i = 0; i < singleBufferSize; ++i) {
		REQUIRE(wrapper.audioBufferChannelView[i] == 3);
	}

	wrapper.audioBufferChannelView.addWithRampFrom(wrapperCopy.audioBufferChannelView, 0.0, 1.0, abl::SamplesRange::allSamples());
	for (size_t i = 0; i < singleBufferSize; ++i) {
		REQUIRE(wrapper.audioBufferChannelView[i] == (i > bufferSize ? i - bufferSize : i) + 3);
	}

	wrapper.audioBufferChannelView.addWithRampFrom(wrapperCopy.audioBufferChannelView, 0.0, 0.5, abl::SamplesRange::allSamples());
	for (size_t i = 0; i < singleBufferSize; ++i) {
		REQUIRE(wrapper.audioBufferChannelView[i] == (TestType(i) / 2) + i + 3);
	}

	const size_t rangeFrom = 2;
	const size_t rangeCount = 4;
	auto rangeWrapper = OffsettedReadCircularAudioBufferChannelViewWrapper<TestType>::createWithFixedValue(bufferSize, singleBufferSize, startOffset, 3);
	rangeWrapper.audioBufferChannelView.addWithRampFrom(wrapperCopy.audioBufferChannelView, 0.0, 0.5, abl::SamplesRange(rangeFrom, rangeCount));
	for (size_t i = 0; i < singleBufferSize; ++i) {
		REQUIRE(rangeWrapper.audioBufferChannelView[i] == (i >= rangeFrom && i < rangeFrom + rangeCount ? i - rangeFrom : 0) + 3);
	}
}

TEMPLATE_TEST_CASE("[OffsettedReadCircularAudioBufferChannelView] Buffer data is scaled with applyGain()", "[OffsettedReadCircularAudioBufferChannelView]", int, double) {
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t startOffset = 28;
	auto wrapper = OffsettedReadCircularAudioBufferChannelViewWrapper<TestType>::createWithIncrementalNumbers(bufferSize, singleBufferSize, startOffset);

	for (size_t i = 0; i < singleBufferSize; ++i) {
		unsigned val = i + 1 + startOffset;
		if (val > bufferSize) val -= bufferSize;
		REQUIRE(wrapper.audioBufferChannelView[i] == val);
	}

	wrapper.audioBufferChannelView.applyGain(0.5, abl::SamplesRange::allSamples());
	for (size_t i = 0; i < singleBufferSize; ++i) {
		unsigned val = i + 1 + startOffset;
		if (val > bufferSize) val -= bufferSize;
		REQUIRE(wrapper.audioBufferChannelView[i] == TestType(val) / 2);
	}

	wrapper.audioBufferChannelView.applyGain(3.0, abl::SamplesRange::allSamples());
	for (size_t i = 0; i < singleBufferSize; ++i) {
		unsigned val = i + 1 + startOffset;
		if (val > bufferSize) val -= bufferSize;
		REQUIRE(wrapper.audioBufferChannelView[i] == (TestType(val) / 2) * 3);
	}

	const size_t rangeFrom = 5;
	const size_t rangeCount = 3;
	auto rangeWrapper = OffsettedReadCircularAudioBufferChannelViewWrapper<TestType>::createWithIncrementalNumbers(bufferSize, singleBufferSize, startOffset);
	rangeWrapper.audioBufferChannelView.applyGain(2.0, abl::SamplesRange(rangeFrom, rangeCount));
	for (size_t i = 0; i < singleBufferSize; ++i) {
		unsigned val = i + 1 + startOffset;
		if (val > bufferSize) val -= bufferSize;
		REQUIRE(rangeWrapper.audioBufferChannelView[i] == (i >= rangeFrom && i < rangeFrom + rangeCount ? val * 2 : val));
	}
}

TEMPLATE_TEST_CASE("[OffsettedReadCircularAudioBufferChannelView] Buffer data is scaled with ramp using applyGainRamp()", "[OffsettedReadCircularAudioBufferChannelView]", int, double) {
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t startOffset = 28;
	auto wrapper = OffsettedReadCircularAudioBufferChannelViewWrapper<TestType>::createWithFixedValue(bufferSize, singleBufferSize, startOffset, 10);

	for (size_t i = 0; i < singleBufferSize; ++i) {
		REQUIRE(wrapper.audioBufferChannelView[i] == 10);
	}

	wrapper.audioBufferChannelView.applyGainRamp(0.0, 1.0, abl::SamplesRange::allSamples());
	for (size_t i = 0; i < singleBufferSize; ++i) {
		REQUIRE(wrapper.audioBufferChannelView[i] == TestType(i) * 10 / singleBufferSize);
	}

	wrapper.audioBufferChannelView.applyGainRamp(0.5, 0.0, abl::SamplesRange::allSamples());
	for (size_t i = 0; i < singleBufferSize; ++i) {
		REQUIRE(wrapper.audioBufferChannelView[i] == (TestType(i) * 10 / singleBufferSize) * (singleBufferSize - i) / (singleBufferSize * 2));
	}

	const size_t rangeFrom = 2;
	const size_t rangeCount = 4;
	auto rangeWrapper = OffsettedReadCircularAudioBufferChannelViewWrapper<TestType>::createWithFixedValue(bufferSize, singleBufferSize, startOffset, 10);
	rangeWrapper.audioBufferChannelView.applyGainRamp(0.0, 1.0, abl::SamplesRange(rangeFrom, rangeCount));
	for (size_t i = 0; i < singleBufferSize; ++i) {
		REQUIRE(rangeWrapper.audioBufferChannelView[i] == (i >= rangeFrom && i < rangeFrom + rangeCount ? TestType(i - rangeFrom) * 20 / singleBufferSize : 10));
	}
}

TEMPLATE_TEST_CASE("[OffsettedReadCircularAudioBufferChannelView] Buffer data is cleared with clear()", "[OffsettedReadCircularAudioBufferChannelView]", int, double) {
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t startOffset = 28;
	auto wrapper = OffsettedReadCircularAudioBufferChannelViewWrapper<TestType>::createWithIncrementalNumbers(bufferSize, singleBufferSize, startOffset);

	wrapper.audioBufferChannelView.clear(abl::SamplesRange::allSamples());

	for (size_t i = 0; i < singleBufferSize; ++i) {
		REQUIRE(wrapper.audioBufferChannelView[i] == 0);
	}

	const size_t rangeFrom = 5;
	const size_t rangeCount = 3;
	auto rangeWrapper = OffsettedReadCircularAudioBufferChannelViewWrapper<TestType>::createWithFixedValue(bufferSize, singleBufferSize, startOffset, 4);
	rangeWrapper.audioBufferChannelView.clear(abl::SamplesRange(rangeFrom, rangeCount));
	for (size_t i = 0; i < singleBufferSize; ++i) {
		REQUIRE(rangeWrapper.audioBufferChannelView[i] == (i >= rangeFrom && i < rangeFrom + rangeCount ? 0 : 4));
	}
}

TEMPLATE_TEST_CASE("[OffsettedReadCircularAudioBufferChannelView] Buffer data is cleared with clearContainerBuffer()", "[OffsettedReadCircularAudioBufferChannelView]", int, double) {
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t startOffset = 28;
	auto wrapper = OffsettedReadCircularAudioBufferChannelViewWrapper<TestType>::createWithIncrementalNumbers(bufferSize, singleBufferSize, startOffset);

	wrapper.audioBufferChannelView.clearContainerBuffer();

	for (size_t i = 0; i < singleBufferSize; ++i) {
		REQUIRE(wrapper.audioBufferChannelView[i] == 0);
	}

	for (size_t i = 0; i < bufferSize; ++i) {
		REQUIRE(wrapper.data[i] == 0);
	}
}

TEMPLATE_TEST_CASE("[OffsettedReadCircularAudioBufferChannelView] Buffer data is reversed with reverse()", "[OffsettedReadCircularAudioBufferChannelView]", int, double) {
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t startOffset = 28;
	auto wrapper = OffsettedReadCircularAudioBufferChannelViewWrapper<TestType>::createWithIncrementalNumbers(bufferSize, singleBufferSize, startOffset);

	const auto splitIndex = bufferSize - startOffset;
	const auto valueBeforeSplit = wrapper.audioBufferChannelView.getSample(splitIndex - 1);
	const auto lastValue = wrapper.audioBufferChannelView.getSample(singleBufferSize - 1);
	wrapper.audioBufferChannelView.reverse(abl::SamplesRange::allSamples());

	for (size_t i = 0; i < singleBufferSize; ++i) {
		REQUIRE(wrapper.audioBufferChannelView[i] == (i >= splitIndex ? valueBeforeSplit - i + splitIndex : lastValue - i));
	}

	const auto rangeFrom = 2;
	const auto rangeCount = 4;
	auto rangeWrapper = OffsettedReadCircularAudioBufferChannelViewWrapper<TestType>::createWithIncrementalNumbers(bufferSize, singleBufferSize, startOffset);
	const auto rangeLastValue = rangeWrapper.audioBufferChannelView.getSample(rangeFrom + rangeCount - 1);
	rangeWrapper.audioBufferChannelView.reverse(abl::SamplesRange(rangeFrom, rangeCount));

	for (size_t i = 0; i < singleBufferSize; ++i) {
		if (i >= rangeFrom && i < rangeFrom + rangeCount) {
			REQUIRE(rangeWrapper.audioBufferChannelView[i] == (i >= splitIndex ? valueBeforeSplit - i + splitIndex : rangeLastValue - i + rangeFrom));
		} else {
			REQUIRE(rangeWrapper.audioBufferChannelView[i] == (i + startOffset + 1 > bufferSize ? i + startOffset + 1 - bufferSize : i + startOffset + 1));
		}
	}
}

TEMPLATE_TEST_CASE("[OffsettedReadCircularAudioBufferChannelView] View report correct higher peak", "[OffsettedReadCircularAudioBufferChannelView]", int, double) {
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t startOffset = 28;
	auto wrapper = OffsettedReadCircularAudioBufferChannelViewWrapper<TestType>::createWithIncrementalNumbers(bufferSize, singleBufferSize, startOffset);

	REQUIRE(wrapper.audioBufferChannelView.getHigherPeak(abl::SamplesRange::allSamples()) == 32);
	REQUIRE(wrapper.audioBufferChannelView.getHigherPeak(abl::SamplesRange(3, 3)) == 32);
	REQUIRE(wrapper.audioBufferChannelView.getHigherPeak(abl::SamplesRange(5, 2)) == 3);
}

TEMPLATE_TEST_CASE("[OffsettedReadCircularAudioBufferChannelView] View report correct rms level", "[OffsettedReadCircularAudioBufferChannelView]", int, double) {
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t startOffset = 28;
	auto wrapper = OffsettedReadCircularAudioBufferChannelViewWrapper<TestType>::createWithIncrementalNumbers(bufferSize, singleBufferSize, startOffset);

	REQUIRE(wrapper.audioBufferChannelView.getRMSLevel(abl::SamplesRange::allSamples()) == TestType(16.5));
	REQUIRE(wrapper.audioBufferChannelView.getRMSLevel(abl::SamplesRange(3, 4)) == TestType(9.5));
	REQUIRE(wrapper.audioBufferChannelView.getRMSLevel(abl::SamplesRange(4, 2)) == TestType(1.5));
}

TEMPLATE_TEST_CASE("[OffsettedReadCircularAudioBufferChannelView] View report correct buffer size", "[OffsettedReadCircularAudioBufferChannelView]", int, double) {
	const size_t bufferSize = 32;
	const size_t singleBufferSize = 8;
	const size_t startOffset = 28;
	auto wrapper = OffsettedReadCircularAudioBufferChannelViewWrapper<TestType>::createWithIncrementalNumbers(bufferSize, singleBufferSize, startOffset);

	REQUIRE(wrapper.audioBufferChannelView.getBufferSize() == singleBufferSize);
	REQUIRE(wrapper.audioBufferChannelView.getContainerBufferSize() == bufferSize);
}

//@todo test out of range assert in all call? (catch2 can't)