// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef AUDIO_BUFFERS_BASICCIRCULARAUDIOBUFFERVIEW_H
#define AUDIO_BUFFERS_BASICCIRCULARAUDIOBUFFERVIEW_H

#include "AudioBufferInterface.h"
#include "CircularAudioBufferChannelView.h"
#include "OffsettedReadCircularAudioBufferChannelView.h"

namespace audioBuffers {

template <NumericType AudioSampleType>
class BasicCircularAudioBufferView : virtual public AudioBufferViewInterface<AudioSampleType> {
public:
//	BasicCircularAudioBufferView(const juce::AudioBuffer<AudioSampleType> &buffer, size_t singleBufferSize, size_t bufferStartOffset = 0, const std::vector<size_t>& channelsMapping = {})
//			: m_data(buffer.getArrayOfWritePointers()),
//			  m_bufferSize(buffer.getNumSamples()),
//			  m_singleBufferSize(singleBufferSize),
//			  m_bufferChannelsCount(buffer.getNumChannels()),
//			  m_channelsMapping(channelsMapping),
//			  m_bufferStartOffset{bufferStartOffset},
//			  m_readSampleOffset{0},
//			  m_writeSampleOffset{0}
//	{}

	BasicCircularAudioBufferView(AudioSampleType **data, size_t channelsCount, size_t bufferSize, size_t singleBufferSize, size_t bufferStartOffset = 0, const std::vector<size_t>& channelsMapping = {})
			: m_data(data),
			  m_bufferSize(bufferSize),
			  m_singleBufferSize(singleBufferSize),
			  m_bufferChannelsCount(channelsCount),
			  m_channelsMapping(channelsMapping),
			  m_bufferStartOffset{bufferStartOffset},
			  m_readSampleOffset{0},
			  m_writeSampleOffset{0}
	{}

	BasicCircularAudioBufferView(const BasicCircularAudioBufferView& otherBuffer)
			: m_data{otherBuffer.m_data},
			  m_bufferSize {otherBuffer.m_bufferSize},
			  m_singleBufferSize {otherBuffer.m_singleBufferSize},
			  m_bufferChannelsCount{otherBuffer.m_bufferChannelsCount},
			  m_channelsMapping{otherBuffer.m_channelsMapping},
			  m_bufferStartOffset{otherBuffer.m_bufferStartOffset}
	{
		m_readSampleOffset.store(otherBuffer.m_readSampleOffset.load());
		m_writeSampleOffset.store(otherBuffer.m_writeSampleOffset.load());
	}

	BasicCircularAudioBufferView(BasicCircularAudioBufferView&& otherBuffer) noexcept
			: m_data{otherBuffer.m_data},
			  m_bufferSize {otherBuffer.m_bufferSize},
			  m_singleBufferSize {otherBuffer.m_singleBufferSize},
			  m_bufferChannelsCount{otherBuffer.m_bufferChannelsCount},
			  m_channelsMapping{otherBuffer.m_channelsMapping},
			  m_bufferStartOffset{otherBuffer.m_bufferStartOffset}
	{
		m_readSampleOffset.store(otherBuffer.m_readSampleOffset.load());
		m_writeSampleOffset.store(otherBuffer.m_writeSampleOffset.load());
		otherBuffer.m_data = nullptr;
		otherBuffer.m_bufferSize = 0;
		otherBuffer.m_singleBufferSize = 0;
		otherBuffer.m_bufferChannelsCount = 0;
		otherBuffer.m_channelsMapping = {};
		otherBuffer.m_bufferStartOffset = 0;
		otherBuffer.m_readSampleOffset = 0;
		otherBuffer.m_writeSampleOffset = 0;
	}

	using GainType = typename AudioBufferViewInterface<AudioSampleType>::GainType;
	using iterator = ParentReferencingReturningUniquePtrIterator<BasicCircularAudioBufferView, AudioBufferChannelViewInterface<AudioSampleType>>;
	using const_iterator = ParentReferencingReturningUniquePtrIterator<const BasicCircularAudioBufferView, const AudioBufferChannelViewInterface<AudioSampleType>>;
	iterator begin() { return iterator(0, *this); }
	iterator end() { return iterator(getChannelsCount(), *this); }

//	using wrapperIterator = typename AudioBufferViewInterface<AudioSampleType>::iterator;
//	wrapperIterator begin() const noexcept override { return wrapperIterator(iterator(0, *this)); }
//	wrapperIterator end() const noexcept override { return wrapperIterator(iterator(getChannelsCount(), *this)); }

	//@todo const iterator?

	bool isEmpty() const override { return m_bufferChannelsCount < 1 || m_bufferSize < 1 || m_singleBufferSize < 1 || !m_data; }

	std::unique_ptr<AudioBufferChannelViewInterface<AudioSampleType>> operator[](size_t channel) override {
		return getChannelView(channel);
	}

	std::unique_ptr<const AudioBufferChannelViewInterface<AudioSampleType>> operator[](size_t channel) const override {
		return getChannelView(channel);
	}

	std::unique_ptr<const AudioBufferChannelViewInterface<AudioSampleType>> getReadOnlyChannelView(size_t channel, SamplesRange samplesRange = {}) const {
		assert(channel < getChannelsCount());
		if(samplesRange.haveRange()) {
			return std::make_unique<const CircularAudioBufferChannelView<AudioSampleType>>(getRangedReadChannelView(channel, samplesRange.startSample, samplesRange.getRealSamplesCount(m_singleBufferSize)));
		} else {
			return std::make_unique<const CircularAudioBufferChannelView<AudioSampleType>>(getReadChannelView(channel));
		}
	}

	std::unique_ptr<AudioBufferChannelViewInterface<AudioSampleType>> getWriteAreaChannelView(size_t channel, SamplesRange samplesRange = {}) const {
		assert(channel < getChannelsCount());
		if(samplesRange.haveRange()) {
			return std::make_unique<CircularAudioBufferChannelView<AudioSampleType>>(getRangedWriteChannelView(channel, samplesRange.startSample, samplesRange.getRealSamplesCount(m_singleBufferSize)));
		} else {
			return std::make_unique<CircularAudioBufferChannelView<AudioSampleType>>(getWriteChannelView(channel));
		}
	}

	std::unique_ptr<AudioBufferChannelViewInterface<AudioSampleType>> getChannelView(size_t channel, SamplesRange samplesRange = {}) const override {
		assert(channel < getChannelsCount());
		if(samplesRange.haveRange()) {
			return std::make_unique<OffsettedReadCircularAudioBufferChannelView<AudioSampleType>>(getRangedOffsettedChannelView(channel, samplesRange.startSample, samplesRange.getRealSamplesCount(m_singleBufferSize)));
		} else {
			return std::make_unique<OffsettedReadCircularAudioBufferChannelView<AudioSampleType>>(getOffsettedChannelView(channel));
		}
	}

	std::unique_ptr<AudioBufferViewInterface<AudioSampleType>> getRangedView(SamplesRange samplesRange = {}) override {
		auto samplesCount = samplesRange.getRealSamplesCount(m_singleBufferSize);
		assert(samplesRange.startSample + samplesCount <= m_singleBufferSize);
		auto sampleOffset = (samplesRange.startSample + m_bufferStartOffset) % m_bufferSize;
		return std::make_unique<BasicCircularAudioBufferView>(m_data, m_bufferChannelsCount, m_bufferSize, samplesRange.getRealSamplesCount(m_singleBufferSize), sampleOffset, m_channelsMapping);
	}

	AudioSampleType getSample(size_t channel, size_t index) const override {
		assert(channel < getChannelsCount());
		return getReadChannelView(channel).getSample(index);
	}

	void setSample(size_t destinationChannel, size_t destinationIndex, const AudioSampleType sample) override {
		assert(destinationChannel < getChannelsCount());
		getWriteChannelView(destinationChannel).setSample(destinationIndex, sample);
	}

	void addSample(size_t destinationChannel, size_t destinationIndex, const AudioSampleType sample) override {
		assert(destinationChannel < getChannelsCount());
		getWriteChannelView(destinationChannel).addSample(destinationIndex, sample);
	}

	void copyFrom(const AudioBufferViewInterface<AudioSampleType>& sourceBuffer, const SamplesRange &destinationSamplesRange = {}, GainType gain = GainType(1)) override {
		assert(sourceBuffer.getChannelsCount() >= getChannelsCount());
		auto samplesCount = destinationSamplesRange.getRealSamplesCount(m_singleBufferSize);
		for(size_t destinationChannel = 0; destinationChannel < getChannelsCount(); destinationChannel++) {
			auto destinationChannelView = getRangedWriteChannelView(destinationChannel, destinationSamplesRange.startSample, samplesCount);
			for(size_t index = 0; index < destinationChannelView.getBufferSize(); ++index) {
				destinationChannelView.setSample(index, sourceBuffer.getSample(destinationChannel, index) * gain);
			}
		}
	}

	void copyWithRampFrom(const AudioBufferViewInterface<AudioSampleType>& sourceBuffer, GainType startGain, GainType endGain, const SamplesRange &destinationSamplesRange = {}) override {
		if(startGain == endGain) {
			copyFrom(sourceBuffer, destinationSamplesRange, startGain);
			return;
		}

		assert(sourceBuffer.getChannelsCount() >= getChannelsCount());
		auto samplesCount = destinationSamplesRange.getRealSamplesCount(m_singleBufferSize);
		assert(samplesCount > 0);
		GainType baseGainIncrement = (endGain - startGain) / static_cast<GainType>(samplesCount);
		GainType currentGain = startGain;

		for(size_t destinationChannel = 0; destinationChannel < getChannelsCount(); ++destinationChannel) {
			auto destinationChannelView = getRangedWriteChannelView(destinationChannel, destinationSamplesRange.startSample, samplesCount);
			for(size_t index = 0; index < destinationChannelView.getBufferSize(); ++index) {
				destinationChannelView.setSample(index, sourceBuffer.getSample(destinationChannel, index) * currentGain);
				currentGain += baseGainIncrement;
			}
			currentGain = startGain;
		}
	}

	void copyIntoChannelFrom(const AudioBufferChannelViewInterface<AudioSampleType> &sourceBufferChannel, size_t destinationChannel, const SamplesRange &destinationSamplesRange = {}, GainType gain = GainType(1)) override {
		assert(destinationChannel < getChannelsCount());
		getWriteChannelView(destinationChannel).copyFrom(sourceBufferChannel, destinationSamplesRange, gain);
	}

	void copyIntoChannelWithRampFrom(const AudioBufferChannelViewInterface<AudioSampleType> &sourceBufferChannel, size_t destinationChannel, GainType startGain, GainType endGain, const SamplesRange &destinationSamplesRange = {}) override {
		assert(destinationChannel < getChannelsCount());
		getWriteChannelView(destinationChannel).copyWithRampFrom(sourceBufferChannel, startGain, endGain, destinationSamplesRange);
	}

	void addFrom(const AudioBufferViewInterface<AudioSampleType>& sourceBuffer, const SamplesRange &destinationSamplesRange = {}, GainType gain = GainType(1)) override {
		assert(sourceBuffer.getChannelsCount() >= getChannelsCount());
		auto samplesCount = destinationSamplesRange.getRealSamplesCount(m_singleBufferSize);
		for(size_t destinationChannel = 0; destinationChannel < getChannelsCount(); destinationChannel++) {
			auto destinationChannelView = getRangedWriteChannelView(destinationChannel, destinationSamplesRange.startSample, samplesCount);
			for(size_t index = 0; index < destinationChannelView.getBufferSize(); ++index) {
				destinationChannelView.addSample(index, sourceBuffer.getSample(destinationChannel, index) * gain);
			}
		}
	}

	void addWithRampFrom(const AudioBufferViewInterface<AudioSampleType>& sourceBuffer, GainType startGain, GainType endGain, const SamplesRange &destinationSamplesRange = {}) override {
		if(startGain == endGain) {
			copyFrom(sourceBuffer, destinationSamplesRange, startGain);
			return;
		}

		assert(sourceBuffer.getChannelsCount() >= getChannelsCount());
		auto samplesCount = destinationSamplesRange.getRealSamplesCount(m_singleBufferSize);
		assert(samplesCount > 0);
		GainType baseGainIncrement = (endGain - startGain) / static_cast<GainType>(samplesCount);
		GainType currentGain = startGain;

		for(size_t destinationChannel = 0; destinationChannel < getChannelsCount(); ++destinationChannel) {
			auto destinationChannelView = getRangedWriteChannelView(destinationChannel, destinationSamplesRange.startSample, samplesCount);
			for(size_t index = 0; index < destinationChannelView.getBufferSize(); ++index) {
				destinationChannelView.addSample(index, sourceBuffer.getSample(destinationChannel, index) * currentGain);
				currentGain += baseGainIncrement;
			}
			currentGain = startGain;
		}
	}

	void addIntoChannelFrom(const AudioBufferChannelViewInterface<AudioSampleType> &sourceBufferChannel, size_t destinationChannel, const SamplesRange &destinationSamplesRange = {}, GainType gain = GainType(1)) override {
		assert(destinationChannel < getChannelsCount());
		getWriteChannelView(destinationChannel).addFrom(sourceBufferChannel, destinationSamplesRange, gain);
	}

	void addIntoChannelWithRampFrom(const AudioBufferChannelViewInterface<AudioSampleType> &sourceBufferChannel, size_t destinationChannel, GainType startGain, GainType endGain, const SamplesRange &destinationSamplesRange = {}) override {
		assert(destinationChannel < getChannelsCount());
		getWriteChannelView(destinationChannel).addWithRampFrom(sourceBufferChannel, startGain, endGain, destinationSamplesRange);
	}

	void applyGain(GainType gain, const SamplesRange &samplesRange = {}) override {
		for(size_t channel = 0; channel < getChannelsCount(); ++channel) {
			getWriteChannelView(channel).applyGain(gain,samplesRange);
		}
	}

	void applyGainToChannel(GainType gain, size_t channel, const SamplesRange &samplesRange = {}) override {
		assert(channel < getChannelsCount());
		getWriteChannelView(channel).applyGain(gain,samplesRange);
	}

	void applyGainRamp(GainType startGain, GainType endGain, const SamplesRange &samplesRange = {}) override {
		for(size_t channel = 0; channel < getChannelsCount(); ++channel) {
			getWriteChannelView(channel).applyGainRamp(startGain, endGain, samplesRange);
		}
	}

	void applyGainRampToChannel(GainType startGain, GainType endGain, size_t channel, const SamplesRange &samplesRange = {}) override {
		assert(channel < getChannelsCount());
		getWriteChannelView(channel).applyGainRamp(startGain, endGain, samplesRange);
	}

	void clear(const SamplesRange &samplesRange = {}) override {
		for(size_t channel = 0; channel < getChannelsCount(); ++channel) {
			getWriteChannelView(channel).clear(samplesRange);
		}
	}

	void clearChannel(size_t channel, const SamplesRange &samplesRange = {}) override {
		assert(channel < getChannelsCount());
		getWriteChannelView(channel).clear(samplesRange);
	}

	void reverse(const SamplesRange &samplesRange = {}) override {
		for(size_t channel = 0; channel < getChannelsCount(); ++channel) {
			getWriteChannelView(channel).reverse(samplesRange);
		}
	}

	void reverseChannel(size_t channel, const SamplesRange &samplesRange = {}) override {
		assert(channel < getChannelsCount());
		getWriteChannelView(channel).reverse(samplesRange);
	}

	AudioSampleType getHigherPeak(const SamplesRange &samplesRange = {}) const override {
		AudioSampleType higherPeak = 0;
		for(size_t channel = 0; channel < getChannelsCount(); ++channel) {
			higherPeak = std::max(getReadChannelView(channel).getHigherPeak(samplesRange), higherPeak);
		}
		return higherPeak;
	}

	AudioSampleType getHigherPeakForChannel(size_t channel, const SamplesRange &samplesRange = {}) const override {
		assert(channel < getChannelsCount());
		return getReadChannelView(channel).getHigherPeak(samplesRange);
	}

	AudioSampleType getRMSLevelForChannel(size_t channel, const SamplesRange &samplesRange = {}) const override {
		assert(channel < getChannelsCount());
		return getReadChannelView(channel).getRMSLevel(samplesRange);
	}

	[[nodiscard]] size_t getBufferSize() const noexcept override { return m_singleBufferSize; }
	[[nodiscard]] size_t getChannelsCount() const noexcept override { return !m_channelsMapping.empty() ? m_channelsMapping.size() : m_bufferChannelsCount; }

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

	[[nodiscard]] inline CircularAudioBufferChannelView<AudioSampleType> getReadChannelView(size_t channel) const {
		return CircularAudioBufferChannelView<AudioSampleType>(m_data[getMappedChannel(channel)], m_bufferSize, m_singleBufferSize, (m_bufferStartOffset + m_readSampleOffset.load()) % m_bufferSize);
	}

	[[nodiscard]] inline CircularAudioBufferChannelView<AudioSampleType> getWriteChannelView(size_t channel) const {
		return CircularAudioBufferChannelView<AudioSampleType>(m_data[getMappedChannel(channel)], m_bufferSize, m_singleBufferSize, (m_bufferStartOffset + m_writeSampleOffset.load()) % m_bufferSize);
	}

	[[nodiscard]] inline OffsettedReadCircularAudioBufferChannelView<AudioSampleType> getOffsettedChannelView(size_t channel) const {
		return OffsettedReadCircularAudioBufferChannelView<AudioSampleType>(m_data[getMappedChannel(channel)], m_bufferSize, m_singleBufferSize, (m_bufferStartOffset + m_readSampleOffset.load()) % m_bufferSize, (m_bufferStartOffset + m_writeSampleOffset.load()) % m_bufferSize);
	}

	[[nodiscard]] inline CircularAudioBufferChannelView<AudioSampleType> getRangedReadChannelView(size_t channel, size_t startOffset, size_t samplesCount) const {
		assert(startOffset + samplesCount <= m_singleBufferSize);
		return CircularAudioBufferChannelView<AudioSampleType>(m_data[getMappedChannel(channel)], m_bufferSize, samplesCount, (m_bufferStartOffset + m_readSampleOffset.load() + startOffset) % m_bufferSize);
	}

	[[nodiscard]] inline CircularAudioBufferChannelView<AudioSampleType> getRangedWriteChannelView(size_t channel, size_t startOffset, size_t samplesCount) const {
		assert(startOffset + samplesCount <= m_singleBufferSize);
		return CircularAudioBufferChannelView<AudioSampleType>(m_data[getMappedChannel(channel)], m_bufferSize, samplesCount, (m_bufferStartOffset + m_writeSampleOffset.load() + startOffset) % m_bufferSize);
	}

	[[nodiscard]] inline OffsettedReadCircularAudioBufferChannelView<AudioSampleType> getRangedOffsettedChannelView(size_t channel, size_t startOffset, size_t samplesCount) const {
		assert(startOffset + samplesCount <= m_singleBufferSize);
		return OffsettedReadCircularAudioBufferChannelView<AudioSampleType>(m_data[getMappedChannel(channel)], m_bufferSize, samplesCount, (m_bufferStartOffset + m_readSampleOffset.load() + startOffset) % m_bufferSize, (m_bufferStartOffset + m_writeSampleOffset.load() + startOffset) % m_bufferSize);
	}

	AudioSampleType** m_data;
	size_t m_bufferSize;
	size_t m_singleBufferSize;
	size_t m_bufferChannelsCount;
	std::vector<size_t> m_channelsMapping;
	size_t m_bufferStartOffset;

	std::atomic<size_t> m_readSampleOffset;
	std::atomic<size_t> m_writeSampleOffset;

};

} // audioBuffers

#endif //AUDIO_BUFFERS_BASICCIRCULARAUDIOBUFFERVIEW_H
