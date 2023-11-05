//
// Assertive MultiThreading Library
//
//  Copyright Marcin Sterkowiec, Piotr Tracz, 2021-2022. Use, modification and
//  distribution is subject to license (see accompanying file license.txt)
//

#pragma once

#include <cmath>
#include <cstdint>
#include <limits>
#include <math.h>
#include <atomic>
#include <numeric>
#include <typeinfo>
#include <type_traits>
#include "amt_cassert.h"
#include "amt_types.h"
#include "amt_compat.h"

#if defined(__AMTL_ASSERTS_ARE_ON__)
#if __AMT_FORCE_SAME_SIZE_FOR_TRIVIAL_TYPES__
#include "amtinternal_hashmap.h"
#endif
#endif

namespace amt
{
	#if defined(__AMTL_ASSERTS_ARE_ON__)

	// This is really tempting to move read/write counters to some global hash_map and enjoy having sizeof(amt::uint8_t)==1 instead of 3
	// This is the case of __AMT_FORCE_SAME_SIZE_FOR_TRIVIAL_TYPES__ - the code will work with all the memcpy/memmove and persistency but will be significantly slower

	template<typename T> 
	class AMTScalarType
	{
		template<typename U>
		friend class AMTScalarType;

		static_assert(std::is_scalar<T>::value, "Template parameter of AMTScalarType has to be a scalar type");
		typedef typename std::conditional<std::is_floating_point<T>::value, T, typename amt::make_unsigned<T>::type>::type unsigned_T;
		typedef typename std::conditional<std::is_floating_point<T>::value, T, typename amt::make_signed<T>::type>::type  signed_T;
		typedef typename std::conditional<!std::is_floating_point<T>::value && std::is_signed<T>::value, typename amt::make_unsigned<T>::type, typename amt::make_signed<T>::type>::type opposite_signedness_T;

	public:
		typedef T UnderlyingType;

	private:
		T m_val;		

		#if !__AMT_FORCE_SAME_SIZE_FOR_TRIVIAL_TYPES__
		mutable std::atomic<AMTCounterType> m_nPendingReadRequests;
		mutable std::atomic<AMTCounterType> m_nPendingWriteRequests;
		#endif

		__AMT_FORCEINLINE__ void Init()
		{
			#if __AMT_FORCE_SAME_SIZE_FOR_TRIVIAL_TYPES__
			static_assert(sizeof(AMTScalarType) == sizeof(T), "sizeof(AMTScalarType) is expected to be the same as sizeof the underlying type");
			AMTCountersHashMap* pHashMap = AMTCountersHashMap::GetCounterHashMap();
			pHashMap->RegisterAddress((void*) this);
			#else
			m_nPendingReadRequests = 0;
			m_nPendingWriteRequests = 0;
			#endif
		}
		__AMT_FORCEINLINE__ void Uninit()
		{
			#if __AMT_FORCE_SAME_SIZE_FOR_TRIVIAL_TYPES__
			AMTCountersHashMap* pHashMap = AMTCountersHashMap::GetCounterHashMap();
			auto pSlot = pHashMap->GetReadWriteCounters((void*) this);
			AMT_VERIFY_SLOT(pSlot);
			AMT_CASSERT(pSlot->m_nPendingWriteRequests == 0);
			AMT_CASSERT(pSlot->m_nPendingReadRequests == 0);
			AMT_CASSERT(pSlot->m_nSlotUsed == 1);
			pHashMap->UnregisterAddress(this);
			#else
			AMT_CASSERT(m_nPendingWriteRequests == 0);
			AMT_CASSERT(m_nPendingReadRequests == 0);
			#endif
		}
		__AMT_FORCEINLINE__ void RegisterReadingThread() const
		{
			#if __AMT_FORCE_SAME_SIZE_FOR_TRIVIAL_TYPES__
			AMTCountersHashMap* pHashMap = AMTCountersHashMap::GetCounterHashMap();
			auto pSlot = pHashMap->GetReadWriteCounters((void*) this);
			AMT_VERIFY_SLOT(pSlot);
			++pSlot->m_nPendingReadRequests;
			AMT_CASSERT(pSlot->m_nPendingWriteRequests == 0);
			AMT_CASSERT(pSlot->m_nSlotUsed == 1);
			#else
			++m_nPendingReadRequests;
			AMT_CASSERT(m_nPendingWriteRequests == 0);
			#endif
		}
		__AMT_FORCEINLINE__ void UnregisterReadingThread() const
		{
			#if __AMT_FORCE_SAME_SIZE_FOR_TRIVIAL_TYPES__
			AMTCountersHashMap* pHashMap = AMTCountersHashMap::GetCounterHashMap();
			auto pSlot = pHashMap->GetReadWriteCounters((void*) this);
			AMT_VERIFY_SLOT(pSlot);
			AMT_CASSERT(pSlot->m_nPendingWriteRequests == 0);
			AMT_CASSERT(pSlot->m_nSlotUsed == 1);
			--pSlot->m_nPendingReadRequests;
			#else
			AMT_CASSERT(m_nPendingWriteRequests == 0);
			--m_nPendingReadRequests;
			#endif
		}
		__AMT_FORCEINLINE__ void RegisterWritingThread() const
		{
			#if __AMT_FORCE_SAME_SIZE_FOR_TRIVIAL_TYPES__
			AMTCountersHashMap* pHashMap = AMTCountersHashMap::GetCounterHashMap();
			auto pSlot = pHashMap->GetReadWriteCounters((void*) this);
			AMT_VERIFY_SLOT(pSlot);
			++pSlot->m_nPendingWriteRequests;
			AMT_CASSERT(pSlot->m_nPendingWriteRequests == 1);
			AMT_CASSERT(pSlot->m_nPendingReadRequests == 0);
			AMT_CASSERT(pSlot->m_nSlotUsed == 1);
			#else
			++m_nPendingWriteRequests;
			AMT_CASSERT(m_nPendingWriteRequests == 1);
			AMT_CASSERT(m_nPendingReadRequests == 0);
			#endif
		}
		__AMT_FORCEINLINE__ void UnregisterWritingThread() const
		{
			#if __AMT_FORCE_SAME_SIZE_FOR_TRIVIAL_TYPES__
			AMTCountersHashMap* pHashMap = AMTCountersHashMap::GetCounterHashMap();
			auto pSlot = pHashMap->GetReadWriteCounters((void*) this);
			AMT_VERIFY_SLOT(pSlot);
			AMT_CASSERT(pSlot->m_nPendingWriteRequests == 1);
			AMT_CASSERT(pSlot->m_nPendingReadRequests == 0);
			AMT_CASSERT(pSlot->m_nSlotUsed == 1);
			--pSlot->m_nPendingWriteRequests;
			#else
			AMT_CASSERT(m_nPendingWriteRequests == 1);
			AMT_CASSERT(m_nPendingReadRequests == 0);
			--m_nPendingWriteRequests;
			#endif
		}


		class CRegisterReadingThread
		{
			template<typename U>
			friend class AMTScalarType;

			const AMTScalarType& m_var;

		public:
			inline CRegisterReadingThread(const AMTScalarType& var) : m_var(var)
			{
				m_var.RegisterReadingThread();
			}
			inline CRegisterReadingThread(const volatile AMTScalarType& var) : m_var((const AMTScalarType&)var)
			{
				m_var.RegisterReadingThread();
			}
			inline ~CRegisterReadingThread() __AMT_CAN_THROW__
			{
				m_var.UnregisterReadingThread();
			}
		};

		class CRegisterWritingThread
		{
			const AMTScalarType& m_var;

		public:
			inline CRegisterWritingThread(const AMTScalarType& var) : m_var(var)
			{
				m_var.RegisterWritingThread();
			}
			inline CRegisterWritingThread(const volatile AMTScalarType& var) : m_var((const AMTScalarType&)var)
			{
				m_var.RegisterWritingThread();
			}
			inline ~CRegisterWritingThread() __AMT_CAN_THROW__
			{
				m_var.UnregisterWritingThread();
			}
		};


	public:
		inline AMTScalarType()
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			Init();
			#endif
			#if __ATM_INITIALIZE_AMTL_VARIABLES__
			m_val = 0; // breaks standard behaviour (no initialization by default) but lets initialize correctly in the following context: i = int() or db = double(); 
			#endif
		}
		inline AMTScalarType(const AMTScalarType& o)
		{
			try{
				#if __AMT_CHECK_MULTITHREADED_ISSUES__
				Init();
				CRegisterWritingThread r(*this);
				CRegisterReadingThread r2(o);
				#endif
				m_val = o.m_val;
			}
			catch(...) {
				#if __AMT_CHECK_MULTITHREADED_ISSUES__
				Uninit();				
				#endif
				throw;
			}				
		}		
		template<typename U, class = typename std::enable_if<std::is_arithmetic<U>::value>::type>
		inline AMTScalarType(const AMTScalarType<U>& o)
		{
			try{
				#if __AMT_CHECK_MULTITHREADED_ISSUES__
				Init();
				CRegisterWritingThread r(*this);
				typename AMTScalarType<U>::CRegisterReadingThread r2(o);
				#endif
				#if __AMT_CHECK_NUMERIC_OVERFLOW__
				if (!std::is_same<T,U>::value)
					CheckAssignmentOverflow(o.m_val);
				#endif
				m_val = o.m_val;
			}
			catch(...) {
				#if __AMT_CHECK_MULTITHREADED_ISSUES__
				Uninit();				
				#endif
				throw;
			}				
		}
		inline AMTScalarType(T t)
		{
			try {
				#if __AMT_CHECK_MULTITHREADED_ISSUES__
				Init();
				CRegisterWritingThread r(*this);
				#endif
				m_val = t;
			}
			catch(...) {
				#if __AMT_CHECK_MULTITHREADED_ISSUES__
				Uninit();				
				#endif
				throw;
			}			
		}
		template<typename U, class = typename std::enable_if<std::is_arithmetic<U>::value>::type>
		inline AMTScalarType(U u) 
		{
			try {
				#if __AMT_CHECK_MULTITHREADED_ISSUES__
				Init();
				CRegisterWritingThread r(*this);
				#endif
				#if __AMT_CHECK_NUMERIC_OVERFLOW__
				if (!std::is_same<T,U>::value)
					CheckAssignmentOverflow(u);
				#endif
				m_val = u;
			}
			catch(...) {
				#if __AMT_CHECK_MULTITHREADED_ISSUES__
				Uninit();				
				#endif
				throw;
			}			
		}
		template<typename U, class = typename std::enable_if<std::is_arithmetic<U>::value>::type>
		inline AMTScalarType(AMTScalarType<U>&& u) 
		{
			try {
				#if __AMT_CHECK_MULTITHREADED_ISSUES__
				Init();
				CRegisterWritingThread r(*this);
				typename AMTScalarType<U>::CRegisterReadingThread r2(u);
				#endif
				#if __AMT_CHECK_NUMERIC_OVERFLOW__
				if (!std::is_same<T,U>::value)
					CheckAssignmentOverflow(u.m_val); //(AMTScalarType<U>::UnderlyingType)u);
				#endif
				m_val = u.m_val;
			}
			catch(...) {
				#if __AMT_CHECK_MULTITHREADED_ISSUES__
				Uninit();				
				#endif
				throw;
			}			
		}

		inline ~AMTScalarType() __AMT_CAN_THROW__
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			Uninit();
			#endif
		}
		inline AMTScalarType& operator = (const T t)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			#endif
			m_val = t;
			return *this;
		}
		inline volatile AMTScalarType& operator = (const T t) volatile
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			#endif
			m_val = t;
			return *this;
		}
		inline AMTScalarType& operator = (const AMTScalarType& var)
		{
			if (&var != this)
			{ 
				#if __AMT_CHECK_MULTITHREADED_ISSUES__						 
				CRegisterWritingThread r(*this);
				CRegisterReadingThread r2(var);
				#endif
				m_val = var.m_val;
				return *this;
			}
			else
			{
				#if __AMT_CHECK_MULTITHREADED_ISSUES__						 
				CRegisterWritingThread r(*this);
				#endif
				m_val = var.m_val;
				return *this;
			}
		}
		inline volatile AMTScalarType& operator = (const AMTScalarType& var) volatile
		{
			if (&var != this)
			{ 
				#if __AMT_CHECK_MULTITHREADED_ISSUES__
				CRegisterWritingThread r(*this);
				CRegisterReadingThread r2(var);
				#endif
				m_val = var.m_val;
				return *this;
			}
			else
			{
				#if __AMT_CHECK_MULTITHREADED_ISSUES__
				CRegisterWritingThread r(*this);
				#endif
				m_val = var.m_val;
				return *this;
			}
		}
		template<typename U, class = typename std::enable_if<std::is_arithmetic<U>::value>::type>
		inline AMTScalarType& operator =(U u)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			#endif
			#if __AMT_CHECK_NUMERIC_OVERFLOW__
			CheckAssignmentOverflow(u);
			#endif
			m_val = u;
			return *this;
		}
		template<typename U, class = typename std::enable_if<std::is_arithmetic<U>::value>::type>
		inline AMTScalarType& operator =(const AMTScalarType<U>& u)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			typename AMTScalarType<U>::CRegisterReadingThread r2(u);
			#endif
			#if __AMT_CHECK_NUMERIC_OVERFLOW__
			CheckAssignmentOverflow(u.m_val);// (AMTScalarType<U>::UnderlyingType)u);
			#endif
			m_val = u.m_val; // (AMTScalarType<U>::UnderlyingType) u;
			return *this;
		}
		template<typename U, class = typename std::enable_if<std::is_arithmetic<U>::value>::type>
		inline AMTScalarType& operator =(AMTScalarType<U>&& u)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			typename AMTScalarType<U>::CRegisterWritingThread r2(u);
			#endif
			#if __AMT_CHECK_NUMERIC_OVERFLOW__
			CheckAssignmentOverflow(u.m_val);// (AMTScalarType<U>::UnderlyingType)u);
			#endif
			m_val = u.m_val; // (AMTScalarType<U>::UnderlyingType) u;
			return *this;
		}
		unsigned_T MakeUnsigned() const
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(*this);
			#endif
			return (unsigned_T)m_val;
		}
		signed_T MakeSigned() const
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(*this);
			#endif
			return (signed_T)m_val;
		}
		#if __AMT_CHECK_NUMERIC_OVERFLOW__
	private:
		template<typename U, typename V, typename ResType>
		__AMT_FORCEINLINE__ static void VerifyOverflow_Add(U u, V v)
		{
			if __AMT_IF_CONSTEXPR__(std::is_floating_point<U>::value || std::is_floating_point<V>::value)
			{
				//  TODO
			}
			else
				if __AMT_IF_CONSTEXPR__(sizeof(U) < sizeof(std::int64_t) && sizeof(V) < sizeof(std::int64_t) && sizeof(ResType) < sizeof(std::int64_t))
				{
					std::int64_t i64 = (std::int64_t) u;
					i64 += v;
					AMT_CASSERT(i64 <= (std::numeric_limits<ResType>::max)());
					AMT_CASSERT(i64 >= (std::numeric_limits<ResType>::min)());
				}
				else
					if (v != 0)
					{
						if (u >= 0 && v > 0)
							AMT_CASSERT(u + v > u && u + v >= v);
						else
							if (u < 0 && v < 0)
								AMT_CASSERT(u + v < u && u + v <= v);
							else
								if (!std::is_signed<ResType>::value)
									if (u < 0)
										AMT_CASSERT(llabs(u) <= llabs(v));
									else
										AMT_CASSERT(llabs(v) <= llabs(u));
					}
		}
		template<typename U, typename V, typename ResType>
		__AMT_FORCEINLINE__ static void VerifyOverflow_Subtract(U u, V v)
		{
			if __AMT_IF_CONSTEXPR__(std::is_floating_point<U>::value || std::is_floating_point<V>::value)
			{
				//  TODO
			}
			else
				if __AMT_IF_CONSTEXPR__(sizeof(ResType) < sizeof(std::int64_t))
				{
					#if __cplusplus >= 201606L || (defined(_MSC_VER) && _MSVC_LANG >= 201606L) // constexpr if available?
					static_assert(std::is_integral<ResType>::value, "Expected integral type");
					#endif

					std::int64_t i64 = (std::int64_t) u;
					i64 -= v;
					AMT_CASSERT(i64 <= (std::numeric_limits<ResType>::max)());
					AMT_CASSERT(i64 >= (std::numeric_limits<ResType>::min)());
				}
				else
					if (v != 0)
						if (u >= 0 && v < 0)
						{
							AMT_CASSERT(v != (std::numeric_limits<V>::min)() || sizeof(V) > 4); // e.g. any char - (-128) is always overflow
							AMT_CASSERT(u - v > u && v - u <= v);
						}
						else
							if (u < 0 && v > 0)
								AMT_CASSERT(u - v < u && v - u >= v);
							else
								if (!std::is_signed<ResType>::value)
									AMT_CASSERT(u >= v);
		}

		template<typename U, typename V, typename ResType>
		__AMT_FORCEINLINE__ static void VerifyOverflow_Mul(U u, V v)
		{
			if  __AMT_IF_CONSTEXPR__(!std::is_floating_point<ResType>::value)
				if  __AMT_IF_CONSTEXPR__(sizeof(ResType) < sizeof(std::int64_t))
				{
					if (std::is_signed<ResType>::value)
					{
						std::int64_t i64 = (std::int64_t) u;
						i64 *= v;
						AMT_CASSERT(i64 <= (std::numeric_limits<ResType>::max)());
						AMT_CASSERT(i64 >= (std::numeric_limits<ResType>::min)());
					}
					else
					{
						std::uint64_t ui64 = (std::int64_t) u;
						ui64 *= v;
						AMT_CASSERT(ui64 <= (std::numeric_limits<ResType>::max)());
						AMT_CASSERT(ui64 >= (std::numeric_limits<ResType>::min)());
					}
				}
				else
				{					
					if (u != 0)
					{
						ResType res = u * v;
						AMT_CASSERT(res / u == v);
						#if __cplusplus >= 201606L || (defined(_MSC_VER) && _MSVC_LANG >= 201606L) // if constexpr available?
						AMT_CASSERT(res % u == 0);
						#endif
					}
				}
			else
			{
				// TODO
			}
		}

		template<typename U, typename V, typename ResType>
		__AMT_FORCEINLINE__ static void VerifyOverflow_Div(U u, V v)
		{
			AMT_CASSERT(v != 0);
			
			if  __AMT_IF_CONSTEXPR__(std::is_floating_point<ResType>::value)
			{
				// TODO
			}
			else
			{
				if  __AMT_IF_CONSTEXPR__(std::is_signed<U>::value)
					if (v == -1)
						AMT_CASSERT(u != (std::numeric_limits<U>::min)()); // e.g. for char we cannot divide -128 by -1
				
				if  __AMT_IF_CONSTEXPR__(std::is_floating_point<V>::value)
				{
					double tmp = (double)u; 
					tmp /= v;
					tmp = floor(tmp);
					AMT_CASSERT(tmp <= (std::numeric_limits<U>::max)()); // e.g. unsigned char 100 / 0.3 overflows
					AMT_CASSERT(tmp >= (std::numeric_limits<U>::min)());
				}

				if  __AMT_IF_CONSTEXPR__(std::is_unsigned<ResType>::value)
					AMT_CASSERT(AMT_SIGNUM(v) + AMT_SIGNUM(u) != 0); // opposite signs?

			}
								
					
		}
		#endif

	public:
		template<typename U>
		inline static auto Unwrap(U& u)
		#if defined(_MSC_VER) && _MSVC_LANG < 201402L
			-> typename UnwrappedType<U, amt::is_specialization<std::remove_cv_t<U>, amt::AMTScalarType>::value>::type
		#endif
		{
			typedef typename UnwrappedType<U, amt::is_specialization<std::remove_cv_t<U>,amt::AMTScalarType>::value>::type ResType;

			ResType res(u);
			return res;
		}

		template <typename U>
		inline AMTScalarType& operator += (const U& u)
		{
			#if __AMT_CHECK_NUMERIC_OVERFLOW__
			VerifyOverflow_Add<T,U,T>(m_val, Unwrap(u));
			#endif

			if (this != (void*)&u)
			{				
				#if __AMT_CHECK_MULTITHREADED_ISSUES__
				CRegisterWritingThread r1(*this);
				//typename AMTScalarType<U>::CRegisterReadingThread r2(u); // TODO
				#endif
				m_val += u;
			}
			else
			{
				auto val = u; // make a copy now to avoid raising issue (multithreaded access conflict false positive)				
				#if __AMT_CHECK_MULTITHREADED_ISSUES__
				CRegisterWritingThread r1(*this);
				//typename AMTScalarType<U>::CRegisterReadingThread r2(u); // TODO
				#endif
				m_val += val;
			}
			return *this;
		}
		template <typename U>
		inline AMTScalarType& operator -= (const U& u)
		{
			#if __AMT_CHECK_NUMERIC_OVERFLOW__
			VerifyOverflow_Subtract<T,U,T>(m_val, u);
			#endif
			if (this != (void*)&u)
			{ 				
				#if __AMT_CHECK_MULTITHREADED_ISSUES__
				CRegisterWritingThread r1(*this);
				//typename AMTScalarType<U>::CRegisterReadingThread r2(u); // TODO
				#endif
				m_val -= u;
			}
			else
			{
				auto val = u; // make a copy now to avoid raising issue (multithreaded access conflict false positive)				
				#if __AMT_CHECK_MULTITHREADED_ISSUES__
				CRegisterWritingThread r1(*this);
				//typename AMTScalarType<U>::CRegisterReadingThread r2(u); // TODO
				#endif
				m_val -= val;
			}
			return *this;
		}
		template <typename U>
		inline AMTScalarType& operator *= (const U& u)
		{
			#if __AMT_CHECK_NUMERIC_OVERFLOW__
			VerifyOverflow_Mul<T,U,T>(m_val, u);
			#endif
			if (this != (void*)&u)
			{ 				
				#if __AMT_CHECK_MULTITHREADED_ISSUES__
				CRegisterWritingThread r1(*this);
				//typename AMTScalarType<U>::CRegisterReadingThread r2(u); // TODO
				#endif
				m_val *= u;
			}
			else
			{
				auto val = u; // make a copy now to avoid raising issue (multithreaded access conflict false positive)				
				#if __AMT_CHECK_MULTITHREADED_ISSUES__
				CRegisterWritingThread r1(*this);
				//typename AMTScalarType<U>::CRegisterReadingThread r2(u); // TODO
				#endif
				m_val *= val;
			}
			return *this;
		}
		template <typename U>
		inline AMTScalarType& operator /= (const U& u)
		{
			#if __AMT_CHECK_NUMERIC_OVERFLOW__
			VerifyOverflow_Div<T,U,T>(m_val, u);
			#endif
			if (this != (void*)&u)
			{ 				
				#if __AMT_CHECK_MULTITHREADED_ISSUES__
				CRegisterWritingThread r1(*this);
				//typename AMTScalarType<U>::CRegisterReadingThread r2(u); // TODO
				#endif
				m_val /= u;
			}
			else
			{
				auto val = u; // make a copy now to avoid raising issue (multithreaded access conflict false positive)				
				#if __AMT_CHECK_MULTITHREADED_ISSUES__
				CRegisterWritingThread r1(*this);
				//typename AMTScalarType<U>::CRegisterReadingThread r2(u); // TODO
				#endif
				m_val /= val;
			}
			return *this;
		}
		template<typename U>
		inline AMTScalarType& operator %= (const U& u)
		{
			#if __AMT_CHECK_NUMERIC_OVERFLOW__
			AMT_CASSERT(u != 0);
			#endif
			if (this != (void*)&u)
			{ 				
				#if __AMT_CHECK_MULTITHREADED_ISSUES__
				CRegisterWritingThread r1(*this);
				//typename AMTScalarType<U>::CRegisterReadingThread r2(u); // TODO
				#endif
				m_val %= u;
			}
			else
			{
				auto val = u; // make a copy now to avoid raising issue (multithreaded access conflict false positive)				
				#if __AMT_CHECK_MULTITHREADED_ISSUES__
				CRegisterWritingThread r1(*this);
				//typename AMTScalarType<U>::CRegisterReadingThread r2(u); // TODO
				#endif
				m_val %= val;
			}
			return *this;
		}
		inline AMTScalarType& operator |= (const T& t)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			#endif
			m_val |= t;
			return *this;
		}
		inline AMTScalarType& operator &= (const T& t)
		{			
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r1(*this);
			#endif
			m_val &= t;
			return *this;
		}
		template<typename U>
		inline AMTScalarType& operator <<= (U u)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r1(*this);
			#endif
			m_val <<= u;
			return *this;
		}
		template<typename U>
		inline AMTScalarType& operator >>= (U u)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r1(*this);
			#endif
			m_val >>= u;
			return *this;
		}
		template<typename U>
		inline AMTScalarType& operator ^= (U u)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r1(*this);
			#endif
			m_val ^= u;
			return *this;
		}


		inline operator T() const volatile
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(*this);
			#endif
			return m_val;
		}

		template<typename Tt>
		class AMTScalarTypePtr
		{
			AMTScalarType<Tt>* m_ptr;

		public:
			inline AMTScalarTypePtr(AMTScalarType<Tt>* ptr) : m_ptr(ptr)
			{
			}
			template<typename U, class = typename std::enable_if<std::is_arithmetic<U>::value>::type>
			inline operator U* () const
			{
				return (U*)&(m_ptr->m_val);
			}
			inline operator AMTScalarType<Tt>*() const
			{
				return m_ptr;
			}
		};

		inline AMTScalarTypePtr<T> operator &() const
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(*this);
			#endif
			AMTScalarTypePtr<T> ptr((AMTScalarType<T>*)this);
			return ptr;
		}

		inline volatile AMTScalarType& operator ++ () volatile
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			#endif
			#if __AMT_CHECK_NUMERIC_OVERFLOW__
			AMT_CASSERT(m_val != (std::numeric_limits<T>::max)());
			#endif
			++ m_val;
			return *this;
		}
		inline volatile AMTScalarType& operator -- () volatile
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			#endif
			#if __AMT_CHECK_NUMERIC_OVERFLOW__
			AMT_CASSERT(m_val != (std::numeric_limits<T>::min)());
			#endif
			--m_val;
			return *this;
		}
		inline AMTScalarType operator ++ (int) volatile
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			#endif
			#if __AMT_CHECK_NUMERIC_OVERFLOW__
			AMT_CASSERT(m_val != (std::numeric_limits<T>::max)());
			#endif
			AMTScalarType ret(m_val);
			++m_val;
			return ret;
		}
		inline AMTScalarType operator -- (int) volatile
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			#endif
			#if __AMT_CHECK_NUMERIC_OVERFLOW__
			AMT_CASSERT(m_val != (std::numeric_limits<T>::min)());
			#endif
			AMTScalarType ret(m_val);
			--m_val;
			return ret;
		}

		// Operators only for overflow control:
		//#if __AMT_CHECK_NUMERIC_OVERFLOW__

		// Addition:
		template<typename U, class = typename std::enable_if<std::is_arithmetic<U>::value>::type>
		inline auto operator + (const AMTScalarType<U>& var2) const
		#if defined(_MSC_VER) && _MSVC_LANG < 201402L
			-> AMTScalarType<AMTL_RESULTANT_TYPE(T, U)>
		#endif
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r1(*this);
			typename AMTScalarType<U>::CRegisterReadingThread r2(var2);
			#endif

			typedef AMTScalarType<AMTL_RESULTANT_TYPE(T, U)> ResType;

			#if __AMT_CHECK_NUMERIC_OVERFLOW__
			VerifyOverflow_Add<T, U, AMTL_RESULTANT_TYPE(T,U)>(m_val, var2.m_val);
			#endif

			ResType ret(m_val + var2.m_val);
			return ret;
		}
		template<typename U, class = typename std::enable_if<std::is_arithmetic<U>::value>::type>
		inline auto operator + (U u) const
		#if defined(_MSC_VER) && _MSVC_LANG < 201402L
			-> AMTScalarType<AMTL_RESULTANT_TYPE(T, U)>
		#endif
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r1(*this);
			#endif

			typedef AMTScalarType< AMTL_RESULTANT_TYPE(T, U)> ResType;

			#if __AMT_CHECK_NUMERIC_OVERFLOW__
			VerifyOverflow_Add<T, U, AMTL_RESULTANT_TYPE(T, U)>(m_val, u);
			#endif

			ResType	ret(m_val + u);
			return ret;
		}
		template<typename U, class = typename std::enable_if<std::is_arithmetic<U>::value>::type>
		inline friend auto operator + (U u, const AMTScalarType& var2) 
		#if defined(_MSC_VER) && _MSVC_LANG < 201402L
			-> AMTScalarType<AMTL_RESULTANT_TYPE(U, T)>
		#endif
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(var2);
			#endif
			
			typedef AMTScalarType<AMTL_RESULTANT_TYPE(U, T)> ResType;

			#if __AMT_CHECK_NUMERIC_OVERFLOW__
			VerifyOverflow_Add<U, T, AMTL_RESULTANT_TYPE(U, T)>(u, var2.m_val);
			#endif

			ResType ret(u + var2.m_val);
			return ret;
		}

		// Subtraction:
		template<typename U, class = typename std::enable_if<std::is_arithmetic<U>::value>::type>
		inline auto operator - (const AMTScalarType<U>& var2) const
		#if defined(_MSC_VER) && _MSVC_LANG < 201402L
			-> AMTScalarType<AMTL_RESULTANT_TYPE(T, U)>
		#endif
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r1(*this);
			typename AMTScalarType<U>::CRegisterReadingThread r2(var2);
			#endif

			typedef AMTScalarType<AMTL_RESULTANT_TYPE(T, U)> ResType;

			#if __AMT_CHECK_NUMERIC_OVERFLOW__
			VerifyOverflow_Subtract<T,U, AMTL_RESULTANT_TYPE(T, U)>(m_val, var2.m_val);
			#endif

			ResType ret(m_val - var2.m_val);
			return ret;
		}
		template<typename U, class = typename std::enable_if<std::is_arithmetic<U>::value>::type>
		inline auto operator - (U u) const
		#if defined(_MSC_VER) && _MSVC_LANG < 201402L
			-> AMTScalarType<AMTL_RESULTANT_TYPE(T, U)>
		#endif
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r1(*this);
			#endif

			typedef AMTScalarType<AMTL_RESULTANT_TYPE(T, U)> ResType;

			#if __AMT_CHECK_NUMERIC_OVERFLOW__
			VerifyOverflow_Subtract<T,U, AMTL_RESULTANT_TYPE(T, U)>(m_val, u);
			#endif

			ResType res = m_val - u;
			return res;
		}
		template<typename U, class = typename std::enable_if<std::is_arithmetic<U>::value>::type>
		inline friend auto operator - (U u, const AMTScalarType<T>& var2) 
		#if defined(_MSC_VER) && _MSVC_LANG < 201402L
			-> AMTScalarType<AMTL_RESULTANT_TYPE(T, U)>
		#endif
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(var2);
			#endif

			typedef AMTScalarType<AMTL_RESULTANT_TYPE(T, U)> ResType;

			#if __AMT_CHECK_NUMERIC_OVERFLOW__
			VerifyOverflow_Subtract<U,T, AMTL_RESULTANT_TYPE(T, U)>(u, var2.m_val);
			#endif

			ResType res = u - var2.m_val;
			return res;
		}

		// Multiplication:
		template<typename U, class = typename std::enable_if<std::is_arithmetic<U>::value>::type>
		inline auto operator * (const AMTScalarType<U>& var2) const
		#if defined(_MSC_VER) && _MSVC_LANG < 201402L
			-> AMTScalarType<AMTL_RESULTANT_TYPE(T, U)>
		#endif
		{
			// TODO: if (this == (void*) &var2)
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r1(*this);
			typename AMTScalarType<U>::CRegisterReadingThread r2(var2);
			#endif

			typedef AMTScalarType<AMTL_RESULTANT_TYPE(T, U)> ResType;

			#if __AMT_CHECK_NUMERIC_OVERFLOW__
			VerifyOverflow_Mul<T, U, AMTL_RESULTANT_TYPE(T, U)>(m_val, var2.m_val);
			#endif

			ResType ret(m_val * var2.m_val);
			return ret;
		}
		template<typename U, class = typename std::enable_if<std::is_arithmetic<U>::value>::type>
		inline auto operator * (U u) const
		#if defined(_MSC_VER) && _MSVC_LANG < 201402L
			-> AMTScalarType<AMTL_RESULTANT_TYPE(T, U)>
		#endif
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r1(*this);
			#endif

			typedef AMTScalarType<AMTL_RESULTANT_TYPE(T, U)> ResType;

			#if __AMT_CHECK_NUMERIC_OVERFLOW__
			VerifyOverflow_Mul<T, U, AMTL_RESULTANT_TYPE(T, U)>(m_val, u);
			#endif

			ResType ret(m_val * u);
			return ret;
		}
		template<typename U, class = typename std::enable_if<std::is_arithmetic<U>::value>::type>
		inline friend auto operator * (U u, const AMTScalarType<T>& var2) 
		#if defined(_MSC_VER) && _MSVC_LANG < 201402L
			-> AMTScalarType<AMTL_RESULTANT_TYPE(T, U)>
		#endif
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(var2);
			#endif

			typedef AMTScalarType<AMTL_RESULTANT_TYPE(T, U)> ResType;

			#if __AMT_CHECK_NUMERIC_OVERFLOW__
			VerifyOverflow_Mul<U, T, AMTL_RESULTANT_TYPE(U, T)>(u, var2.m_val);
			#endif

			ResType ret(u * var2.m_val);
			return ret;
		}

		// Division:
		template<typename U, class = typename std::enable_if<std::is_arithmetic<U>::value>::type>
		inline auto operator / (const AMTScalarType<U>& var2) const
		#if defined(_MSC_VER) && _MSVC_LANG < 201402L
			-> AMTScalarType<AMTL_RESULTANT_TYPE(T, U)>
		#endif
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r1(*this);
			typename AMTScalarType<U>::CRegisterReadingThread r2(var2);
			#endif

			typedef AMTScalarType<AMTL_RESULTANT_TYPE(T, U)> ResType;

			#if __AMT_CHECK_NUMERIC_OVERFLOW__
			VerifyOverflow_Div<T, U, AMTL_RESULTANT_TYPE(T, U)>(m_val, var2.m_val);
			#endif

			ResType ret(m_val / var2.m_val);
			return ret;
		}
		template<typename U, class = typename std::enable_if<std::is_arithmetic<U>::value>::type>
		inline auto operator / (U u) const
		#if defined(_MSC_VER) && _MSVC_LANG < 201402L
			-> AMTScalarType<AMTL_RESULTANT_TYPE(T, U)>
		#endif
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r1(*this);
			#endif

			typedef AMTScalarType<AMTL_RESULTANT_TYPE(T, U)> ResType;

			#if __AMT_CHECK_NUMERIC_OVERFLOW__
			VerifyOverflow_Div<T, U, AMTL_RESULTANT_TYPE(T, U)>(m_val, u);
			#endif

			ResType	ret(m_val / u);
			return ret;
		}
		template<typename U, class = typename std::enable_if<std::is_arithmetic<U>::value>::type>
		inline friend auto operator / (U u, const AMTScalarType<T>& var2) 
		#if defined(_MSC_VER) && _MSVC_LANG < 201402L
			-> AMTScalarType<AMTL_RESULTANT_TYPE(U, T)>
		#endif
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(var2);
			#endif

			typedef AMTScalarType<AMTL_RESULTANT_TYPE(U,T)> ResType;

			#if __AMT_CHECK_NUMERIC_OVERFLOW__
			VerifyOverflow_Div<U, T, AMTL_RESULTANT_TYPE(U,T)>(u, var2.m_val);
			#endif

			ResType ret(u / var2.m_val);
			return ret;
		}

		// Modulo division:
		template<typename U, class = typename std::enable_if<std::is_arithmetic<U>::value>::type>
		inline friend AMTScalarType<T> operator % (const AMTScalarType<T>& var1, U u) 
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r1(var1);
			#endif
			#if __AMT_CHECK_NUMERIC_OVERFLOW__
			AMT_CASSERT(u != 0);
			#endif
			AMTScalarType<T> ret(var1.m_val % u);
			return ret;
		}
		template<typename U, class = typename std::enable_if<std::is_arithmetic<U>::value>::type>
		inline friend AMTScalarType<T> operator % (U u, const AMTScalarType<T>& var2) 
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(var2);
			#endif
			#if __AMT_CHECK_NUMERIC_OVERFLOW__
			AMT_CASSERT(var2.m_val != 0);
			#endif
			AMTScalarType<T> ret(u % var2.m_val);
			return ret;
		}

		private:
			template<typename U, class = typename std::enable_if<std::is_arithmetic<U>::value>::type>
			void CheckAssignmentOverflow(U u)
			{
				if __AMT_IF_CONSTEXPR__(!std::is_same<U, bool>::value)
				{
					if __AMT_IF_CONSTEXPR__(std::is_floating_point<U>::value)
						if __AMT_IF_CONSTEXPR__(std::is_floating_point<T>::value)
						{
							if (sizeof(T) < sizeof(U))
							{
								AMT_CASSERT(u >= -(std::numeric_limits<T>::max)() && u <= (std::numeric_limits<T>::max)());
								AMT_CASSERT(u >= -std::numeric_limits<T>::epsilon() && u <= std::numeric_limits<T>::epsilon()); // otherwise we get significant loss on value quality
							}
						}
						else
						{
							AMT_CASSERT(u >= (std::numeric_limits<T>::min)() && u <= (std::numeric_limits<T>::max)());
						}
					else
						if __AMT_IF_CONSTEXPR__(!std::is_floating_point<T>::value)
						{
							if __AMT_IF_CONSTEXPR__(std::is_unsigned<T>::value == std::is_unsigned<U>::value)
							{
								if __AMT_IF_CONSTEXPR__(sizeof(T) < sizeof(U))
								{
									AMT_CASSERT(u >= (std::numeric_limits<T>::min)() && u <= (std::numeric_limits<T>::max)());
								}
							}
							else
								if __AMT_IF_CONSTEXPR__(std::is_unsigned<U>::value)
								{
									AMT_CASSERT(u <= (std::numeric_limits<T>::max)());
								}
								else
									if __AMT_IF_CONSTEXPR__(std::is_unsigned<T>::value)
									{
										AMT_CASSERT(u >= 0 && ((typename std::make_unsigned<U>::type) u) <= (std::numeric_limits<T>::max)());
									}
									else
										AMT_CASSERT(u >= (std::numeric_limits<T>::min)() && u <= (std::numeric_limits<T>::max)());
						}
				}
			}

	};
	

	template<typename T>
	class AMTPointerType 
	{
		template<typename U>
		friend class AMTPointerType;

		typedef typename std::remove_pointer<T>::type TypePointedTo;
		static_assert(std::is_pointer<T>::value, "Template parameter of AMTPointerType has to be a pointer type");

	private:
		T m_val;

		#if !__AMT_FORCE_SAME_SIZE_FOR_TRIVIAL_TYPES__
		mutable std::atomic<AMTCounterType> m_nPendingReadRequests;
		mutable std::atomic<AMTCounterType> m_nPendingWriteRequests;
		#endif

		__AMT_FORCEINLINE__ void Init()
		{
			#if __AMT_FORCE_SAME_SIZE_FOR_TRIVIAL_TYPES__
			static_assert(sizeof(AMTPointerType) == sizeof(T), "sizeof(AMTPointerType) is expected to be the same as sizeof the underlying type");
			AMTCountersHashMap* pHashMap = AMTCountersHashMap::GetCounterHashMap();
			pHashMap->RegisterAddress((void*) this);
			#else
			m_nPendingReadRequests = 0;
			m_nPendingWriteRequests = 0;
			#endif
		}
		__AMT_FORCEINLINE__ void Uninit()
		{
			#if __AMT_FORCE_SAME_SIZE_FOR_TRIVIAL_TYPES__
			AMTCountersHashMap* pHashMap = AMTCountersHashMap::GetCounterHashMap();
			auto pSlot = pHashMap->GetReadWriteCounters((void*) this);
			AMT_VERIFY_SLOT(pSlot);
			AMT_CASSERT(pSlot->m_nPendingWriteRequests == 0);
			AMT_CASSERT(pSlot->m_nPendingReadRequests == 0);
			AMT_CASSERT(pSlot->m_nSlotUsed == 1);
			pHashMap->UnregisterAddress(this);
			#else
			AMT_CASSERT(m_nPendingWriteRequests == 0);
			AMT_CASSERT(m_nPendingReadRequests == 0);
			#endif
		}
		__AMT_FORCEINLINE__ void RegisterReadingThread() const
		{
			#if __AMT_FORCE_SAME_SIZE_FOR_TRIVIAL_TYPES__
			AMTCountersHashMap* pHashMap = AMTCountersHashMap::GetCounterHashMap();
			auto pSlot = pHashMap->GetReadWriteCounters((void*) this);
			AMT_VERIFY_SLOT(pSlot);
			++pSlot->m_nPendingReadRequests;
			AMT_CASSERT(pSlot->m_nPendingWriteRequests == 0);
			AMT_CASSERT(pSlot->m_nSlotUsed == 1);
			#else
			++m_nPendingReadRequests;
			AMT_CASSERT(m_nPendingWriteRequests == 0);
			#endif
		}
		__AMT_FORCEINLINE__ void UnregisterReadingThread() const
		{
			#if __AMT_FORCE_SAME_SIZE_FOR_TRIVIAL_TYPES__
			AMTCountersHashMap* pHashMap = AMTCountersHashMap::GetCounterHashMap();
			auto pSlot = pHashMap->GetReadWriteCounters((void*) this);
			AMT_VERIFY_SLOT(pSlot);
			AMT_CASSERT(pSlot->m_nPendingWriteRequests == 0);
			AMT_CASSERT(pSlot->m_nSlotUsed == 1);
			--pSlot->m_nPendingReadRequests;
			#else
			AMT_CASSERT(m_nPendingWriteRequests == 0);
			--m_nPendingReadRequests;
			#endif
		}
		__AMT_FORCEINLINE__ void RegisterWritingThread() const
		{
			#if __AMT_FORCE_SAME_SIZE_FOR_TRIVIAL_TYPES__
			AMTCountersHashMap* pHashMap = AMTCountersHashMap::GetCounterHashMap();
			auto pSlot = pHashMap->GetReadWriteCounters((void*) this);
			AMT_VERIFY_SLOT(pSlot);
			++pSlot->m_nPendingWriteRequests;
			AMT_CASSERT(pSlot->m_nPendingWriteRequests == 1);
			AMT_CASSERT(pSlot->m_nPendingReadRequests == 0);
			AMT_CASSERT(pSlot->m_nSlotUsed == 1);
			#else
			++m_nPendingWriteRequests;
			AMT_CASSERT(m_nPendingWriteRequests == 1);
			AMT_CASSERT(m_nPendingReadRequests == 0);
			#endif
		}
		__AMT_FORCEINLINE__ void UnregisterWritingThread() const
		{
			#if __AMT_FORCE_SAME_SIZE_FOR_TRIVIAL_TYPES__
			AMTCountersHashMap* pHashMap = AMTCountersHashMap::GetCounterHashMap();
			auto pSlot = pHashMap->GetReadWriteCounters((void*) this);
			AMT_VERIFY_SLOT(pSlot);
			AMT_CASSERT(pSlot->m_nPendingWriteRequests == 1);
			AMT_CASSERT(pSlot->m_nPendingReadRequests == 0);
			AMT_CASSERT(pSlot->m_nSlotUsed == 1);
			--pSlot->m_nPendingWriteRequests;
			#else
			AMT_CASSERT(m_nPendingWriteRequests == 1);
			AMT_CASSERT(m_nPendingReadRequests == 0);
			--m_nPendingWriteRequests;
			#endif
		}

		class CRegisterReadingThread
		{
			const AMTPointerType& m_var;

		public:
			inline CRegisterReadingThread(const AMTPointerType& var) : m_var(var)
			{
				m_var.RegisterReadingThread();
			}
			inline CRegisterReadingThread(const volatile AMTPointerType& var) : m_var((const AMTPointerType&)var)
			{
				m_var.RegisterReadingThread();
			}
			inline ~CRegisterReadingThread() __AMT_CAN_THROW__
			{
				m_var.UnregisterReadingThread();
			}
		};

		class CRegisterWritingThread
		{
			const AMTPointerType& m_var;

		public:
			inline CRegisterWritingThread(const AMTPointerType& var) : m_var(var)
			{
				m_var.RegisterWritingThread();
			}
			inline CRegisterWritingThread(const volatile AMTPointerType& var) : m_var((const AMTPointerType&)var)
			{
				m_var.RegisterWritingThread();
			}
			inline ~CRegisterWritingThread() __AMT_CAN_THROW__
			{
				m_var.UnregisterWritingThread();
			}
		};


	public:
		//inline AMTPointerType(T t) : AMTScalarType(t)
		//{
		//}
		inline T operator ->()
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(*this);
			#endif
			return m_val;
		}
		inline const T operator ->() const
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(*this);
			#endif
			return m_val;
		}		
		inline TypePointedTo& operator *()
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(*this);
			#endif
			return *m_val;
		}
		inline const TypePointedTo& operator *() const
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(*this);
			#endif
			return *m_val;
		}
		template<typename U, class = typename std::enable_if<std::is_integral<U>::value>::type>
		inline TypePointedTo& operator [](U n)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(*this);
			#endif
			return m_val[n];
		}
		template<typename U, class = typename std::enable_if<std::is_integral<U>::value>::type>
		inline const TypePointedTo& operator [](U n) const
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(*this);
			#endif
			return m_val[n];
		}
		inline AMTPointerType()
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			Init();
			#endif
			#if __ATM_INITIALIZE_AMTL_VARIABLES__
			m_val = 0; // breaks standard behaviour (no initialization by default) but lets initialize correctly in the following context: ptr = T(), where T is a pointer type 
			#endif			
		}
		inline AMTPointerType(const AMTPointerType& o)
		{
			try {
				#if __AMT_CHECK_MULTITHREADED_ISSUES__
				Init();
				CRegisterWritingThread r(*this);
				CRegisterReadingThread r2(o);
				#endif
				m_val = o.m_val;
			}
			catch(...) {
				#if __AMT_CHECK_MULTITHREADED_ISSUES__
				Uninit();				
				#endif
				throw;
			}			
		}		
		template<typename U, class = typename std::enable_if<std::is_pointer<U>::value>::type>
		inline AMTPointerType(const AMTPointerType<U>& o)
		{
			try {
				#if __AMT_CHECK_MULTITHREADED_ISSUES__
				Init();
				CRegisterWritingThread r(*this);
				typename AMTPointerType<U>::CRegisterReadingThread r2(o);
				#endif
				m_val = (T) o.m_val;
			}
			catch(...) {
				#if __AMT_CHECK_MULTITHREADED_ISSUES__
				Uninit();				
				#endif
				throw;
			}			
		}
		inline AMTPointerType(T t)
		{
			try {
				#if __AMT_CHECK_MULTITHREADED_ISSUES__
				Init();
				CRegisterWritingThread r(*this);
				#endif
				m_val = t;
			}
			catch(...) {
				#if __AMT_CHECK_MULTITHREADED_ISSUES__
				Uninit();				
				#endif
				throw;
			}			
		}
		inline ~AMTPointerType() __AMT_CAN_THROW__
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			Uninit();
			#endif
		}

		inline AMTPointerType& operator = (const T t)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			#endif
			m_val = t;
			return *this;
		}
		inline volatile AMTPointerType& operator = (const T t) volatile
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			#endif
			m_val = t;
			return *this;
		}
		// Added 2023-04-13... somewhow didn't compile with __AMT_FORCE_SAME_SIZE_FOR_TRIVIAL_TYPES__ 0...
		inline AMTPointerType& operator = (const AMTPointerType<T>& var)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			CRegisterReadingThread r2(var);
			#endif
			m_val = (T) var.m_val;
			return *this;
		}		
		template<typename U, class = typename std::enable_if<std::is_pointer<U>::value>::type>
		inline AMTPointerType& operator = (const AMTPointerType<U>& var)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			typename AMTPointerType<U>::CRegisterReadingThread r2(var);
			#endif
			m_val = (T) var.m_val;
			return *this;
		}		
		template<typename U, class = typename std::enable_if<std::is_pointer<U>::value>::type>
		inline volatile AMTPointerType& operator = (const AMTPointerType<U>& var) volatile
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			typename AMTPointerType<U>::CRegisterReadingThread r2(var);
			#endif
			m_val = (T) var.m_val;
			return *this;
		}
		inline AMTPointerType& operator |= (const AMTPointerType& var)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			CRegisterReadingThread r2(var);
			#endif
			m_val |= var.m_val;
			return *this;
		}
		inline AMTPointerType& operator &= (const AMTPointerType& var)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			CRegisterReadingThread r2(var);
			#endif
			m_val &= var.m_val;
			return *this;
		}
		inline operator T&()
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(*this);
			#endif
			return m_val;
		}
		inline operator const T() const volatile
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(*this);
			#endif
			return m_val;
		}
		inline T* operator &()
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(*this);
			#endif
			return &m_val;
		}
		inline AMTPointerType& operator ++ ()
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			#endif
			++m_val;
			return *this;
		}
		inline AMTPointerType& operator -- ()
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			#endif
			--m_val;
			return *this;
		}
		inline AMTPointerType operator ++ (int) // postfix
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			#endif
			AMTPointerType ret(m_val);
			++m_val;
			return ret;
		}
		inline AMTPointerType operator -- (int) // postfix
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			#endif
			AMTPointerType ret(m_val);
			--m_val;
			return ret;
		}

		template<typename U, class = typename std::enable_if<std::is_integral<U>::value>::type>
		inline AMTPointerType operator + (U n) const
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(*this);
			#endif
			AMTPointerType res(m_val + n);
			return res;
		}
		template<typename U, class = typename std::enable_if<std::is_integral<U>::value>::type>
		inline AMTPointerType operator - (U n) const
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(*this);
			#endif
			AMTPointerType res(m_val - n);
			return res;
		}

	};
	
	using _char = AMTScalarType<char>;
	using _wchar = AMTScalarType<wchar_t>;
	using int8_t = AMTScalarType<std::int8_t>;
	using uint8_t = AMTScalarType<std::uint8_t>;
	using int16_t = AMTScalarType<std::int16_t>;
	using uint16_t = AMTScalarType<std::uint16_t>;
	using int32_t = AMTScalarType<std::int32_t>;
	using uint32_t = AMTScalarType<std::uint32_t>;
	using int64_t = AMTScalarType<std::int64_t>;
	using uint64_t = AMTScalarType<std::uint64_t>;

	using _float = AMTScalarType<float>;
	using _double = AMTScalarType<double>;

	template<typename T>
	using raw_ptr = AMTPointerType<T*>;
	
	// Auxiliary structs to make code compile before C++17 (without if constexpr):
	template<bool IS_SCALAR>
	struct AmtCastToSigned
	{
		template<typename T>
		static inline typename std::make_signed<T>::type Cast(AMTScalarType<T> x)
		{
			return x.MakeSigned();
		}
	};
	template<>
	struct AmtCastToSigned<true>
	{
		template<typename T = int>
		static inline typename std::make_signed<T>::type Cast(T x)
		{
			AMT_CASSERT(false);
			return 0; // not used actually, just a syntactic trick
		}
	};
	template<bool IS_SCALAR>
	struct AmtNumToSigned
	{
		template<typename T> 
		static inline int Cast(T& x)
		{
			AMT_CASSERT(false);
			return 0; // not used actually, just a syntactic trick
		}
	};
	template<>
	struct AmtNumToSigned<true>
	{
		template<typename T>
		static inline typename std::make_signed<typename std::conditional<std::is_same<T, bool>::value, std::uint8_t, T>::type>::type Cast(T x)
		{
			return x;
		}
	};

	#define AMT_NUM_TO_SIGNED(x) ((std::make_signed<std::conditional<std::is_same<decltype(x), bool>::value, std::uint8_t, decltype(x)>::type>::type)x) 		
	#define AMT_NUM_TO_UNSIGNED(x) ((std::make_unsigned<std::conditional<std::is_same<decltype(x), bool>::value, std::int8_t, decltype(x)>::type>::type)x) 		
	#define AMT_CAST_TO_SIGNED(x) (std::is_scalar<decltype(x)>::value ? amt::AmtNumToSigned<std::is_scalar<decltype(x)>::value>::Cast(x) : amt::AmtCastToSigned<std::is_scalar<decltype(x)>::value>::Cast(x))
	// to-do: #define AMT_CAST_TO_UNSIGNED(x) 

	#else // #if defined(__AMTL_ASSERTS_ARE_ON__)

	template<typename T>
	using AMTScalarType = T;

	template<typename T>
	using AMTPointerType = T;

	using _char = char;
	using _wchar = wchar_t;
	using int8_t = std::int8_t;
	using uint8_t = std::uint8_t;
	using int16_t = std::int16_t;
	using uint16_t = std::uint16_t;
	using int32_t = std::int32_t;
	using uint32_t = std::uint32_t;
	using int64_t = std::int64_t;
	using uint64_t = std::uint64_t;

	using _float = float;
	using _double = double;

	template<typename T>
	using raw_ptr = T*;

	#define AMT_CAST_TO_SIGNED(x) x
	#define AMT_CAST_TO_UNSIGNED(x) x

	#endif

	// ---------------
	// Set of types that guarantee the same size as their original equivalents from std, regardless of current value of setting __AMT_FORCE_SAME_SIZE_FOR_TRIVIAL_TYPES__ 
	#if defined(__AMTL_ASSERTS_ARE_ON__) && __AMT_FORCE_SAME_SIZE_FOR_TRIVIAL_TYPES__ != 0
	using _CHAR = amt::_char;
	using INT8_t = amt::int8_t;
	using INT16_t = amt::int16_t;
	using INT32_t = amt::int32_t;
	using INT64_t = amt::int64_t;
	using UINT8_t = amt::uint8_t;
	using UINT16_t = amt::uint16_t;
	using UINT32_t = amt::uint32_t;
	using UINT64_t = amt::uint64_t;
	using _FLOAT = amt::_float;
	using _DOUBLE = amt::_double;
	template<typename T>
	using RAW_PTR = AMTPointerType<T*>;
	// ...
	#else

	using _CHAR = char;
	using INT8_t = std::int8_t;
	using INT16_t = std::int16_t;
	using INT32_t = std::int32_t;
	using INT64_t = std::int64_t;
	using UINT8_t = std::uint8_t;
	using UINT16_t = std::uint16_t;
	using UINT32_t = std::uint32_t;
	using UINT64_t = std::uint64_t;

	using _FLOAT = float;
	using _DOUBLE = double;

	template<typename T>
	using RAW_PTR = T*;
	//...
	#endif
} // namespace amt

#if defined(__AMTL_ASSERTS_ARE_ON__)
namespace std {
	template<typename T> 
	class numeric_limits<amt::AMTScalarType<T>> : public std::numeric_limits<T>{};
}
#endif
