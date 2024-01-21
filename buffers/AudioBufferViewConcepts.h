// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef AUDIOBUFFERS_AUDIOBUFFERVIEWCONCEPTS_H
#define AUDIOBUFFERS_AUDIOBUFFERVIEWCONCEPTS_H

#include <numeric>

#include "../datatypes/NumericConcept.h"
#include "../datatypes/SamplesRange.h"
#include "AudioBufferChannelViewWrapper.h"

namespace abl {

template<typename T, typename SampleType>
concept AudioBufferReadableType = NumericType<SampleType> && requires(T audioBuffer, size_t channel, size_t index, SamplesRange samplesRange)
{
	{ audioBuffer.isEmpty() } -> std::same_as<bool>;
	{ audioBuffer.operator[](index) } -> std::same_as<AudioBufferChannelViewWrapper<SampleType>>;
	//@todo test const operator[]
	//@todo test getRangedView?
	{ audioBuffer.getChannelView(index, samplesRange) } -> std::same_as<AudioBufferChannelViewWrapper<SampleType>>;
	{ audioBuffer.getSample(channel, index) } -> std::same_as<SampleType>;
	{ audioBuffer.getHigherPeak(samplesRange) } -> std::same_as<SampleType>;
	{ audioBuffer.getHigherPeakForChannel(channel, samplesRange) } -> std::same_as<SampleType>;
	{ audioBuffer.getRMSLevelForChannel(channel, samplesRange) } -> std::same_as<SampleType>;
	{ audioBuffer.getBufferSize() } -> std::same_as<size_t>;
	{ audioBuffer.getChannelsCount() } -> std::same_as<size_t>;
	{ audioBuffer.getChannelsMapping() } -> std::same_as<const std::vector<size_t>&>;
};

template<typename T, typename SampleType>
concept AudioBufferType = AudioBufferReadableType<T, SampleType> &&
                                 requires(T audioBuffer, size_t channel, size_t index, SampleType sampleType, SamplesRange samplesRange,
										 T::GainType gainType, const std::vector<size_t>& channelsMapping, size_t startChannel, size_t channelsCount)
{
	{ audioBuffer.setSample(channel, index, sampleType) };
	{ audioBuffer.addSample(channel, index, sampleType) };
	//@todo copy/add function testable?
	//	{ audioBuffer.copyFrom(sourceBuffer, samplesRange, gainType) };
	//	{ audioBuffer.copyWithRampFrom(sourceBuffer, gainType, gainType, samplesRange) };
	//	{ audioBuffer.copyIntoChannelFrom(sourceBufferChannel, channel, samplesRange, gainType) };
	//	{ audioBuffer.copyIntoChannelWithRampFrom(sourceBufferChannel, channel, gainType, gainType, samplesRange) };
	//	{ audioBuffer.addFrom(sourceBuffer, samplesRange, gainType) };
	//	{ audioBuffer.addWithRampFrom(sourceBuffer, gainType, gainType, samplesRange) };
	//	{ audioBuffer.addIntoChannelFrom(sourceBufferChannel, channel, samplesRange, gainType) };
	//	{ audioBuffer.addIntoChannelWithRampFrom(sourceBufferChannel, channel, gainType, gainType, samplesRange) };
	{ audioBuffer.applyGain(gainType, samplesRange) };
	{ audioBuffer.applyGainToChannel(gainType, channel, samplesRange) };
	{ audioBuffer.applyGainRamp(gainType, gainType, samplesRange) };
	{ audioBuffer.applyGainRampToChannel(gainType, gainType, channel, samplesRange) };
	{ audioBuffer.clear(samplesRange) };
	{ audioBuffer.clearChannel(channel, samplesRange) };
	{ audioBuffer.reverse(samplesRange) };
	{ audioBuffer.reverseChannel(channel, samplesRange) };
	{ audioBuffer.setChannelsMapping(channelsMapping) };
	{ audioBuffer.createSequentialChannelsMapping(startChannel, channelsCount) };
	//@todo test iterator?
};

template<typename T, typename SampleType>
concept CircularAudioBufferReadableType = AudioBufferReadableType<T, SampleType> &&
	requires(T audioBuffer, std::optional<size_t> increment)
{
	{ audioBuffer.getReadIndex() } -> std::same_as<size_t>;
	{ audioBuffer.getWriteIndex() } -> std::same_as<size_t>;
	{ audioBuffer.isDataAvailable() } -> std::same_as<bool>;
	{ audioBuffer.getBaseBufferSize() } -> std::same_as<size_t>;
};

template<typename T, typename SampleType>
concept CircularAudioBufferType = AudioBufferReadableType<T, SampleType> &&
	CircularAudioBufferReadableType<T, SampleType> &&
	requires(T audioBuffer, std::optional<size_t> increment)
{
	{ audioBuffer.incrementReadIndex(increment) };
	{ audioBuffer.incrementWriteIndex(increment) };
	{ audioBuffer.resetWriteIndexToReadIndexPosition() };
	{ audioBuffer.resetIndexes() };
};

template<typename T, typename SampleType>
concept DelayedCircularAudioBufferReadableType = AudioBufferReadableType<T, SampleType> &&
	requires(T audioBuffer, std::optional<size_t> increment)
{
	{ audioBuffer.getIndex() } -> std::same_as<size_t>;
	{ audioBuffer.getDelayInSamples() } -> std::same_as<size_t>;
	{ audioBuffer.getBaseBufferSize() } -> std::same_as<size_t>;
};

template<typename T, typename SampleType>
concept DelayedCircularAudioBufferType = AudioBufferReadableType<T, SampleType> &&
	DelayedCircularAudioBufferReadableType<T, SampleType> &&
	requires(T audioBuffer, std::optional<size_t> increment, size_t delay)
{
	{ audioBuffer.incrementIndex(increment) };
	{ audioBuffer.resetIndex() };
	{ audioBuffer.setDelayInSamples(delay) };
};

} // abl

#endif //AUDIOBUFFERS_AUDIOBUFFERVIEWCONCEPTS_H
