// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef AUDIO_BUFFERS_AUDIOBUFFERINTERFACE_H
#define AUDIO_BUFFERS_AUDIOBUFFERINTERFACE_H

#include <vector>
#include <optional>

#include "../datatypes/Concepts.h"
#include "../datatypes/SamplesRange.h"
#include "../memory/GenericPointerIterator.h"
#include "../memory/CircularIterator.h"
#include "../memory/VariantRandomAccessIteratorWrapper.h"
#include "../memory/ParentReferencingReturningUniquePtrIterator.h"
#include "AudioBufferChannelViewInterface.h"

namespace audioBuffers {

template <NumericType AudioSampleType>
class AudioBufferView;

template <NumericType AudioSampleType>
class BasicCircularAudioBufferView;

template <NumericType AudioSampleType>
class AudioBufferViewInterface {
public:
	virtual ~AudioBufferViewInterface() = default;

	using GainType = typename std::conditional<std::is_integral_v<AudioSampleType>, double, AudioSampleType>::type;

//	using iterator = VariantRandomAccessIteratorWrapper<std::unique_ptr<AudioBufferChannelViewInterface<AudioSampleType>>,
//														ParentReferencingReturningUniquePtrIterator<AudioBufferView<AudioSampleType>, AudioBufferChannelViewInterface<AudioSampleType>>,
//														ParentReferencingReturningUniquePtrIterator<BasicCircularAudioBufferView<AudioSampleType>, AudioBufferChannelViewInterface<AudioSampleType>>>;
//	virtual iterator begin() const noexcept = 0;
//	virtual iterator end() const noexcept = 0;

	virtual bool isEmpty() const = 0;

	virtual std::unique_ptr<AudioBufferChannelViewInterface<AudioSampleType>> operator[](size_t channel) = 0;
	virtual std::unique_ptr<const AudioBufferChannelViewInterface<AudioSampleType>> operator[](size_t channel) const = 0;
	virtual std::unique_ptr<AudioBufferChannelViewInterface<AudioSampleType>> getChannelView(size_t channel, SamplesRange samplesRange = {}) const = 0;

	virtual std::unique_ptr<AudioBufferViewInterface<AudioSampleType>> getRangedView(SamplesRange samplesRange = {}) = 0;

	[[nodiscard]] virtual AudioSampleType getSample(size_t channel, size_t index) const = 0;
	virtual void setSample(size_t destinationChannel, size_t destinationIndex, const AudioSampleType sample) = 0;
	virtual void addSample(size_t destinationChannel, size_t destinationIndex, const AudioSampleType sample) = 0;

	virtual void copyFrom(const AudioBufferViewInterface<AudioSampleType>& sourceBuffer, const SamplesRange &destinationSamplesRange = {}, GainType gain = GainType(1)) = 0;
	virtual void copyWithRampFrom(const AudioBufferViewInterface<AudioSampleType>& sourceBuffer, GainType startGain, GainType endGain, const SamplesRange &destinationSamplesRange = {}) = 0;
	virtual void copyIntoChannelFrom(const AudioBufferChannelViewInterface<AudioSampleType> &sourceBufferChannel, size_t destinationChannel, const SamplesRange &destinationSamplesRange = {}, GainType gain = GainType(1)) = 0;
	virtual void copyIntoChannelWithRampFrom(const AudioBufferChannelViewInterface<AudioSampleType> &sourceBufferChannel, size_t destinationChannel, GainType startGain, GainType endGain, const SamplesRange &destinationSamplesRange = {}) = 0;
	virtual void addFrom(const AudioBufferViewInterface<AudioSampleType>& sourceBuffer, const SamplesRange &destinationSamplesRange = {}, GainType gain = GainType(1)) = 0;
	virtual void addWithRampFrom(const AudioBufferViewInterface<AudioSampleType>& sourceBuffer, GainType startGain, GainType endGain, const SamplesRange &destinationSamplesRange = {}) = 0;
	virtual void addIntoChannelFrom(const AudioBufferChannelViewInterface<AudioSampleType> &sourceBufferChannel, size_t destinationChannel, const SamplesRange &destinationSamplesRange = {}, GainType gain = GainType (1)) = 0;
	virtual void addIntoChannelWithRampFrom(const AudioBufferChannelViewInterface<AudioSampleType> &sourceBufferChannel, size_t destinationChannel, GainType startGain, GainType endGain, const SamplesRange &destinationSamplesRange = {}) = 0;

	virtual void applyGain(GainType gain, const SamplesRange &samplesRange = {}) = 0;
	virtual void applyGainToChannel(GainType gain, size_t channel, const SamplesRange &samplesRange = {}) = 0;
	virtual void applyGainRamp(GainType startGain, GainType endGain, const SamplesRange &samplesRange = {}) = 0;
	virtual void applyGainRampToChannel(GainType startGain, GainType endGain, size_t channel, const SamplesRange &samplesRange = {}) = 0;

	virtual void clear(const SamplesRange &samplesRange = {}) = 0;
	virtual void clearChannel(size_t channel, const SamplesRange &samplesRange = {}) = 0;
	virtual void reverse(const SamplesRange &samplesRange = {}) = 0;
	virtual void reverseChannel(size_t channel, const SamplesRange &samplesRange = {}) = 0;

	virtual AudioSampleType getHigherPeak(const SamplesRange &samplesRange = {}) const = 0;
	virtual AudioSampleType getHigherPeakForChannel(size_t channel, const SamplesRange &samplesRange = {}) const = 0;
	virtual AudioSampleType getRMSLevelForChannel(size_t channel, const SamplesRange &samplesRange = {}) const = 0;

	[[nodiscard]] virtual size_t getBufferSize() const noexcept = 0;
	[[nodiscard]] virtual size_t getChannelsCount() const noexcept = 0;
};

template <NumericType AudioSampleType>
class CircularAudioBufferViewInterface : virtual AudioBufferViewInterface<AudioSampleType> {
public:
	virtual void incrementReadIndex(std::optional<size_t> increment = {}) noexcept = 0;
	virtual void incrementWriteIndex(std::optional<size_t> increment = {}) noexcept = 0;
	virtual void resetIndexes() noexcept = 0;
	virtual void resetWriteIndexToReadIndexPosition() noexcept = 0;
	[[nodiscard]] virtual bool isDataAvailable() const noexcept = 0;
	[[nodiscard]] virtual size_t getReadIndex() const noexcept = 0;
	[[nodiscard]] virtual size_t getWriteIndex() const noexcept = 0;

	[[nodiscard]] virtual size_t getBaseBufferSize() const noexcept = 0;
};

template <NumericType AudioSampleType>
class DelayedCircularAudioBufferViewInterface : virtual AudioBufferViewInterface<AudioSampleType> {
public:
	virtual void incrementIndex(std::optional<size_t> increment = {}) noexcept = 0;
	virtual void resetIndex() noexcept = 0;
	[[nodiscard]] virtual size_t getIndex() const noexcept = 0;

	[[nodiscard]] virtual size_t getDelayInSamples() const noexcept = 0;
	virtual void setDelayInSamples(size_t delay) noexcept = 0;

	[[nodiscard]] virtual size_t getBaseBufferSize() const noexcept = 0;
};

} // audioBuffers

#endif //AUDIO_BUFFERS_AUDIOBUFFERINTERFACE_H
