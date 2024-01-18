// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef AUDIO_BUFFERS_PARENTREFERENCINGRETURNINGUNIQUEPTRITERATOR_H
#define AUDIO_BUFFERS_PARENTREFERENCINGRETURNINGUNIQUEPTRITERATOR_H

#include <iterator>
#include <memory>

namespace audioBuffers {

template <typename ParentType, typename ReturnType>
class ParentReferencingReturningUniquePtrIterator {
public:
	using iterator_category = std::random_access_iterator_tag;
	using value_type = std::unique_ptr<ReturnType>;
	using reference = std::unique_ptr<ReturnType>;
	using pointer = ReturnType*;
	using difference_type = std::ptrdiff_t;

//	ParentReferencingReturningUniquePtrIterator() : m_index{0}, m_parentBuffer{nullptr} {}
	ParentReferencingReturningUniquePtrIterator(size_t startIndex, ParentType& parentBuffer) : m_index(startIndex), m_parentBuffer{parentBuffer} {}
	ParentReferencingReturningUniquePtrIterator(const ParentReferencingReturningUniquePtrIterator &otherIterator) : m_index(otherIterator.m_index), m_parentBuffer{otherIterator.m_parentBuffer} {}
	/* inline iterator& operator=(AudioSampleType* rhs) {m_ptr = rhs; return *this;} */
	inline ParentReferencingReturningUniquePtrIterator& operator=(const ParentReferencingReturningUniquePtrIterator &otherIterator) {m_index = otherIterator.m_index; m_parentBuffer = otherIterator.m_parentBuffer; return *this;}
	inline ParentReferencingReturningUniquePtrIterator& operator+=(difference_type difference) { m_index += difference;  return *this;}
	inline ParentReferencingReturningUniquePtrIterator& operator-=(difference_type difference) { m_index -= difference;  return *this;}
	inline std::unique_ptr<ReturnType> operator*() const { return m_parentBuffer[m_index]; }
	inline ReturnType* operator->() const { return m_parentBuffer[m_index].get(); }
	inline std::unique_ptr<ReturnType> operator[](difference_type difference) const { return m_parentBuffer[difference]; }

	inline ParentReferencingReturningUniquePtrIterator& operator++() { ++m_index;  return *this; }
	inline ParentReferencingReturningUniquePtrIterator& operator--() { --m_index;  return *this; }
	inline ParentReferencingReturningUniquePtrIterator operator++(int) { ParentReferencingReturningUniquePtrIterator tmp(*this); ++m_index;  return tmp; }
	inline ParentReferencingReturningUniquePtrIterator operator--(int) { ParentReferencingReturningUniquePtrIterator tmp(*this); --m_index;  return tmp; }
	inline ParentReferencingReturningUniquePtrIterator operator+(const ParentReferencingReturningUniquePtrIterator& otherIterator) { return ParentReferencingReturningUniquePtrIterator(m_index+otherIterator.m_index, m_parentBuffer); }
	inline difference_type operator-(const ParentReferencingReturningUniquePtrIterator& otherIterator) const { return m_index - otherIterator.ptr; }
	inline ParentReferencingReturningUniquePtrIterator operator+(difference_type difference) const { return ParentReferencingReturningUniquePtrIterator(m_index + difference, m_parentBuffer); }
	inline ParentReferencingReturningUniquePtrIterator operator-(difference_type difference) const { return ParentReferencingReturningUniquePtrIterator(m_index - difference, m_parentBuffer); }
	friend inline ParentReferencingReturningUniquePtrIterator operator+(difference_type lhs, const ParentReferencingReturningUniquePtrIterator& otherIterator) { return ParentReferencingReturningUniquePtrIterator(lhs+otherIterator.m_index, otherIterator.m_parentBuffer); }
	friend inline ParentReferencingReturningUniquePtrIterator operator-(difference_type lhs, const ParentReferencingReturningUniquePtrIterator& otherIterator) { return ParentReferencingReturningUniquePtrIterator(lhs-otherIterator.m_index, otherIterator.m_parentBuffer); }

	inline bool operator==(const ParentReferencingReturningUniquePtrIterator& otherIterator) const { return m_index == otherIterator.m_index; }
	inline bool operator!=(const ParentReferencingReturningUniquePtrIterator& otherIterator) const { return m_index != otherIterator.m_index; }
	inline bool operator>(const ParentReferencingReturningUniquePtrIterator& otherIterator) const { return m_index > otherIterator.m_index; }
	inline bool operator<(const ParentReferencingReturningUniquePtrIterator& otherIterator) const { return m_index < otherIterator.m_index; }
	inline bool operator>=(const ParentReferencingReturningUniquePtrIterator& otherIterator) const { return m_index >= otherIterator.m_index; }
	inline bool operator<=(const ParentReferencingReturningUniquePtrIterator& otherIterator) const { return m_index <= otherIterator.m_index; }

private:
	size_t m_index;
	ParentType& m_parentBuffer;
};

} // audioBuffers

#endif //AUDIO_BUFFERS_PARENTREFERENCINGRETURNINGUNIQUEPTRITERATOR_H
