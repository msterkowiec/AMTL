//
// Assertive MultiThreading Library
//
//  Copyright Marcin Sterkowiec, Piotr Tracz, 2021. Use, modification and
//  distribution is subject to license (see accompanying file license.txt)
//

#pragma once

#include <cmath>
#include <cstdint>
#include <limits>
#include <type_traits>
#include "amt_cassert.h"
#include "amt_types.h"

#if defined(_DEBUG) || defined(__AMT_RELEASE_WITH_ASSERTS__)
#if __AMT_FORCE_SAME_SIZE_FOR_TRIVIAL_TYPES__
#include "amtinternal_hashmap.h"
#endif
#endif

namespace amt
{
	#if defined(_DEBUG) || defined(__AMT_RELEASE_WITH_ASSERTS__)

	// This is really tempting to move read/write counters to some global hash_map and enjoy having sizeof(amt::uint8_t)==1 instead of 3
	// This is the case of __AMT_FORCE_SAME_SIZE_FOR_TRIVIAL_TYPES__ - the code will work with all the memcpy/memmove and persistency but will be significantly slower

	template<typename T> 
	class AMTScalarType
	{
		static_assert(std::is_scalar<T>::value, "Template parameter of AMTScalarType has to be a scalar type");
		typedef typename std::make_unsigned<T>::type unsigned_T;
		typedef typename std::make_signed<T>::type signed_T;
		typedef typename std::conditional<std::is_signed<T>::value, typename std::make_unsigned<T>::type, typename std::make_signed<T>::type>::type opposite_signedness_T;

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
			inline ~CRegisterReadingThread()
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
			inline ~CRegisterWritingThread()
			{
				m_var.UnregisterWritingThread();
			}
		};


	public:
		inline AMTScalarType()
		{
			Init();
			// No m_val initialization to let program behave in a standard way
		}
		inline AMTScalarType(const AMTScalarType& o)
		{
			Init();
			CRegisterWritingThread r(*this);
			CRegisterReadingThread r2(o);
			m_val = o.m_val;
		}
		/*inline AMTScalarType(const AMTScalarType<opposite_signedness_T>& o)
		{
			Init();
			CRegisterWritingThread r(*this);
			CRegisterReadingThread r2(o);
			// to-do: check if conversion to type with opposite signedness is ok!!
			m_val = o.m_val;
		}*/
		inline AMTScalarType(T t)
		{
			Init();
			CRegisterWritingThread r(*this);
			m_val = t;
		}
		/*inline AMTScalarType(opposite_signedness_T t)
		{
			Init();
			CRegisterWritingThread r(*this);
			// to-do: check if conversion to type with opposite signedness is ok!!
			m_val = t;
		}*/
		inline ~AMTScalarType()
		{
			Uninit();
		}
		#if !__AMT_CHECK_NUMERIC_OVERFLOW__
		inline operator bool() const
		{
			CRegisterReadingThread r(*this);
			return m_val != 0;
		}
		inline operator bool() const volatile
		{
			CRegisterReadingThread r(*this);
			return m_val != 0;
		}
		#endif
		inline AMTScalarType& operator = (const T t)
		{
			CRegisterWritingThread r(*this);
			m_val = t;
			return *this;
		}
		inline volatile AMTScalarType& operator = (const T t) volatile
		{
			CRegisterWritingThread r(*this);
			m_val = t;
			return *this;
		}
		inline AMTScalarType& operator = (const AMTScalarType& var)
		{
			CRegisterWritingThread r(*this);
			CRegisterReadingThread r2(var);
			m_val = var.m_val;
			return *this;
		}
		inline volatile AMTScalarType& operator = (const AMTScalarType& var) volatile
		{
			CRegisterWritingThread r(*this);
			CRegisterReadingThread r2(var);
			m_val = var.m_val;
			return *this;
		}
		/*inline AMTScalarType& operator |= (const AMTScalarType& var)
		{
			CRegisterWritingThread r(*this);
			CRegisterReadingThread r2(var);
			m_val |= var.m_val;
			return *this;
		}
		inline AMTScalarType& operator &= (const AMTScalarType& var)
		{
			CRegisterWritingThread r(*this);
			CRegisterReadingThread r2(var);
			m_val &= var.m_val;
			return *this;
		}*/
		/*inline operator T()
		{
			CRegisterReadingThread r(*this);
			return m_val;
		}*/
		unsigned_T MakeUnsigned() const
		{
			CRegisterReadingThread r(*this);
			return (unsigned_T)m_val;
		}
		signed_T MakeSigned() const
		{
			CRegisterReadingThread r(*this);
			return (signed_T)m_val;
		}
		#if __AMT_CHECK_NUMERIC_OVERFLOW__
	private:
		template<typename U, typename V>
		__AMT_FORCEINLINE__ static void VerifyOverflow_Add(U u, V v)
		{
			if (!std::is_floating_point<U>::value)
				if (std::is_floating_point<V>::value)
				{
					//  to-do
				}
				else
					if (sizeof(U) < sizeof(std::int64_t) && sizeof(V) < sizeof(std::int64_t))
					{
						std::int64_t i64 = (std::int64_t) u;
						i64 += v;
						AMT_CASSERT(i64 <= (std::numeric_limits<U>::max)());
						AMT_CASSERT(i64 >= (std::numeric_limits<U>::min)());
					}
					else
					{
						//  to-do
					}
		}
		template<typename U, typename V>
		__AMT_FORCEINLINE__ static void VerifyOverflow_Subtract(U u, V v)
		{
			if (!std::is_floating_point<U>::value)
				if (std::is_floating_point<V>::value)
				{
					//  to-do
				}
				else
					if (sizeof(U) < sizeof(std::int64_t) && sizeof(V) < sizeof(std::int64_t))
					{
						std::int64_t i64 = (std::int64_t) u;
						i64 -= v;
						AMT_CASSERT(i64 <= (std::numeric_limits<U>::max)());
						AMT_CASSERT(i64 >= (std::numeric_limits<U>::min)());
					}
					else
					{
						//  to-do
					}
		}

		template<typename U, typename V>
		__AMT_FORCEINLINE__ static void VerifyOverflow_Mul(U u, V v)
		{
			if (!std::is_floating_point<U>::value)
				if (std::is_floating_point<V>::value)
				{
					double tmp = (double) u;
					tmp *= v;
					tmp = floor(tmp);
					AMT_CASSERT(tmp <= (std::numeric_limits<U>::max)()); // e.g. unsigned char 100 * 2.6 overflows
					AMT_CASSERT(tmp >= (std::numeric_limits<U>::min)());
				}
				else
					if (sizeof(U) < sizeof(std::int64_t) && sizeof(V) < sizeof(std::int64_t))
					{
						if (std::is_signed<U>::value)
						{
							std::int64_t i64 = (std::int64_t) u;
							i64 *= v;
							AMT_CASSERT(i64 <= (std::numeric_limits<U>::max)());
							AMT_CASSERT(i64 >= (std::numeric_limits<U>::min)());
						}
						else
						{
							std::uint64_t ui64 = (std::int64_t) u;
							ui64 *= v;
							AMT_CASSERT(ui64 <= (std::numeric_limits<U>::max)());
							AMT_CASSERT(ui64 >= (std::numeric_limits<U>::min)());
						}
					}
					else
					{
						//  to-do
					}
		}

		template<typename U, typename V>
		__AMT_FORCEINLINE__ static void VerifyOverflow_Div(U u, V v)
		{
			AMT_CASSERT(v != 0);
			
			if (!std::is_floating_point<U>::value)
			{
				if (std::is_signed<U>::value)
					if (v == -1)
						AMT_CASSERT(u != (std::numeric_limits<U>::min)()); // e.g. for char we cannot divide -128 by -1

				if (std::is_floating_point<V>::value)
				{
					double tmp = (double)u; 
					tmp /= v;
					tmp = floor(tmp);
					AMT_CASSERT(tmp <= (std::numeric_limits<U>::max)()); // e.g. unsigned char 100 / 0.3 overflows
					AMT_CASSERT(tmp >= (std::numeric_limits<U>::min)());
				}
			}
								
			if (std::is_unsigned<U>::value)
				if (std::is_signed<V>::value)
					if (v < 0)
						AMT_CASSERT(false); // to be verified!!

					
		}

	public:
		inline AMTScalarType& operator += (const T& t)
		{
			CRegisterReadingThread r1(*this);
			VerifyOverflow_Add(m_val, t);
			m_val += t;
			return *this;
		}
		inline AMTScalarType& operator -= (const T& t)
		{
			CRegisterReadingThread r1(*this);
			VerifyOverflow_Subtract(m_val, t);
			m_val -= t;
			return *this;
		}
		template <typename U>
		inline AMTScalarType& operator *= (const U& u)
		{
			CRegisterReadingThread r1(*this);
			VerifyOverflow_Mul(m_val, u);
			m_val *= u;
			return *this;
		}
		template <typename U>
		inline AMTScalarType& operator /= (const U& u)
		{
			CRegisterReadingThread r1(*this);
			VerifyOverflow_Div(m_val, u);
			m_val /= u;
			return *this;
		}		
		inline AMTScalarType& operator |= (const T& t)
		{
			CRegisterReadingThread r1(*this);
			m_val |= t;
			return *this;
		}
		inline AMTScalarType& operator &= (const T& t)
		{
			CRegisterReadingThread r1(*this);
			m_val &= t;
			return *this;
		}
		#endif
		inline operator T() const volatile
		{
			CRegisterReadingThread r(*this);
			return m_val;
		}
		/*template<class = typename std::enable_if<std::is_signed<T>::value>::type>
		operator unsigned_T() const volatile
		{
			CRegisterReadingThread r(*this);
			// to-do: add value verification here!!!
			return m_val;
		}
		template<class = typename std::enable_if<std::is_unsigned<T>::value>::type>
		operator signed_T() const volatile
		{
			CRegisterReadingThread r(*this);
			// to-do: add value verification here!!!
			return m_val;
		}*/

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
		/*inline AMTScalarType* operator &() const
		{
		CRegisterReadingThread r(*this);
		return this;
		}*/
		inline AMTScalarTypePtr<T> operator &() const
		{
			CRegisterReadingThread r(*this);
			//return this;
			AMTScalarTypePtr<T> ptr((AMTScalarType<T>*)this);
			return ptr;
		}


		inline volatile AMTScalarType& operator ++ () volatile
		{
			CRegisterWritingThread r(*this);
			#if __AMT_CHECK_NUMERIC_OVERFLOW__
			AMT_CASSERT(m_val != (std::numeric_limits<T>::max)());
			#endif
			++ m_val;
			return *this;
		}
		inline volatile AMTScalarType& operator -- () volatile
		{
			CRegisterWritingThread r(*this);
			#if __AMT_CHECK_NUMERIC_OVERFLOW__
			AMT_CASSERT(m_val != (std::numeric_limits<T>::min)());
			#endif
			--m_val;
			return *this;
		}
		inline AMTScalarType operator ++ (int) volatile
		{
			CRegisterWritingThread r(*this);
			#if __AMT_CHECK_NUMERIC_OVERFLOW__
			AMT_CASSERT(m_val != (std::numeric_limits<T>::max)());
			#endif
			AMTScalarType ret(m_val);
			++m_val;
			return ret;
		}
		inline AMTScalarType operator -- (int) volatile
		{
			CRegisterWritingThread r(*this);
			#if __AMT_CHECK_NUMERIC_OVERFLOW__
			AMT_CASSERT(m_val != (std::numeric_limits<T>::min)());
			#endif
			AMTScalarType ret(m_val);
			--m_val;
			return ret;
		}

		/*inline AMTScalarType& operator += (const AMTScalarType& var)
		{
			CRegisterWritingThread r(*this);
			CRegisterReadingThread r2(var);
			m_val += var.m_val;
			return *this;
		}*/
		/*inline AMTScalarType& operator += (T t)
		{
			CRegisterWritingThread r(*this);
			m_val += t;
			return *this;
		}*/
		/*inline AMTScalarType& operator -= (const AMTScalarType& var)
		{
			CRegisterWritingThread r(*this);
			CRegisterReadingThread r2(var);
			m_val -= var.m_val;
			return *this;
		}*/
		/*inline AMTScalarType& operator -= (T t)
		{
			CRegisterWritingThread r(*this);
			m_val -= t;
			return *this;
		}*/

		// Operators only for overflow control:
		#if __AMT_CHECK_NUMERIC_OVERFLOW__

		// Addition:
		/*inline friend AMTScalarType operator + (const AMTScalarType& var1, const AMTScalarType& var2)
		{
			CRegisterReadingThread r1(var1);
			CRegisterReadingThread r2(var2);
			VerifyOverflow_Add(var1.m_val, var2.m_val);
			AMTScalarType<T> ret(var1.m_val + var2.m_val);
			return ret;
		}*/
		template<typename U, class = typename std::enable_if<std::is_arithmetic<U>::value>::type>
		inline friend AMTScalarType operator + (const AMTScalarType& var1, U u)
		{
			CRegisterReadingThread r1(var1);
			VerifyOverflow_Add(var1.m_val, u);
			AMTScalarType<T> ret(var1.m_val + u);
			return ret;
		}
		template<typename U, class = typename std::enable_if<std::is_arithmetic<U>::value>::type>
		inline friend AMTScalarType operator + (U u, const AMTScalarType& var2)
		{
			CRegisterReadingThread r(var2);
			VerifyOverflow_Add(u, var2.m_val);
			AMTScalarType<T> ret(u + var2.m_val);
			return ret;
		}

		// Subtraction:
		/*inline friend AMTScalarType<T> operator - (const AMTScalarType<T>& var1, const AMTScalarType<T>& var2)
		{
			CRegisterReadingThread r1(var1);
			CRegisterReadingThread r2(var2);
			VerifyOverflow_Subtract(var1.m_val, var2.m_val);
			AMTScalarType<T> ret(var1.m_val - var2.m_val);
			return ret;
		}*/
		template<typename U, class = typename std::enable_if<std::is_arithmetic<U>::value>::type>
		inline friend T operator - (const AMTScalarType<T>& var1, U u)
		{
			CRegisterReadingThread r1(var1);
			VerifyOverflow_Subtract(var1.m_val, u);
			return var1.m_val - u;
		}
		template<typename U, class = typename std::enable_if<std::is_arithmetic<U>::value>::type>
		inline friend U operator - (U u, const AMTScalarType<T>& var2)
		{
			CRegisterReadingThread r(var2);
			VerifyOverflow_Subtract(u, var2.m_val);
			return u - var2.m_val;
		}

		// Multiplication:
		/*inline friend AMTScalarType<T> operator * (const AMTScalarType<T>& var1, const AMTScalarType<T>& var2)
		{
			CRegisterReadingThread r1(var1);
			CRegisterReadingThread r2(var2);
			VerifyOverflow_Mul(var1.m_val, var2.m_val);
			AMTScalarType<T> ret(var1.m_val * var2.m_val);
			return ret;
		}*/
		template<typename U, class = typename std::enable_if<std::is_arithmetic<U>::value>::type>
		inline friend AMTScalarType<T> operator * (const AMTScalarType<T>& var1, U u)
		{
			CRegisterReadingThread r1(var1);
			VerifyOverflow_Mul(var1.m_val, u);
			AMTScalarType<T> ret(var1.m_val * u);
			return ret;
		}
		template<typename U, class = typename std::enable_if<std::is_arithmetic<U>::value>::type>
		inline friend AMTScalarType<T> operator * (U u, const AMTScalarType<T>& var2)
		{
			CRegisterReadingThread r(var2);
			VerifyOverflow_Mul(u, var2.m_val);
			AMTScalarType<T> ret(u * var2.m_val);
			return ret;
		}

		// Division:
		/*inline friend AMTScalarType<T> operator / (const AMTScalarType<T>& var1, const AMTScalarType<T>& var2)
		{
			CRegisterReadingThread r1(var1);
			CRegisterReadingThread r2(var2);
			VerifyOverflow_Div(var1.m_val, var2.m_val);
			AMTScalarType<T> ret(var1.m_val / var2.m_val);
			return ret;
		}*/
		template<typename U, class = typename std::enable_if<std::is_arithmetic<U>::value>::type>
		inline friend AMTScalarType<T> operator / (const AMTScalarType<T>& var1, U u)
		{
			CRegisterReadingThread r1(var1);
			VerifyOverflow_Div(var1.m_val, u);
			AMTScalarType<T> ret(var1.m_val / u);
			return ret;
		}
		template<typename U, class = typename std::enable_if<std::is_arithmetic<U>::value>::type>
		inline friend AMTScalarType<T> operator / (U u, const AMTScalarType<T>& var2)
		{
			CRegisterReadingThread r(var2);
			VerifyOverflow_Div(u, var2.m_val);
			AMTScalarType<T> ret(u / var2.m_val);
			return ret;
		}

		/*template<typename U, class = typename std::enable_if<std::is_arithmetic<U>::value>::type>
		inline friend AMTScalarType<T> operator & (const AMTScalarType<T>& var1, U u)
		{
			CRegisterReadingThread r1(var1);
			AMTScalarType<T> ret(var1.m_val & u);
			return ret;
		}
		template<typename U, class = typename std::enable_if<std::is_arithmetic<U>::value>::type>
		inline friend AMTScalarType<T> operator | (U u, const AMTScalarType<T>& var2)
		{
			CRegisterReadingThread r(var2);
			AMTScalarType<T> ret(u | var2.m_val);
			return ret;
		}*/

		#endif
	};
	

	template<typename T>
	class AMTPointerType 
	{
		typedef typename std::remove_pointer<T>::type TypePointedTo;
		static_assert(std::is_pointer<T>::value, "Template parameter of AMTPointerType has to be a trivial type");

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
			inline ~CRegisterReadingThread()
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
			inline ~CRegisterWritingThread()
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
			CRegisterReadingThread r(*this);
			return m_val;
		}
		inline const T operator ->() const
		{
			CRegisterReadingThread r(*this);
			return m_val;
		}		
		inline TypePointedTo& operator *()
		{
			CRegisterReadingThread r(*this);
			return *m_val;
		}
		inline const TypePointedTo& operator *() const
		{
			CRegisterReadingThread r(*this);
			return *m_val;
		}

		inline AMTPointerType()
		{
			Init();
			// No m_val initialization to let program behave in a standard way
		}
		inline AMTPointerType(const AMTPointerType& o)
		{
			Init();
			CRegisterWritingThread r(*this);
			CRegisterReadingThread r2(o);
			m_val = o.m_val;
		}
		inline AMTPointerType(T t)
		{
			Init();
			CRegisterWritingThread r(*this);
			m_val = t;
		}
		inline ~AMTPointerType()
		{
			Uninit();
		}
		/*inline operator bool() const
		{
			CRegisterReadingThread r(*this);
			return m_val != nullptr;
		}
		inline operator bool() const volatile
		{
			CRegisterReadingThread r(*this);
			return m_val != nullptr;
		}*/

		inline AMTPointerType& operator = (const T t)
		{
			CRegisterWritingThread r(*this);
			m_val = t;
			return *this;
		}
		inline volatile AMTPointerType& operator = (const T t) volatile
		{
			CRegisterWritingThread r(*this);
			m_val = t;
			return *this;
		}
		inline AMTPointerType& operator = (const AMTPointerType& var)
		{
			CRegisterWritingThread r(*this);
			CRegisterReadingThread r2(var);
			m_val = var.m_val;
			return *this;
		}
		inline volatile AMTPointerType& operator = (const AMTPointerType& var) volatile
		{
			CRegisterWritingThread r(*this);
			CRegisterReadingThread r2(var);
			m_val = var.m_val;
			return *this;
		}
		inline AMTPointerType& operator |= (const AMTPointerType& var)
		{
			CRegisterWritingThread r(*this);
			CRegisterReadingThread r2(var);
			m_val |= var.m_val;
			return *this;
		}
		inline AMTPointerType& operator &= (const AMTPointerType& var)
		{
			CRegisterWritingThread r(*this);
			CRegisterReadingThread r2(var);
			m_val &= var.m_val;
			return *this;
		}
		inline operator T&()
		{
			CRegisterReadingThread r(*this);
			return m_val;
		}
		inline operator const T() const volatile
		{
			CRegisterReadingThread r(*this);
			return m_val;
		}
		template<typename U, class = typename std::enable_if<std::is_pointer<U>::value>::type>
		inline operator U()
		{
			CRegisterReadingThread r(*this);
			return (U) m_val;
		}		
		inline T* operator &()
		{
			CRegisterReadingThread r(*this);
			return &m_val;
		}
		inline AMTPointerType& operator ++ ()
		{
			CRegisterWritingThread r(*this);
			++m_val;
			return *this;
		}
		inline AMTPointerType& operator -- ()
		{
			CRegisterWritingThread r(*this);
			--m_val;
			return *this;
		}
		inline AMTPointerType operator ++ (int) // postfix
		{
			CRegisterWritingThread r(*this);
			AMTPointerType ret(m_val);
			++m_val;
			return ret;
		}
		inline AMTPointerType operator -- (int) // postfix
		{
			CRegisterWritingThread r(*this);
			AMTPointerType ret(m_val);
			--m_val;
			return ret;
		}

		/*inline AMTPointerType& operator += (size_t n)
		{
			CRegisterWritingThread r(*this);
			m_val += n;
			return *this;
		}
		inline AMTPointerType& operator -= (size_t n)
		{
			CRegisterWritingThread r(*this);
			m_val -= n;
			return *this;
		}*/
		/*inline AMTPointerType& operator += (T t)
		{
			CRegisterWritingThread r(*this);
			m_val += t;
			return *this;
		}*/
		/*inline AMTPointerType& operator -= (const AMTPointerType& var)
		{
			CRegisterWritingThread r(*this);
			CRegisterReadingThread r2(var);
			m_val -= var.m_val;
			return *this;
		}
		inline AMTPointerType& operator -= (T t)
		{
			CRegisterWritingThread r(*this);
			m_val -= t;
			return *this;
		}*/


	};
	
	using _char = AMTScalarType<char>;
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

	#else // #if defined(_DEBUG) || defined(__AMT_RELEASE_WITH_ASSERTS__)

	template<typename T>
	using AMTScalarType = T;

	template<typename T>
	using AMTPointerType = T;

	using _char = char;
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
	#if (defined(_DEBUG) || defined(__AMT_RELEASE_WITH_ASSERTS__)) && __AMT_FORCE_SAME_SIZE_FOR_TRIVIAL_TYPES__ != 0
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
}
