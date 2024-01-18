// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef AUDIO_BUFFERS_CIRCULARAUDIOBUFFERVIEW_H
#define AUDIO_BUFFERS_CIRCULARAUDIOBUFFERVIEW_H

#include "BasicCircularAudioBufferView.h"

namespace audioBuffers {

template <NumericType AudioSampleType>
class CircularAudioBufferView : public BasicCircularAudioBufferView<AudioSampleType>, public CircularAudioBufferViewInterface<AudioSampleType> {
public:
//	CircularAudioBufferView(const juce::AudioBuffer<AudioSampleType> &buffer, size_t singleBufferSize, size_t bufferStartOffset = 0, const std::vector<size_t>& channelsMapping = {}, size_t startReadIndex = 0, size_t startWriteIndex = 0)
//			: BasicCircularAudioBufferView<AudioSampleType>(buffer, singleBufferSize, bufferStartOffset, channelsMapping), m_readIndex{startReadIndex}, m_writeIndex{startWriteIndex} {}

	CircularAudioBufferView(AudioSampleType **data, size_t channelsCount, size_t bufferSize, size_t singleBufferSize, size_t bufferStartOffset = 0, const std::vector<size_t>& channelsMapping = {}, size_t startReadIndex = 0, size_t startWriteIndex = 0)
			: BasicCircularAudioBufferView<AudioSampleType>(data, channelsCount, bufferSize, singleBufferSize, bufferStartOffset, channelsMapping), m_readIndex{startReadIndex}, m_writeIndex{startWriteIndex} {}

	CircularAudioBufferView(const CircularAudioBufferView& otherBuffer)
			: BasicCircularAudioBufferView<AudioSampleType>(otherBuffer)
	{
		m_readIndex.store(otherBuffer.m_readIndex.load());
		m_writeIndex.store(otherBuffer.m_writeIndex.load());
	}

	CircularAudioBufferView(CircularAudioBufferView&& otherBuffer) noexcept
			: BasicCircularAudioBufferView<AudioSampleType>(std::move(otherBuffer))
	{
		m_readIndex.store(otherBuffer.m_readIndex.load());
		m_writeIndex.store(otherBuffer.m_writeIndex.load());
		otherBuffer.m_readIndex = 0;
		otherBuffer.m_writeIndex = 0;
	}

	std::unique_ptr<AudioBufferViewInterface<AudioSampleType>> getRangedView(SamplesRange samplesRange) override {
		auto sampleOffset = samplesRange.startSample + BasicCircularAudioBufferView<AudioSampleType>::m_bufferStartOffset;
		auto& bufferSize = BasicCircularAudioBufferView<AudioSampleType>::m_bufferSize;
		if(sampleOffset > bufferSize) sampleOffset -= bufferSize;
		return std::make_unique<CircularAudioBufferView>(
				BasicCircularAudioBufferView<AudioSampleType>::m_data,
				BasicCircularAudioBufferView<AudioSampleType>::m_bufferChannelsCount,
				bufferSize,
				samplesRange.getRealSamplesCount(BasicCircularAudioBufferView<AudioSampleType>::m_singleBufferSize),
				sampleOffset,
				BasicCircularAudioBufferView<AudioSampleType>::m_channelsMapping
		);
	}

	void incrementReadIndex(std::optional<size_t> increment = {}) noexcept override {
		auto incrementValue = increment.has_value() ? increment.value() : BasicCircularAudioBufferView<AudioSampleType>::m_singleBufferSize;
		assert(m_readIndex + incrementValue <= m_writeIndex); //@todo bound the increment value to save from this case?
		m_readIndex.store(m_readIndex.load() + incrementValue);
		BasicCircularAudioBufferView<AudioSampleType>::m_readSampleOffset.store(m_readIndex % BasicCircularAudioBufferView<AudioSampleType>::m_bufferSize);
	}

	void incrementWriteIndex(std::optional<size_t> increment = {}) noexcept override {
		m_writeIndex.store(m_writeIndex.load() + (increment.has_value() ? increment.value() : BasicCircularAudioBufferView<AudioSampleType>::m_singleBufferSize));
		BasicCircularAudioBufferView<AudioSampleType>::m_writeSampleOffset.store(m_writeIndex % BasicCircularAudioBufferView<AudioSampleType>::m_bufferSize);
	}

	void resetWriteIndexToReadIndexPosition() noexcept override {
		m_writeIndex.store(m_readIndex.load());
		BasicCircularAudioBufferView<AudioSampleType>::m_writeSampleOffset.store(BasicCircularAudioBufferView<AudioSampleType>::m_readSampleOffset.load());
	}

	void resetIndexes() noexcept override {
		m_readIndex = 0;
		m_writeIndex = 0;
		BasicCircularAudioBufferView<AudioSampleType>::m_readSampleOffset = 0;
		BasicCircularAudioBufferView<AudioSampleType>::m_writeSampleOffset = 0;
	}

	size_t getReadIndex() const noexcept override { return m_readIndex.load();}
	size_t getWriteIndex() const noexcept override { return m_writeIndex.load(); }

	[[nodiscard]] bool isDataAvailable() const noexcept override {
		return m_writeIndex > m_readIndex;
	}

	[[nodiscard]] size_t getBaseBufferSize() const noexcept override { return BasicCircularAudioBufferView<AudioSampleType>::m_bufferSize; }

protected:
	std::atomic<size_t> m_readIndex;
	std::atomic<size_t> m_writeIndex;
};

} // engine::showmanager

#endif //AUDIO_BUFFERS_CIRCULARAUDIOBUFFERVIEW_H
