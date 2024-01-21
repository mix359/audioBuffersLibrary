// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef ABL_AUDIOBUFFERVIEW_H
#define ABL_AUDIOBUFFERVIEW_H

#include <memory>
#include <numeric>
#include <span>

#include "AudioBufferViewConcepts.h"
#include "AudioBufferChannelView.h"
#include "AudioBufferChannelViewWrapper.h"
#include "../memory/ParentReferencingIterator.h"

namespace abl {

template <NumericType AudioSampleType>
class AudioBufferView {
public:
//	explicit AudioBufferView(const juce::AudioBuffer<AudioSampleType> &buffer, const std::vector<size_t>& channelsMapping = {}, size_t bufferStartOffset = 0)
//			: m_data(buffer.getArrayOfWritePointers()),
//			  m_bufferSize(buffer.getNumSamples()),
//			  m_bufferChannelsCount(buffer.getNumChannels()),
//			  m_channelsMapping(channelsMapping),
//			  m_bufferStartOffset{bufferStartOffset} {}

	AudioBufferView(AudioSampleType **data, size_t channelsCount, size_t bufferSize, const std::vector<size_t>& channelsMapping = {}, size_t bufferStartOffset = 0)
			: m_data(data),
			  m_bufferSize(bufferSize),
			  m_bufferChannelsCount(channelsCount),
			  m_channelsMapping(channelsMapping),
			  m_bufferStartOffset{bufferStartOffset} {}


	AudioBufferView(const AudioBufferView& otherBuffer)
			: m_data{otherBuffer.m_data},
			  m_bufferSize {otherBuffer.m_bufferSize},
			  m_bufferChannelsCount{otherBuffer.m_bufferChannelsCount},
			  m_channelsMapping{otherBuffer.m_channelsMapping},
			  m_bufferStartOffset{otherBuffer.m_bufferStartOffset}
	{}

	AudioBufferView(AudioBufferView&& otherBuffer) noexcept
			: m_data{otherBuffer.m_data},
			  m_bufferSize {otherBuffer.m_bufferSize},
			  m_bufferChannelsCount{otherBuffer.m_bufferChannelsCount},
			  m_channelsMapping{otherBuffer.m_channelsMapping},
			  m_bufferStartOffset{otherBuffer.m_bufferStartOffset}
	{
		otherBuffer.m_data = nullptr;
		otherBuffer.m_bufferSize = 0;
		otherBuffer.m_bufferChannelsCount = 0;
		otherBuffer.m_channelsMapping = {};
		otherBuffer.m_bufferStartOffset = 0;
	}

	using GainType = typename std::conditional<std::is_integral_v<AudioSampleType>, double, AudioSampleType>::type;
	using iterator = ParentReferencingIterator<AudioBufferView, AudioBufferChannelViewWrapper<AudioSampleType>>;
	iterator begin() { return iterator(0, *this); }
	iterator end() { return iterator(getChannelsCount(), *this); }


//
//	using wrapperIterator = typename AudioBufferView<AudioSampleType>::iterator;
//	wrapperIterator begin() const noexcept { return wrapperIterator(iterator(0, *this)); }
//	wrapperIterator end() const noexcept { return wrapperIterator(iterator(getChannelsCount(), *this)); }


	bool isEmpty() const { return m_bufferChannelsCount < 1 || m_bufferSize < 1 || !m_data; }

//	[[nodiscard]] AudioSampleType* const* getRawData() const { return m_data; }

	AudioBufferChannelViewWrapper<AudioSampleType> operator[](size_t channel) {
		return getChannelView(channel);
	}

	const AudioBufferChannelViewWrapper<AudioSampleType> operator[](size_t channel) const {
		return getChannelView(channel);
	}

	AudioBufferChannelViewWrapper<AudioSampleType> getChannelView(size_t channel, SamplesRange samplesRange = {}) const {
		assert(channel < getChannelsCount());
		if(samplesRange.haveRange()) {
			auto samplesCount = samplesRange.getRealSamplesCount(m_bufferSize);
			assert(samplesRange.startSample + samplesCount <= m_bufferSize);
			return AudioBufferChannelViewWrapper<AudioSampleType>(AudioBufferChannelView<AudioSampleType>(m_data[getMappedChannel(channel)] + m_bufferStartOffset + samplesRange.startSample, samplesRange.getRealSamplesCount(m_bufferSize)));
		} else {
			return AudioBufferChannelViewWrapper<AudioSampleType>(AudioBufferChannelView<AudioSampleType>(m_data[getMappedChannel(channel)] + m_bufferStartOffset, m_bufferSize));
		}
	}

	AudioBufferView<AudioSampleType> getRangedView(SamplesRange samplesRange = {}) {
		auto samplesCount = samplesRange.getRealSamplesCount(m_bufferSize);
		assert(samplesRange.startSample + samplesCount <= m_bufferSize);
		return AudioBufferView(m_data, m_bufferChannelsCount, samplesCount, m_channelsMapping, samplesRange.startSample);
	}

	AudioSampleType getSample(size_t channel, size_t index) const {
		assert(channel < getChannelsCount());
		return getTemporaryChannelView(channel).getSample(index);
	}

	void setSample(size_t destinationChannel, size_t destinationIndex, const AudioSampleType sample) {
		assert(destinationChannel < getChannelsCount());
		getTemporaryChannelView(destinationChannel).setSample(destinationIndex, sample);
	}

	void addSample(size_t destinationChannel, size_t destinationIndex, const AudioSampleType sample) {
		assert(destinationChannel < getChannelsCount());
		getTemporaryChannelView(destinationChannel).addSample(destinationIndex, sample);
	}

	void copyFrom(const AudioBufferReadableType<AudioSampleType> auto &sourceBuffer, const SamplesRange &destinationSamplesRange = {}, GainType gain = GainType(1)) {
		assert(sourceBuffer.getChannelsCount() >= getChannelsCount());
		auto samplesCount = destinationSamplesRange.getRealSamplesCount(m_bufferSize);
		for(size_t destinationChannel = 0; destinationChannel < getChannelsCount(); destinationChannel++) {
//			getTemporaryChannelView(destinationChannel).copyFrom(sourceBuffer.getChannelView(), destinationSamplesRange, gain);
			auto destinationChannelView = getTemporaryRangedChannelView(destinationChannel, destinationSamplesRange.startSample, samplesCount);
			for(size_t index = 0; index < destinationChannelView.getBufferSize(); ++index) {
				destinationChannelView.setSample(index, sourceBuffer.getSample(destinationChannel, index) * gain);
			}
		}
	}

	void copyWithRampFrom(const AudioBufferReadableType<AudioSampleType> auto &sourceBuffer, GainType startGain, GainType endGain, const SamplesRange &destinationSamplesRange = {}) {
		if(startGain == endGain) {
			copyFrom(sourceBuffer, destinationSamplesRange, startGain);
			return;
		}

		assert(sourceBuffer.getChannelsCount() >= getChannelsCount());
		auto samplesCount = destinationSamplesRange.getRealSamplesCount(m_bufferSize);
		assert(samplesCount > 0);
		GainType baseGainIncrement = (endGain - startGain) / static_cast<GainType>(samplesCount);
		GainType currentGain = startGain;

		for(size_t destinationChannel = 0; destinationChannel < getChannelsCount(); ++destinationChannel) {
			auto destinationChannelView = getTemporaryRangedChannelView(destinationChannel, destinationSamplesRange.startSample, samplesCount);
			for(size_t index = 0; index < destinationChannelView.getBufferSize(); ++index) {
				destinationChannelView.setSample(index, sourceBuffer.getSample(destinationChannel, index) * currentGain);
				currentGain += baseGainIncrement;
			}
			currentGain = startGain;
		}
	}

	void copyIntoChannelFrom(const AudioBufferChannelReadableType<AudioSampleType> auto &sourceBufferChannel, size_t destinationChannel, const SamplesRange &destinationSamplesRange = {}, GainType gain = GainType(1)) {
		assert(destinationChannel < getChannelsCount());
		getTemporaryChannelView(destinationChannel).copyFrom(sourceBufferChannel, destinationSamplesRange, gain);
	}

	void copyIntoChannelWithRampFrom(const AudioBufferChannelReadableType<AudioSampleType> auto &sourceBufferChannel, size_t destinationChannel, GainType startGain, GainType endGain, const SamplesRange &destinationSamplesRange = {}) {
		assert(destinationChannel < getChannelsCount());
		getTemporaryChannelView(destinationChannel).copyWithRampFrom(sourceBufferChannel, startGain, endGain, destinationSamplesRange);
	}

	void addFrom(const AudioBufferReadableType<AudioSampleType> auto &sourceBuffer, const SamplesRange &destinationSamplesRange = {}, GainType gain = GainType(1)) {
		assert(sourceBuffer.getChannelsCount() >= getChannelsCount());
		auto samplesCount = destinationSamplesRange.getRealSamplesCount(m_bufferSize);
		for(size_t destinationChannel = 0; destinationChannel < getChannelsCount(); destinationChannel++) {
			auto destinationChannelView = getTemporaryRangedChannelView(destinationChannel, destinationSamplesRange.startSample, samplesCount);
			for(size_t index = 0; index < destinationChannelView.getBufferSize(); ++index) {
				destinationChannelView.addSample(index, sourceBuffer.getSample(destinationChannel, index) * gain);
			}
		}
	}

	void addWithRampFrom(const AudioBufferReadableType<AudioSampleType> auto &sourceBuffer, GainType startGain, GainType endGain, const SamplesRange &destinationSamplesRange = {}) {
		if(startGain == endGain) {
			copyFrom(sourceBuffer, destinationSamplesRange, startGain);
			return;
		}

		assert(sourceBuffer.getChannelsCount() >= getChannelsCount());
		auto samplesCount = destinationSamplesRange.getRealSamplesCount(m_bufferSize);
		assert(samplesCount > 0);
		GainType baseGainIncrement = (endGain - startGain) / static_cast<GainType>(samplesCount);
		GainType currentGain = startGain;

		for(size_t destinationChannel = 0; destinationChannel < getChannelsCount(); ++destinationChannel) {
			auto destinationChannelView = getTemporaryRangedChannelView(destinationChannel, destinationSamplesRange.startSample, samplesCount);
			for(size_t index = 0; index < destinationChannelView.getBufferSize(); ++index) {
				destinationChannelView.addSample(index, sourceBuffer.getSample(destinationChannel, index) * currentGain);
				currentGain += baseGainIncrement;
			}
			currentGain = startGain;
		}
	}

	void addIntoChannelFrom(const AudioBufferChannelReadableType<AudioSampleType> auto &sourceBufferChannel, size_t destinationChannel, const SamplesRange &destinationSamplesRange = {}, GainType gain = GainType(1)) {
		assert(destinationChannel < getChannelsCount());
		getTemporaryChannelView(destinationChannel).addFrom(sourceBufferChannel, destinationSamplesRange, gain);
	}

	void addIntoChannelWithRampFrom(const AudioBufferChannelReadableType<AudioSampleType> auto &sourceBufferChannel, size_t destinationChannel, GainType startGain, GainType endGain, const SamplesRange &destinationSamplesRange = {}) {
		assert(destinationChannel < getChannelsCount());
		getTemporaryChannelView(destinationChannel).addWithRampFrom(sourceBufferChannel, startGain, endGain, destinationSamplesRange);
	}

	void applyGain(GainType gain, const SamplesRange &samplesRange = {}) {
		for(size_t channel = 0; channel < getChannelsCount(); ++channel) {
			getTemporaryChannelView(channel).applyGain(gain, samplesRange);
		}
	}

	void applyGainToChannel(GainType gain, size_t channel, const SamplesRange &samplesRange = {}) {
		assert(channel < getChannelsCount());
		getTemporaryChannelView(channel).applyGain(gain, samplesRange);
	}

	void applyGainRamp(GainType startGain, GainType endGain, const SamplesRange &samplesRange = {}) {
		for(size_t channel = 0; channel < getChannelsCount(); ++channel) {
			getTemporaryChannelView(channel).applyGainRamp(startGain, endGain, samplesRange);
		}
	}

	void applyGainRampToChannel(GainType startGain, GainType endGain, size_t channel, const SamplesRange &samplesRange = {}) {
		assert(channel < getChannelsCount());
		getTemporaryChannelView(channel).applyGainRamp(startGain, endGain, samplesRange);
	}

	void clear(const SamplesRange &samplesRange = {}) {
		for(size_t channel = 0; channel < getChannelsCount(); ++channel) {
			getTemporaryChannelView(channel).clear(samplesRange);
		}
	}

	void clearChannel(size_t channel, const SamplesRange &samplesRange = {}) {
		assert(channel < getChannelsCount());
		getTemporaryChannelView(channel).clear(samplesRange);
	}

	void reverse(const SamplesRange &samplesRange = {}) {
		for(size_t channel = 0; channel < getChannelsCount(); ++channel) {
			getTemporaryChannelView(channel).reverse(samplesRange);
		}
	}

	void reverseChannel(size_t channel, const SamplesRange &samplesRange = {}) {
		assert(channel < getChannelsCount());
		getTemporaryChannelView(channel).reverse(samplesRange);
	}

	AudioSampleType getHigherPeak(const SamplesRange &samplesRange = {}) const {
		AudioSampleType higherPeak = 0;
		for(size_t channel = 0; channel < getChannelsCount(); ++channel) {
			higherPeak = std::max(getTemporaryChannelView(channel).getHigherPeak(samplesRange), higherPeak);
		}
		return higherPeak;
	}

	AudioSampleType getHigherPeakForChannel(size_t channel, const SamplesRange &samplesRange = {}) const {
		assert(channel < getChannelsCount());
		return getTemporaryChannelView(channel).getHigherPeak(samplesRange);
	}

	AudioSampleType getRMSLevelForChannel(size_t channel, const SamplesRange &samplesRange = {}) const {
		assert(channel < getChannelsCount());
		return getTemporaryChannelView(channel).getRMSLevel(samplesRange);
	}

	[[nodiscard]] size_t getBufferSize() const noexcept { return m_bufferSize; }
	[[nodiscard]] size_t getChannelsCount() const noexcept { return !m_channelsMapping.empty() ? m_channelsMapping.size() : m_bufferChannelsCount; }

	[[nodiscard]] const std::vector<size_t>& getChannelsMapping() const noexcept { return m_channelsMapping; }
	void setChannelsMapping(const std::vector<size_t>& channelsMapping) { m_channelsMapping = channelsMapping; }
	void createSequentialChannelsMapping(size_t startChannel, size_t channelsCount) {
		assert(channelsCount > 0);
		assert(startChannel < channelsCount);
		assert(channelsCount <= m_bufferChannelsCount);
		if(m_channelsMapping.size() != channelsCount) {
			m_channelsMapping.resize(channelsCount);
		}
		std::iota(m_channelsMapping.begin(), m_channelsMapping.end(), startChannel);
	}

protected:
	[[nodiscard]] inline size_t getMappedChannel(size_t channel) const noexcept {
		if(!m_channelsMapping.empty()) {
			assert(channel < m_channelsMapping.size());
			return m_channelsMapping[channel];
		}

		return channel;
	}

	[[nodiscard]] inline AudioBufferChannelView<AudioSampleType> getTemporaryChannelView(size_t channel) const {
		return AudioBufferChannelView<AudioSampleType>(m_data[getMappedChannel(channel)] + m_bufferStartOffset, m_bufferSize);
	}

	[[nodiscard]] inline AudioBufferChannelView<AudioSampleType> getTemporaryRangedChannelView(size_t channel, size_t startOffset, size_t samplesCount) const {
		assert(startOffset + samplesCount <= m_bufferSize);
		return AudioBufferChannelView<AudioSampleType>(m_data[getMappedChannel(channel)] + m_bufferStartOffset + startOffset, samplesCount);
	}

	AudioSampleType** m_data;
	size_t m_bufferSize = 0;
	size_t m_bufferChannelsCount = 0;
	std::vector<size_t> m_channelsMapping;
	size_t m_bufferStartOffset = 0;
};

} // abl

#endif //ABL_AUDIOBUFFERVIEW_H
