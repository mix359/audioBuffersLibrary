#ifndef ABL_PARENTREFERENCINGITERATOR_H
#define ABL_PARENTREFERENCINGITERATOR_H

#include <iterator>
#include <memory>

namespace abl {

template <typename ParentType, typename ReturnType>
class ParentReferencingIterator {
public:
	using iterator_category = std::random_access_iterator_tag;
	using value_type = std::unique_ptr<ReturnType>;
	using reference = std::unique_ptr<ReturnType>;
	using pointer = ReturnType*;
	using difference_type = std::ptrdiff_t;

//	ParentReferencingIterator() : m_index{0}, m_parentBuffer{nullptr} {}
	ParentReferencingIterator(size_t startIndex, ParentType& parentBuffer) : m_index(startIndex), m_parentBuffer{parentBuffer} {}
	ParentReferencingIterator(const ParentReferencingIterator &otherIterator) : m_index(otherIterator.m_index), m_parentBuffer{otherIterator.m_parentBuffer} {}
	/* inline iterator& operator=(AudioSampleType* rhs) {m_ptr = rhs; return *this;} */
	inline ParentReferencingIterator& operator=(const ParentReferencingIterator &otherIterator) {m_index = otherIterator.m_index; m_parentBuffer = otherIterator.m_parentBuffer; return *this;}
	inline ParentReferencingIterator& operator+=(difference_type difference) { m_index += difference;  return *this;}
	inline ParentReferencingIterator& operator-=(difference_type difference) { m_index -= difference;  return *this;}
	inline ReturnType operator*() const { return m_parentBuffer[m_index]; }
	inline ReturnType* operator->() const { return m_parentBuffer[m_index].get(); }
	inline ReturnType operator[](difference_type difference) const { return m_parentBuffer[difference]; }

	inline ParentReferencingIterator& operator++() { ++m_index;  return *this; }
	inline ParentReferencingIterator& operator--() { --m_index;  return *this; }
	inline ParentReferencingIterator operator++(int) { ParentReferencingIterator tmp(*this); ++m_index;  return tmp; }
	inline ParentReferencingIterator operator--(int) { ParentReferencingIterator tmp(*this); --m_index;  return tmp; }
	inline ParentReferencingIterator operator+(const ParentReferencingIterator& otherIterator) { return ParentReferencingIterator(m_index+otherIterator.m_index, m_parentBuffer); }
	inline difference_type operator-(const ParentReferencingIterator& otherIterator) const { return m_index - otherIterator.ptr; }
	inline ParentReferencingIterator operator+(difference_type difference) const { return ParentReferencingIterator(m_index + difference, m_parentBuffer); }
	inline ParentReferencingIterator operator-(difference_type difference) const { return ParentReferencingIterator(m_index - difference, m_parentBuffer); }
	friend inline ParentReferencingIterator operator+(difference_type lhs, const ParentReferencingIterator& otherIterator) { return ParentReferencingIterator(lhs+otherIterator.m_index, otherIterator.m_parentBuffer); }
	friend inline ParentReferencingIterator operator-(difference_type lhs, const ParentReferencingIterator& otherIterator) { return ParentReferencingIterator(lhs-otherIterator.m_index, otherIterator.m_parentBuffer); }

	inline bool operator==(const ParentReferencingIterator& otherIterator) const { return m_index == otherIterator.m_index; }
	inline bool operator!=(const ParentReferencingIterator& otherIterator) const { return m_index != otherIterator.m_index; }
	inline bool operator>(const ParentReferencingIterator& otherIterator) const { return m_index > otherIterator.m_index; }
	inline bool operator<(const ParentReferencingIterator& otherIterator) const { return m_index < otherIterator.m_index; }
	inline bool operator>=(const ParentReferencingIterator& otherIterator) const { return m_index >= otherIterator.m_index; }
	inline bool operator<=(const ParentReferencingIterator& otherIterator) const { return m_index <= otherIterator.m_index; }

private:
	size_t m_index;
	ParentType& m_parentBuffer;
};

} // abl

#endif //ABL_PARENTREFERENCINGITERATOR_H
