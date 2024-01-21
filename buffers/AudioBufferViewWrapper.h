// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef ABL_AUDIOBUFFERVIEWWRAPPER_H
#define ABL_AUDIOBUFFERVIEWWRAPPER_H

#include "../datatypes/SamplesRange.h"
#include "../datatypes/NumericConcept.h"
#include "../memory/GenericPointerIterator.h"
#include "../memory/CircularIterator.h"
#include "../memory/VariantRandomAccessIteratorWrapper.h"
#include "AudioBufferViewConcepts.h"
#include "AudioBufferView.h"
#include "CircularAudioBufferView.h"
#include "DelayedCircularAudioBufferView.h"
#include "AudioBufferChannelViewWrapper.h"
#include "CircularAudioBufferChannelView.h"

#include <boost/variant2.hpp>

namespace abl {

template <NumericType AudioSampleType>
class AudioBufferViewWrapper {
public:
	using channelViews_type = boost::variant2::variant<AudioBufferView<AudioSampleType>, CircularAudioBufferView<AudioSampleType>, DelayedCircularAudioBufferView<AudioSampleType>>;
	using GainType = typename std::conditional<std::is_integral_v<AudioSampleType>, double, AudioSampleType>::type;

	AudioBufferViewWrapper(channelViews_type const& channelView) noexcept (std::is_nothrow_copy_constructible_v<channelViews_type>) : m_bufferView(channelView) {}
	AudioBufferViewWrapper(AudioBufferViewWrapper const&) = default;
	AudioBufferViewWrapper(AudioBufferViewWrapper&&) = default;

	using iterator = VariantRandomAccessIteratorWrapper<AudioBufferChannelViewWrapper<AudioSampleType>, typename AudioBufferView<AudioSampleType>::iterator, typename BasicCircularAudioBufferView<AudioSampleType>::iterator>;
	constexpr iterator begin() const noexcept { return boost::variant2::visit([](auto&& audioBufferChannelView) -> iterator { return audioBufferChannelView.begin(); }, m_bufferView); }
	constexpr iterator end() const noexcept { return boost::variant2::visit([](auto&& audioBufferChannelView) -> iterator { return audioBufferChannelView.end(); }, m_bufferView); }

	constexpr bool isEmpty() { return boost::variant2::visit([](auto&& audioBufferChannelView) -> bool { return audioBufferChannelView.isEmpty(); }, m_bufferView); }

	constexpr AudioBufferChannelViewWrapper<AudioSampleType> operator[](size_t index) noexcept { return boost::variant2::visit([index](auto&& audioBufferChannelView) -> AudioBufferChannelViewWrapper<AudioSampleType> { return audioBufferChannelView.operator[](index); }, m_bufferView); }
	constexpr const AudioBufferChannelViewWrapper<AudioSampleType> operator[](size_t index) const noexcept { return boost::variant2::visit([index](auto&& audioBufferChannelView) -> const AudioBufferChannelViewWrapper<AudioSampleType> { return audioBufferChannelView.operator[](index); }, m_bufferView); }
	constexpr AudioBufferChannelViewWrapper<AudioSampleType> getChannelView(size_t channel, const SamplesRange &samplesRange = {}) const noexcept { return boost::variant2::visit([channel, samplesRange](auto&& audioBufferChannelView) -> AudioBufferChannelViewWrapper<AudioSampleType> { return audioBufferChannelView.getChannelView(channel, samplesRange); }, m_bufferView); }
	constexpr AudioBufferViewWrapper<AudioSampleType> getRangedView(const SamplesRange &samplesRange = {}) const noexcept { return boost::variant2::visit([samplesRange](auto&& audioBufferChannelView) -> AudioBufferChannelViewWrapper<AudioSampleType> { return AudioBufferChannelViewWrapper<AudioSampleType>(audioBufferChannelView.getRangedView(samplesRange)); }, m_bufferView); }

	constexpr const AudioBufferChannelViewWrapper<AudioSampleType> getSample(size_t channel, size_t index) const noexcept { return boost::variant2::visit([channel, index](auto&& audioBufferChannelView) -> const AudioBufferChannelViewWrapper<AudioSampleType> { return audioBufferChannelView.getSample(channel, index); }, m_bufferView); }

	constexpr void setSample(size_t channel, size_t index, AudioSampleType sample) noexcept { boost::variant2::visit([channel, index, &sample](auto&& audioBufferChannelView) -> void { audioBufferChannelView.setSample(channel, index, sample); }, m_bufferView); }
	constexpr void addSample(size_t channel, size_t index, AudioSampleType sample) noexcept { boost::variant2::visit([channel, index, &sample](auto&& audioBufferChannelView) -> void { audioBufferChannelView.addSample(channel, index, sample); }, m_bufferView); }

	constexpr void copyFrom(const AudioBufferReadableType<AudioSampleType> auto &sourceBuffer, const SamplesRange &destinationSamplesRange = {}, GainType gain = AudioSampleType(1)) { boost::variant2::visit([&sourceBuffer, &destinationSamplesRange, gain](auto&& audioBufferChannelView) -> void { audioBufferChannelView.copyFrom(sourceBuffer, destinationSamplesRange, gain); }, m_bufferView); }
	constexpr void copyWithRampFrom(const AudioBufferReadableType<AudioSampleType> auto &sourceBuffer, GainType startGain, GainType endGain, const SamplesRange &destinationSamplesRange = {}) { boost::variant2::visit([&sourceBuffer, startGain, endGain, &destinationSamplesRange](auto&& audioBufferChannelView) -> void { audioBufferChannelView.copyWithRampFrom(sourceBuffer, startGain, endGain, destinationSamplesRange); }, m_bufferView); }
	constexpr void copyIntoChannelFrom(const AudioBufferChannelReadableType<AudioSampleType> auto &sourceBufferChannel, size_t destinationChannel, const SamplesRange &destinationSamplesRange = {}, GainType gain = AudioSampleType(1)) { boost::variant2::visit([&sourceBufferChannel, destinationChannel, &destinationSamplesRange, gain](auto&& audioBufferChannelView) -> void { audioBufferChannelView.copyIntoChannelFrom(sourceBufferChannel, destinationChannel, destinationSamplesRange, gain); }, m_bufferView); }
	constexpr void copyIntoChannelWithRampFrom(const AudioBufferChannelReadableType<AudioSampleType> auto &sourceBufferChannel, size_t destinationChannel, GainType startGain, GainType endGain, const SamplesRange &destinationSamplesRange = {}) { boost::variant2::visit([&sourceBufferChannel, destinationChannel, startGain, endGain, &destinationSamplesRange](auto&& audioBufferChannelView) -> void { audioBufferChannelView.copyIntoChannelWithRampFrom(sourceBufferChannel, destinationChannel, startGain, endGain, destinationSamplesRange); }, m_bufferView); }
	constexpr void addFrom(const AudioBufferReadableType<AudioSampleType> auto &sourceBuffer, const SamplesRange &destinationSamplesRange = {}, GainType gain = AudioSampleType(1)) { boost::variant2::visit([&sourceBuffer, &destinationSamplesRange, gain](auto&& audioBufferChannelView) -> void { audioBufferChannelView.addFrom(sourceBuffer, destinationSamplesRange, gain); }, m_bufferView); }
	constexpr void addWithRampFrom(const AudioBufferReadableType<AudioSampleType> auto &sourceBuffer, GainType startGain, GainType endGain, const SamplesRange &destinationSamplesRange = {}) { boost::variant2::visit([&sourceBuffer, startGain, endGain, &destinationSamplesRange](auto&& audioBufferChannelView) -> void { audioBufferChannelView.addWithRampFrom(sourceBuffer, startGain, endGain, destinationSamplesRange); }, m_bufferView); }
	constexpr void addIntoChannelFrom(const AudioBufferChannelReadableType<AudioSampleType> auto &sourceBufferChannel, size_t destinationChannel, const SamplesRange &destinationSamplesRange = {}, GainType gain = AudioSampleType(1)) { boost::variant2::visit([&sourceBufferChannel, destinationChannel, &destinationSamplesRange, gain](auto&& audioBufferChannelView) -> void { audioBufferChannelView.addIntoChannelFrom(sourceBufferChannel, destinationChannel, destinationSamplesRange, gain); }, m_bufferView); }
	constexpr void addIntoChannelWithRampFrom(const AudioBufferChannelReadableType<AudioSampleType> auto &sourceBufferChannel, size_t destinationChannel, GainType startGain, GainType endGain, const SamplesRange &destinationSamplesRange = {}) { boost::variant2::visit([&sourceBufferChannel, destinationChannel, startGain, endGain, &destinationSamplesRange](auto&& audioBufferChannelView) -> void { audioBufferChannelView.addIntoChannelWithRampFrom(sourceBufferChannel, destinationChannel, startGain, endGain, destinationSamplesRange); }, m_bufferView); }

	constexpr void applyGain(GainType gain, const SamplesRange& samplesRange = {}) noexcept { boost::variant2::visit([gain, &samplesRange](auto&& audioBufferChannelView) -> void { audioBufferChannelView.applyGain(gain, samplesRange); }, m_bufferView); }
	constexpr void applyGainToChannel(GainType gain, size_t channel, const SamplesRange& samplesRange = {}) noexcept { boost::variant2::visit([gain,channel,  &samplesRange](auto&& audioBufferChannelView) -> void { audioBufferChannelView.applyGainToChannel(gain, channel, samplesRange); }, m_bufferView); }
	constexpr void applyGainRamp(GainType startGain, GainType endGain, const SamplesRange& samplesRange = {}) noexcept { boost::variant2::visit([startGain, endGain, &samplesRange](auto&& audioBufferChannelView) -> void { audioBufferChannelView.applyGainRamp(startGain, endGain, samplesRange); }, m_bufferView); }
	constexpr void applyGainRampToChannel(GainType startGain, GainType endGain,size_t channel,  const SamplesRange& samplesRange = {}) noexcept { boost::variant2::visit([startGain, endGain, channel, &samplesRange](auto&& audioBufferChannelView) -> void { audioBufferChannelView.applyGainRampToChannel(startGain, endGain, channel, samplesRange); }, m_bufferView); }
	constexpr void clear(const SamplesRange& samplesRange = {}) noexcept { boost::variant2::visit([&samplesRange](auto&& audioBufferChannelView) -> void { audioBufferChannelView.clear(samplesRange); }, m_bufferView); }
	constexpr void clearChannel(size_t channel, const SamplesRange& samplesRange = {}) noexcept { boost::variant2::visit([channel, &samplesRange](auto&& audioBufferChannelView) -> void { audioBufferChannelView.clearChannel(channel, samplesRange); }, m_bufferView); }
	constexpr void reverse(const SamplesRange& samplesRange = {}) noexcept { boost::variant2::visit([&samplesRange](auto&& audioBufferChannelView) -> void { audioBufferChannelView.reverse(samplesRange); }, m_bufferView); }
	constexpr void reverseChannel(size_t channel, const SamplesRange& samplesRange = {}) noexcept { boost::variant2::visit([channel, &samplesRange](auto&& audioBufferChannelView) -> void { audioBufferChannelView.reverseChannel(channel, samplesRange); }, m_bufferView); }

	constexpr AudioSampleType getHigherPeak(const SamplesRange& samplesRange = {}) const noexcept { return boost::variant2::visit([&samplesRange](auto&& audioBufferChannelView) -> AudioSampleType { return audioBufferChannelView.getHigherPeak(samplesRange); }, m_bufferView); }
	constexpr AudioSampleType getHigherPeakForChannel(size_t channel, const SamplesRange& samplesRange = {}) const noexcept { return boost::variant2::visit([channel, &samplesRange](auto&& audioBufferChannelView) -> AudioSampleType { return audioBufferChannelView.getHigherPeakForChannel(channel, samplesRange); }, m_bufferView); }
	constexpr AudioSampleType getRMSLevelForChannel(size_t channel, const SamplesRange& samplesRange = {}) const noexcept { return boost::variant2::visit([channel, &samplesRange](auto&& audioBufferChannelView) -> AudioSampleType { return audioBufferChannelView.getRMSLevelForChannel(channel, samplesRange); }, m_bufferView); }

	[[nodiscard]] constexpr size_t getBufferSize() const noexcept { return boost::variant2::visit([](auto&& audioBufferChannelView) -> size_t { return audioBufferChannelView.getBufferSize(); }, m_bufferView); }
	[[nodiscard]] constexpr size_t getChannelsCount() const noexcept { return boost::variant2::visit([](auto&& audioBufferChannelView) -> size_t { return audioBufferChannelView.getChannelsCount(); }, m_bufferView); }

	[[nodiscard]] constexpr const std::vector<size_t>& getChannelsMapping() const noexcept { return boost::variant2::visit([](auto&& audioBufferChannelView) -> size_t { return audioBufferChannelView.getChannelsMapping(); }, m_bufferView); }
	constexpr void setChannelsMapping(const std::vector<size_t>& channelsMapping) noexcept { boost::variant2::visit([&channelsMapping](auto&& audioBufferChannelView) -> void { audioBufferChannelView.setChannelsMapping(channelsMapping); }, m_bufferView); }
	constexpr void createSequentialChannelsMapping(size_t startChannel, size_t channelsCount) noexcept { boost::variant2::visit([startChannel, channelsCount](auto&& audioBufferChannelView) -> void { audioBufferChannelView.createSequentialChannelsMapping(startChannel, channelsCount); }, m_bufferView); }

	//isAudioBufferView()
	//isCircularAudioBufferView()
	//isDelayedCircularAudioBufferView()
	//getAsAudioBufferView()
	//getAsCircularAudioBufferView()
	//getAsDelayedCircularAudioBufferView()

protected:
	channelViews_type m_bufferView;
};

} // audiobuffer

#endif //ABL_AUDIOBUFFERVIEWWRAPPER_H
