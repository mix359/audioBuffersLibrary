// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef ABL_CIRCULARITERATOR_H
#define ABL_CIRCULARITERATOR_H

#include <iterator>

namespace abl {

template <typename T>
class CircularIterator {
public:
	using iterator_category = std::random_access_iterator_tag;
	using value_type = T;
	using reference = T&;
	using pointer = T*;
	using difference_type = std::ptrdiff_t;

	CircularIterator() : m_containerStart{nullptr}, m_containerEnd{nullptr}, m_current{nullptr}, m_index{0} {}
	CircularIterator(T* containerStart, T* containerEnd, T* current, size_t currentIndex) : m_containerStart{containerStart}, m_containerEnd{containerEnd}, m_current{current}, m_index{currentIndex} {}
	CircularIterator(const CircularIterator &otherIterator) : m_containerStart{otherIterator.m_containerStart}, m_containerEnd{otherIterator.m_containerEnd}, m_current{otherIterator.m_current}, m_index{otherIterator.m_index} {}
	inline T& operator*() const { return *m_current; }
	inline T* operator->() const { return m_current; }
	inline T& operator[](difference_type difference) const { return *(*this + difference); }

	inline CircularIterator& operator++() { increment();  return *this; }
	inline CircularIterator& operator--() { decrement();  return *this; }
	inline CircularIterator operator++(int) { CircularIterator tmp(*this); increment();  return tmp; }
	inline CircularIterator operator--(int) { CircularIterator tmp(*this); decrement();  return tmp; }

	inline CircularIterator operator+(difference_type difference) const { CircularIterator tmp(*this); tmp.advanceBy(difference); return tmp; }
	inline CircularIterator operator-(difference_type difference) const { CircularIterator tmp(*this); tmp.advanceBy(-difference); return tmp; }
	//inline CircularIterator operator+(const CircularIterator& otherIterator) const { CircularIterator tmp(*this); tmp.advanceBy(std::distance(m_containerStart,otherIterator.m_current)); return tmp; }
	inline difference_type operator-(const CircularIterator& otherIterator) const { return difference_type(m_current - otherIterator.m_current); }
	inline CircularIterator& operator+=(difference_type difference) { advanceBy(difference);  return *this;}
	inline CircularIterator& operator-=(difference_type difference) { advanceBy(-difference);  return *this;}
	friend inline CircularIterator operator+(difference_type difference, const CircularIterator& otherIterator) { CircularIterator tmp(otherIterator); tmp.advanceBy(difference); return tmp; }
	//friend inline CircularIterator operator-(difference_type difference, const CircularIterator& otherIterator) { CircularIterator tmp(otherIterator); tmp.m_current = difference - otherIterator.m_current; tmp.m_index = difference - otherIterator.m_index; return tmp; }

	inline bool operator==(const CircularIterator& otherIterator) const { return m_index == otherIterator.m_index; }
	inline bool operator!=(const CircularIterator& otherIterator) const { return m_index != otherIterator.m_index; }
	inline auto operator<=>(const CircularIterator& otherIterator) const { return m_index <=> otherIterator.m_index; }

private:
	void increment() {
		//@todo protect from nullptr?
		++m_index;
		if(++m_current >= m_containerEnd) {
			m_current = m_containerStart;
		}
	}

	void decrement() {
		//@todo protect from nullptr?
		--m_index;
		if(--m_current < m_containerStart) {
			m_current = m_containerEnd - 1;
		}
	}

	void advanceBy(difference_type difference) {
		//@todo protect from nullptr?
		if(difference == 0) {
			return;
		}

		auto limitedDifference = difference % std::distance(m_containerStart, m_containerEnd);
		if(limitedDifference > 0) {
			difference_type distanceToEnd = std::distance(m_current, m_containerEnd);
			m_current = distanceToEnd < limitedDifference ? m_containerStart + (limitedDifference - distanceToEnd) : m_current + limitedDifference;
		} else {
			difference_type distanceToStart = std::distance(m_containerStart, m_current);
			m_current = distanceToStart < -limitedDifference ? m_containerEnd - (-limitedDifference - distanceToStart) : m_current + limitedDifference;
		}

		m_index += difference;
	}

	T* m_containerStart;
	T* m_containerEnd;
	T* m_current;
	size_t m_index;
};

} // abl

#endif //ABL_CIRCULARITERATOR_H
