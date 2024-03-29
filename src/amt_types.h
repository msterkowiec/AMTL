//
// Assertive MultiThreading Library
//
//  Copyright Marcin Sterkowiec, Piotr Tracz, 2021-2022. Use, modification and
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
	using IsConstIter = std::is_same<T, typename Container::const_iterator>;

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

	template <class T, template <class...> class Template>
	struct is_specialization : std::false_type {};

	template <template <class...> class Template, class... Args>
	struct is_specialization<Template<Args...>, Template> : std::true_type{};

	template<typename T, typename Enable = void>
	struct make_signed {
		typedef typename std::make_signed<T>::type type;
	};

	template<typename T>
	struct make_signed<T,
		typename std::enable_if<std::is_floating_point<T>::value>::type> {
		typedef T type;
	};

	template<typename T, typename Enable = void>
	struct make_unsigned {
		typedef typename std::make_unsigned<T>::type type;
	};

	template<typename T>
	struct make_unsigned<T,
		typename std::enable_if<std::is_floating_point<T>::value>::type> {
		typedef T type;
	};

	template<typename U, bool isAMTScalarType = false>
	struct UnwrappedType
	{
		typedef U type;
	};
	template<typename U>
	struct UnwrappedType<U, true>
	{
		typedef typename U::UnderlyingType type;
	};

	template<typename T, typename = void>
	struct is_iterator
	{
		static constexpr bool value = false;
	};

	template<typename T>
	struct is_iterator<T, typename std::enable_if<!std::is_same<typename std::iterator_traits<T>::value_type, void>::value>::type>
	{
		static constexpr bool value = true;
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

#ifdef __AMTL_ASSERTS_ARE_ON__
#define AMTL_SELECT_FLOATING_POINT_TYPE(T,U) typename std::conditional<\
													std::is_floating_point<U>::value, \
													typename std::conditional<\
																!std::is_floating_point<T>::value || (sizeof(U) > sizeof(T)), \
																U, \
																T\
																	>::type, \
													T\
																	  >::type

#define AMTL_TYPE_IS_LONGLONG(T) ((sizeof(T) > 4) && (std::is_integral<T>::value))

#define AMTL_SELECT_INT_TYPE(T, U) typename std::conditional<std::is_same<T,unsigned int>::value || std::is_same<U,unsigned int>::value, unsigned int,\
													typename std::conditional<sizeof(unsigned long) == 4 && (std::is_same<T,unsigned long>::value || std::is_same<U,unsigned long>::value),unsigned long, int>::type\
																							>::type

#define AMTL_SELECT_LONGLONG_TYPE(T, U) typename std::conditional<std::is_same<T,unsigned long long>::value || std::is_same<U,unsigned long long>::value, \
																  unsigned long long, \
																  typename std::conditional<(sizeof(unsigned long) > 4) && (std::is_same<T,unsigned long>::value || std::is_same<U,unsigned long>::value),unsigned long,\
																			typename std::conditional<(sizeof(long) == 4) || std::is_same<T, long long>::value || std::is_same<U, long long>::value,long long, long>::type\
																				>::type\
																	>::type

#define AMTL_LONG_LONG_OR_INT(T,U) typename std::conditional<AMTL_TYPE_IS_LONGLONG(T),AMTL_SELECT_LONGLONG_TYPE(T, U),typename std::conditional<AMTL_TYPE_IS_LONGLONG(U),U,AMTL_SELECT_INT_TYPE(T, U)>::type>::type

#define AMTL_RESULTANT_TYPE(T,U) typename std::conditional<\
													std::is_floating_point<U>::value, \
													typename std::conditional<\
																!std::is_floating_point<T>::value || (sizeof(U) > sizeof(T)), \
																U, \
																T\
																	>::type, \
													typename std::conditional<\
																std::is_floating_point<T>::value, \
																T,\
																AMTL_LONG_LONG_OR_INT(T,U)\
													                >::type \
																	  >::type

#define AMT_SIGNUM(a) (((a)>0)-((a)<0))

#endif
