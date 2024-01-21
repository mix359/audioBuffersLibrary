// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef ABL_DELAYEDCIRCULARAUDIOBUFFER_H
#define ABL_DELAYEDCIRCULARAUDIOBUFFER_H

#include "DelayedCircularAudioBufferView.h"
#include "AudioBufferWithMemoryManagement.h"

namespace abl {

template <NumericType AudioSampleType>
class DelayedCircularAudioBuffer : public DelayedCircularAudioBufferView<AudioSampleType>, public AudioBufferWithMemoryManagement<AudioSampleType> {
public:
	explicit DelayedCircularAudioBuffer(size_t bufferSize, size_t singleBufferSize, size_t delayInSamples, size_t channelsCount = 2, size_t bufferStartOffset = 0, const std::vector<size_t>& channelsMapping = {}, size_t startIndex = 0)
		: DelayedCircularAudioBufferView<AudioSampleType>(prepareAllocatedSpace(channelsCount, bufferSize), channelsCount, bufferSize, singleBufferSize, delayInSamples, bufferStartOffset, channelsMapping, startIndex)
	{
		//assert(bufferSize > 0);
		//assert(channelsCount > 0);
	}

	//Full buffer copy
	explicit DelayedCircularAudioBuffer(AudioSampleType **sourceData, size_t channelsCount, size_t bufferSize, size_t singleBufferSize, size_t delayInSamples, size_t bufferStartOffset = 0, const std::vector<size_t>& channelsMapping = {}, size_t startIndex = 0)
		: DelayedCircularAudioBufferView<AudioSampleType>(prepareAllocatedSpace(channelsCount, bufferSize, false), channelsCount, bufferSize, singleBufferSize, delayInSamples, bufferStartOffset, channelsMapping, startIndex)
	{
		for(size_t channel = 0; channel < channelsCount; ++channel) {
			std::copy(sourceData[channel], sourceData[channel] + bufferSize, BasicCircularAudioBufferView<AudioSampleType>::m_data[channel]);
		}
	}

	//Full buffer copy
//	explicit DelayedCircularAudioBuffer(const juce::AudioBuffer<AudioSampleType> &sourceBuffer, size_t singleBufferSize, size_t delayInSamples, size_t bufferStartOffset = 0, const std::vector<size_t>& channelsMapping = {}, size_t startIndex = 0)
//		: DelayedCircularAudioBufferView<AudioSampleType>(prepareAllocatedSpace(sourceBuffer.getNumChannels(), sourceBuffer.getNumSamples(), sourceBuffer.hasBeenCleared()), sourceBuffer.getNumChannels(), sourceBuffer.getNumSamples(), singleBufferSize, delayInSamples, bufferStartOffset, channelsMapping, startIndex)
//	{
//		if(sourceBuffer.getNumChannels() > 0 && sourceBuffer.getNumSamples() > 0) {
//			for(size_t channel = 0; channel < sourceBuffer.getNumChannels(); ++channel) {
//				auto sourceData = sourceBuffer.getReadPointer(channel);
//				std::copy(sourceData[channel], sourceData[channel] + sourceBuffer.getNumSamples(), BasicCircularAudioBufferView<AudioSampleType>::m_data[channel]);
//			}
//		}
//	}

	//Full buffer copy
	explicit DelayedCircularAudioBuffer(const AudioBufferReadableType<AudioSampleType> auto &sourceBuffer, size_t singleBufferSize, size_t delayInSamples, size_t bufferStartOffset = 0, const std::vector<size_t>& channelsMapping = {}, size_t startIndex = 0)
		: DelayedCircularAudioBufferView<AudioSampleType>(prepareAllocatedSpace(sourceBuffer.getChannelsCount(), sourceBuffer.getBufferSize(), sourceBuffer.isEmpty()), sourceBuffer.getChannelsCount(), sourceBuffer.getBufferSize(), singleBufferSize, delayInSamples, bufferStartOffset, channelsMapping, startIndex)
	{
		if(!sourceBuffer.isEmpty()) {
			for(size_t channel = 0; channel < sourceBuffer.getChannelsCount(); ++channel) {
				for(size_t i = 0; i < sourceBuffer.getBufferSize(); ++i) {
					BasicCircularAudioBufferView<AudioSampleType>::m_data[channel][i] = sourceBuffer.getSample(channel, i);
				}
			}
		}
	}

	//@todo create a constructor or factory method that create the buffer from a single buffer?

	DelayedCircularAudioBuffer(const DelayedCircularAudioBuffer &otherBuffer) : DelayedCircularAudioBufferView<AudioSampleType>(
			prepareAllocatedSpace(otherBuffer.m_bufferChannelsCount, otherBuffer.m_bufferSize, otherBuffer.isEmpty()),
			otherBuffer.m_bufferChannelsCount,
			otherBuffer.m_bufferSize,
			otherBuffer.m_singleBufferSize,
			otherBuffer.m_delayInSamples,
			otherBuffer.m_bufferStartOffset,
			otherBuffer.m_channelsMapping,
			otherBuffer.m_index
	) {
		if(!otherBuffer.isEmpty()) {
			for(size_t channel = 0; channel < otherBuffer.getChannelsCount(); ++channel) {
				std::copy(otherBuffer.m_data[channel], otherBuffer.m_data[channel] + otherBuffer.m_bufferSize, BasicCircularAudioBufferView<AudioSampleType>::m_data[channel]);
			}
		}
	}

	DelayedCircularAudioBuffer& operator= (const DelayedCircularAudioBuffer &otherBuffer) {
		BasicCircularAudioBufferView<AudioSampleType>::m_bufferChannelsCount = otherBuffer.m_bufferChannelsCount;
		BasicCircularAudioBufferView<AudioSampleType>::m_bufferSize = otherBuffer.m_bufferSize;
		BasicCircularAudioBufferView<AudioSampleType>::m_singleBufferSize = otherBuffer.m_singleBufferSize;
		BasicCircularAudioBufferView<AudioSampleType>::m_bufferStartOffset = otherBuffer.m_bufferStartOffset;
		BasicCircularAudioBufferView<AudioSampleType>::m_channelsMapping = otherBuffer.m_channelsMapping;
		BasicCircularAudioBufferView<AudioSampleType>::m_readSampleOffset.store(otherBuffer.m_readSampleOffset);
		BasicCircularAudioBufferView<AudioSampleType>::m_writeSampleOffset.store(otherBuffer.m_writeSampleOffset);
		DelayedCircularAudioBufferView<AudioSampleType>::m_delayInSamples.store(otherBuffer.m_delayInSamples.load());
		DelayedCircularAudioBufferView<AudioSampleType>::m_index.store(otherBuffer.m_index.load());

		BasicCircularAudioBufferView<AudioSampleType>::m_data = prepareAllocatedSpace(otherBuffer.m_bufferChannelsCount, otherBuffer.m_bufferSize, otherBuffer.isEmpty());
		if(!otherBuffer.isEmpty()) {
			for(size_t channel = 0; channel < otherBuffer.m_bufferChannelsCount; ++channel) {
				std::copy(otherBuffer.m_data[channel], otherBuffer.m_data[channel] + otherBuffer.m_bufferSize, BasicCircularAudioBufferView<AudioSampleType>::m_data[channel]);
			}
		}

		return *this;
	}

	DelayedCircularAudioBuffer (DelayedCircularAudioBuffer&& otherBuffer) noexcept : DelayedCircularAudioBufferView<AudioSampleType>(
			otherBuffer.m_data,
			otherBuffer.m_bufferChannelsCount,
			otherBuffer.m_bufferSize,
			otherBuffer.m_singleBufferSize,
			otherBuffer.m_delayInSamples,
			otherBuffer.m_bufferStartOffset,
			std::move(otherBuffer.m_channelsMapping),
			otherBuffer.m_index
	) {
		otherBuffer.m_data = nullptr;
		otherBuffer.m_bufferSize = 0;
		otherBuffer.m_singleBufferSize = 0;
		otherBuffer.m_bufferChannelsCount = 0;
		otherBuffer.m_delayInSamples = 0;
		otherBuffer.m_bufferStartOffset = 0;
		otherBuffer.m_channelsMapping = {};
		otherBuffer.m_readSampleOffset = 0;
		otherBuffer.m_writeSampleOffset = 0;
		otherBuffer.m_index = 0;
	}

	DelayedCircularAudioBuffer& operator= (DelayedCircularAudioBuffer&& otherBuffer) noexcept {
		BasicCircularAudioBufferView<AudioSampleType>::m_bufferChannelsCount = otherBuffer.m_bufferChannelsCount;
		BasicCircularAudioBufferView<AudioSampleType>::m_bufferSize = otherBuffer.m_bufferSize;
		BasicCircularAudioBufferView<AudioSampleType>::m_singleBufferSize = otherBuffer.m_singleBufferSize;
		BasicCircularAudioBufferView<AudioSampleType>::m_bufferStartOffset = otherBuffer.m_bufferStartOffset;
		BasicCircularAudioBufferView<AudioSampleType>::m_readSampleOffset.store(otherBuffer.m_readSampleOffset);
		BasicCircularAudioBufferView<AudioSampleType>::m_writeSampleOffset.store(otherBuffer.m_writeSampleOffset);
		BasicCircularAudioBufferView<AudioSampleType>::m_data = otherBuffer.m_data;
		BasicCircularAudioBufferView<AudioSampleType>::m_channelsMapping = std::move(otherBuffer.m_channelsMapping);
		DelayedCircularAudioBufferView<AudioSampleType>::m_delayInSamples.store(otherBuffer.m_delayInSamples.load());
		DelayedCircularAudioBufferView<AudioSampleType>::m_index.store(otherBuffer.m_index.load());
		otherBuffer.m_data = nullptr;
		otherBuffer.m_bufferSize = 0;
		otherBuffer.m_singleBufferSize = 0;
		otherBuffer.m_bufferChannelsCount = 0;
		otherBuffer.m_delayInSamples = 0;
		otherBuffer.m_bufferStartOffset = 0;
		otherBuffer.m_channelsMapping = {};
		otherBuffer.m_readSampleOffset = 0;
		otherBuffer.m_writeSampleOffset = 0;
		otherBuffer.m_index = 0;
		return *this;
	}

	virtual ~DelayedCircularAudioBuffer() {
		setInternalData(nullptr);
	};

	void resize(size_t channelsCount, size_t bufferSize, bool keepExistingContent = false, bool clearExtraSpace = true, bool avoidReallocating = false) override {
		if(channelsCount == BasicCircularAudioBufferView<AudioSampleType>::m_bufferChannelsCount && bufferSize == BasicCircularAudioBufferView<AudioSampleType>::m_bufferSize) {
			return;
		}

		AudioBufferWithMemoryManagement<AudioSampleType>::doResize(channelsCount, bufferSize, keepExistingContent, clearExtraSpace, avoidReallocating,
		                                                           BasicCircularAudioBufferView<AudioSampleType>::m_bufferChannelsCount,
		                                                           BasicCircularAudioBufferView<AudioSampleType>::m_bufferSize,
		                                                           BasicCircularAudioBufferView<AudioSampleType>::m_data);

		BasicCircularAudioBufferView<AudioSampleType>::m_bufferChannelsCount = channelsCount;
		BasicCircularAudioBufferView<AudioSampleType>::m_bufferSize = bufferSize;
	}

protected:
	AudioSampleType** prepareAllocatedSpace(size_t channelsCount, size_t bufferSize, bool zeroData = true) override {
		//@todo use preallocated space? (like string that have a basic space, and if you don't use more than that it already come with the object)

		return allocateContiguous2DArray<AudioSampleType>(channelsCount, bufferSize, zeroData);
	}

	void setInternalData(AudioSampleType** newData, bool deleteOld = true) override {
		if(deleteOld && BasicCircularAudioBufferView<AudioSampleType>::m_data) {
			deallocateContiguous2DArray<AudioSampleType>(BasicCircularAudioBufferView<AudioSampleType>::m_data);
		}
		BasicCircularAudioBufferView<AudioSampleType>::m_data = newData;
	}
};


} // abl

#endif //ABL_DELAYEDCIRCULARAUDIOBUFFER_H
