// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef AUDIOBUFFERS_2DARRAYALLOCATOR_H
#define AUDIOBUFFERS_2DARRAYALLOCATOR_H

#include <memory>

namespace abl {

template<typename T>
T** allocateContiguous2DArray(size_t rows, size_t rowSize, bool zeroData = true) {
	if(rows < 1) {
		return nullptr;
	}
	auto array = new T*[rows];

	if(rowSize > 0) {
		array[0] = new T[rows * rowSize];

		if (zeroData) {
			for (size_t i = 0; i < rowSize; ++i) {
				array[0][i] = 0;
			}
		}

		for (auto row = 1; row < rows; ++row) {
			array[row] = array[row - 1] + rowSize;
			if (zeroData) {
				for (size_t i = 0; i < rowSize; ++i) {
					array[row][i] = 0;
				}
			}
		}
	} else {
		for (auto row = 0; row < rows; ++row) {
			array[row] = nullptr;
		}
	}

	return array;
}

template<typename T>
void rearrangeContiguous2DArray(T** array, size_t originalRows, size_t originalRowSize, size_t newRows, size_t newRowSize) {
	auto originalSize = originalRows * originalRowSize;
	auto newSize = newRows * newRowSize;
	assert(newSize <= originalSize);
	assert(newRows <= originalRows); //cannot expand the rows numbers without changing the pointer array

	for(size_t row = 1; row < newRows; ++row) {
		array[row] = array[row - 1] + newRowSize;
	}

	if(originalRows > newRows) {
		for(size_t row = newRows; row < originalRows; ++row) {
			array[row] = nullptr;
		}
	}
}

template<typename T>
void deallocateContiguous2DArray(T** array) {
	if(array) {
		delete[] array[0];
		delete[] array;
	}
}

}

#endif //AUDIOBUFFERS_2DARRAYALLOCATOR_H
