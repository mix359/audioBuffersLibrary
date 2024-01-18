// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef AUDIO_BUFFERS_AUDIOBUFFER_H
#define AUDIO_BUFFERS_AUDIOBUFFER_H

#include "AudioBufferInterface.h"
#include "AudioBufferView.h"
#include "AudioBufferWithMemoryManagement.h"

namespace audioBuffers {

template <NumericType AudioSampleType>
class AudioBuffer : public AudioBufferView<AudioSampleType>, public AudioBufferWithMemoryManagement<AudioSampleType> {
public:
	//@todo empty constructor? (no size or channel count)

	explicit AudioBuffer(size_t bufferSize, size_t channelsCount = 2, const std::vector<size_t>& channelsMapping = {})
		: AudioBufferView<AudioSampleType>(prepareAllocatedSpace(channelsCount, bufferSize), channelsCount, bufferSize, channelsMapping)
	{
		//assert(bufferSize > 0);
		//assert(channelsCount > 0);
	}

	explicit AudioBuffer(AudioSampleType **sourceData, size_t channelsCount, size_t bufferSize, const std::vector<size_t>& channelsMapping = {})
		: AudioBufferView<AudioSampleType>(prepareAllocatedSpace(channelsCount, bufferSize, false), channelsCount, bufferSize, channelsMapping)
	{
		//assert(sourceData);
		//assert(bufferSize > 0);
		//assert(channelsCount > 0);

		for(size_t channel = 0; channel < channelsCount; ++channel) {
			std::copy(sourceData[channel], sourceData[channel] + bufferSize, AudioBufferView<AudioSampleType>::m_data[channel]);
		}
	}

//	explicit AudioBuffer(const juce::AudioBuffer<AudioSampleType> &sourceBuffer, const std::vector<size_t>& channelsMapping = {})
//		: AudioBufferView<AudioSampleType>(prepareAllocatedSpace(sourceBuffer.getNumChannels(), sourceBuffer.getNumSamples(), sourceBuffer.hasBeenCleared()), sourceBuffer.getNumChannels(), sourceBuffer.getNumSamples(), channelsMapping)
//	{
//		if(sourceBuffer.getNumChannels() > 0 && sourceBuffer.getNumSamples() > 0) {
//			for(size_t channel = 0; channel < sourceBuffer.getNumChannels(); ++channel) {
//				auto sourceData = sourceBuffer.getReadPointer(channel);
//				std::copy(sourceData[channel], sourceData[channel] + sourceBuffer.getNumSamples(), AudioBufferView<AudioSampleType>::m_data[channel]);
//			}
//		}
//	}

	explicit AudioBuffer(const AudioBufferViewInterface<AudioSampleType> &sourceBuffer, const std::vector<size_t>& channelsMapping = {})
		: AudioBufferView<AudioSampleType>(prepareAllocatedSpace(sourceBuffer.getChannelsCount(), sourceBuffer.getBufferSize(), sourceBuffer.isEmpty()), sourceBuffer.getChannelsCount(), sourceBuffer.getBufferSize(), channelsMapping)
	{
		if(!sourceBuffer.isEmpty()) {
			for(size_t channel = 0; channel < sourceBuffer.getChannelsCount(); ++channel) {
				for(size_t i = 0; i < sourceBuffer.getBufferSize(); ++i) {
					AudioBufferView<AudioSampleType>::m_data[channel][i] = sourceBuffer.getSample(channel, i);
				}
			}
		}
	}

	AudioBuffer(const AudioBuffer &otherBuffer)
		: AudioBufferView<AudioSampleType>(prepareAllocatedSpace(otherBuffer.getChannelsCount(), otherBuffer.getBufferSize(), otherBuffer.isEmpty()), otherBuffer.getChannelsCount(), otherBuffer.getBufferSize(), otherBuffer.getChannelsMapping())
	{
		if(!otherBuffer.isEmpty()) {
			for(size_t channel = 0; channel < otherBuffer.getChannelsCount(); ++channel) {
				std::copy(otherBuffer.m_data[channel], otherBuffer.m_data[channel] + otherBuffer.getBufferSize(), AudioBufferView<AudioSampleType>::m_data[channel]);
			}
		}
	}

	AudioBuffer& operator= (const AudioBuffer &otherBuffer) {
		AudioBufferView<AudioSampleType>::m_bufferChannelsCount = otherBuffer.getChannelsCount();
		AudioBufferView<AudioSampleType>::m_bufferSize = otherBuffer.getBufferSize();

		AudioBufferView<AudioSampleType>::m_data = prepareAllocatedSpace(otherBuffer.getChannelsCount(), otherBuffer.getBufferSize(), otherBuffer.isEmpty());
		if(!otherBuffer.isEmpty()) {
			for(size_t channel = 0; channel < otherBuffer.getChannelsCount(); ++channel) {
				std::copy(otherBuffer.m_data[channel], otherBuffer.m_data[channel] + otherBuffer.getBufferSize(), AudioBufferView<AudioSampleType>::m_data[channel]);
			}
		}

		AudioBufferView<AudioSampleType>::m_channelsMapping = otherBuffer.getChannelsMapping();
		return *this;
	}

	AudioBuffer (AudioBuffer&& otherBuffer) noexcept
		: AudioBufferView<AudioSampleType>(otherBuffer.m_data, otherBuffer.m_bufferChannelsCount, otherBuffer.m_bufferSize, otherBuffer.m_channelsMapping) {
		otherBuffer.m_data = nullptr;
		otherBuffer.m_bufferSize = 0;
		otherBuffer.m_bufferChannelsCount = 0;
		otherBuffer.m_channelsMapping = {};
	}

	AudioBuffer& operator= (AudioBuffer&& otherBuffer) noexcept {
		AudioBufferView<AudioSampleType>::m_bufferChannelsCount = otherBuffer.m_bufferChannelsCount;
		AudioBufferView<AudioSampleType>::m_bufferSize = otherBuffer.m_bufferSize;
		AudioBufferView<AudioSampleType>::m_data = otherBuffer.m_data;
		AudioBufferView<AudioSampleType>::m_channelsMapping = std::move(otherBuffer.m_channelsMapping);
		otherBuffer.m_data = nullptr;
		otherBuffer.m_bufferSize = 0;
		otherBuffer.m_bufferChannelsCount = 0;
		return *this;
	}

	virtual ~AudioBuffer() {
		setInternalData(nullptr);
	};

	void resize(size_t channelsCount, size_t bufferSize, bool keepExistingContent = false, bool clearExtraSpace = true, bool avoidReallocating = false) override {
		if(channelsCount == AudioBufferView<AudioSampleType>::m_bufferChannelsCount && bufferSize == AudioBufferView<AudioSampleType>::m_bufferSize) {
			return;
		}

		AudioBufferWithMemoryManagement<AudioSampleType>::doResize(channelsCount, bufferSize, keepExistingContent, clearExtraSpace, avoidReallocating,
																   AudioBufferView<AudioSampleType>::m_bufferChannelsCount,
																   AudioBufferView<AudioSampleType>::m_bufferSize,
																   AudioBufferView<AudioSampleType>::m_data);

		AudioBufferView<AudioSampleType>::m_bufferChannelsCount = channelsCount;
		AudioBufferView<AudioSampleType>::m_bufferSize = bufferSize;
	}

protected:
	AudioSampleType** prepareAllocatedSpace(size_t channelsCount, size_t bufferSize, bool zeroData = true) override {
		//@todo use preallocated space? (like string that have a basic space, and if you don't use more than that it already come with the object)

		return allocateContiguous2DArray<AudioSampleType>(channelsCount, bufferSize, zeroData);
	}

	void setInternalData(AudioSampleType** newData, bool deleteOld = true) override {
		if(deleteOld && AudioBufferView<AudioSampleType>::m_data) {
			deallocateContiguous2DArray<AudioSampleType>(AudioBufferView<AudioSampleType>::m_data);
		}
		AudioBufferView<AudioSampleType>::m_data = newData;
	}

};

} // audioBuffers

#endif //AUDIO_BUFFERS_AUDIOBUFFER_H
