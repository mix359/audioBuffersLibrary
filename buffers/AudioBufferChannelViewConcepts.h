// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef AUDIOBUFFERS_AUDIOBUFFERCHANNELVIEWCONCEPTS_H
#define AUDIOBUFFERS_AUDIOBUFFERCHANNELVIEWCONCEPTS_H

#include <numeric>

#include "../datatypes/NumericConcept.h"
#include "../datatypes/SamplesRange.h"

namespace abl {

template<typename T, typename SampleType>
concept AudioBufferChannelReadableType = NumericType<SampleType> && requires(T audioBufferChannel, size_t index, SamplesRange samplesRange)
{
	//@todo test iterator?
	{ audioBufferChannel.isEmpty() } -> std::same_as<bool>;
	{ audioBufferChannel.operator[](index) } -> std::same_as<SampleType&>;
	//@todo test const operator[]
	{ audioBufferChannel.getSample(index) } -> std::same_as<SampleType>;
	{ audioBufferChannel.getHigherPeak(samplesRange) } -> std::same_as<SampleType>;
	{ audioBufferChannel.getRMSLevel(samplesRange) } -> std::same_as<SampleType>;
	{ audioBufferChannel.getBufferSize() } -> std::same_as<size_t>;
};

template<typename T, typename SampleType>
concept AudioBufferChannelType = AudioBufferChannelReadableType<T, SampleType> &&
                                 requires(T audioBufferChannel, size_t index, SampleType sampleType, SamplesRange samplesRange, T::GainType gainType)
{
	{ audioBufferChannel.setSample(index, sampleType) };
	{ audioBufferChannel.addSample(index, sampleType) };
	//@todo copy/add function testable?
	//	{ audioBufferChannel.copyFrom(sourceBufferChannel, samplesRange, gainType) };
	//	{ audioBufferChannel.copyWithRampFrom(sourceBufferChannel, gainType, gainType, samplesRange) };
	//	{ audioBufferChannel.addFrom(sourceBufferChannel, samplesRange, gainType) };
	//	{ audioBufferChannel.addWithRampFrom(sourceBufferChannel, gainType, gainType, samplesRange) };
	{ audioBufferChannel.applyGain(gainType, samplesRange) };
	{ audioBufferChannel.applyGainRamp(gainType, gainType, samplesRange) };
	{ audioBufferChannel.clear(samplesRange) };
	{ audioBufferChannel.reverse(samplesRange) };
};

} // abl

#endif //AUDIOBUFFERS_AUDIOBUFFERCHANNELVIEWCONCEPTS_H
