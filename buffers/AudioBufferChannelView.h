// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef AUDIO_BUFFERS_AUDIOBUFFERCHANNELVIEW_H
#define AUDIO_BUFFERS_AUDIOBUFFERCHANNELVIEW_H

#include <numeric>
#include <span>

#include "AudioBufferInterface.h"

namespace audioBuffers {

template <NumericType AudioSampleType>
class AudioBufferChannelView : public AudioBufferChannelViewInterface<AudioSampleType> {
public:
//	AudioBufferChannelView(const juce::AudioBuffer<AudioSampleType>& buffer, size_t channel) : m_data(buffer.getWritePointer(channel)), m_bufferSize(buffer.getNumSamples()) {}
	AudioBufferChannelView(AudioSampleType* data, size_t bufferSize) : m_data(data), m_bufferSize(bufferSize) {}
	explicit AudioBufferChannelView(std::span<AudioSampleType> data) : m_data(data.front()), m_bufferSize(data.size()) {}

	AudioBufferChannelView(const AudioBufferChannelView& otherBuffer)
			: m_data{otherBuffer.m_data},
			  m_bufferSize {otherBuffer.m_bufferSize}
	{}

	AudioBufferChannelView(AudioBufferChannelView&& otherBuffer) noexcept
			: m_data{otherBuffer.m_data},
			  m_bufferSize {otherBuffer.m_bufferSize}
	{
		otherBuffer.m_data = nullptr;
		otherBuffer.m_bufferSize = 0;
	}

	using GainType = typename AudioBufferChannelViewInterface<AudioSampleType>::GainType;
	using iterator = GenericPointerIterator<AudioSampleType>;
	using wrapperIterator = typename AudioBufferChannelViewInterface<AudioSampleType>::iterator;
	wrapperIterator begin() const noexcept { return wrapperIterator(iterator(&m_data[0])); }
	wrapperIterator end() const noexcept { return wrapperIterator(iterator(&m_data[m_bufferSize])); }

	iterator test_begin() const noexcept { return iterator(&m_data[0]); }
	iterator test_end() const noexcept { return iterator(&m_data[m_bufferSize]); }

	const AudioSampleType* getRawData() noexcept { return m_data; }
	bool isEmpty() { return m_bufferSize < 1 || !m_data; }

	AudioSampleType& operator[](size_t index) noexcept {
		assert(index < m_bufferSize);
		return m_data[index];
	}

	AudioSampleType operator[](size_t index) const noexcept {
		assert(index < m_bufferSize);
		return m_data[index];
	}

	AudioSampleType getSample(size_t index) const noexcept {
		assert(index < m_bufferSize);
		return m_data[index];
	}

	void setSample(size_t index, AudioSampleType sample) noexcept {
		assert(index < m_bufferSize);
		m_data[index] = sample;
	}

	void addSample(size_t index, AudioSampleType sample) noexcept {
		assert(index < m_bufferSize);
		m_data[index] += sample;
	}

	void copyFrom(const AudioBufferChannelViewInterface<AudioSampleType> &sourceBufferChannel, const SamplesRange &destinationSamplesRange = {}, GainType gain = AudioSampleType(1)) {
		auto samplesCount = getSamplesCountFromRange(destinationSamplesRange);
		assert(samplesCount <= sourceBufferChannel.getBufferSize());

		for(size_t index = 0; index < samplesCount; ++index) {
			m_data[index + destinationSamplesRange.startSample] = sourceBufferChannel.getSample(index) * gain;
		}
	}

	void copyWithRampFrom(const AudioBufferChannelViewInterface<AudioSampleType> &sourceBufferChannel, GainType startGain, GainType endGain, const SamplesRange &destinationSamplesRange = {}) {
		if(startGain == endGain) {
			copyFrom(sourceBufferChannel, destinationSamplesRange, startGain);
			return;
		}

		auto samplesCount = getSamplesCountFromRange(destinationSamplesRange);
		assert(samplesCount <= sourceBufferChannel.getBufferSize());
		GainType baseGainIncrement = (endGain - startGain) / static_cast<GainType>(samplesCount);
		GainType currentGain = startGain;

		for(size_t index = 0; index < samplesCount; ++index) {
			m_data[index + destinationSamplesRange.startSample] = sourceBufferChannel.getSample(index) * currentGain;
			currentGain += baseGainIncrement;
		}
	}

	void addFrom(const AudioBufferChannelViewInterface<AudioSampleType> &sourceBufferChannel, const SamplesRange &destinationSamplesRange = {}, GainType gain = AudioSampleType(1)) {
		auto samplesCount = getSamplesCountFromRange(destinationSamplesRange);
		assert(samplesCount <= sourceBufferChannel.getBufferSize());

		for(size_t index = 0; index < samplesCount; ++index) {
			m_data[index + destinationSamplesRange.startSample] += sourceBufferChannel.getSample(index) * gain;
		}
	}

	void addWithRampFrom(const AudioBufferChannelViewInterface<AudioSampleType> &sourceBufferChannel, GainType startGain, GainType endGain, const SamplesRange &destinationSamplesRange = {}) {
		if(startGain == endGain) {
			copyFrom(sourceBufferChannel, destinationSamplesRange, startGain);
			return;
		}

		auto samplesCount = getSamplesCountFromRange(destinationSamplesRange);
		assert(samplesCount <= sourceBufferChannel.getBufferSize());
		GainType baseGainIncrement = (endGain - startGain) / static_cast<GainType>(samplesCount);
		GainType currentGain = startGain;

		for(size_t index = 0; index < samplesCount; ++index) {
			m_data[index + destinationSamplesRange.startSample] += sourceBufferChannel.getSample(index) * currentGain;
			currentGain += baseGainIncrement;
		}
	}

	void applyGain(GainType gain, const SamplesRange& samplesRange = {}) noexcept {
		auto samplesCount = getSamplesCountFromRange(samplesRange);
		for(size_t index = 0; index < samplesCount; ++index) {
			m_data[index + samplesRange.startSample] *= gain;
		}
	}

	void applyGainRamp(GainType startGain, GainType endGain, const SamplesRange& samplesRange = {}) noexcept {
		if(startGain == endGain) {
			applyGain(startGain, samplesRange);
			return;
		}

		auto samplesCount = getSamplesCountFromRange(samplesRange);
		GainType baseGainIncrement = (endGain - startGain) / static_cast<GainType>(samplesCount);
		GainType currentGain = startGain;

		for(size_t index = 0; index < samplesCount; ++index) {
			m_data[index + samplesRange.startSample] *= currentGain;
			currentGain += baseGainIncrement;
		}
	}

	void clear(const SamplesRange& samplesRange = {}) noexcept {
		auto samplesCount = getSamplesCountFromRange(samplesRange);
		for(size_t index = 0; index < samplesCount; ++index) {
			m_data[index + samplesRange.startSample] = 0;
		}
	}

	void reverse(const SamplesRange& samplesRange = {}) noexcept {
		std::reverse(m_data + samplesRange.startSample, m_data + samplesRange.startSample + getSamplesCountFromRange(samplesRange));
	}

	AudioSampleType getHigherPeak(const SamplesRange& samplesRange = {}) const noexcept {
		return *std::max_element(m_data + samplesRange.startSample, m_data + samplesRange.startSample + getSamplesCountFromRange(samplesRange), [](AudioSampleType a, AudioSampleType b) { return std::abs(a) < std::abs(b); });
	}

	AudioSampleType getRMSLevel(const SamplesRange& samplesRange = {}) const noexcept {
		auto samplesCount = getSamplesCountFromRange(samplesRange);
		return std::accumulate(m_data + samplesRange.startSample, m_data + samplesRange.startSample + samplesCount, AudioSampleType(0)) / samplesCount;
	}

	[[nodiscard]] size_t getBufferSize() const noexcept { return m_bufferSize; }

protected:
	[[nodiscard]] size_t getSamplesCountFromRange(const SamplesRange& samplesRange) const {
		auto samplesCount = samplesRange.getRealSamplesCount(m_bufferSize);
		assert(samplesCount > 0);
		assert(samplesRange.startSample + samplesCount <= m_bufferSize);
		return samplesCount;
	}

	AudioSampleType* m_data;
	size_t m_bufferSize;
};

} // audioBuffers

#endif //AUDIO_BUFFERS_AUDIOBUFFERCHANNELVIEW_H
