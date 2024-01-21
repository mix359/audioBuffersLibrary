// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef ABL_NUMERIC_CONCEPT_H
#define ABL_NUMERIC_CONCEPT_H

#include <type_traits>

template<typename T>
concept NumericType = requires(T param)
{
	requires std::is_integral_v<T> || std::is_floating_point_v<T>;
	requires !std::is_same_v<bool, T>;
	requires std::is_arithmetic_v<decltype(param +1)>;
	requires !std::is_pointer_v<T>;
};

#endif //ABL_NUMERIC_CONCEPT_H
