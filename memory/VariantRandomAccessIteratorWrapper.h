// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef AUDIO_BUFFERS_VARIANTRANDOMACCESSITERATORWRAPPER_H
#define AUDIO_BUFFERS_VARIANTRANDOMACCESSITERATORWRAPPER_H

#include <concepts>
#include <iterator>
#include <cassert>
#include <boost/variant2.hpp>

namespace audioBuffers {

template <typename _Iterator, typename _Value, typename _Reference, typename _Difference>
concept random_access_iterator_for_value = std::random_access_iterator<_Iterator>
                                           && std::same_as<std::iter_value_t<_Iterator>, _Value>
                                           && std::same_as<std::iter_reference_t<_Iterator>, _Reference>
                                           && std::same_as<std::iter_difference_t<_Iterator>, _Difference>;

template <typename _Iterator, typename _Value>
concept random_access_iterator_for = std::same_as<std::remove_cvref_t<_Value>, std::remove_const_t<_Value>>
                                     && random_access_iterator_for_value<_Iterator, std::remove_const_t<_Value>, _Value&, std::ptrdiff_t>;


template <typename _Value, random_access_iterator_for<_Value> ... _Iterator>
class VariantRandomAccessIteratorWrapper {
public:
	using iterator_category = std::random_access_iterator_tag;
	using difference_type   = std::ptrdiff_t;
	using value_type        = std::remove_cv_t<_Value>;
	using reference         = _Value&;
	using iterator_type     = boost::variant2::variant<_Iterator...>;

	VariantRandomAccessIteratorWrapper() = default;

	template <random_access_iterator_for<_Value> _Iter, typename... _Args>
	VariantRandomAccessIteratorWrapper(std::in_place_type_t<_Iter>, _Args&&... args) noexcept (std::is_nothrow_constructible_v<_Iter, _Args&&...>) : m_iterator(std::in_place_type<_Iter>, std::forward<_Args>(args)...) {}
	VariantRandomAccessIteratorWrapper(iterator_type const& iter) noexcept (std::is_nothrow_copy_constructible_v<iterator_type>) : m_iterator(iter) {}
	VariantRandomAccessIteratorWrapper(VariantRandomAccessIteratorWrapper const&) = default;
	VariantRandomAccessIteratorWrapper(VariantRandomAccessIteratorWrapper&&) = default;

	constexpr VariantRandomAccessIteratorWrapper& operator=(VariantRandomAccessIteratorWrapper const& iter) noexcept (std::is_nothrow_copy_assignable_v<iterator_type>) //
	requires std::is_copy_assignable_v<iterator_type> {
		m_iterator = iter.m_iterator;
		return *this;
	}

	constexpr VariantRandomAccessIteratorWrapper& operator=(VariantRandomAccessIteratorWrapper&& iter) noexcept (std::is_nothrow_move_assignable_v<iterator_type>)
	requires std::is_move_assignable_v<iterator_type> {
		m_iterator = std::move(iter.m_iterator);
		return *this;
	}

	template <typename _Tp>
	constexpr VariantRandomAccessIteratorWrapper& operator=(_Tp const& x) noexcept (std::is_nothrow_assignable_v<iterator_type, _Tp&>)
	requires std::is_assignable_v<iterator_type, _Tp&> {
		m_iterator = x;
		return *this;
	}

	template <typename _Tp>
	constexpr VariantRandomAccessIteratorWrapper& operator=(_Tp&& x) noexcept (std::is_nothrow_assignable_v<iterator_type, _Tp&&>)
	requires std::is_assignable_v<iterator_type, _Tp&&> {
		m_iterator = std::move(x);
		return *this;
	}

	[[nodiscard]] constexpr reference operator*() const noexcept {
		return boost::variant2::visit([](auto&& iter) -> reference {
			return *iter;
		}, m_iterator);
	}

	constexpr VariantRandomAccessIteratorWrapper& operator++() noexcept {
		boost::variant2::visit([](auto&& iter) {
			++iter;
		}, m_iterator);
		return *this;
	}

	constexpr VariantRandomAccessIteratorWrapper operator++(int) noexcept {
		VariantRandomAccessIteratorWrapper rv(*this);
		++*this;
		return rv;
	}

	constexpr VariantRandomAccessIteratorWrapper& operator--() noexcept {
		boost::variant2::visit([](auto&& iter) {
			--iter;
		}, m_iterator);
		return *this;
	}

	constexpr VariantRandomAccessIteratorWrapper operator--(int) noexcept {
		VariantRandomAccessIteratorWrapper rv(*this);
		--*this;
		return rv;
	}

	constexpr VariantRandomAccessIteratorWrapper operator+(difference_type difference) const {
		return boost::variant2::visit([difference](auto&& iter) -> VariantRandomAccessIteratorWrapper {
			return VariantRandomAccessIteratorWrapper(iter + difference);
		}, m_iterator);
	}

	constexpr VariantRandomAccessIteratorWrapper operator-(difference_type difference) const {
		return boost::variant2::visit([difference](auto&& iter) -> VariantRandomAccessIteratorWrapper {
			return VariantRandomAccessIteratorWrapper(iter - difference);
		}, m_iterator);
	}

	constexpr difference_type operator-(const VariantRandomAccessIteratorWrapper& otherIterator) const {
		return boost::variant2::visit([](auto&& iter, auto&& otherIter) -> difference_type {
			if constexpr (std::same_as<std::remove_const_t<decltype(iter)>, std::remove_const_t<decltype(otherIter)>>) {
				return difference_type(iter - otherIter); //
			}
			assert(false);
			return difference_type();
		}, m_iterator, otherIterator.m_iterator);
	}

	constexpr VariantRandomAccessIteratorWrapper& operator+=(difference_type difference) {
		boost::variant2::visit([difference](auto&& iter) {
			iter += difference;
		}, m_iterator);
		return *this;
	}

	constexpr VariantRandomAccessIteratorWrapper& operator-=(difference_type difference) {
		boost::variant2::visit([difference](auto&& iter) {
			iter -= difference;
		}, m_iterator);
		return *this;
	}

	friend constexpr VariantRandomAccessIteratorWrapper operator+(difference_type difference, const VariantRandomAccessIteratorWrapper& otherIterator) {
		return boost::variant2::visit([difference](auto&& iter) -> VariantRandomAccessIteratorWrapper {
			return VariantRandomAccessIteratorWrapper(iter + difference);
		}, otherIterator.m_iterator);
	}

	[[nodiscard]] friend constexpr bool operator==(VariantRandomAccessIteratorWrapper const& a, VariantRandomAccessIteratorWrapper const& b) noexcept {
		return (a.m_iterator == b.m_iterator);
	}

	[[nodiscard]] friend constexpr bool operator!=(VariantRandomAccessIteratorWrapper const& a, VariantRandomAccessIteratorWrapper const& b) noexcept {
		return (a.m_iterator != b.m_iterator);
	}

	[[nodiscard]] friend constexpr bool operator<(VariantRandomAccessIteratorWrapper const& a, VariantRandomAccessIteratorWrapper const& b) noexcept {
		return a.m_iterator < b.m_iterator;
	}

	[[nodiscard]] friend constexpr bool operator<=(VariantRandomAccessIteratorWrapper const& a, VariantRandomAccessIteratorWrapper const& b) noexcept {
		return a.m_iterator <= b.m_iterator;
	}

	[[nodiscard]] friend constexpr bool operator>(VariantRandomAccessIteratorWrapper const& a, VariantRandomAccessIteratorWrapper const& b) noexcept {
		return a.m_iterator > b.m_iterator;
	}

	[[nodiscard]] friend constexpr bool operator>=(VariantRandomAccessIteratorWrapper const& a, VariantRandomAccessIteratorWrapper const& b) noexcept {
		return a.m_iterator >= b.m_iterator;
	}

protected:
	iterator_type m_iterator;
};

} // audioBuffers

#endif //AUDIO_BUFFERS_VARIANTRANDOMACCESSITERATORWRAPPER_H