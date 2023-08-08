// enum_operator.hpp
// Copyright (c) 2023 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef LEXERTL_ENUM_OPERATOR_HPP
#define LEXERTL_ENUM_OPERATOR_HPP

#include <cstdint>
#include <type_traits>

namespace lexertl
{
	// Operator to convert enum class to underlying type (usually int)
	// Example:
	// enum class number { one = 1, two, three };
	// int num = *number::two;
	template <typename T>
	auto operator*(T e) noexcept ->
		std::enable_if_t<std::is_enum<T>::value, uint16_t>
	{
		return static_cast<uint16_t>(e);
	}

	// This is the compile time version of the above operator
	// (e.g. Setting a C style array size using an enum)
	template <typename T>
	constexpr auto operator+(T e) noexcept ->
		std::enable_if_t<std::is_enum<T>::value, uint16_t>
	{
		return static_cast<uint16_t>(e);
	}
}

#endif
