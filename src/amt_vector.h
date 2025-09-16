//
// Assertive MultiThreading Library
//
//  Copyright Marcin Sterkowiec, Piotr Tracz, 2021-2023. Use, modification and
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
	template<typename T, class Alloc = std::allocator<T>>
	using vector = std::vector<T, Alloc>;
#else

	template<typename T, class Alloc = std::allocator<T>>
	class vector : public std::vector<T, Alloc>
	{
		typedef std::vector<T, Alloc> Base;
		typedef typename Base::value_type ValueType;
		typedef typename Base::allocator_type AllocatorType;				

	public:
		using value_type = T;
		using allocator_type = Alloc;
		using reference = T&;
		using const_reference = const T&;
		using pointer = T*; // std::allocator_traits<allocator_type>::pointer;
		using const_pointer = const T*; // std::allocator_traits<allocator_type>::const_pointer;
		using size_type = size_t;
		using difference_type = ptrdiff_t;

	private:

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

		// Count of operations that invalidate iterators - this single count seems good enough:
		mutable std::atomic<std::uint64_t> m_nCountOperInvalidateIter; 

		inline void Init()
		{
			m_nPendingReadRequests = 0;
			m_nPendingWriteRequests = 0;
			m_nPendingPartialReadRequests = 0;
			m_nPendingPartialWriteRequests = 0;
			m_nCountOperInvalidateIter = 0;
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

		// ITER can be vector's iterator, const_iterator etc.
		template<class ITER>
		class IteratorBase : public ITER
		{
			//typedef typename ITER::difference_type difference_type;
			//typedef typename ITER::value_type value_type;
			//typedef typename ITER::pointer pointer;
			//typedef typename ITER::reference reference;
			//typedef typename ITER::iterator_category iterator_category;

			using non_const_iter = typename Base::iterator;
			using const_iter = typename Base::const_iterator;			
			using non_const_reviter = typename Base::reverse_iterator;
			using const_reviter = typename Base::const_reverse_iterator;
			static const bool is_const_iter = std::is_same<ITER, const_iter>::value || std::is_same<ITER, const_reviter>::value;
			// Friends - make IteratorBase<X> a friend of IteratorBase<X const>
			friend std::conditional_t<std::is_same<ITER, non_const_iter>::value, IteratorBase<const_iter>, void>;
			friend std::conditional_t<std::is_same<ITER, non_const_reviter>::value, IteratorBase<const_reviter>, void>;

			std::int64_t m_nCountOperInvalidateIter; // the counter that container had, while creating iterator
			const vector* m_pVec;

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
				static inline void RunCheck(const ITER_TYPE& it, const vector& vec)
				{
					AMT_CASSERT(it != ((Base*)&vec)->begin());
				}
			};
			template<typename V>
			struct SAssertNotBegin<true, V>
			{
				template<typename ITER_TYPE>
				static inline void RunCheck(const ITER_TYPE& it, const vector& vec)
				{
					AMT_CASSERT(it != ((Base*)&vec)->rbegin());
				}
			};
			template<typename V>
			struct SAssertNotBegin<2, V>
			{
				template<typename ITER_TYPE>
				static inline void RunCheck(const ITER_TYPE& it, const vector& vec)
				{
					AMT_CASSERT(it != ((Base*)&vec)->crbegin());
				}
			};
			template<char IS_REVERSE_ITER, typename = void>
			struct SAssertNotEnd
			{
				template<typename ITER_TYPE>
				static inline void RunCheck(const ITER_TYPE& it, const vector& vec)
				{
					AMT_CASSERT(it != ((Base*)&vec)->end());
				}
			};
			template<typename V>
			struct SAssertNotEnd<true, V>
			{
				template<typename ITER_TYPE>
				static inline void RunCheck(const ITER_TYPE& it, const vector& vec)
				{
					AMT_CASSERT(it != ((Base*)&vec)->rend());
				}
			};
			template<typename V>
			struct SAssertNotEnd<2, V>
			{
				template<typename ITER_TYPE>
				static inline void RunCheck(const ITER_TYPE& it, const vector& vec)
				{
					AMT_CASSERT(it != ((Base*)&vec)->crend());
				}
			};

		public:

			#if defined(_MSC_VER)		
			using _Prevent_inheriting_unwrap = IteratorBase;
			#endif

			__AMT_FORCEINLINE__ IteratorBase()
			{
				m_pVec = nullptr;
				m_nCountOperInvalidateIter = (size_t) -1;
				#if __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__
				m_nPendingReadRequests = 0;
				m_nPendingWriteRequests = 0;
				#endif
			}		
			__AMT_FORCEINLINE__ IteratorBase(ITER it, const vector* pVec) : ITER()
			{
				*((ITER*)this) = it;
				m_pVec = pVec;
				m_nCountOperInvalidateIter = m_pVec->m_nCountOperInvalidateIter;
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
				m_pVec = o.m_pVec;
				m_nCountOperInvalidateIter = o.m_nCountOperInvalidateIter;
				*((ITER*)this) = *((ITER*)&o);
			}
			// Two extra constructors for const cast actually (so that iterator can be smoothly converted to const_iterator and reverse_iterator to const_reverse_iterator)
			template <class U = ITER>
			IteratorBase(std::enable_if_t<std::is_same<U, const_iter>::value, IteratorBase<non_const_iter> const&> o)
			{
				#if __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__
				m_nPendingReadRequests = 0;
				m_nPendingWriteRequests = 0;	
				CRegisterWritingThread r(*this);
				typename IteratorBase<non_const_iter>::CRegisterReadingThread r2(o);
				#endif
				#if __AMT_CHECK_ITERATORS_VALIDITY__
				o.AssertIsValid();
				#endif
				m_pVec = o.m_pVec;
				m_nCountOperInvalidateIter = o.m_nCountOperInvalidateIter;				
				*((ITER*)this) = *((ITER*)&o);
			}
			template <class U = ITER>
			IteratorBase(std::enable_if_t<std::is_same<U, const_reviter>::value, IteratorBase<non_const_reviter> const&> o)
			{
				#if __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__
				m_nPendingReadRequests = 0;
				m_nPendingWriteRequests = 0;	
				CRegisterWritingThread r(*this);
				typename IteratorBase<non_const_reviter>::CRegisterReadingThread r2(o);
				#endif
				#if __AMT_CHECK_ITERATORS_VALIDITY__
				o.AssertIsValid();
				#endif
				m_pVec = o.m_pVec;
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
					m_pVec = o.m_pVec;
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
					m_pVec = o.m_pVec;
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
					m_pVec = o.m_pVec;
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
					m_pVec = o.m_pVec;
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
				AMT_CASSERT(it1.m_pVec == it2.m_pVec);
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
				AMT_CASSERT(it1.m_pVec == it2.m_pVec);
				#endif
				return *((ITER*)&it1) != *((ITER*)&it2);
			}
			#if 0
			__AMT_FORCEINLINE__ bool operator < (const IteratorBase& o) const
			{
				#if __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__
				CRegisterReadingThread r(*this);
				CRegisterReadingThread r2(o);
				#endif
				#if __AMT_CHECK_ITERATORS_VALIDITY__
				this->AssertIsValid();
				o.AssertIsValid();				
				AMT_CASSERT(m_pVec == o.m_pVec);
				#endif
				return *((ITER*)this) < *((ITER*)&o);
			}
			#endif
			__AMT_FORCEINLINE__ ~IteratorBase() __AMT_CAN_THROW__
			{
				#if __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__
				CRegisterWritingThread r(*this); // not necessarily wrapped up in #if __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__
				AMT_CASSERT(m_nPendingReadRequests == 0);
				AMT_CASSERT(m_nPendingWriteRequests == 1);
				#endif
			}

			auto base() const
			{
				if constexpr (std::is_same<ITER, typename Base::reverse_iterator>::value)
				{
					return iterator(ITER::base(), m_pVec);
				}
				else if constexpr (std::is_same<ITER, typename Base::const_reverse_iterator>::value)
				{
					return const_iterator(ITER::base(), m_pVec);
				}
				else
					static_assert(false, "Cannot use base() for this iterator type");
			}

			__AMT_FORCEINLINE__ bool IsIteratorValid() const
			{
				#if __AMT_CHECK_ITERATORS_VALIDITY__
				return m_nCountOperInvalidateIter == m_pVec->m_nCountOperInvalidateIter; // good enough
				#else
				return true;
				#endif
			}
			__AMT_FORCEINLINE__ void AssertIsValid(const vector* pVec = nullptr) const
			{
				AMT_CASSERT(m_pVec != nullptr);
				AMT_CASSERT(m_pVec == pVec || pVec == nullptr); // passing vector is not mandatory but lets make sure that iterator is not used versus wrong object
				#if __AMT_CHECK_ITERATORS_VALIDITY__
				AMT_CASSERT(IsIteratorValid());
				#endif
			}
			__AMT_FORCEINLINE__ void AssertNotBegin() const
			{			
				SAssertNotBegin<IsRevIter<Base, ITER>::value + IsConstRevIter<Base, ITER>::value>::template RunCheck<ITER>(*((ITER*)this), *m_pVec);
			}
			__AMT_FORCEINLINE__ void AssertNotEnd() const
			{
				SAssertNotEnd<IsRevIter<Base, ITER>::value + IsConstRevIter<Base, ITER>::value>::template RunCheck<ITER>(*((ITER*)this), *m_pVec);
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
				auto it = *this; // copy constructor verifies thread synchronization (CRegisterReadingThread/CRegisterWritingThread), so it should be done earlier to avoid false assertion failure
				#if __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__
				CRegisterWritingThread r(*this);
				#endif
				#if __AMT_CHECK_ITERATORS_VALIDITY__
				AssertIsValid();
				AssertNotEnd();
				#endif				
				((ITER*)this)->operator++();
				return it;
			}
			__AMT_FORCEINLINE__ IteratorBase operator--(int) // postfix operator
			{
				auto it = *this; // copy constructor verifies thread synchronization (CRegisterReadingThread/CRegisterWritingThread), so it should be done earlier to avoid false assertion failure
				#if __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__
				CRegisterWritingThread r(*this);
				#endif
				#if __AMT_CHECK_ITERATORS_VALIDITY__
				AssertIsValid();
				AssertNotBegin();
				#endif				
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

			IteratorBase operator+(const difference_type n) const __AMT_NOEXCEPT__
			{
				#if __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__
				CRegisterWritingThread r(*this);
				#endif
				#if __AMT_CHECK_ITERATORS_VALIDITY__
				AssertIsValid();
				AssertNotEnd();
				#endif
				auto baseIter = ((ITER*)this)->operator+(n);
				IteratorBase resIter(baseIter, m_pVec);
				return resIter;
			}
			IteratorBase operator-(const difference_type n) const __AMT_NOEXCEPT__
			{
				#if __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__
				CRegisterWritingThread r(*this);
				#endif
				#if __AMT_CHECK_ITERATORS_VALIDITY__
				AssertIsValid();
				AssertNotEnd();
				#endif
				auto baseIter = ((ITER*)this)->operator-(n);
				IteratorBase resIter(baseIter, m_pVec);
				return resIter;
			}
			IteratorBase& operator+=(const difference_type n) __AMT_NOEXCEPT__
			{
				#if __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__
				CRegisterWritingThread r(*this);
				#endif
				#if __AMT_CHECK_ITERATORS_VALIDITY__
				AssertIsValid();
				AssertNotEnd();
				#endif
				((ITER*)this)->operator+=(n);
				return *this;
			}
			IteratorBase& operator-=(const difference_type n) __AMT_NOEXCEPT__
			{
				#if __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__
				CRegisterWritingThread r(*this);
				#endif
				#if __AMT_CHECK_ITERATORS_VALIDITY__
				AssertIsValid();
				//AssertNotEnd(); // commented out - allow for some arithmetics on end iterator
				#endif
				((ITER*)this)->operator-=(n); 
				return *this;
			}
			difference_type operator-(const IteratorBase& o) const __AMT_NOEXCEPT__
			{
				#if __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__
				CRegisterReadingThread r(*this);
				CRegisterReadingThread r2(o);
				#endif
				#if __AMT_CHECK_ITERATORS_VALIDITY__
				AssertIsValid();
				//AssertNotEnd(); // commented out - allow for some arithmetics on end iterator
				#endif
				return *((ITER*)this) - *((ITER*)&o);
			}
			T& operator[](const difference_type n) const __AMT_NOEXCEPT__
			{
				#if __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__
				CRegisterReadingThread r(*this);
				#endif
				#if __AMT_CHECK_ITERATORS_VALIDITY__
				AssertIsValid();
				AssertNotEnd();
				#endif
				size_t curIdx = *((ITER*)this) - m_pVec->begin();
				AMT_CASSERT(curIdx + n < m_pVec->size());
				#if __AMT_LET_DESTRUCTORS_THROW__ // this macro means we are in UTs... let the UT complete smoothly in this error condition...
				if (curIdx + n >= m_pVec->size())
				{
					static T val{}; // workaround for UTs...
					return val; 
				}
				#endif
				return ITER::operator[](n);
			}
		};	// end of class IteratorBase

	public:
		using iterator = IteratorBase<typename Base::iterator>;
		using const_iterator = IteratorBase<typename Base::const_iterator>;
		using reverse_iterator = IteratorBase<typename Base::reverse_iterator>;
		using const_reverse_iterator = IteratorBase<typename Base::const_reverse_iterator>;

		inline vector() : Base()
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			Init();
			#endif
		}
		explicit vector(const Alloc& alloc) : Base(alloc)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			Init();
			#endif
		}
		inline vector(const vector& o) : Base(o)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(o);
			Init();			
			CRegisterWritingThread r2(*this);
			#endif
		}
		vector(const vector& o, Alloc& alloc) : Base(o, alloc)
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
		vector(vector&& o, const Alloc& alloc) : Base(o, alloc)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(o);
			Init();
			CRegisterWritingThread r2(*this);
			#endif
		}
		template< class InputIt, std::enable_if_t<amt::is_iterator<InputIt>::value, int> = 0  >
		inline vector(InputIt begin, InputIt end, const Alloc& alloc = Alloc()) : Base(begin, end, alloc)
		{			
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			Init();
			CRegisterWritingThread r2(*this);
			#endif			
		}
		inline vector(std::initializer_list<T> list, const Alloc& alloc = Alloc()) : Base(list, alloc)
		{			
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			Init();			
			CRegisterWritingThread r(*this);			
			#endif	
		}
		inline vector(size_t size) : Base(size)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			Init();
			CRegisterWritingThread r2(*this);
			#endif
		}
		inline vector(size_t size, const T& val, const Alloc& alloc = Alloc()) : Base(size, val, alloc)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			Init();
			CRegisterWritingThread r2(*this);
			#endif
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
			CRegisterReadingThread r(*this); // TODO: it seems debatable how to deal with the non-const version of this method; assuming that vector's data type is also some amt type (like amt::int32_t) there is no problem - potential issue may be detected anyway
			#endif
			return ((Base*)this)->data();
		}
		inline const ValueType* data() const __AMT_NOEXCEPT__
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(*this);
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
		__AMT_FORCEINLINE__ const ValueType& front() const
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterPartiallyReadingThread r(*this);
			#endif
			return ((Base*)this)->front();
		}
		__AMT_FORCEINLINE__ ValueType& front()
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterPartiallyReadingThread r(*this);
			#endif
			return ((Base*)this)->front();
		}

		__AMT_FORCEINLINE__ const ValueType& back() const
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterPartiallyReadingThread r(*this); 
			#endif
			return ((Base*)this)->back();
		}
		__AMT_FORCEINLINE__ ValueType& back()
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterPartiallyReadingThread r(*this); 
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
			auto baseIter = ((Base*)this)->emplace(pos, std::forward<Args>(args)...);
			iterator res(baseIter, this);
			return res;
		}
		iterator insert(const_iterator pos, const ValueType& val)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			#endif
			auto baseIter = ((Base*)this)->insert(pos, val);
			iterator res(baseIter, this);
			return res;
		}
		iterator insert(const_iterator pos, ValueType&& val)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			#endif
			auto baseIter = ((Base*)this)->insert(pos, val);
			iterator res(baseIter, this);
			return res;
		}
		iterator insert(const_iterator pos, size_t n, const ValueType& val)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			#endif
			auto baseIter = ((Base*)this)->insert(pos, n, val);
			iterator res(baseIter, this);
			return res;
		}
		iterator insert(const_iterator pos, std::initializer_list<ValueType> il)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			#endif
			auto baseIter = ((Base*)this)->insert(pos, il);
			iterator res(baseIter, this);
			return res;
		}
		template <class InputIterator, std::enable_if_t<amt::is_iterator<InputIterator>::value, int> = 0 >
		iterator insert(const_iterator pos, InputIterator first, InputIterator last)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			#endif
			auto baseIter = ((Base*)this)->insert(pos, first, last);
			iterator res(baseIter, this);
			return res;
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
			auto baseIter = ((Base*)this)->erase(it);
			iterator resIter(baseIter, this);
			return resIter;
		}
		__AMT_FORCEINLINE__ iterator erase(iterator itFirst, iterator itLast)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			#endif
			auto baseIter = ((Base*)this)->erase(itFirst, itLast);
			iterator resIter(baseIter, this);
			return resIter;
		}

		template <class InputIterator, std::enable_if_t<amt::is_iterator<InputIterator>::value, int> = 0 >
		inline void assign(InputIterator first, InputIterator last)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			#endif
			((Base*)this)->assign(static_cast<typename Base::iterator>(first), static_cast<typename Base::iterator>(last));
		}
		inline void assign(size_t n, const ValueType& val)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			#endif
			((Base*)this)->assign(n, val);
		}
		void assign(std::initializer_list<ValueType> il)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			#endif
			((Base*)this)->assign(il);
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
		vector& operator = (std::initializer_list<T> list)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			#endif
			clear();
			reserve(list.size());
			for (auto it = list.begin(); it != list.end(); ++it)
				push_back(*it);
			return *this;
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

	// TODO: Partial specialization for vector of bool:
	// Probably should internally keep all the bools/bits using amt::vector<amt::uint8_t> and it will make all the checks of multithreaded access conflicts
	template<class Alloc>
	class vector<bool, Alloc> : public std::vector<bool, Alloc>
	{
		// to be done using
		// amt::vector<amt::uint8_t> data_;
		// then n-th element will be located at data_[n/8] & (1 << (n%8))
		// (reference to it might be a bit tricky)
	};

#endif

} // end of namespace amt


