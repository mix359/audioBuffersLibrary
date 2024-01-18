// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef AUDIO_BUFFERS_OFFSETTEDREADCIRCULARAUDIOBUFFERCHANNELVIEW_H
#define AUDIO_BUFFERS_OFFSETTEDREADCIRCULARAUDIOBUFFERCHANNELVIEW_H

#include <numeric>
#include <span>

#include "AudioBufferInterface.h"

namespace audioBuffers {

template <NumericType AudioSampleType>
class OffsettedReadCircularAudioBufferChannelView : public AudioBufferChannelViewInterface<AudioSampleType> {
public:
	OffsettedReadCircularAudioBufferChannelView(AudioSampleType* data, size_t bufferSize, size_t singleBufferSize, size_t readStartOffset, size_t writeStartOffset)
			: m_data(data), m_bufferSize(bufferSize), m_singleBufferSize{singleBufferSize}, m_readStartOffset{readStartOffset}, m_writeStartOffset{writeStartOffset}
	{
		assert(bufferSize == 0 || readStartOffset < bufferSize || writeStartOffset < bufferSize);
		m_lastReadSampleIndex = readStartOffset + singleBufferSize >= m_bufferSize ? m_bufferSize - readStartOffset - 1 : readStartOffset + singleBufferSize;
		m_lastWriteSampleIndex = writeStartOffset + singleBufferSize >= m_bufferSize ? m_bufferSize - writeStartOffset - 1 : writeStartOffset + singleBufferSize;
	}

	explicit OffsettedReadCircularAudioBufferChannelView(std::span<AudioSampleType> data, size_t singleBufferSize, size_t readStartOffset, size_t writeStartOffset)
			: m_data(data.front()), m_bufferSize(data.size()), m_singleBufferSize{singleBufferSize}, m_readStartOffset{readStartOffset}, m_writeStartOffset{writeStartOffset}
	{
		assert(m_bufferSize == 0 || readStartOffset < m_bufferSize || writeStartOffset < m_bufferSize);
		m_lastReadSampleIndex = readStartOffset + singleBufferSize >= m_bufferSize ? m_bufferSize - readStartOffset - 1 : readStartOffset + singleBufferSize;
		m_lastWriteSampleIndex = writeStartOffset + singleBufferSize >= m_bufferSize ? m_bufferSize - writeStartOffset - 1 : writeStartOffset + singleBufferSize;
	}

	OffsettedReadCircularAudioBufferChannelView(const OffsettedReadCircularAudioBufferChannelView& otherBuffer)
			: m_data{otherBuffer.m_data},
			  m_bufferSize {otherBuffer.m_bufferSize},
			  m_singleBufferSize {otherBuffer.m_singleBufferSize},
			  m_readStartOffset{otherBuffer.m_readStartOffset},
			  m_writeStartOffset{otherBuffer.m_writeStartOffset},
			  m_lastReadSampleIndex{otherBuffer.m_lastReadSampleIndex},
			  m_lastWriteSampleIndex{otherBuffer.m_lastWriteSampleIndex}
	{}

	OffsettedReadCircularAudioBufferChannelView(OffsettedReadCircularAudioBufferChannelView&& otherBuffer) noexcept
			: m_data{otherBuffer.m_data},
			  m_bufferSize {otherBuffer.m_bufferSize},
			  m_singleBufferSize {otherBuffer.m_singleBufferSize},
			  m_readStartOffset{otherBuffer.m_readStartOffset},
			  m_writeStartOffset{otherBuffer.m_writeStartOffset},
			  m_lastReadSampleIndex{otherBuffer.m_lastReadSampleIndex},
			  m_lastWriteSampleIndex{otherBuffer.m_lastWriteSampleIndex}
	{
		otherBuffer.m_data = nullptr;
		otherBuffer.m_bufferSize = 0;
		otherBuffer.m_singleBufferSize = 0;
		otherBuffer.m_readStartOffset = 0;
		otherBuffer.m_writeStartOffset = 0;
		otherBuffer.m_lastReadSampleIndex = 0;
		otherBuffer.m_lastWriteSampleIndex = 0;
	}

	using GainType = typename AudioBufferViewInterface<AudioSampleType>::GainType;
	using iterator = CircularIterator<AudioSampleType>;
	using const_iterator = const CircularIterator<AudioSampleType>;
	using wrapperIterator = typename AudioBufferChannelViewInterface<AudioSampleType>::iterator;
	wrapperIterator begin() const noexcept { return wrapperIterator(const_iterator(m_data, m_data + m_bufferSize, m_data + m_readStartOffset, 0)); }
	wrapperIterator end() const noexcept { return wrapperIterator(const_iterator(m_data, m_data + m_bufferSize, m_data + m_lastReadSampleIndex + 1, m_singleBufferSize)); }
	wrapperIterator writeBegin() const noexcept { return wrapperIterator(iterator(m_data, m_data + m_bufferSize, m_data + m_writeStartOffset, 0)); }
	wrapperIterator writeEnd() const noexcept { return wrapperIterator(iterator(m_data, m_data + m_bufferSize, m_data + m_lastWriteSampleIndex + 1, m_singleBufferSize)); }

	bool isEmpty() { return m_bufferSize < 1 || m_singleBufferSize < 1 || !m_data; }

	AudioSampleType& operator[](size_t index) noexcept {
		return m_data[getOffsettedBoundedReadSampleIndex(index)];
	}

	AudioSampleType operator[](size_t index) const noexcept {
		return m_data[getOffsettedBoundedReadSampleIndex(index)];
	}

	AudioSampleType getSample(size_t index) const noexcept {
		return m_data[getOffsettedBoundedReadSampleIndex(index)];
	}

	void setSample(size_t index, AudioSampleType sample) noexcept {
		m_data[getOffsettedBoundedWriteSampleIndex(index)] = sample;
	}

	void addSample(size_t index, AudioSampleType sample) noexcept {
		m_data[getOffsettedBoundedWriteSampleIndex(index)] += sample;
	}

	void copyFrom(const AudioBufferChannelViewInterface<AudioSampleType> &sourceBufferChannel, const SamplesRange &destinationSamplesRange, GainType gain = GainType(1)) {
		auto samplesCount = getSamplesCountFromRange(destinationSamplesRange);
		assert(samplesCount <= sourceBufferChannel.getBufferSize());
		assert(destinationSamplesRange.startSample + samplesCount <= m_singleBufferSize);

		for(size_t index = 0; index < samplesCount; ++index) {
			setSample(index + destinationSamplesRange.startSample, sourceBufferChannel.getSample(index) * gain);
		}
	}

	void copyWithRampFrom(const AudioBufferChannelViewInterface<AudioSampleType> &sourceBufferChannel, GainType startGain, GainType endGain, const SamplesRange &destinationSamplesRange) {
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

	void addFrom(const AudioBufferChannelViewInterface<AudioSampleType> &sourceBufferChannel, const SamplesRange &destinationSamplesRange, GainType gain = GainType(1)) {
		auto samplesCount = getSamplesCountFromRange(destinationSamplesRange);
		assert(samplesCount <= sourceBufferChannel.getBufferSize());
		assert(destinationSamplesRange.startSample + samplesCount <= m_singleBufferSize);

		for(size_t index = 0; index < samplesCount; ++index) {
			addSample(index + destinationSamplesRange.startSample, sourceBufferChannel.getSample(index) * gain);
		}
	}

	void addWithRampFrom(const AudioBufferChannelViewInterface<AudioSampleType> &sourceBufferChannel, GainType startGain, GainType endGain, const SamplesRange &destinationSamplesRange) {
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

	void applyGain(GainType gain, const SamplesRange& samplesRange) noexcept {
		auto samplesCount = getSamplesCountFromRange(samplesRange);
		assert(samplesRange.startSample + samplesCount <= m_singleBufferSize);
		for(size_t index = 0; index < samplesCount; ++index) {
			setSample(index + samplesRange.startSample, getSample(index + samplesRange.startSample) * gain);
		}
	}

	void applyGainRamp(GainType startGain, GainType endGain, const SamplesRange& samplesRange) noexcept {
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

	void clear(const SamplesRange& samplesRange = SamplesRange::allSamples()) noexcept {
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

	void reverse(const SamplesRange& samplesRange) noexcept {
		auto samplesCount = getSamplesCountFromRange(samplesRange);
		assert(samplesRange.startSample + samplesCount <= m_singleBufferSize);
		std::reverse(writeBegin() + samplesRange.startSample, writeBegin() + (samplesRange.startSample + samplesCount));
	}

	AudioSampleType getHigherPeak(const SamplesRange& samplesRange) const noexcept {
		auto samplesCount = getSamplesCountFromRange(samplesRange);
		assert(samplesRange.startSample + samplesCount <= m_singleBufferSize);
		auto startSample = samplesRange.startSample + m_readStartOffset;
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

	AudioSampleType getRMSLevel(const SamplesRange& samplesRange) const noexcept {
		auto samplesCount = getSamplesCountFromRange(samplesRange);
		assert(samplesRange.startSample + samplesCount <= m_singleBufferSize);
		auto startSample = samplesRange.startSample + m_readStartOffset;
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

	[[nodiscard]] inline size_t getOffsettedBoundedReadSampleIndex(size_t index) const {
		assert(index < m_singleBufferSize);
		auto offsettedIndex = m_readStartOffset + index;
		return offsettedIndex >= m_bufferSize ? offsettedIndex - m_bufferSize : offsettedIndex;
	}

	[[nodiscard]] inline size_t getOffsettedBoundedWriteSampleIndex(size_t index) const {
		assert(index < m_singleBufferSize);
		auto offsettedIndex = m_writeStartOffset + index;
		return offsettedIndex >= m_bufferSize ? offsettedIndex - m_bufferSize : offsettedIndex;
	}

	AudioSampleType* m_data;
	size_t m_bufferSize;
	size_t m_singleBufferSize;
	size_t m_readStartOffset;
	size_t m_writeStartOffset;
	size_t m_lastReadSampleIndex;
	size_t m_lastWriteSampleIndex;
};

} // audioBuffers

#endif //AUDIO_BUFFERS_OFFSETTEDREADCIRCULARAUDIOBUFFERCHANNELVIEW_H
