//
// Assertive MultiThreading Library
//
//  Copyright Marcin Sterkowiec, Piotr Tracz, 2021-2022. Use, modification and
//  distribution is subject to license (see accompanying file license.txt)
//

#pragma once

#include <set>
#include <atomic>

#include "amt_cassert.h"
#include "amt_types.h"
#include "amt_pod.h"

namespace amt
{

#if !defined(__AMTL_ASSERTS_ARE_ON__)
	template<typename T, class Compare = std::less<T>, class Allocator = std::allocator<T>, bool PREVENT_AMT_WRAP = false >
	using set = std::set<T, Compare, Allocator>;
#else

	template<typename T, class Compare = std::less<T>, class Alloc = std::allocator<T>, bool PREVENT_AMT_WRAP = false >
	class set : public std::set<__AMT_TRY_WRAP_TYPE__(T, PREVENT_AMT_WRAP, Alloc), Compare, __AMT_CHANGE_ALLOCATOR_IF_NEEDED__(T, PREVENT_AMT_WRAP, Alloc)>
	{
		typedef std::set<__AMT_TRY_WRAP_TYPE__(T, PREVENT_AMT_WRAP, Alloc), Compare, __AMT_CHANGE_ALLOCATOR_IF_NEEDED__(T, PREVENT_AMT_WRAP, Alloc)> Base;

		mutable std::atomic<AMTCounterType> m_nPendingReadRequests;
		mutable std::atomic<AMTCounterType> m_nPendingWriteRequests;
		mutable std::atomic<std::uint64_t> m_nCountOperInvalidateIter; // count of operations that invalidate iterators - this single count seems good enough

		inline void Init()
		{
			m_nPendingReadRequests = 0;
			m_nPendingWriteRequests = 0;
			m_nCountOperInvalidateIter = 0;
		}
		__AMT_FORCEINLINE__ void RegisterReadingThread() const
		{
			++m_nPendingReadRequests;
			AMT_CASSERT(m_nPendingWriteRequests == 0);
		}
		__AMT_FORCEINLINE__ void UnregisterReadingThread() const
		{
			AMT_CASSERT(m_nPendingWriteRequests == 0);
			--m_nPendingReadRequests;
		}
		__AMT_FORCEINLINE__ void RegisterWritingThread() const
		{
			++m_nPendingWriteRequests;
			AMT_CASSERT(m_nPendingWriteRequests == 1);
			AMT_CASSERT(m_nPendingReadRequests == 0);
		}
		__AMT_FORCEINLINE__ void UnregisterWritingThread() const
		{
			AMT_CASSERT(m_nPendingWriteRequests == 1);
			AMT_CASSERT(m_nPendingReadRequests == 0);
			--m_nPendingWriteRequests;
		}

		class CRegisterReadingThread
		{
			const set& m_set;

		public:
			inline CRegisterReadingThread(const set& set) : m_set(set)
			{
				m_set.RegisterReadingThread();
			}
			inline ~CRegisterReadingThread() __AMT_CAN_THROW__
			{
				m_set.UnregisterReadingThread();
			}
		};
		class CRegisterWritingThread
		{
			const set& m_set;

		public:
			inline CRegisterWritingThread(const set& set) : m_set(set)
			{
				m_set.RegisterWritingThread();
			}
			inline ~CRegisterWritingThread() __AMT_CAN_THROW__
			{
				m_set.UnregisterWritingThread();
			}
		};

		// ITER can be iterator, const_iterator etc.
		template<class ITER>
		class IteratorBase : public ITER
		{
			std::int64_t m_nCountOperInvalidateIter; // the counter that container had, while creating iterator
			const set* m_pSet;

			#if __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__
			mutable std::atomic<AMTCounterType> m_nPendingReadRequests;
			mutable std::atomic<AMTCounterType> m_nPendingWriteRequests;
			#endif

			__AMT_FORCEINLINE__ void RegisterReadingThread() const
			{
				#if __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__
				++m_nPendingReadRequests;
				AMT_CASSERT(m_nPendingWriteRequests == 0);
				#endif
			}
			__AMT_FORCEINLINE__ void UnregisterReadingThread() const
			{
				#if __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__
				AMT_CASSERT(m_nPendingWriteRequests == 0);
				--m_nPendingReadRequests;
				#endif
			}
			__AMT_FORCEINLINE__ void RegisterWritingThread() const
			{
				#if __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__
				++m_nPendingWriteRequests;
				AMT_CASSERT(m_nPendingWriteRequests == 1);
				AMT_CASSERT(m_nPendingReadRequests == 0);
				#endif
			}
			__AMT_FORCEINLINE__ void UnregisterWritingThread() const
			{
				#if __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__
				AMT_CASSERT(m_nPendingWriteRequests == 1);
				AMT_CASSERT(m_nPendingReadRequests == 0);
				--m_nPendingWriteRequests;
				#endif
			}
			class CRegisterReadingThread
			{
				const IteratorBase& m_it;

			public:
				inline CRegisterReadingThread(const IteratorBase& it) : m_it(it)
				{
					m_it.RegisterReadingThread();
				}
				inline ~CRegisterReadingThread() __AMT_CAN_THROW__
				{
					m_it.UnregisterReadingThread();
				}
			};
			class CRegisterWritingThread
			{
				const IteratorBase& m_it;

			public:
				inline CRegisterWritingThread(const IteratorBase& it) : m_it(it)
				{
					m_it.RegisterWritingThread();
				}
				inline ~CRegisterWritingThread() __AMT_CAN_THROW__
				{
					m_it.UnregisterWritingThread();
				}
			};

			// These helper structures are to work around situations when if constexpr is not available yet:
			template<char IS_REVERSE_ITER, typename = void>
			struct SAssertNotBegin
			{
				template<typename ITER_TYPE>
				static inline void RunCheck(const ITER_TYPE& it, const set& set)
				{
					AMT_CASSERT(it != ((Base*)&set)->begin());
				}
			};
			template<typename V>
			struct SAssertNotBegin<true, V>
			{
				template<typename ITER_TYPE>
				static inline void RunCheck(const ITER_TYPE& it, const set& set)
				{
					AMT_CASSERT(it != ((Base*)&set)->rbegin());
				}
			};
			template<typename V>
			struct SAssertNotBegin<2, V>
			{
				template<typename ITER_TYPE>
				static inline void RunCheck(const ITER_TYPE& it, const set& set)
				{
					AMT_CASSERT(it != ((Base*)&set)->crbegin());
				}
			};
			template<char IS_REVERSE_ITER, typename = void>
			struct SAssertNotEnd
			{
				template<typename ITER_TYPE>
				static inline void RunCheck(const ITER_TYPE& it, const set& set)
				{
					AMT_CASSERT(it != ((Base*)&set)->end());
				}
			};
			template<typename V>
			struct SAssertNotEnd<true, V>
			{
				template<typename ITER_TYPE>
				static inline void RunCheck(const ITER_TYPE& it, const set& set)
				{
					AMT_CASSERT(it != ((Base*)&set)->rend());
				}
			};
			template<typename V>
			struct SAssertNotEnd<2, V>
			{
				template<typename ITER_TYPE>
				static inline void RunCheck(const ITER_TYPE& it, const set& set)
				{
					AMT_CASSERT(it != ((Base*)&set)->crend());
				}
			};

		public:
			__AMT_FORCEINLINE__ IteratorBase()
			{
				m_pSet = nullptr;
				m_nCountOperInvalidateIter = (size_t) -1;
				#if __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__
				m_nPendingReadRequests = 0;
				m_nPendingWriteRequests = 0;
				#endif
			}
			__AMT_FORCEINLINE__ IteratorBase(ITER it, const set* pSet) : ITER()
			{
				*((ITER*)this) = it;
				m_pSet = pSet;
				m_nCountOperInvalidateIter = m_pSet->m_nCountOperInvalidateIter;
				#if __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__
				m_nPendingReadRequests = 0;
				m_nPendingWriteRequests = 0;
				#endif
			}
			__AMT_FORCEINLINE__ IteratorBase(const IteratorBase& o)
			{
				#if __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__
				m_nPendingReadRequests = 0;
				m_nPendingWriteRequests = 0;
				CRegisterWritingThread r(*this);
				CRegisterReadingThread r2(o);
				#endif
				#if __AMT_CHECK_ITERATORS_VALIDITY__
				o.AssertIsValid();
				#endif
				m_pSet = o.m_pSet;
				m_nCountOperInvalidateIter = o.m_nCountOperInvalidateIter;
				*((ITER*)this) = *((ITER*)&o);
			}
			__AMT_FORCEINLINE__ IteratorBase& operator = (const IteratorBase& o)
			{
				if (this != &o)
				{ 
					#if __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__
					CRegisterWritingThread r(*this);
					CRegisterReadingThread r2(o);
					#endif
					#if __AMT_CHECK_ITERATORS_VALIDITY__
					o.AssertIsValid();
					#endif
					m_pSet = o.m_pSet;
					*((ITER*)this) = *((ITER*)&o);
					m_nCountOperInvalidateIter = o.m_nCountOperInvalidateIter;
					return *this;
				}
				else
				{
					#if __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__
					CRegisterWritingThread r(*this);
					#endif
					#if __AMT_CHECK_ITERATORS_VALIDITY__
					o.AssertIsValid();
					#endif
					m_pSet = o.m_pSet;
					*((ITER*)this) = *((ITER*)&o);
					m_nCountOperInvalidateIter = o.m_nCountOperInvalidateIter;
					return *this;
				}
			}
			__AMT_FORCEINLINE__ IteratorBase& operator = (IteratorBase&& o)
			{
				if (this != &o)
				{ 
					#if __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__
					CRegisterWritingThread r(*this);
					CRegisterWritingThread r2(o);
					#endif
					#if __AMT_CHECK_ITERATORS_VALIDITY__
					o.AssertIsValid();
					#endif	
					m_pSet = o.m_pSet;
					*((ITER*)this) = std::move(*((ITER*)&o));
					m_nCountOperInvalidateIter = o.m_nCountOperInvalidateIter;
					return *this;
				}
				else
				{
					#if __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__
					CRegisterWritingThread r(*this);
					#endif
					#if __AMT_CHECK_ITERATORS_VALIDITY__
					o.AssertIsValid();										
					#endif
					m_pSet = o.m_pSet;
					*((ITER*)this) = std::move(*((ITER*)&o));
					m_nCountOperInvalidateIter = o.m_nCountOperInvalidateIter;
					return *this;
				}
			}
			__AMT_FORCEINLINE__ friend bool operator == (const IteratorBase& it1, const IteratorBase& it2)
			{
				#if __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__
				CRegisterReadingThread r(it1);
				CRegisterReadingThread r2(it2);
				#endif
				#if __AMT_CHECK_ITERATORS_VALIDITY__
				it1.AssertIsValid();
				it2.AssertIsValid();				
				AMT_CASSERT(it1.m_pSet == it2.m_pSet);
				#endif
				return *((ITER*)&it1) == *((ITER*)&it2);
			}
			__AMT_FORCEINLINE__ friend bool operator != (const IteratorBase& it1, const IteratorBase& it2)
			{
				#if __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__
				CRegisterReadingThread r(it1);
				CRegisterReadingThread r2(it2);
				#endif
				#if __AMT_CHECK_ITERATORS_VALIDITY__
				it1.AssertIsValid();
				it2.AssertIsValid();				
				AMT_CASSERT(it1.m_pSet == it2.m_pSet);
				#endif
				return *((ITER*)&it1) != *((ITER*)&it2);
			}
			__AMT_FORCEINLINE__ ~IteratorBase() __AMT_CAN_THROW__
			{
				#if __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__
				CRegisterWritingThread r(*this); // not necessarily wrapped up in #if __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__
				AMT_CASSERT(m_nPendingReadRequests == 0);
				AMT_CASSERT(m_nPendingWriteRequests == 1);
				#endif
			}
			__AMT_FORCEINLINE__ bool IsIteratorValid() const
			{
				#if __AMT_CHECK_ITERATORS_VALIDITY__
				return m_nCountOperInvalidateIter == m_pSet->m_nCountOperInvalidateIter; // good enough
				#else
				return true;
				#endif
			}
			__AMT_FORCEINLINE__ void AssertIsValid(const set* pSet = nullptr) const
			{
				AMT_CASSERT(m_pSet != nullptr);
				AMT_CASSERT(m_pSet == pSet || pSet == nullptr); // passing set is not mandatory but lets make sure that iterator is not used versus wrong object
				#if __AMT_CHECK_ITERATORS_VALIDITY__
				AMT_CASSERT(IsIteratorValid());
				#endif
			}
			__AMT_FORCEINLINE__ void AssertNotBegin() const
			{			
				SAssertNotBegin<IsRevIter<Base, ITER>::value + IsConstRevIter<Base, ITER>::value>::template RunCheck<ITER>(*((ITER*)this), *m_pSet);
			}
			__AMT_FORCEINLINE__ void AssertNotEnd() const
			{
				SAssertNotEnd<IsRevIter<Base, ITER>::value + IsConstRevIter<Base, ITER>::value>::template RunCheck<ITER>(*((ITER*)this), *m_pSet);
			}
			__AMT_FORCEINLINE__ IteratorBase& operator++()
			{
				#if __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__
				CRegisterWritingThread r(*this);
				#endif
				#if __AMT_CHECK_ITERATORS_VALIDITY__
				AssertIsValid();
				AssertNotEnd();
				#endif
				((ITER*)this)->operator++();
				return *this;
			}
			__AMT_FORCEINLINE__ IteratorBase& operator--()
			{
				#if __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__
				CRegisterWritingThread r(*this);
				#endif
				#if __AMT_CHECK_ITERATORS_VALIDITY__
				AssertIsValid();
				AssertNotBegin();
				#endif
				((ITER*)this)->operator--();
				return *this;
			}
			__AMT_FORCEINLINE__ IteratorBase operator++(int) // postfix operator
			{
				#if __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__
				CRegisterWritingThread r(*this);
				#endif
				#if __AMT_CHECK_ITERATORS_VALIDITY__
				AssertIsValid();
				AssertNotEnd();
				#endif
				auto it = *this;
				((ITER*)this)->operator++();
				return it;
			}
			__AMT_FORCEINLINE__ IteratorBase operator--(int) // postfix operator
			{
				#if __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__
				CRegisterWritingThread r(*this);
				#endif
				#if __AMT_CHECK_ITERATORS_VALIDITY__
				AssertIsValid();
				AssertNotBegin();
				#endif
				auto it = *this;
				((ITER*)this)->operator--();
				return it;
			}		
			
			#if defined(_MSC_VER) && _MSVC_LANG < 201402L
			T* operator ->()
			#else
			auto operator ->()
			#endif
			{
				#if __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__
				CRegisterReadingThread r(*this);
				#endif
				#if __AMT_CHECK_ITERATORS_VALIDITY__
				AssertIsValid();
				AssertNotEnd();
				#endif
				#if defined(_MSC_VER) && _MSVC_LANG < 201402L
				return (const T*) ((ITER*)this)->operator->();
				#else
				return ((ITER*)this)->operator->();
				#endif
			}

			#if defined(_MSC_VER) && _MSVC_LANG < 201402L
			const T* operator ->() const
			#else
			const auto operator ->() const
			#endif
			{
				#if __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__
				CRegisterReadingThread r(*this);
				#endif
				#if __AMT_CHECK_ITERATORS_VALIDITY__
				AssertIsValid();
				AssertNotEnd();
				#endif
				#if defined(_MSC_VER) && _MSVC_LANG < 201402L
				return (const T*) ((ITER*)this)->operator->();
				#else
				return ((ITER*)this)->operator->();
				#endif
			}


			#if defined(_MSC_VER) && _MSVC_LANG < 201402L
			T& operator *()
			#else
			auto& operator *()
			#endif
			{
				#if __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__
				CRegisterReadingThread r(*this);
				#endif
				#if __AMT_CHECK_ITERATORS_VALIDITY__
				AssertIsValid();
				AssertNotEnd();
				#endif
				#if defined(_MSC_VER) && _MSVC_LANG < 201402L
				return (const T&) ((ITER*)this)->operator*();
				#else
				return ((ITER*)this)->operator*();
				#endif
			}
			
			#if defined(_MSC_VER) && _MSVC_LANG < 201402L
			const T& operator *() const
			#else
			const auto& operator *() const
			#endif
			{
				#if __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__
				CRegisterReadingThread r(*this);
				#endif
				#if __AMT_CHECK_ITERATORS_VALIDITY__
				AssertIsValid();
				AssertNotEnd();
				#endif
				#if defined(_MSC_VER) && _MSVC_LANG < 201402L
				return (const T&) ((ITER*)this)->operator*();
				#else
				return ((ITER*)this)->operator*();
				#endif
			}			
		};		
		

	public:

		using iterator = IteratorBase<typename Base::iterator>;
		using const_iterator = IteratorBase<typename Base::const_iterator>;
		using reverse_iterator = IteratorBase<typename Base::reverse_iterator>;
		using const_reverse_iterator = IteratorBase<typename Base::const_reverse_iterator>;

		inline set() : Base()
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			Init();
			#endif
		}

		inline set(const set& o) : Base(o)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(o);
			Init();
			CRegisterWritingThread r2(*this);
			#endif
		}
		inline set(set&& o)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(o);
			Init();
			CRegisterWritingThread r2(*this);
			#endif
			*((Base*)this) = std::move(*((Base*)&o));
		}
		inline set(std::initializer_list<T> list) : Base(list)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			Init();
			CRegisterWritingThread r(*this);
			#endif
		}
		template< class InputIterator >
		inline set(InputIterator first, InputIterator last) : Base(first, last)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			Init();
			CRegisterWritingThread r(*this);
			#endif
		}
		inline set& operator = (const set& o)
		{
			if (this != &o)
			{
				#if __AMT_CHECK_MULTITHREADED_ISSUES__
				CRegisterReadingThread r(o);
				CRegisterWritingThread r2(*this);
				#endif
				++m_nCountOperInvalidateIter;
				*((Base*)this) = *((Base*)&o);
				return *this;			
			}
			else
			{
				#if __AMT_CHECK_MULTITHREADED_ISSUES__
				CRegisterWritingThread r(*this);
				#endif
				++m_nCountOperInvalidateIter;
				*((Base*)this) = *((Base*)&o);
				return *this;		
			}
		}
		inline set& operator = (set&& o)
		{
			if (this != &o)
			{ 
				#if __AMT_CHECK_MULTITHREADED_ISSUES__
				CRegisterWritingThread r(o);
				CRegisterWritingThread r2(*this);
				#endif
				++m_nCountOperInvalidateIter; // is this needed? (will be overwritten within a split second...)
				*((Base*)this) = std::move(*((Base*)&o));
				++o.m_nCountOperInvalidateIter;
				return *this;
			}
			else
			{
				#if __AMT_CHECK_MULTITHREADED_ISSUES__
				CRegisterWritingThread r(*this);
				#endif
				++m_nCountOperInvalidateIter; // is this needed? (will be overwritten within a split second...)
				*((Base*)this) = std::move(*((Base*)&o));
				++o.m_nCountOperInvalidateIter;
				return *this;
			}
		}
		inline ~set() __AMT_CAN_THROW__
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			#endif
			++ m_nCountOperInvalidateIter;
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			AMT_CASSERT(m_nPendingWriteRequests == 1);
			AMT_CASSERT(m_nPendingReadRequests == 0);
			#endif
		}

		__AMT_FORCEINLINE__ size_t size() const
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(*this);			
			#endif
			return ((Base*)this)->size();
		}
		__AMT_FORCEINLINE__ bool empty() const
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(*this);
			#endif
			return ((Base*)this)->empty();
		}
		__AMT_FORCEINLINE__ void clear()
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			#endif
			++ m_nCountOperInvalidateIter;
			((Base*)this)->clear();
		}

		__AMT_FORCEINLINE__ friend bool operator == (const set& m1, const set& m2)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(m1);
			CRegisterReadingThread r2(m2);
			#endif
			return *((Base*)&m1) == *((Base*)&m2);
		}
		__AMT_FORCEINLINE__ friend bool operator != (const set& m1, const set& m2)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(m1);
			CRegisterReadingThread r2(m2);
			#endif
			return *((Base*)&m1) != *((Base*)&m2);
		}
		__AMT_FORCEINLINE__ friend bool operator < (const set& m1, const set& m2)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(m1);
			CRegisterReadingThread r2(m2);
			#endif
			return *((Base*)&m1) < *((Base*)&m2);
		}
		__AMT_FORCEINLINE__ friend bool operator <= (const set& m1, const set& m2)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(m1);
			CRegisterReadingThread r2(m2);
			#endif
			return *((Base*)&m1) <= *((Base*)&m2);
		}
		__AMT_FORCEINLINE__ friend bool operator > (const set& m1, const set& m2)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(m1);
			CRegisterReadingThread r2(m2);
			#endif
			return *((Base*)&m1) > *((Base*)&m2);
		}
		__AMT_FORCEINLINE__ friend bool operator >= (const set& m1, const set& m2)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(m1);
			CRegisterReadingThread r2(m2);
			#endif
			return *((Base*)&m1) >= *((Base*)&m2);
		}

		__AMT_FORCEINLINE__ iterator find(const T& key)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(*this);
			#endif
			auto baseIt = ((Base*)this)->find(key);
			iterator it(baseIt, this);
			return it;
		}
		__AMT_FORCEINLINE__ const_iterator find(const T& k) const
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(*this);
			#endif
			auto baseIt = ((Base*)this)->find(k);
			iterator it(baseIt, this);
			return it;
		}
		__AMT_FORCEINLINE__  size_t count(const T& k) const
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(*this);
			#endif
			return ((Base*)this)->count(k);
		}
		__AMT_FORCEINLINE__ void erase(iterator it)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			#endif
			#if __AMT_CHECK_ITERATORS_VALIDITY__
			it.AssertIsValid(this);
			#endif
			++m_nCountOperInvalidateIter;
			((Base*)this)->erase(it);
		}
		__AMT_FORCEINLINE__ size_t erase(const T& key)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			#endif
			++m_nCountOperInvalidateIter;
			return ((Base*)this)->erase(key);
		}
		__AMT_FORCEINLINE__ void erase(iterator first, iterator last)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			#endif
			#if __AMT_CHECK_ITERATORS_VALIDITY__
			first.AssertIsValid(this);
			last.AssertIsValid(this);
			#endif
			++m_nCountOperInvalidateIter;
			((Base*)this)->erase(first, last);
		}

		__AMT_FORCEINLINE__ void swap(set& o)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			CRegisterWritingThread r2(o);
			#endif
			++m_nCountOperInvalidateIter;
			++o.m_nCountOperInvalidateIter;
			return ((Base*)this)->swap(*((Base*)&o));
		}
		template< class... Args >
		__AMT_FORCEINLINE__ std::pair<iterator, bool> emplace(Args&&... args)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			#endif
			auto resBase = ((Base*)this)->emplace(std::forward<Args>(args)...);		
			if (resBase.second)
				++m_nCountOperInvalidateIter;
			std::pair<iterator, bool> res(iterator(resBase.first, this), resBase.second);
			return res;
		}
		template< class... Args >
		__AMT_FORCEINLINE__ iterator emplace_hint(const_iterator position, Args&&... args)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			#endif
			#if __AMT_CHECK_ITERATORS_VALIDITY__
			position.AssertIsValid();
			#endif
			auto resBase = ((Base*)this)->emplace_hint(position, std::forward<Args>(args)...);
			iterator res(resBase, this);
			return res;
		}
		std::pair<iterator, bool> insert(const T& val)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			#endif
			auto resBase =((Base*)this)->insert(val);
			if (resBase.second)
				++m_nCountOperInvalidateIter;
			std::pair<iterator, bool> res(make_pair(iterator(resBase.first, this), resBase.second));
			return res;
		}
		std::pair<iterator, bool> insert(T&& val)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			#endif
			auto resBase =((Base*)this)->insert(std::move(val));
			if (resBase.second)
				++m_nCountOperInvalidateIter;
			std::pair<iterator, bool> res(make_pair(iterator(resBase.first, this), resBase.second));
			return res;
		}
		iterator insert(iterator position, const T& val)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			#endif
			#if __AMT_CHECK_ITERATORS_VALIDITY__
			position.AssertIsValid(this);
			#endif
			++m_nCountOperInvalidateIter;
			auto resBase = ((Base*)this)->insert(position, val);
			iterator res(resBase, this);
			return res;
		}
		#ifdef _WIN32 // temporary workaround for compilation on Linux
		template <class InputIterator>
		void insert(InputIterator first, InputIterator last)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			#endif
			#if __AMT_CHECK_ITERATORS_VALIDITY__
			first.AssertIsValid(this);
			last.AssertIsValid(this);
			#endif
			++m_nCountOperInvalidateIter;
			((Base*)this)->insert<InputIterator>(first, last);
		}
		#endif
		iterator lower_bound(const T& k)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(*this);
			#endif
			auto resBase = ((Base*)this)->lower_bound(k);
			iterator res(resBase, this);
			return res;
		}
		const_iterator lower_bound(const T& k) const
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(*this);
			#endif
			auto resBase = ((Base*)this)->lower_bound(k);
			iterator res(resBase, this);
			return res;
		}
		iterator upper_bound(const T& k)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(*this);
			#endif
			auto resBase = ((Base*)this)->upper_bound(k);
			iterator res(resBase, this);
			return res;
		}
		const_iterator upper_bound(const T& k) const
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(*this);
			#endif
			auto resBase =((Base*)this)->upper_bound(k);
			const_iterator res(resBase, this);
			return res;
		}
		std::pair<const_iterator, const_iterator> equal_range(const T& k) const
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(*this);
			#endif
			auto resBase = ((Base*)this)->equal_range(k);
			std::pair<const_iterator, const_iterator> res(const_iterator(resBase.first, this), const_iterator(resBase.second, this));
			return res;
		}
		std::pair<iterator, iterator> equal_range(const T& k)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(*this);
			#endif
			auto resBase = ((Base*)this)->equal_range(k);
			std::pair<iterator, iterator> res(iterator(resBase.first, this), iterator(resBase.second, this));
			return res;
		}

		// Iterators:
		inline iterator begin()
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(*this);
			#endif
			auto resBase = ((Base*)this)->begin();
			iterator res(resBase, this);
			return res;
		}
		inline const_iterator begin() const
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(*this);
			#endif
			auto resBase = ((Base*)this)->begin();
			const_iterator res(resBase, this);
			return res;
		}
		inline const_iterator cbegin() const __AMT_NOEXCEPT__
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(*this);
			#endif
			auto resBase = ((Base*)this)->cbegin();
			const_iterator res(resBase, this);
			return res;
		}
		inline reverse_iterator rbegin()
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(*this);
			#endif
			auto resBase = ((Base*)this)->rbegin();
			reverse_iterator res(resBase, this);
			return res;
		}
		inline const_reverse_iterator rbegin() const
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(*this);
			#endif
			auto resBase = ((Base*)this)->rbegin();
			const_reverse_iterator res(resBase, this);
			return res;
		}
		inline const_reverse_iterator crbegin() const __AMT_NOEXCEPT__
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(*this);
			#endif
			auto resBase = ((Base*)this)->crbegin();
			const_reverse_iterator res(resBase, this);
			return res;
		}
		inline iterator end()
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(*this);
			#endif
			auto resBase = ((Base*)this)->end();
			iterator res(resBase, this);
			return res;
		}
		inline const_iterator end() const
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(*this);
			#endif
			auto resBase = ((Base*)this)->end();
			const_iterator res(resBase, this);
			return res;
		}
		inline const_iterator cend() const __AMT_NOEXCEPT__
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(*this);
			#endif
			auto resBase = ((Base*)this)->cend();
			const_iterator res(resBase, this);
			return res;
		}
		inline reverse_iterator rend()
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(*this);
			#endif
			auto resBase = ((Base*)this)->rend();
			reverse_iterator res(resBase, this);
			return res;
		}
		inline const_reverse_iterator rend() const
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(*this);
			#endif
			auto resBase = ((Base*)this)->rend();
			const_reverse_iterator res(resBase, this);
			return res;
		}
		inline const_reverse_iterator crend() const __AMT_NOEXCEPT__
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(*this);
			#endif
			auto resBase = ((Base*)this)->crend();
			const_reverse_iterator res(resBase, this);
			return res;
		}
	};

#endif

}
