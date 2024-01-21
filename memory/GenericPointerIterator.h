// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef ABL_GENERICPOINTERITERATOR_H
#define ABL_GENERICPOINTERITERATOR_H

#include <iterator>

namespace abl {

template <typename T>
class GenericPointerIterator {
public:
	using iterator_category = std::random_access_iterator_tag;
	using value_type = T;
	using reference = T&;
	using pointer = T*;
	using difference_type = std::ptrdiff_t;

	GenericPointerIterator() : m_current{nullptr} {}
	GenericPointerIterator(T* current) : m_current{current} {}
	GenericPointerIterator(const GenericPointerIterator &otherIterator) : m_current{otherIterator.m_current} {}
	inline T& operator*() const { return *m_current; }
	inline T* operator->() const { return m_current; }
	inline T& operator[](difference_type difference) const { return *(*this + difference); }

	inline GenericPointerIterator& operator++() { ++m_current;  return *this; }
	inline GenericPointerIterator& operator--() { --m_current;  return *this; }
	inline GenericPointerIterator operator++(int) { GenericPointerIterator tmp(*this); ++m_current;  return tmp; }
	inline GenericPointerIterator operator--(int) { GenericPointerIterator tmp(*this); --m_current;  return tmp; }

	inline GenericPointerIterator operator+(difference_type difference) const { return GenericPointerIterator(m_current + difference); }
	inline GenericPointerIterator operator-(difference_type difference) const { return GenericPointerIterator(m_current - difference); }
	//inline GenericPointerIterator operator+(const GenericPointerIterator& otherIterator) const noexcept { return GenericPointerIterator(m_current + otherIterator.m_current); }
	inline difference_type operator-(const GenericPointerIterator& otherIterator) const { return difference_type(m_current - otherIterator.m_current); }
	inline GenericPointerIterator& operator+=(difference_type difference) { m_current += difference;  return *this;}
	inline GenericPointerIterator& operator-=(difference_type difference) { m_current -= difference;  return *this;}
	friend inline GenericPointerIterator operator+(difference_type difference, const GenericPointerIterator& iterator) { return GenericPointerIterator(difference + iterator.m_current); }
	//friend inline GenericPointerIterator operator-(difference_type difference, const GenericPointerIterator& iterator) { return GenericPointerIterator(difference - iterator.m_current); }

	inline bool operator==(const GenericPointerIterator& otherIterator) const { return m_current == otherIterator.m_current; }
	inline bool operator!=(const GenericPointerIterator& otherIterator) const { return m_current != otherIterator.m_current; }
	inline auto operator<=>(const GenericPointerIterator& otherIterator) const { return m_current <=> otherIterator.m_current; }

private:
	T* m_current;
};

} // abl

#endif //ABL_GENERICPOINTERITERATOR_H
