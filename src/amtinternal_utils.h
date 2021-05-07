//
// Assertive MultiThreading Library
//
//  Copyright Marcin Sterkowiec, Piotr Tracz, 2021. Use, modification and
//  distribution is subject to license (see accompanying file license.txt)
//

#pragma once

namespace amt
{
	template<size_t NUM>
	struct NumOfBits
	{
		static const size_t value = ((NUM & 1) ? 1 : 0) + NumOfBits<(NUM >> 1)>::value;
	};

	template<>
	struct NumOfBits<0>
	{
		static const size_t value = 0;
	};
}


