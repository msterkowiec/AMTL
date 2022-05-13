//
// Assertive MultiThreading Library
//
//  Copyright Marcin Sterkowiec, Piotr Tracz, 2021-2022. Use, modification and
//  distribution is subject to license (see accompanying file license.txt)
//

#pragma once

#include <vector>
#include <atomic>

#include "amt_cassert.h"
#include "amt_types.h"
#include "amt_pod.h"

namespace amt
{

#if !defined(__AMTL_ASSERTS_ARE_ON__)
	template<typename T, class Alloc = std::allocator<T>, bool PREVENT_AMT_WRAP = false>
	using vector = std::vector<T, Alloc>;
#else

	template<typename T, class Alloc = std::allocator<T>, bool PREVENT_AMT_WRAP = false>
	class vector : public std::vector<__AMT_TRY_WRAP_TYPE__(T, PREVENT_AMT_WRAP, Alloc), __AMT_CHANGE_ALLOCATOR_IF_NEEDED__(T, PREVENT_AMT_WRAP, Alloc)>
	{
		typedef __AMT_TRY_WRAP_TYPE__(T, PREVENT_AMT_WRAP, Alloc) ValueType;
		typedef __AMT_CHANGE_ALLOCATOR_IF_NEEDED__(T, PREVENT_AMT_WRAP, Alloc) AllocatorType;
		typedef std::vector<ValueType, AllocatorType> Base;

		/*  * read
			* partial read(single index)
			* partial write(e.g.push_back without reallocation)
			* write
			Reads conflict with each other, all writes conflict with each other and with full read and, additionally, write conflicts with anything else			
        */
		mutable std::atomic<AMTCounterType> m_nPendingReadRequests;
		mutable std::atomic<AMTCounterType> m_nPendingPartialReadRequests;
		mutable std::atomic<AMTCounterType> m_nPendingWriteRequests;
		mutable std::atomic<AMTCounterType> m_nPendingPartialWriteRequests;

		inline void Init()
		{
			m_nPendingReadRequests = 0;
			m_nPendingWriteRequests = 0;
			m_nPendingPartialReadRequests = 0;
			m_nPendingPartialWriteRequests = 0;

		}
		__AMT_FORCEINLINE__ void RegisterReadingThread() const
		{
			++m_nPendingReadRequests;
			AMT_CASSERT(m_nPendingWriteRequests == 0 && m_nPendingPartialWriteRequests == 0); // + ?
		}
		__AMT_FORCEINLINE__ void UnregisterReadingThread() const
		{
			AMT_CASSERT(m_nPendingWriteRequests == 0 && m_nPendingPartialWriteRequests == 0); // + ?
			--m_nPendingReadRequests;
		}
		__AMT_FORCEINLINE__ void RegisterWritingThread() const
		{
			++m_nPendingWriteRequests;
			AMT_CASSERT(m_nPendingWriteRequests == 1 && m_nPendingPartialWriteRequests == 0);
			AMT_CASSERT(m_nPendingReadRequests == 0 && m_nPendingPartialReadRequests == 0); // + ?
		}
		__AMT_FORCEINLINE__ void UnregisterWritingThread() const
		{
			AMT_CASSERT(m_nPendingWriteRequests == 1 && m_nPendingPartialWriteRequests == 0);
			AMT_CASSERT(m_nPendingReadRequests == 0 && m_nPendingPartialReadRequests == 0) ; // + ?
			--m_nPendingWriteRequests;
		}

		__AMT_FORCEINLINE__ void RegisterPartiallyReadingThread() const
		{
			++m_nPendingPartialReadRequests;
			AMT_CASSERT(m_nPendingWriteRequests == 0);
			#if __AMT_REPORT_DOUBTFUL_CASES_WITH_VECTOR__
			AMT_CASSERT(m_nPendingPartialWriteRequests == 0);
			#endif
		}
		__AMT_FORCEINLINE__ void UnregisterPartiallyReadingThread() const
		{
			AMT_CASSERT(m_nPendingWriteRequests == 0 && m_nPendingPartialWriteRequests == 0); // + ?
			#if __AMT_REPORT_DOUBTFUL_CASES_WITH_VECTOR__
			AMT_CASSERT(m_nPendingPartialWriteRequests == 0);
			#endif
			--m_nPendingPartialReadRequests;
		}
		__AMT_FORCEINLINE__ void RegisterPartiallyWritingThread() const
		{
			++m_nPendingPartialWriteRequests;
			AMT_CASSERT(m_nPendingWriteRequests == 0 && m_nPendingPartialWriteRequests == 1);
			AMT_CASSERT(m_nPendingReadRequests == 0);
			#if __AMT_REPORT_DOUBTFUL_CASES_WITH_VECTOR__
			AMT_CASSERT(m_nPendingPartialReadRequests == 0);
			#endif
		}
		__AMT_FORCEINLINE__ void UnregisterPartiallyWritingThread() const
		{
			AMT_CASSERT(m_nPendingWriteRequests == 0 && m_nPendingPartialWriteRequests == 1);
			AMT_CASSERT(m_nPendingReadRequests == 0);
			#if __AMT_REPORT_DOUBTFUL_CASES_WITH_VECTOR__
			AMT_CASSERT(m_nPendingPartialReadRequests == 0);
			#endif
			--m_nPendingPartialWriteRequests;
		}

		class CRegisterReadingThread
		{
			const vector& m_vec;

		public:
			inline CRegisterReadingThread(const vector& vec) : m_vec(vec)
			{
				m_vec.RegisterReadingThread();
			}
			inline ~CRegisterReadingThread() __AMT_CAN_THROW__
			{
				m_vec.UnregisterReadingThread();
			}
		};
		class CRegisterPartiallyReadingThread
		{
			const vector& m_vec;

		public:
			inline CRegisterPartiallyReadingThread(const vector& vec) : m_vec(vec)
			{
				m_vec.RegisterPartiallyReadingThread();
			}
			inline ~CRegisterPartiallyReadingThread() __AMT_CAN_THROW__
			{
				m_vec.UnregisterPartiallyReadingThread();
			}
		};
		class CRegisterWritingThread
		{
			const vector& m_vec;

		public:
			inline CRegisterWritingThread(const vector& vec) : m_vec(vec)
			{
				m_vec.RegisterWritingThread();
			}
			inline ~CRegisterWritingThread() __AMT_CAN_THROW__
			{
				m_vec.UnregisterWritingThread();
			}
		};
		class CRegisterPartiallyWritingThread
		{
			const vector& m_vec;

		public:
			inline CRegisterPartiallyWritingThread(const vector& vec) : m_vec(vec)
			{
				m_vec.RegisterPartiallyWritingThread();
			}
			inline ~CRegisterPartiallyWritingThread() __AMT_CAN_THROW__
			{
				m_vec.UnregisterPartiallyWritingThread();
			}
		};

	public:
		typedef typename Base::iterator iterator;

		inline vector() : Base()
		{
			Init();
		}
		inline vector(const vector& o) : Base(o)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(o);
			Init();			
			CRegisterWritingThread r2(*this);
			#endif
		}
		inline vector(vector&& o) 
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(o);
			Init();
			CRegisterWritingThread r2(*this);
			#endif
			*((Base*)this) = std::move(*((Base*)&o));
		}
		template< class InputIt >
		inline vector(InputIt begin, InputIt end)
		{
			{
				#if __AMT_CHECK_MULTITHREADED_ISSUES__
				Init();
				CRegisterWritingThread r2(*this);
				#endif
			}
			reserve(std::distance(begin, end));
			for (auto it = begin; it != end; ++it)
				push_back(*it);
		}
		inline vector(std::initializer_list<T> list)
		{
			{
				#if __AMT_CHECK_MULTITHREADED_ISSUES__
				Init();			
				CRegisterWritingThread r(*this);			
				#endif
			}
			reserve(list.size());
			for (auto it = list.begin(); it != list.end(); ++it)
				push_back(*it);
		}
		inline vector(size_t size)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			Init();
			CRegisterWritingThread r2(*this);
			#endif
			//((Base*)this)->resize(size, T(0));
			ResizeWithInitHelper<ValueType, AllocatorType, std::is_scalar<T>::value || amt::is_specialization<T, amt::AMTScalarType>::value>::resize(size, *((Base*)this)); // this trick is needed when wrapping up is on (__AMT_TRY_TO_AUTOMATICALLY_WRAP_UP_CONTAINERS_TYPES__)
		}
		inline vector(size_t size, const T& val)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			Init();
			CRegisterWritingThread r2(*this);
			#endif
			((Base*)this)->resize(size, val);
		}
		inline ~vector() __AMT_CAN_THROW__
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			AMT_CASSERT(m_nPendingReadRequests == 0);
			AMT_CASSERT(m_nPendingWriteRequests == 1);
			AMT_CASSERT(m_nPendingPartialReadRequests == 0);
			AMT_CASSERT(m_nPendingPartialWriteRequests == 0);
			#endif
		}
		__AMT_FORCEINLINE__ size_t size() const
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(*this); // it is assumed to be full read in order to conflict with any time of write
			#endif
			return ((Base*)this)->size();
		}
		__AMT_FORCEINLINE__ size_t capacity() const
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterPartiallyReadingThread r(*this); 
			#endif
			return ((Base*)this)->capacity();
		}
		__AMT_FORCEINLINE__ void clear()
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			#endif
			((Base*)this)->clear();
		}
		__AMT_FORCEINLINE__ bool empty() const
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterPartiallyReadingThread r(*this); 
			#endif
			return ((Base*)this)->empty();
		}
		__AMT_FORCEINLINE__ ValueType& operator[](size_t idx)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterPartiallyReadingThread r(*this);
			#endif
			AMT_CASSERT(idx < ((Base*)this)->size());
			return ((Base*)this)->operator[](idx);
		}
		__AMT_FORCEINLINE__ const ValueType& operator[](size_t idx) const
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterPartiallyReadingThread r(*this);
			#endif
			AMT_CASSERT(idx < ((Base*)this)->size());
			return ((Base*)this)->operator[](idx);
		}
		__AMT_FORCEINLINE__ ValueType& at(size_t idx)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterPartiallyReadingThread r(*this);
			#endif
			AMT_CASSERT(idx < ((Base*)this)->size());
			return ((Base*)this)->at(idx);
		}
		__AMT_FORCEINLINE__ const ValueType& at(size_t idx) const
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterPartiallyReadingThread r(*this);
			#endif
			AMT_CASSERT(idx < ((Base*)this)->size());
			return ((Base*)this)->at(idx);
		}

		inline ValueType* data() __AMT_NOEXCEPT__
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterPartiallyReadingThread r(*this);
			#endif
			return ((Base*)this)->data();
		}
		inline const ValueType* data() const __AMT_NOEXCEPT__
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterPartiallyReadingThread r(*this);
			#endif
			return ((Base*)this)->data();
		}
		
	private:

		// Helper struct to work around missing if constexpr
		template<typename U, typename AllocatorType, bool is_scalar = false>
		struct ResizeWithInitHelper
		{
			static void resize(size_t n, std::vector<U, AllocatorType>& vec)
			{
				vec.resize(n);
			}
		};
		template<typename U, typename AllocatorType>
		struct ResizeWithInitHelper<U, AllocatorType, true>
		{
			static void resize(size_t n, std::vector<U, AllocatorType>& vec)
			{
				vec.resize(n, U(0));
			}
		};

	public:
		__AMT_FORCEINLINE__ void reserve(size_t n)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			#endif
			((Base*)this)->reserve(n);
		}
		__AMT_FORCEINLINE__ void resize(size_t n)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this); // maybe to be verified later (PartiallyWriting under some circumstances?)
			#endif
			ResizeWithInitHelper<ValueType, AllocatorType, std::is_scalar<T>::value || amt::is_specialization<T, amt::AMTScalarType>::value>::resize(n, *((Base*)this)); // this trick is needed when wrapping up is on (__AMT_TRY_TO_AUTOMATICALLY_WRAP_UP_CONTAINERS_TYPES__)
			//((Base*)this)->resize(n, T(0));
		}
		__AMT_FORCEINLINE__ void resize(size_t n, const ValueType& val)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this); // maybe to be verified later (PartiallyWriting under some circumstances?)
			#endif
			((Base*)this)->resize(n, val);
		}
		__AMT_FORCEINLINE__ void shrink_to_fit()
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			#endif
			((Base*)this)->shrink_to_fit();
		}
		__AMT_FORCEINLINE__ ValueType& front() const
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterPartiallyReadingThread r(*this);
			#endif
			return ((Base*)this)->front();
		}
		__AMT_FORCEINLINE__ ValueType& back() const
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(*this); // partially?
			#endif
			return ((Base*)this)->back();
		}
		__AMT_FORCEINLINE__ void push_back(const T& t)
		{
			if (((Base*)this)->capacity() > ((Base*)this)->size())
			{
				#if __AMT_CHECK_MULTITHREADED_ISSUES__
				CRegisterPartiallyWritingThread r(*this);
				#endif
				((Base*)this)->push_back(t);
			}
			else
			{
				#if __AMT_CHECK_MULTITHREADED_ISSUES__
				CRegisterWritingThread r(*this);
				#endif
				((Base*)this)->push_back(t);
			}
		}
		__AMT_FORCEINLINE__ void push_back(T&& t)
		{
			if (((Base*)this)->capacity() > ((Base*)this)->size())
			{
				#if __AMT_CHECK_MULTITHREADED_ISSUES__
				CRegisterPartiallyWritingThread r(*this);
				#endif
				((Base*)this)->push_back(std::move(t));
			}
			else
			{
				#if __AMT_CHECK_MULTITHREADED_ISSUES__
				CRegisterWritingThread r(*this);
				#endif
				((Base*)this)->push_back(std::move(t));
			}
		}

		__AMT_FORCEINLINE__ void pop_back()
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this); // partially? however now, contrary to push_back without allocation, an element disappers, so cuncurrent read at the back() can fail
			#endif
			return ((Base*)this)->pop_back();
		}
		__AMT_FORCEINLINE__ void emplace_back(const T& t)
		{
			if (((Base*)this)->capacity() > ((Base*)this)->size())
			{
				#if __AMT_CHECK_MULTITHREADED_ISSUES__
				CRegisterPartiallyWritingThread r(*this);
				#endif
				return ((Base*)this)->push_back(t);
			}
			else
			{
				#if __AMT_CHECK_MULTITHREADED_ISSUES__
				CRegisterWritingThread r(*this);
				#endif
				return ((Base*)this)->push_back(t);
			}
		}
		#if __cplusplus >= 201703L
		template< class... Args >
		__AMT_FORCEINLINE__ ValueType& emplace_back(Args&&... args)
		{
			if (((Base*)this)->capacity() > ((Base*)this)->size())
			{
				#if __AMT_CHECK_MULTITHREADED_ISSUES__
				CRegisterPartiallyWritingThread r(*this);
				#endif
				return ((Base*)this)->emplace_back(std::forward<Args>(args)...);
			}
			else
			{
				#if __AMT_CHECK_MULTITHREADED_ISSUES__
				CRegisterWritingThread r(*this);
				#endif
				return ((Base*)this)->emplace_back(std::forward<Args>(args)...);
			}
		}
		#else
		template< class... Args >
		__AMT_FORCEINLINE__ void emplace_back(Args&&... args)
		{
			if (((Base*)this)->capacity() > ((Base*)this)->size())
			{
				#if __AMT_CHECK_MULTITHREADED_ISSUES__
				CRegisterPartiallyWritingThread r(*this);
				#endif
				((Base*)this)->emplace_back(std::forward<Args>(args)...);
			}
			else
			{
				#if __AMT_CHECK_MULTITHREADED_ISSUES__
				CRegisterWritingThread r(*this);
				#endif
				((Base*)this)->emplace_back(std::forward<Args>(args)...);
			}
		}
		#endif
		template< class... Args >
		__AMT_FORCEINLINE__ iterator emplace(const_iterator pos, Args&&... args)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			#endif
			return ((Base*)this)->emplace(pos, std::forward<Args>(args)...);
		}
		__AMT_FORCEINLINE__ void swap(vector& o)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			CRegisterWritingThread r2(o);
			#endif
			return ((Base*)this)->swap(*((Base*)&o));
		}

		__AMT_FORCEINLINE__ iterator erase(iterator it)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			#endif
			return ((Base*)this)->erase(it);
		}
		__AMT_FORCEINLINE__ iterator erase(iterator itFirst, iterator itLast)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			#endif
			return ((Base*)this)->erase(itFirst, itLast);
		}

		//__AMT_FORCEINLINE__ void erase(size_t i)
		//{
		//	CRegisterWritingThread r(*this);
		//	((Base*)this)->erase(i);
		//}

		template <class InputIterator>
		inline void assign(InputIterator first, InputIterator last)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			#endif
			((Base*)this)->assign(first, last);
		}
		inline void assign(size_t n, const ValueType& val)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			#endif
			((Base*)this)->assign(n, val);
		}

		__AMT_FORCEINLINE__ vector& operator = (const vector& o)
		{
			if (&o != this)
			{ 
				#if __AMT_CHECK_MULTITHREADED_ISSUES__
				CRegisterReadingThread r(o);
				CRegisterWritingThread r2(*this);
				#endif
				*((Base*)this) = *((Base*)&o);
				return *this;
			}
			else
			{
				#if __AMT_CHECK_MULTITHREADED_ISSUES__				
				CRegisterWritingThread r(*this);
				#endif
				*((Base*)this) = *((Base*)&o);
				return *this;
			}
		}
		__AMT_FORCEINLINE__ vector& operator = (vector&& o)
		{
			if (&o != this)
			{ 
				#if __AMT_CHECK_MULTITHREADED_ISSUES__
				CRegisterWritingThread r(o);
				CRegisterWritingThread r2(*this);
				#endif
				*((Base*)this) = std::move(*((Base*)&o));
				return *this;
			}
			else
			{
				#if __AMT_CHECK_MULTITHREADED_ISSUES__
				CRegisterWritingThread r2(*this);
				#endif
				*((Base*)this) = std::move(*((Base*)&o));
				return *this;
			}
		}

		__AMT_FORCEINLINE__ friend bool operator < (const vector& v1, const vector& v2)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(v1);
			CRegisterReadingThread r2(v2);
			#endif
			return *((Base*)&v1) < *((Base*)&v2);
		}
		__AMT_FORCEINLINE__ friend bool operator <= (const vector& v1, const vector&v2)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(v1);
			CRegisterReadingThread r2(v2);
			#endif
			return *((Base*)&v1) <= *((Base*)&v2);
		}
		__AMT_FORCEINLINE__ friend bool operator > (const vector& v1, const vector& v2)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(v1);
			CRegisterReadingThread r2(v2);
			#endif
			return *((Base*)&v1) > *((Base*)&v2);
		}
		__AMT_FORCEINLINE__ friend bool operator >= (const vector& v1, const vector& v2)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(v1);
			CRegisterReadingThread r2(v2);
			#endif
			return *((Base*)&v1) >= *((Base*)&v2);
		}

	};

	// TODO: Partial specialization for vector of bool:
	template<class Alloc>
	class vector<bool, Alloc> : public std::vector<bool, Alloc>
	{
		// to be done...
	};


#endif

}
