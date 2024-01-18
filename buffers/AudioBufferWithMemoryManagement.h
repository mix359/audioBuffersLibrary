// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef AUDIO_BUFFERS_AUDIOBUFFERWITHMEMORYMANAGEMENT_H
#define AUDIO_BUFFERS_AUDIOBUFFERWITHMEMORYMANAGEMENT_H

#include <algorithm>
#include "../datatypes/Concepts.h"
#include "../memory/2DArrayAllocator.h"

namespace audioBuffers {

template <NumericType AudioSampleType>
class AudioBufferWithMemoryManagement {
public:
	virtual void resize(size_t channelsCount, size_t bufferSize, bool keepExistingContent = false, bool clearExtraSpace = true, bool avoidReallocating = false) = 0;

protected:
	virtual void setInternalData(AudioSampleType** newData, bool deleteOld = true) = 0;
	virtual AudioSampleType** prepareAllocatedSpace(size_t channelsCount, size_t bufferSize, bool zeroData = true) = 0;

	void doResize(size_t channelsCount, size_t bufferSize, bool keepExistingContent, bool clearExtraSpace, bool avoidReallocating, size_t currentChannelsCount, size_t currentBufferSize, AudioSampleType** currentData) {
		if(channelsCount == currentChannelsCount && bufferSize == currentBufferSize) {
			return;
		}

		if(channelsCount < 1 || bufferSize < 1) { //@todo permit resize to zero?
			setInternalData(nullptr);
			return;
		}

		if(channelsCount > currentChannelsCount || bufferSize > currentBufferSize) {
			auto currentSize = currentBufferSize * currentChannelsCount;
			auto newSize = channelsCount * bufferSize;
			//If avoidReallocating = true, try to save the space already allocated
			if(avoidReallocating && !keepExistingContent && (currentSize >= newSize) && channelsCount <= currentChannelsCount) {
				//to permit channelsCount > currentChannelsCount a new pointers array should be created
				rearrangeContiguous2DArray<AudioSampleType>(currentData, currentChannelsCount, currentBufferSize, channelsCount, bufferSize);

				for(size_t channel = 0; channel < channelsCount; ++channel) {
					for(size_t i = 0; i < bufferSize; ++i) {
						currentData[channel][i] = 0;
					}
				}
				return;
			}

			//Otherwise I need to reallocate because the space required is bigger. Even if only the channels count have grown it's better to reallocate to a contiguous space
			auto newData = allocateContiguous2DArray<AudioSampleType>(channelsCount, bufferSize, !keepExistingContent);

			if(keepExistingContent) {
				for(size_t channel = 0; channel < channelsCount; ++channel) {
					if(channel < currentChannelsCount) {
						for(size_t i = 0; i < bufferSize; ++i) {
							if(i < currentBufferSize) {
								newData[channel][i] = currentData[channel][i];
							} else if(clearExtraSpace) {
								newData[channel][i] = 0;
							}
						}
					} else if(clearExtraSpace) {
						for(size_t i = 0; i < bufferSize; ++i) {
							newData[channel][i] = 0;
						}
					}
				}
			}

			setInternalData(newData);
		} else if(avoidReallocating) { //leave the allocated memory as is, loosing space but avoiding reallocation
			if(!keepExistingContent) {
				for(size_t channel = 0; channel < channelsCount; ++channel) {
					for(size_t i = 0; i < bufferSize; ++i) {
						currentData[channel][i] = 0;
					}
				}
			}
		} else {
			auto newData = allocateContiguous2DArray<AudioSampleType>(channelsCount, bufferSize, !keepExistingContent);

			if(keepExistingContent) {
				for(size_t channel = 0; channel < channelsCount; ++channel) {
					std::copy(currentData[channel], currentData[channel] + bufferSize, newData[channel]);
				}
			}

			setInternalData(newData);
		}
	}
};

} // engine::showmanager

#endif //AUDIO_BUFFERS_AUDIOBUFFERWITHMEMORYMANAGEMENT_H
