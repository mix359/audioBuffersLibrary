// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef AUDIO_BUFFERS_SAMPLESRANGE_H
#define AUDIO_BUFFERS_SAMPLESRANGE_H

#include <assert.h>

namespace audioBuffers {

struct SamplesRange {
	size_t startSample;
	int samplesCount;

	SamplesRange(size_t newStartSample = 0, int newSamplesCount = -1) : startSample{newStartSample}, samplesCount{newSamplesCount} {
		assert(newSamplesCount >= 0 || newSamplesCount == -1);
	}
	static SamplesRange allSamples() {
		SamplesRange newSampleRange{};
		newSampleRange.startSample = 0;
		newSampleRange.samplesCount = -1;
		return newSampleRange;
	}
	static SamplesRange allSamplesStartingFrom(size_t startSample) {
		SamplesRange newSampleRange{};
		newSampleRange.startSample = startSample;
		newSampleRange.samplesCount = -1;
		return newSampleRange;
	}
	static SamplesRange allSamplesUntilCount(int sampleCount) {
		SamplesRange newSampleRange{};
		newSampleRange.startSample = 0;
		newSampleRange.samplesCount = sampleCount;
		return newSampleRange;
	}

	[[nodiscard]] size_t getRealSamplesCount(int bufferSize) const { return samplesCount == -1 ? bufferSize - startSample : samplesCount; }
	void setRealSamplesCount(int bufferSize) { if(samplesCount == -1) samplesCount = bufferSize - (int)startSample; }
	[[nodiscard]] bool haveRange() const { return startSample > 0 || samplesCount > 0; }
};

} // audioBuffers

#endif //AUDIO_BUFFERS_SAMPLESRANGE_H
