// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef AUDIO_BUFFERS_AUDIOBUFFERCHANNELVIEWINTERFACE_H
#define AUDIO_BUFFERS_AUDIOBUFFERCHANNELVIEWINTERFACE_H

#include "../datatypes/SamplesRange.h"
#include "../datatypes/Concepts.h"
#include "../memory/GenericPointerIterator.h"
#include "../memory/CircularIterator.h"
#include "../memory/VariantRandomAccessIteratorWrapper.h"

namespace audioBuffers {

template <NumericType AudioSampleType>
class AudioBufferChannelViewInterface {
public:
	virtual ~AudioBufferChannelViewInterface() = default;

	using GainType = typename std::conditional<std::is_integral_v<AudioSampleType>, double, AudioSampleType>::type;

	using iterator = VariantRandomAccessIteratorWrapper<AudioSampleType, GenericPointerIterator<AudioSampleType>, CircularIterator<AudioSampleType>>;
	virtual iterator begin() const noexcept = 0;
	virtual iterator end() const noexcept = 0;

	virtual bool isEmpty() = 0;

	virtual AudioSampleType& operator[](size_t index) noexcept = 0;
	virtual AudioSampleType operator[](size_t index) const noexcept = 0;
	virtual AudioSampleType getSample(size_t index) const noexcept = 0;

	virtual void setSample(size_t index, AudioSampleType sample) noexcept = 0;
	virtual void addSample(size_t index, AudioSampleType sample) noexcept = 0;

	virtual void copyFrom(const AudioBufferChannelViewInterface<AudioSampleType> &sourceBufferChannel, const SamplesRange &destinationSamplesRange = {}, GainType gain = AudioSampleType(1)) = 0;
	virtual void copyWithRampFrom(const AudioBufferChannelViewInterface<AudioSampleType> &sourceBufferChannel, GainType startGain, GainType endGain, const SamplesRange &destinationSamplesRange = {}) = 0;
	virtual void addFrom(const AudioBufferChannelViewInterface<AudioSampleType> &sourceBufferChannel, const SamplesRange &destinationSamplesRange = {}, GainType gain = AudioSampleType(1)) = 0;
	virtual void addWithRampFrom(const AudioBufferChannelViewInterface<AudioSampleType> &sourceBufferChannel, GainType startGain, GainType endGain, const SamplesRange &destinationSamplesRange = {}) = 0;

	virtual void applyGain(GainType gain, const SamplesRange& samplesRange = {}) noexcept = 0;
	virtual void applyGainRamp(GainType startGain, GainType endGain, const SamplesRange& samplesRange = {}) noexcept = 0;
	virtual void clear(const SamplesRange& samplesRange = {}) noexcept = 0;
	virtual void reverse(const SamplesRange& samplesRange = {}) noexcept = 0;

	virtual AudioSampleType getHigherPeak(const SamplesRange& samplesRange = {}) const noexcept = 0;
	virtual AudioSampleType getRMSLevel(const SamplesRange& samplesRange = {}) const noexcept = 0;

	[[nodiscard]] virtual size_t getBufferSize() const noexcept = 0;
};

} // audioBuffers

#endif //AUDIO_BUFFERS_AUDIOBUFFERCHANNELVIEWINTERFACE_H
