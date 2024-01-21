// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef ABL_CIRCULARAUDIOBUFFER_H
#define ABL_CIRCULARAUDIOBUFFER_H

#include "CircularAudioBufferView.h"
#include "AudioBufferWithMemoryManagement.h"
#include "AudioBufferViewConcepts.h"

namespace abl {

template <NumericType AudioSampleType>
class CircularAudioBuffer : public CircularAudioBufferView<AudioSampleType>, public AudioBufferWithMemoryManagement<AudioSampleType> {
public:

	explicit CircularAudioBuffer(size_t bufferSize, size_t singleBufferSize, size_t channelsCount = 2, size_t bufferStartOffset = 0, const std::vector<size_t>& channelsMapping = {}, size_t startReadIndex = 0, size_t startWriteIndex = 0)
		: CircularAudioBufferView<AudioSampleType>(prepareAllocatedSpace(channelsCount, bufferSize), channelsCount, bufferSize, singleBufferSize, bufferStartOffset, channelsMapping, startReadIndex, startWriteIndex)
	{
		//assert(bufferSize > 0);
		//assert(channelsCount > 0);
	}

	static CircularAudioBuffer createFor(size_t singleBuffersCount, size_t singleBufferSize, size_t channelsCount = 2, size_t bufferStartOffset = 0, const std::vector<size_t>& channelsMapping = {}, size_t startReadIndex = 0, size_t startWriteIndex = 0) {
		return CircularAudioBuffer(singleBuffersCount * singleBufferSize, singleBufferSize, channelsCount, bufferStartOffset, channelsMapping, startReadIndex, startWriteIndex);
	}

	//Full buffer copy
	explicit CircularAudioBuffer(AudioSampleType **sourceData, size_t channelsCount, size_t bufferSize, size_t singleBufferSize, size_t bufferStartOffset = 0, const std::vector<size_t>& channelsMapping = {}, size_t startReadIndex = 0, size_t startWriteIndex = 0)
		: CircularAudioBufferView<AudioSampleType>(prepareAllocatedSpace(channelsCount, bufferSize, false), channelsCount, bufferSize, singleBufferSize, bufferStartOffset, channelsMapping, startReadIndex, startWriteIndex)
	{
		for(size_t channel = 0; channel < channelsCount; ++channel) {
			std::copy(sourceData[channel], sourceData[channel] + bufferSize, BasicCircularAudioBufferView<AudioSampleType>::m_data[channel]);
		}
	}

	//Full buffer copy
//	explicit CircularAudioBuffer(const juce::AudioBuffer<AudioSampleType> &sourceBuffer, size_t singleBufferSize, size_t bufferStartOffset = 0, const std::vector<size_t>& channelsMapping = {}, size_t startReadIndex = 0, size_t startWriteIndex = 0)
//		: CircularAudioBufferView<AudioSampleType>(prepareAllocatedSpace(sourceBuffer.getNumChannels(), sourceBuffer.getNumSamples(), sourceBuffer.hasBeenCleared()), sourceBuffer.getNumChannels(), sourceBuffer.getNumSamples(), singleBufferSize, bufferStartOffset, channelsMapping, startReadIndex, startWriteIndex)
//	{
//		if(sourceBuffer.getNumChannels() > 0 && sourceBuffer.getNumSamples() > 0) {
//			for(size_t channel = 0; channel < sourceBuffer.getNumChannels(); ++channel) {
//				auto sourceData = sourceBuffer.getReadPointer(channel);
//				std::copy(sourceData[channel], sourceData[channel] + sourceBuffer.getNumSamples(), BasicCircularAudioBufferView<AudioSampleType>::m_data[channel]);
//			}
//		}
//	}

	//Full buffer copy
	explicit CircularAudioBuffer(const AudioBufferReadableType<AudioSampleType> auto &sourceBuffer, size_t singleBufferSize, size_t bufferStartOffset = 0, const std::vector<size_t>& channelsMapping = {}, size_t startReadIndex = 0, size_t startWriteIndex = 0)
		: CircularAudioBufferView<AudioSampleType>(prepareAllocatedSpace(sourceBuffer.getChannelsCount(), sourceBuffer.getBufferSize(), sourceBuffer.isEmpty()), sourceBuffer.getChannelsCount(), sourceBuffer.getBufferSize(), singleBufferSize, bufferStartOffset, channelsMapping, startReadIndex, startWriteIndex)
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

	CircularAudioBuffer(const CircularAudioBuffer &otherBuffer) : CircularAudioBufferView<AudioSampleType>(
			prepareAllocatedSpace(otherBuffer.m_bufferChannelsCount, otherBuffer.m_bufferSize, otherBuffer.isEmpty()),
			otherBuffer.m_bufferChannelsCount,
			otherBuffer.m_bufferSize,
			otherBuffer.m_singleBufferSize,
			otherBuffer.m_bufferStartOffset,
			otherBuffer.m_channelsMapping,
			otherBuffer.m_readIndex,
			otherBuffer.m_writeIndex
	) {
		if(!otherBuffer.isEmpty()) {
			for(size_t channel = 0; channel < otherBuffer.getChannelsCount(); ++channel) {
				std::copy(otherBuffer.m_data[channel], otherBuffer.m_data[channel] + otherBuffer.m_bufferSize, BasicCircularAudioBufferView<AudioSampleType>::m_data[channel]);
			}
		}
	}

	CircularAudioBuffer& operator= (const CircularAudioBuffer &otherBuffer) {
		BasicCircularAudioBufferView<AudioSampleType>::m_bufferChannelsCount = otherBuffer.m_bufferChannelsCount;
		BasicCircularAudioBufferView<AudioSampleType>::m_bufferSize = otherBuffer.m_bufferSize;
		BasicCircularAudioBufferView<AudioSampleType>::m_singleBufferSize = otherBuffer.m_singleBufferSize;
		BasicCircularAudioBufferView<AudioSampleType>::m_bufferStartOffset = otherBuffer.m_bufferStartOffset;
		BasicCircularAudioBufferView<AudioSampleType>::m_channelsMapping = otherBuffer.m_channelsMapping;
		BasicCircularAudioBufferView<AudioSampleType>::m_readSampleOffset.store(otherBuffer.m_readSampleOffset);
		BasicCircularAudioBufferView<AudioSampleType>::m_writeSampleOffset.store(otherBuffer.m_writeSampleOffset);
		CircularAudioBufferView<AudioSampleType>::m_readIndex.store(otherBuffer.m_readIndex.load());
		CircularAudioBufferView<AudioSampleType>::m_writeIndex.store(otherBuffer.m_writeIndex.load());

		BasicCircularAudioBufferView<AudioSampleType>::m_data = prepareAllocatedSpace(otherBuffer.m_bufferChannelsCount, otherBuffer.m_bufferSize, otherBuffer.isEmpty());
		if(!otherBuffer.isEmpty()) {
			for(size_t channel = 0; channel < otherBuffer.m_bufferChannelsCount; ++channel) {
				std::copy(otherBuffer.m_data[channel], otherBuffer.m_data[channel] + otherBuffer.m_bufferSize, BasicCircularAudioBufferView<AudioSampleType>::m_data[channel]);
			}
		}

		return *this;
	}

	CircularAudioBuffer (CircularAudioBuffer&& otherBuffer) noexcept : CircularAudioBufferView<AudioSampleType>(
			otherBuffer.m_data,
			otherBuffer.m_bufferChannelsCount,
			otherBuffer.m_bufferSize,
			otherBuffer.m_singleBufferSize,
			otherBuffer.m_bufferStartOffset,
			std::move(otherBuffer.m_channelsMapping),
			otherBuffer.m_readIndex,
			otherBuffer.m_writeIndex
	) {
		otherBuffer.m_data = nullptr;
		otherBuffer.m_bufferSize = 0;
		otherBuffer.m_singleBufferSize = 0;
		otherBuffer.m_bufferChannelsCount = 0;
		otherBuffer.m_bufferStartOffset = 0;
		otherBuffer.m_channelsMapping = {};
		otherBuffer.m_readSampleOffset = 0;
		otherBuffer.m_writeSampleOffset = 0;
		otherBuffer.m_readIndex = 0;
		otherBuffer.m_writeIndex = 0;
	}

	CircularAudioBuffer& operator= (CircularAudioBuffer&& otherBuffer) noexcept {
		BasicCircularAudioBufferView<AudioSampleType>::m_bufferChannelsCount = otherBuffer.m_bufferChannelsCount;
		BasicCircularAudioBufferView<AudioSampleType>::m_bufferSize = otherBuffer.m_bufferSize;
		BasicCircularAudioBufferView<AudioSampleType>::m_singleBufferSize = otherBuffer.m_singleBufferSize;
		BasicCircularAudioBufferView<AudioSampleType>::m_bufferStartOffset = otherBuffer.m_bufferStartOffset;
		BasicCircularAudioBufferView<AudioSampleType>::m_readSampleOffset.store(otherBuffer.m_readSampleOffset);
		BasicCircularAudioBufferView<AudioSampleType>::m_writeSampleOffset.store(otherBuffer.m_writeSampleOffset);
		BasicCircularAudioBufferView<AudioSampleType>::m_data = otherBuffer.m_data;
		BasicCircularAudioBufferView<AudioSampleType>::m_channelsMapping = std::move(otherBuffer.m_channelsMapping);
		CircularAudioBufferView<AudioSampleType>::m_readIndex.store(otherBuffer.m_readIndex.load());
		CircularAudioBufferView<AudioSampleType>::m_writeIndex.store(otherBuffer.m_writeIndex.load());
		otherBuffer.m_data = nullptr;
		otherBuffer.m_bufferSize = 0;
		otherBuffer.m_singleBufferSize = 0;
		otherBuffer.m_bufferChannelsCount = 0;
		otherBuffer.m_bufferStartOffset = 0;
		otherBuffer.m_channelsMapping = {};
		otherBuffer.m_readSampleOffset = 0;
		otherBuffer.m_writeSampleOffset = 0;
		otherBuffer.m_readIndex = 0;
		otherBuffer.m_writeIndex = 0;
		return *this;
	}

	virtual ~CircularAudioBuffer() {
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

#endif //ABL_CIRCULARAUDIOBUFFER_H
