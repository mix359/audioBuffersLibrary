// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef AUDIO_BUFFERS_DELAYEDCIRCULARAUDIOBUFFERVIEW_H
#define AUDIO_BUFFERS_DELAYEDCIRCULARAUDIOBUFFERVIEW_H

#include "BasicCircularAudioBufferView.h"

namespace audioBuffers {

template <NumericType AudioSampleType>
class DelayedCircularAudioBufferView : public BasicCircularAudioBufferView<AudioSampleType>, public DelayedCircularAudioBufferViewInterface<AudioSampleType> {
public:
//	DelayedCircularAudioBufferView(const juce::AudioBuffer<AudioSampleType> &buffer, size_t singleBufferSize, size_t delayInSamples, size_t bufferStartOffset = 0, const std::vector<size_t>& channelsMapping = {}, size_t startIndex = 0)
//			: BasicCircularAudioBufferView<AudioSampleType>(buffer, singleBufferSize, bufferStartOffset, channelsMapping), m_index{startIndex}, m_delayInSamples{delayInSamples}
//	{
//		BasicCircularAudioBufferView<AudioSampleType>::m_readSampleOffset.store(startIndex % BasicCircularAudioBufferView<AudioSampleType>::m_bufferSize);
//		BasicCircularAudioBufferView<AudioSampleType>::m_writeSampleOffset.store(BasicCircularAudioBufferView<AudioSampleType>::m_readSampleOffset + m_delayInSamples);
//	}

	DelayedCircularAudioBufferView(AudioSampleType **data, size_t channelsCount, size_t bufferSize, size_t singleBufferSize, size_t delayInSamples, size_t bufferStartOffset = 0, const std::vector<size_t>& channelsMapping = {}, size_t startIndex = 0)
			: BasicCircularAudioBufferView<AudioSampleType>(data, channelsCount, bufferSize, singleBufferSize, bufferStartOffset, channelsMapping), m_index{startIndex}, m_delayInSamples{delayInSamples}
	{
		BasicCircularAudioBufferView<AudioSampleType>::m_readSampleOffset.store(startIndex % BasicCircularAudioBufferView<AudioSampleType>::m_bufferSize);
		BasicCircularAudioBufferView<AudioSampleType>::m_writeSampleOffset.store(BasicCircularAudioBufferView<AudioSampleType>::m_readSampleOffset + m_delayInSamples);
	}

	DelayedCircularAudioBufferView(const DelayedCircularAudioBufferView& otherBuffer)
			: BasicCircularAudioBufferView<AudioSampleType>(otherBuffer)
	{
		m_index.store(otherBuffer.m_index.load());
		m_delayInSamples.store(otherBuffer.m_delayInSamples.load());
	}

	DelayedCircularAudioBufferView(DelayedCircularAudioBufferView&& otherBuffer) noexcept
			: BasicCircularAudioBufferView<AudioSampleType>(std::move(otherBuffer))
	{
		m_index.store(otherBuffer.m_index.load());
		m_delayInSamples.store(otherBuffer.m_delayInSamples.load());
		otherBuffer.m_index = 0;
		otherBuffer.m_delayInSamples = 0;
	}

	std::unique_ptr<AudioBufferViewInterface<AudioSampleType>> getRangedView(SamplesRange samplesRange) override {
		auto sampleOffset = samplesRange.startSample + BasicCircularAudioBufferView<AudioSampleType>::m_bufferStartOffset;
		auto& bufferSize = BasicCircularAudioBufferView<AudioSampleType>::m_bufferSize;
		if(sampleOffset > bufferSize) sampleOffset -= bufferSize;
		return std::make_unique<DelayedCircularAudioBufferView>(
				BasicCircularAudioBufferView<AudioSampleType>::m_data,
				BasicCircularAudioBufferView<AudioSampleType>::m_bufferChannelsCount,
				bufferSize,
				samplesRange.getRealSamplesCount(BasicCircularAudioBufferView<AudioSampleType>::m_singleBufferSize),
				m_delayInSamples,
				sampleOffset,
				BasicCircularAudioBufferView<AudioSampleType>::m_channelsMapping
		);
	}

	void incrementIndex(std::optional<size_t> increment = {}) noexcept override {
		m_index.store(m_index.load() + (increment.has_value() ? increment.value() : BasicCircularAudioBufferView<AudioSampleType>::m_singleBufferSize));
		BasicCircularAudioBufferView<AudioSampleType>::m_readSampleOffset.store(m_index % BasicCircularAudioBufferView<AudioSampleType>::m_bufferSize);
		BasicCircularAudioBufferView<AudioSampleType>::m_writeSampleOffset.store(BasicCircularAudioBufferView<AudioSampleType>::m_readSampleOffset + m_delayInSamples);
	}

	void resetIndex() noexcept override {
		m_index = 0;
		BasicCircularAudioBufferView<AudioSampleType>::m_readSampleOffset.store(0);
		BasicCircularAudioBufferView<AudioSampleType>::m_writeSampleOffset.store(m_delayInSamples);
	}

	size_t getIndex() const noexcept override { return m_index.load(); }

	size_t getDelayInSamples() const noexcept override { return m_delayInSamples.load(); }
	void setDelayInSamples(size_t delay) noexcept override {
		m_delayInSamples.store(delay);
		BasicCircularAudioBufferView<AudioSampleType>::m_writeSampleOffset.store(BasicCircularAudioBufferView<AudioSampleType>::m_readSampleOffset + m_delayInSamples);
		//@todo resample the difference?
	}

	[[nodiscard]] size_t getBaseBufferSize() const noexcept override { return BasicCircularAudioBufferView<AudioSampleType>::m_bufferSize; }

protected:
	std::atomic<size_t> m_index;
	std::atomic<size_t> m_delayInSamples;
};

} // engine::showmanager

#endif //AUDIO_BUFFERS_DELAYEDCIRCULARAUDIOBUFFERVIEW_H
