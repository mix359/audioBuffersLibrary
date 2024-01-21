// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef ABL_AUDIOBUFFERCHANNELVIEWWRAPPER_H
#define ABL_AUDIOBUFFERCHANNELVIEWWRAPPER_H

#include "../datatypes/SamplesRange.h"
#include "../datatypes/NumericConcept.h"
#include "../memory/GenericPointerIterator.h"
#include "../memory/CircularIterator.h"
#include "../memory/VariantRandomAccessIteratorWrapper.h"
#include "AudioBufferChannelView.h"
#include "CircularAudioBufferChannelView.h"
#include "OffsettedReadCircularAudioBufferChannelView.h"

#include <boost/variant2.hpp>

namespace abl {

template <NumericType AudioSampleType>
class AudioBufferChannelViewWrapper {
public:
	using channelViews_type = boost::variant2::variant<AudioBufferChannelView<AudioSampleType>, CircularAudioBufferChannelView<AudioSampleType>, OffsettedReadCircularAudioBufferChannelView<AudioSampleType>>;
	using GainType = typename std::conditional<std::is_integral_v<AudioSampleType>, double, AudioSampleType>::type;

	AudioBufferChannelViewWrapper(channelViews_type const& channelView) noexcept (std::is_nothrow_copy_constructible_v<channelViews_type>) : m_channelView(channelView) {}
	AudioBufferChannelViewWrapper(AudioBufferChannelViewWrapper const&) = default;
	AudioBufferChannelViewWrapper(AudioBufferChannelViewWrapper&&) = default;

	using iterator =VariantRandomAccessIteratorWrapper<AudioSampleType, GenericPointerIterator<AudioSampleType>, CircularIterator<AudioSampleType>>;
	constexpr iterator begin() const noexcept { return boost::variant2::visit([](auto&& audioBufferChannelView) -> iterator { return audioBufferChannelView.begin(); }, m_channelView); }
	constexpr iterator end() const noexcept { return boost::variant2::visit([](auto&& audioBufferChannelView) -> iterator { return audioBufferChannelView.end(); }, m_channelView); }

	constexpr bool isEmpty() { return boost::variant2::visit([](auto&& audioBufferChannelView) -> bool { return audioBufferChannelView.isEmpty(); }, m_channelView); }

	constexpr AudioSampleType& operator[](size_t index) noexcept { return boost::variant2::visit([index](auto&& audioBufferChannelView) -> AudioSampleType& { return audioBufferChannelView.operator[](index); }, m_channelView); }
	constexpr AudioSampleType operator[](size_t index) const noexcept { return boost::variant2::visit([index](auto&& audioBufferChannelView) -> AudioSampleType { return audioBufferChannelView.operator[](index); }, m_channelView); }
	constexpr AudioSampleType getSample(size_t index) const noexcept { return boost::variant2::visit([index](auto&& audioBufferChannelView) -> AudioSampleType { return audioBufferChannelView.getSample(index); }, m_channelView); }

	constexpr void setSample(size_t index, AudioSampleType sample) noexcept { boost::variant2::visit([index, &sample](auto&& audioBufferChannelView) -> void { audioBufferChannelView.setSample(index, sample); }, m_channelView); }
	constexpr void addSample(size_t index, AudioSampleType sample) noexcept { boost::variant2::visit([index, &sample](auto&& audioBufferChannelView) -> void { audioBufferChannelView.addSample(index, sample); }, m_channelView); }

	constexpr void copyFrom(const AudioBufferChannelReadableType<AudioSampleType> auto &sourceBufferChannel, const SamplesRange &destinationSamplesRange = {}, GainType gain = AudioSampleType(1)) { boost::variant2::visit([&sourceBufferChannel, &destinationSamplesRange, gain](auto&& audioBufferChannelView) -> void { audioBufferChannelView.copyFrom(sourceBufferChannel, destinationSamplesRange, gain); }, m_channelView); }
	constexpr void copyWithRampFrom(const AudioBufferChannelReadableType<AudioSampleType> auto &sourceBufferChannel, GainType startGain, GainType endGain, const SamplesRange &destinationSamplesRange = {}) { boost::variant2::visit([&sourceBufferChannel, startGain, endGain, &destinationSamplesRange](auto&& audioBufferChannelView) -> void { audioBufferChannelView.copyWithRampFrom(sourceBufferChannel, startGain, endGain, destinationSamplesRange); }, m_channelView); }
	constexpr void addFrom(const AudioBufferChannelReadableType<AudioSampleType> auto &sourceBufferChannel, const SamplesRange &destinationSamplesRange = {}, GainType gain = AudioSampleType(1)) { boost::variant2::visit([&sourceBufferChannel, &destinationSamplesRange, gain](auto&& audioBufferChannelView) -> void { audioBufferChannelView.addFrom(sourceBufferChannel, destinationSamplesRange, gain); }, m_channelView); }
	constexpr void addWithRampFrom(const AudioBufferChannelReadableType<AudioSampleType> auto &sourceBufferChannel, GainType startGain, GainType endGain, const SamplesRange &destinationSamplesRange = {}) { boost::variant2::visit([&sourceBufferChannel, startGain, endGain, &destinationSamplesRange](auto&& audioBufferChannelView) -> void { audioBufferChannelView.addWithRampFrom(sourceBufferChannel, startGain, endGain, destinationSamplesRange); }, m_channelView); }

	constexpr void applyGain(GainType gain, const SamplesRange& samplesRange = {}) noexcept { boost::variant2::visit([gain, &samplesRange](auto&& audioBufferChannelView) -> void { audioBufferChannelView.applyGain(gain, samplesRange); }, m_channelView); }
	constexpr void applyGainRamp(GainType startGain, GainType endGain, const SamplesRange& samplesRange = {}) noexcept { boost::variant2::visit([startGain, endGain, &samplesRange](auto&& audioBufferChannelView) -> void { audioBufferChannelView.applyGainRamp(startGain, endGain, samplesRange); }, m_channelView); }
	constexpr void clear(const SamplesRange& samplesRange = {}) noexcept { boost::variant2::visit([&samplesRange](auto&& audioBufferChannelView) -> void { audioBufferChannelView.clear(samplesRange); }, m_channelView); }
	constexpr void reverse(const SamplesRange& samplesRange = {}) noexcept { boost::variant2::visit([&samplesRange](auto&& audioBufferChannelView) -> void { audioBufferChannelView.reverse(samplesRange); }, m_channelView); }

	constexpr AudioSampleType getHigherPeak(const SamplesRange& samplesRange = {}) const noexcept { return boost::variant2::visit([&samplesRange](auto&& audioBufferChannelView) -> AudioSampleType { return audioBufferChannelView.getHigherPeak(samplesRange); }, m_channelView); }
	constexpr AudioSampleType getRMSLevel(const SamplesRange& samplesRange = {}) const noexcept { return boost::variant2::visit([&samplesRange](auto&& audioBufferChannelView) -> AudioSampleType { return audioBufferChannelView.getRMSLevel(samplesRange); }, m_channelView); }

	[[nodiscard]] constexpr size_t getBufferSize() const noexcept { return boost::variant2::visit([](auto&& audioBufferChannelView) -> size_t { return audioBufferChannelView.getBufferSize(); }, m_channelView); }

	//isAudioBufferChannelView()
	//isCircularAudioBufferChannelView()
	//getAudioBufferChannelView()
	//getCircularAudioBufferChannelView()

protected:
	channelViews_type m_channelView;
};

} // audiobuffer

#endif //ABL_AUDIOBUFFERCHANNELVIEWWRAPPER_H
