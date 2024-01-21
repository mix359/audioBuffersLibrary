// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef ABL_CIRCULARAUDIOBUFFERCHANNELVIEW_H
#define ABL_CIRCULARAUDIOBUFFERCHANNELVIEW_H

#include <numeric>
#include <span>

#include "AudioBufferChannelViewConcepts.h"
#include "../memory/GenericPointerIterator.h"
#include "../memory/CircularIterator.h"
#include "../memory/VariantRandomAccessIteratorWrapper.h"

namespace abl {

template <NumericType AudioSampleType>
class CircularAudioBufferChannelView {
public:
	CircularAudioBufferChannelView(AudioSampleType* data, size_t bufferSize, size_t singleBufferSize, size_t startOffset)
			: m_data(data), m_bufferSize(bufferSize), m_singleBufferSize{singleBufferSize}, m_startOffset{startOffset}
	{
		assert(bufferSize == 0 || startOffset < bufferSize);
		m_lastSampleIndex = startOffset + singleBufferSize >= m_bufferSize ? m_bufferSize - startOffset - 1 : startOffset + singleBufferSize;
	}

	explicit CircularAudioBufferChannelView(std::span<AudioSampleType> data, size_t singleBufferSize, size_t startOffset)
			: m_data(data.front()), m_bufferSize(data.size()), m_singleBufferSize{singleBufferSize}, m_startOffset{startOffset}
	{
		assert(m_bufferSize == 0 || startOffset < m_bufferSize);
		m_lastSampleIndex = startOffset + singleBufferSize >= m_bufferSize ? m_bufferSize - startOffset - 1 : startOffset + singleBufferSize;
	}

	CircularAudioBufferChannelView(const CircularAudioBufferChannelView& otherBuffer)
			: m_data{otherBuffer.m_data},
			  m_bufferSize {otherBuffer.m_bufferSize},
			  m_singleBufferSize {otherBuffer.m_singleBufferSize},
			  m_startOffset{otherBuffer.m_startOffset},
			  m_lastSampleIndex{otherBuffer.m_lastSampleIndex}
	{}

	CircularAudioBufferChannelView(CircularAudioBufferChannelView&& otherBuffer) noexcept
			: m_data{otherBuffer.m_data},
			  m_bufferSize {otherBuffer.m_bufferSize},
			  m_singleBufferSize {otherBuffer.m_singleBufferSize},
			  m_startOffset{otherBuffer.m_startOffset},
			  m_lastSampleIndex{otherBuffer.m_lastSampleIndex}
	{
		otherBuffer.m_data = nullptr;
		otherBuffer.m_bufferSize = 0;
		otherBuffer.m_singleBufferSize = 0;
		otherBuffer.m_startOffset = 0;
		otherBuffer.m_lastSampleIndex = 0;
	}

	using GainType = typename std::conditional<std::is_integral_v<AudioSampleType>, double, AudioSampleType>::type;
	using iterator = CircularIterator<AudioSampleType>;
	using wrapperIterator = VariantRandomAccessIteratorWrapper<AudioSampleType, GenericPointerIterator<AudioSampleType>, CircularIterator<AudioSampleType>>;
	wrapperIterator begin() const noexcept { return wrapperIterator(iterator(m_data, m_data + m_bufferSize, m_data + m_startOffset, 0)); }
	wrapperIterator end() const noexcept { return wrapperIterator(iterator(m_data, m_data + m_bufferSize, m_data + m_lastSampleIndex + 1, m_singleBufferSize)); }

	bool isEmpty() { return m_bufferSize < 1 || m_singleBufferSize < 1 || !m_data; }

	AudioSampleType& operator[](size_t index) noexcept {
		return m_data[getOffsettedBoundedSampleIndex(index)];
	}

	AudioSampleType operator[](size_t index) const noexcept {
		return m_data[getOffsettedBoundedSampleIndex(index)];
	}

	AudioSampleType getSample(size_t index) const noexcept {
		return m_data[getOffsettedBoundedSampleIndex(index)];
	}

	void setSample(size_t index, AudioSampleType sample) noexcept {
		m_data[getOffsettedBoundedSampleIndex(index)] = sample;
	}

	void addSample(size_t index, AudioSampleType sample) noexcept {
		m_data[getOffsettedBoundedSampleIndex(index)] += sample;
	}

	void copyFrom(const AudioBufferChannelReadableType<AudioSampleType> auto &sourceBufferChannel, const SamplesRange &destinationSamplesRange = {}, GainType gain = GainType(1)) {
		auto samplesCount = getSamplesCountFromRange(destinationSamplesRange);
		assert(samplesCount <= sourceBufferChannel.getBufferSize());
		assert(destinationSamplesRange.startSample + samplesCount <= m_singleBufferSize);

		for(size_t index = 0; index < samplesCount; ++index) {
			setSample(index + destinationSamplesRange.startSample, sourceBufferChannel.getSample(index) * gain);
		}
	}

	void copyWithRampFrom(const AudioBufferChannelReadableType<AudioSampleType> auto &sourceBufferChannel, GainType startGain, GainType endGain, const SamplesRange &destinationSamplesRange = {}) {
		if(startGain == endGain) {
			copyFrom(sourceBufferChannel, destinationSamplesRange, startGain);
			return;
		}

		auto samplesCount = getSamplesCountFromRange(destinationSamplesRange);
		assert(samplesCount <= sourceBufferChannel.getBufferSize());
		assert(destinationSamplesRange.startSample + samplesCount <= m_singleBufferSize);
		GainType baseGainIncrement = (endGain - startGain) / static_cast<GainType>(samplesCount);
		GainType currentGain = startGain;

		for(size_t index = 0; index < samplesCount; ++index) {
			setSample(index + destinationSamplesRange.startSample, sourceBufferChannel.getSample(index) * currentGain);
			currentGain += baseGainIncrement;
		}
	}

	void addFrom(const AudioBufferChannelReadableType<AudioSampleType> auto &sourceBufferChannel, const SamplesRange &destinationSamplesRange = {}, GainType gain = GainType(1)) {
		auto samplesCount = getSamplesCountFromRange(destinationSamplesRange);
		assert(samplesCount <= sourceBufferChannel.getBufferSize());
		assert(destinationSamplesRange.startSample + samplesCount <= m_singleBufferSize);

		for(size_t index = 0; index < samplesCount; ++index) {
			addSample(index + destinationSamplesRange.startSample, sourceBufferChannel.getSample(index) * gain);
		}
	}

	void addWithRampFrom(const AudioBufferChannelReadableType<AudioSampleType> auto &sourceBufferChannel, GainType startGain, GainType endGain, const SamplesRange &destinationSamplesRange = {}) {
		if(startGain == endGain) {
			copyFrom(sourceBufferChannel, destinationSamplesRange, startGain);
			return;
		}

		auto samplesCount = getSamplesCountFromRange(destinationSamplesRange);
		assert(samplesCount <= sourceBufferChannel.getBufferSize());
		assert(destinationSamplesRange.startSample + samplesCount <= m_singleBufferSize);
		GainType baseGainIncrement = (endGain - startGain) / static_cast<GainType>(samplesCount);
		GainType currentGain = startGain;

		for(size_t index = 0; index < samplesCount; ++index) {
			addSample(index + destinationSamplesRange.startSample, sourceBufferChannel.getSample(index) * currentGain);
			currentGain += baseGainIncrement;
		}
	}

	void applyGain(GainType gain, const SamplesRange& samplesRange = {}) noexcept {
		auto samplesCount = getSamplesCountFromRange(samplesRange);
		assert(samplesRange.startSample + samplesCount <= m_singleBufferSize);
		for(size_t index = 0; index < samplesCount; ++index) {
			setSample(index + samplesRange.startSample, getSample(index + samplesRange.startSample) * gain);
		}
	}

	void applyGainRamp(GainType startGain, GainType endGain, const SamplesRange& samplesRange = {}) noexcept {
		if(startGain == endGain) {
			applyGain(startGain, samplesRange);
			return;
		}

		auto samplesCount = getSamplesCountFromRange(samplesRange);
		assert(samplesRange.startSample + samplesCount <= m_singleBufferSize);
		GainType baseGainIncrement = (endGain - startGain) / static_cast<GainType>(samplesCount);
		GainType currentGain = startGain;

		for(size_t index = 0; index < samplesCount; ++index) {
			setSample(index + samplesRange.startSample, getSample(index + samplesRange.startSample) * currentGain);
			currentGain += baseGainIncrement;
		}
	}

	void clear(const SamplesRange& samplesRange = {}) noexcept {
		auto samplesCount = getSamplesCountFromRange(samplesRange);
		assert(samplesRange.startSample + samplesCount <= m_singleBufferSize);
		for(size_t index = 0; index < samplesCount; ++index) {
			setSample(index + samplesRange.startSample, 0);
		}
	}

	void clearContainerBuffer() noexcept {
		for(size_t index = 0; index < m_bufferSize; ++index) {
			m_data[index] = 0;
		}
	}

	void reverse(const SamplesRange& samplesRange = {}) noexcept {
		auto samplesCount = getSamplesCountFromRange(samplesRange);
		assert(samplesRange.startSample + samplesCount <= m_singleBufferSize);
		std::reverse(begin() + samplesRange.startSample, begin() + (samplesRange.startSample + samplesCount));
	}

	AudioSampleType getHigherPeak(const SamplesRange& samplesRange = {}) const noexcept {
		auto samplesCount = getSamplesCountFromRange(samplesRange);
		assert(samplesRange.startSample + samplesCount <= m_singleBufferSize);
		auto startSample = samplesRange.startSample + m_startOffset;
		AudioSampleType higherElement = 0;
		if(startSample > m_bufferSize) {
			startSample -= m_bufferSize;
		} else if(startSample + samplesCount > m_bufferSize) {
			higherElement = *std::max_element(m_data + startSample, m_data + m_bufferSize, [](AudioSampleType a, AudioSampleType b) { return std::abs(a) < std::abs(b); });
			samplesCount -= m_bufferSize - startSample;
			startSample = 0;
		}

		return std::max(higherElement, *std::max_element(m_data + startSample, m_data + startSample + samplesCount, [](AudioSampleType a, AudioSampleType b) { return std::abs(a) < std::abs(b); }));
	}

	AudioSampleType getRMSLevel(const SamplesRange& samplesRange = {}) const noexcept {
		auto samplesCount = getSamplesCountFromRange(samplesRange);
		assert(samplesRange.startSample + samplesCount <= m_singleBufferSize);
		auto startSample = samplesRange.startSample + m_startOffset;
		auto remainingSamplesCount = samplesCount;
		AudioSampleType accumulator = 0;
		if(startSample > m_bufferSize) {
			startSample -= m_bufferSize;
		} else if(startSample + samplesCount > m_bufferSize) {
			accumulator = std::accumulate(m_data + startSample, m_data + m_bufferSize, AudioSampleType(0));
			remainingSamplesCount -= m_bufferSize - startSample;
			startSample = 0;
		}

		return std::accumulate(m_data + startSample, m_data + startSample + remainingSamplesCount, accumulator) / samplesCount;
	}

	[[nodiscard]] size_t getBufferSize() const noexcept { return m_singleBufferSize; }
	[[nodiscard]] size_t getContainerBufferSize() const noexcept { return m_bufferSize; }

protected:
	[[nodiscard]] size_t getSamplesCountFromRange(const SamplesRange& samplesRange) const {
		auto samplesCount = samplesRange.getRealSamplesCount(m_singleBufferSize);
		assert(samplesCount > 0);
		assert(samplesCount <= m_bufferSize);
		return samplesCount;
	}

	[[nodiscard]] inline size_t getOffsettedBoundedSampleIndex(size_t index) const {
		assert(index < m_singleBufferSize);
		auto offsettedIndex = m_startOffset + index;
		return offsettedIndex >= m_bufferSize ? offsettedIndex - m_bufferSize : offsettedIndex;
	}

	AudioSampleType* m_data;
	size_t m_bufferSize;
	size_t m_singleBufferSize;
	size_t m_startOffset;
	size_t m_lastSampleIndex;
};

} // abl

#endif //ABL_CIRCULARAUDIOBUFFERCHANNELVIEW_H
