//
// Assertive MultiThreading Library
//
//  Copyright Marcin Sterkowiec, Piotr Tracz, 2021. Use, modification and
//  distribution is subject to license (see accompanying file license.txt)
//

#pragma once

#include <iterator>
#include <type_traits>

#include "amt_config.h"
#include "amt_compat.h"

namespace amt
{

	// To minimize memory usage, though unsigned short or size_t might well be reasoable (today it is hard to imagine 256 threads competing for resource at the same time but it may change) :
	typedef unsigned char AMTCounterType;

	template<typename Container, typename T>
	using IsRegRevIter = std::is_same<T, typename Container::reverse_iterator>;

	template<typename Container, typename T>
	using IsConstRevIter = std::is_same<T, typename Container::const_reverse_iterator>;

	template<typename Container, typename T>
	struct IsRevIter
	{
		const static bool value = IsRegRevIter<Container, T>::value
			|| IsConstRevIter<Container, T>::value;
	};

}

#if __AMT_DEBUG__
#define AMT_DEBUG_CASSERT(a) AMT_CASSERT(a)
#else
#define AMT_DEBUG_CASSERT(a)
#endif

//
// Values in containters will automatically be wrapped up for simple types - temporarily excluding bool
//
#if __AMT_TRY_TO_AUTOMATICALLY_WRAP_UP_CONTAINERS_TYPES__ > 0
	#define __AMT_TRY_WRAP_TYPE__(T, dontWrap, Allocator) typename std::conditional<																    \
												  std::is_scalar<T>::value && !(dontWrap) && std::is_same<Allocator,std::allocator<T>>::value && !std::is_same<T, bool>::value,          \
												  typename std::conditional<std::is_pointer<T>::value, AMTPointerType<T>, AMTScalarType<T>>::type,      \
												  T                                                                                                     \
											   >::type 

	// We need this too because if we replace value type (T) on the fly, we also have to change allocator type (C++17 forces it). This is also why we avoid auto-replace value type if non-standard allocator is used
	#define __AMT_CHANGE_ALLOCATOR_IF_NEEDED__(T, dontWrap, Allocator) typename std::conditional<                                                                                                             \
	   								 			  	 		              std::is_scalar<T>::value && !(dontWrap) && std::is_same<Allocator,std::allocator<T>>::value && !std::is_same<T, bool>::value,                                        \
															              typename std::conditional<std::is_pointer<T>::value, std::allocator<AMTPointerType<T>>, std::allocator<AMTScalarType<T>> >::type,   \
															              std::allocator<T>                                                                                                                   \
											                          >::type

	#define __AMT_TRY_WRAP_MAPPED_TYPE__(Key, T, dontWrap, Allocator) typename std::conditional<                                                                                                 \
	   								 			  	 		              std::is_scalar<T>::value && !(dontWrap) && std::is_same<Allocator,std::allocator<std::pair<const Key, T>>>::value && !std::is_same<T, bool>::value,     \
															              typename std::conditional<std::is_pointer<T>::value, AMTPointerType<T>, AMTScalarType<T>>::type,                       \
															              T                                                                                                                      \
											                          >::type 

	#define __AMT_CHANGE_MAP_ALLOCATOR_IF_NEEDED__(Key, T, dontWrap, Allocator) typename std::conditional<                                                                                      \
	   								 			  	 		              std::is_scalar<T>::value && !(dontWrap) && std::is_same<Allocator,std::allocator<std::pair<const Key, T>>>::value && !std::is_same<T, bool>::value,    \
															              typename std::conditional<std::is_pointer<T>::value, std::allocator<std::pair<const Key, AMTPointerType<T>>>, std::allocator<std::pair<const Key, AMTScalarType<T>>> >::type,   \
															              std::allocator<std::pair<const Key, T>>                                                                                \
											                          >::type

#else
#define __AMT_TRY_WRAP_TYPE__(T, dontWrap, Allocator) T
#define __AMT_TRY_WRAP_MAPPED_TYPE__(Key, T, dontWrap, Allocator) T
#define __AMT_CHANGE_ALLOCATOR_IF_NEEDED__(T, dontWrap, Allocator) Allocator
#define __AMT_CHANGE_MAP_ALLOCATOR_IF_NEEDED__(Key, T, dontWrap, Allocator) Allocator
#endif


//#define __AMT_TRY_WRAP_TYPE__(T) typename std::conditional<																						\
	//										std::is_trivial<T>::value,																				\
	//										typename std::conditional<																				\
	//												std::is_scalar<T>::value,																		\
	//												typename std::conditional<std::is_pointer<T>::value, AMTPointerType<T>, AMTScalarType<T>>::type,\
	//												AMTTrivialType<T>																				\
	//											>::type,																							\
	//										T																										\
	//									 >::type 
