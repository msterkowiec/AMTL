//
// Assertive MultiThreading Library
//
//  Copyright Marcin Sterkowiec, Piotr Tracz, 2021. Use, modification and
//  distribution is subject to license (see accompanying file license.txt)
//

#pragma once

#include <map>
#include <atomic>

#include "amt_cassert.h"
#include "amt_types.h"
#include "amt_pod.h"

namespace amt
{

#if !defined(_DEBUG) && !defined(__AMT_RELEASE_WITH_ASSERTS__)
	template<typename Key, typename T, class Compare = std::less<Key>, class Allocator = std::allocator<std::pair<const Key, T> >, bool PREVENT_AMT_WRAP = false>
	using map = std::map<Key, T, Compare, Allocator>;
#else


	template<typename Key, typename T, class Compare = std::less<Key>, class Allocator = std::allocator<std::pair<const Key, T> >, bool PREVENT_AMT_WRAP = false>
	class map : public std::map<Key, __AMT_TRY_WRAP_MAPPED_TYPE__(Key, T, PREVENT_AMT_WRAP, Allocator), Compare, __AMT_CHANGE_MAP_ALLOCATOR_IF_NEEDED__(Key, T, PREVENT_AMT_WRAP, Allocator)>
	{		
		typedef __AMT_TRY_WRAP_MAPPED_TYPE__(Key, T, PREVENT_AMT_WRAP, Allocator) ValueType;
		typedef std::map<Key, ValueType, Compare, __AMT_CHANGE_MAP_ALLOCATOR_IF_NEEDED__(Key, T, PREVENT_AMT_WRAP, Allocator)> Base;

		mutable std::atomic<AMTCounterType> m_nPendingReadRequests;
		mutable std::atomic<AMTCounterType> m_nPendingWriteRequests;
		mutable std::atomic<std::uint64_t> m_nCountOperInvalidateIter; // count of operations that invalidate iterators - this single count seems good enough

		inline void Init()
		{
			m_nPendingReadRequests = 0;
			m_nPendingWriteRequests = 0;
			m_nCountOperInvalidateIter = 0;
		}
		__FORCEINLINE__ void RegisterReadingThread() const
		{
			++m_nPendingReadRequests;
			AMT_CASSERT(m_nPendingWriteRequests == 0);
		}
		__FORCEINLINE__ void UnregisterReadingThread() const
		{
			AMT_CASSERT(m_nPendingWriteRequests == 0);
			--m_nPendingReadRequests;
		}
		__FORCEINLINE__ void RegisterWritingThread() const
		{
			++m_nPendingWriteRequests;
			AMT_CASSERT(m_nPendingWriteRequests == 1);
			AMT_CASSERT(m_nPendingReadRequests == 0);
		}
		__FORCEINLINE__ void UnregisterWritingThread() const
		{
			AMT_CASSERT(m_nPendingWriteRequests == 1);
			AMT_CASSERT(m_nPendingReadRequests == 0);
			--m_nPendingWriteRequests;
		}

		class CRegisterReadingThread
		{
			const map& m_map;

		public:
			inline CRegisterReadingThread(const map& map) : m_map(map)
			{
				m_map.RegisterReadingThread();
			}
			inline ~CRegisterReadingThread()
			{
				m_map.UnregisterReadingThread();
			}
		};
		class CRegisterWritingThread
		{
			const map& m_map;

		public:
			inline CRegisterWritingThread(const map& map) : m_map(map)
			{
				m_map.RegisterWritingThread();
			}
			inline ~CRegisterWritingThread()
			{
				m_map.UnregisterWritingThread();
			}
		};

		// ITER can be iterator, const_iterator etc.
		template<class ITER>
		class IteratorBase : public ITER
		{
			std::int64_t m_nCountOperInvalidateIter; // the counter that container had, while creating iterator
			const map* m_pMap;

			#if __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__
			mutable std::atomic<AMTCounterType> m_nPendingReadRequests;
			mutable std::atomic<AMTCounterType> m_nPendingWriteRequests;
			#endif

			__FORCEINLINE__ void RegisterReadingThread() const
			{
				#if __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__
				++m_nPendingReadRequests;
				AMT_CASSERT(m_nPendingWriteRequests == 0);
				#endif
			}
			__FORCEINLINE__ void UnregisterReadingThread() const
			{
				#if __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__
				AMT_CASSERT(m_nPendingWriteRequests == 0);
				--m_nPendingReadRequests;
				#endif
			}
			__FORCEINLINE__ void RegisterWritingThread() const
			{
				#if __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__
				++m_nPendingWriteRequests;
				AMT_CASSERT(m_nPendingWriteRequests == 1);
				AMT_CASSERT(m_nPendingReadRequests == 0);
				#endif
			}
			__FORCEINLINE__ void UnregisterWritingThread() const
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
				inline ~CRegisterReadingThread()
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
				inline ~CRegisterWritingThread()
				{
					m_it.UnregisterWritingThread();
				}
			};

			// These helper structures are to work around situations when if constexpr is not available yet:
			template<char IS_REVERSE_ITER = false>
			struct SAssertNotBegin
			{
				template<typename ITER_TYPE>
				static inline void RunCheck(const ITER_TYPE& it, const map& map)
				{
					AMT_CASSERT(it != ((Base*)&map)->begin());
				}
			};
			template<>
			struct SAssertNotBegin<true>
			{
				template<typename ITER_TYPE>
				static inline void RunCheck(const ITER_TYPE& it, const map& map)
				{
					AMT_CASSERT(it != ((Base*)&map)->rbegin());
				}
			};
			template<>
			struct SAssertNotBegin<2>
			{
				template<typename ITER_TYPE>
				static inline void RunCheck(const ITER_TYPE& it, const map& map)
				{
					AMT_CASSERT(it != ((Base*)&map)->crbegin());
				}
			};
			template<char IS_REVERSE_ITER = false>
			struct SAssertNotEnd
			{
				template<typename ITER_TYPE>
				static inline void RunCheck(const ITER_TYPE& it, const map& map)
				{
					AMT_CASSERT(it != ((Base*)&map)->end());
				}
			};
			template<>
			struct SAssertNotEnd<true>
			{
				template<typename ITER_TYPE>
				static inline void RunCheck(const ITER_TYPE& it, const map& map)
				{
					AMT_CASSERT(it != ((Base*)&map)->rend());
				}
			};
			template<>
			struct SAssertNotEnd<2>
			{
				template<typename ITER_TYPE>
				static inline void RunCheck(const ITER_TYPE& it, const map& map)
				{
					AMT_CASSERT(it != ((Base*)&map)->crend());
				}
			};

		public:
			__FORCEINLINE__ IteratorBase()
			{
				m_pMap = nullptr;
				m_nCountOperInvalidateIter = (size_t) -1;
				#if __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__
				m_nPendingReadRequests = 0;
				m_nPendingWriteRequests = 0;
				#endif
			}
			__FORCEINLINE__ IteratorBase(ITER it, const map* pMap) : ITER()
			{
				*((ITER*)this) = it;
				m_pMap = pMap;
				m_nCountOperInvalidateIter = m_pMap->m_nCountOperInvalidateIter;
				#if __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__
				m_nPendingReadRequests = 0;
				m_nPendingWriteRequests = 0;
				#endif
			}
			__FORCEINLINE__ IteratorBase(const IteratorBase& o)
			{
				#if __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__
				m_nPendingReadRequests = 0;
				m_nPendingWriteRequests = 0;
				CRegisterWritingThread r(*this);
				CRegisterReadingThread r2(o);
				#endif
				o.AssertIsValid();
				m_pMap = o.m_pMap;
				m_nCountOperInvalidateIter = o.m_nCountOperInvalidateIter;
				*((ITER*)this) = *((ITER*)&o);
			}
			__FORCEINLINE__ IteratorBase& operator = (const IteratorBase& o)
			{
				#if __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__
				CRegisterWritingThread r(*this);
				CRegisterReadingThread r2(o);
				#endif
				o.AssertIsValid();
				m_pMap = o.m_pMap;
				*((ITER*)this) = *((ITER*)&o);
				m_nCountOperInvalidateIter = o.m_nCountOperInvalidateIter;
				return *this;
			}
			__FORCEINLINE__ friend bool operator == (const IteratorBase& it1, const IteratorBase& it2)
			{
				#if __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__
				CRegisterReadingThread r(it1);
				CRegisterReadingThread r2(it2);
				#endif
				it1.AssertIsValid();
				it2.AssertIsValid();
				AMT_CASSERT(it1.m_pMap == it2.m_pMap);
				return *((ITER*)&it1) == *((ITER*)&it2);
			}
			__FORCEINLINE__ friend bool operator != (const IteratorBase& it1, const IteratorBase& it2)
			{
				#if __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__
				CRegisterReadingThread r(it1);
				CRegisterReadingThread r2(it2);
				#endif
				it1.AssertIsValid();
				it2.AssertIsValid();
				AMT_CASSERT(it1.m_pMap == it2.m_pMap);
				return *((ITER*)&it1) != *((ITER*)&it2);
			}
			__FORCEINLINE__ ~IteratorBase()
			{
				CRegisterWritingThread r(*this); // not necessarily wrapped up in #if __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__
				AMT_CASSERT(m_nPendingReadRequests == 0);
				AMT_CASSERT(m_nPendingWriteRequests == 1);
			}
			__FORCEINLINE__ bool IsIteratorValid() const
			{
				#if __AMT_CHECK_ITERATORS_VALIDITY__
				return m_nCountOperInvalidateIter == m_pMap->m_nCountOperInvalidateIter; // good enough
				#else
				return true;
				#endif
			}
			__FORCEINLINE__ void AssertIsValid(const map* pMap = nullptr) const
			{
				AMT_CASSERT(m_pMap != nullptr);
				AMT_CASSERT(m_pMap == pMap || pMap == nullptr); // passing map is not mandatory but lets make sure that iterator is not used versus wrong object
				AMT_CASSERT(IsIteratorValid());
			}
			__FORCEINLINE__ void AssertNotBegin() const
			{			
				SAssertNotBegin<IsRevIter<Base, ITER>::value + IsConstRevIter<Base, ITER>::value>::RunCheck<ITER>(*((ITER*)this), *m_pMap);
			}
			__FORCEINLINE__ void AssertNotEnd() const
			{
				SAssertNotEnd<IsRevIter<Base, ITER>::value + IsConstRevIter<Base, ITER>::value>::RunCheck<ITER>(*((ITER*)this), *m_pMap);
			}
			__FORCEINLINE__ IteratorBase& operator++()
			{
				CRegisterWritingThread r(*this);
				AssertIsValid();
				AssertNotEnd();
				((ITER*)this)->operator++();
				return *this;
			}
			__FORCEINLINE__ IteratorBase& operator--()
			{
				CRegisterWritingThread r(*this);
				AssertIsValid();
				AssertNotBegin();
				((ITER*)this)->operator--();
				return *this;
			}
			__FORCEINLINE__ IteratorBase operator++(int) // postfix operator
			{
				CRegisterWritingThread r(*this);
				AssertIsValid();
				AssertNotEnd();
				auto it = *this;
				((ITER*)this)->operator++();
				return it;
			}
			__FORCEINLINE__ IteratorBase operator--(int) // postfix operator
			{
				CRegisterWritingThread r(*this);
				AssertIsValid();
				AssertNotBegin();
				auto it = *this;
				((ITER*)this)->operator--();
				return it;
			}		
			
			#if defined(_MSC_VER) && __cplusplus < 201402L
			std::pair<Key, ValueType>* operator ->()
			#else
			auto operator ->()
			#endif
			{
				CRegisterReadingThread r(*this);
				AssertIsValid();
				AssertNotEnd();
				#if defined(_MSC_VER) && __cplusplus < 201402L
				return (std::pair<Key, ValueType>*) ((ITER*)this)->operator->();
				#else
				return ((ITER*)this)->operator->();
				#endif
			}

			#if defined(_MSC_VER) && __cplusplus < 201402L
			std::pair<Key, ValueType>& operator *()
			#else
			auto operator *()
			#endif
			{
				CRegisterReadingThread r(*this);
				AssertIsValid();
				AssertNotEnd();
				#if defined(_MSC_VER) && __cplusplus < 201402L
				return (std::pair<Key, ValueType>&) ((ITER*)this)->operator*();
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

		//typedef typename Base::iterator iterator;
		//typedef typename Base::const_iterator const_iterator;
		//typedef typename Base::reverse_iterator reverse_iterator; 
		//typedef typename Base::const_reverse_iterator const_reverse_iterator;

		inline map() : Base()
		{
			Init();
		}

		inline map(const map& o) : Base(o)
		{
			CRegisterReadingThread r(o);
			Init();
			CRegisterWritingThread r2(*this);
		}
		inline ~map()
		{
			CRegisterWritingThread r(*this);
			++ m_nCountOperInvalidateIter;
			AMT_CASSERT(m_nPendingWriteRequests == 1);
			AMT_CASSERT(m_nPendingReadRequests == 0);
		}

		__FORCEINLINE__ size_t size() const
		{
			CRegisterReadingThread r(*this);			
			return ((Base*)this)->size();
		}
		__FORCEINLINE__ bool empty() const
		{
			CRegisterReadingThread r(*this);
			return ((Base*)this)->empty();
		}
		__FORCEINLINE__ void clear()
		{
			CRegisterWritingThread r(*this);
			++ m_nCountOperInvalidateIter;
			((Base*)this)->clear();
		}

		__FORCEINLINE__ ValueType& operator[](const Key& key)
		{
			if (((Base*)this)->find(key) == ((Base*)this)->end())
			{
				CRegisterWritingThread r(*this);
				++m_nCountOperInvalidateIter;
				return ((Base*)this)->operator[](key);
			}
			else
			{
				CRegisterReadingThread r(*this);				
				return ((Base*)this)->operator[](key);
			}
		}

		__FORCEINLINE__ ValueType& at(const Key& key)
		{
			CRegisterReadingThread r(*this);
			return ((Base*)this)-at(key);
		}
		__FORCEINLINE__ const ValueType& at(const Key& key) const
		{
			CRegisterReadingThread r(*this);
			return ((Base*)this)->at(key);
		}
		__FORCEINLINE__ map& operator = (const map& o)
		{
			CRegisterReadingThread r(o);
			CRegisterWritingThread r2(*this);
			++ m_nCountOperInvalidateIter;
			*((Base*)this) = *((Base*)&o);
			return *this;
		}
		__FORCEINLINE__ friend bool operator == (const map& m1, const map& m2)
		{
			CRegisterReadingThread r(m1);
			CRegisterReadingThread r2(m2);
			return *((Base*)&m1) == *((Base*)&m2);
		}
		__FORCEINLINE__ friend bool operator != (const map& m1, const map& m2)
		{
			CRegisterReadingThread r(m1);
			CRegisterReadingThread r2(m2);
			return *((Base*)&m1) != *((Base*)&m2);
		}
		__FORCEINLINE__ friend bool operator < (const map& m1, const map& m2)
		{
			CRegisterReadingThread r(m1);
			CRegisterReadingThread r2(m2);
			return *((Base*)&m1) < *((Base*)&m2);
		}
		__FORCEINLINE__ friend bool operator <= (const map& m1, const map& m2)
		{
			CRegisterReadingThread r(m1);
			CRegisterReadingThread r2(m2);
			return *((Base*)&m1) <= *((Base*)&m2);
		}
		__FORCEINLINE__ friend bool operator > (const map& m1, const map& m2)
		{
			CRegisterReadingThread r(m1);
			CRegisterReadingThread r2(m2);
			return *((Base*)&m1) > *((Base*)&m2);
		}
		__FORCEINLINE__ friend bool operator >= (const map& m1, const map& m2)
		{
			CRegisterReadingThread r(m1);
			CRegisterReadingThread r2(m2);
			return *((Base*)&m1) >= *((Base*)&m2);
		}

		__FORCEINLINE__ iterator find(const Key& key)
		{
			CRegisterReadingThread r(*this);
			auto baseIt = ((Base*)this)->find(key);
			iterator it(baseIt, this);
			return it;
		}
		__FORCEINLINE__ const_iterator find(const Key& k) const
		{
			CRegisterReadingThread r(*this);
			auto baseIt = ((Base*)this)->find(k);
			iterator it(baseIt, this);
			return it;
		}
		__FORCEINLINE__  size_t count(const Key& k) const
		{
			CRegisterReadingThread r(*this);
			return ((Base*)this)->count(k);
		}
		__FORCEINLINE__ void erase(iterator it)
		{
			CRegisterWritingThread r(*this);
			it.AssertIsValid(this);
			++m_nCountOperInvalidateIter;
			((Base*)this)->erase(it);
		}
		__FORCEINLINE__ size_t erase(const Key& key)
		{
			CRegisterWritingThread r(*this);
			++m_nCountOperInvalidateIter;
			return ((Base*)this)->erase(key);
		}
		__FORCEINLINE__ void erase(iterator first, iterator last)
		{
			CRegisterWritingThread r(*this);
			first.AssertIsValid(this);
			last.AssertIsValid(this);
			++m_nCountOperInvalidateIter;
			((Base*)this)->erase(first, last);
		}

		__FORCEINLINE__ void swap(map& o)
		{
			CRegisterWritingThread r(*this);
			CRegisterWritingThread r2(o);
			++m_nCountOperInvalidateIter;
			++o.m_nCountOperInvalidateIter;
			return ((Base*)this)->swap(*((Base*)&o));
		}
		template< class... Args >
		__FORCEINLINE__ std::pair<iterator, bool> emplace(Args&&... args)
		{
			CRegisterWritingThread r(*this);
			auto resBase = ((Base*)this)->emplace(args...);		
			if (resBase.second)
				++m_nCountOperInvalidateIter;
			std::pair<iterator, bool> res(iterator(resBase.first, this), resBase.second);
			return res;
		}
		template< class... Args >
		__FORCEINLINE__ iterator emplace_hint(const_iterator position, Args&&... args)
		{
			CRegisterWritingThread r(*this);
			position.AssertIsValid();
			auto resBase = ((Base*)this)->emplace_hint(position, args...);
			iterator res(resBase, this);
			return res;
		}
		std::pair<iterator, bool> insert(const std::pair<Key, ValueType>& val)
		{
			CRegisterWritingThread r(*this);
			auto resBase =((Base*)this)->insert(val);
			if (resBase.second)
				++m_nCountOperInvalidateIter;
			std::pair<iterator, bool> res(make_pair(iterator(resBase.first, this), resBase.second));
			return res;
		}
		iterator insert(iterator position, const ValueType& val)
		{
			CRegisterWritingThread r(*this);
			position.AssertIsValid(this);
			++m_nCountOperInvalidateIter;
			auto resBase = ((Base*)this)->insert(position, val);
			iterator res(resBase, this);
			return res;
		}
		template <class InputIterator>
		void insert(InputIterator first, InputIterator last)
		{
			CRegisterWritingThread r(*this);
			first.AssertIsValid(this);
			last.AssertIsValid(this);
			++m_nCountOperInvalidateIter;
			((Base*)this)->insert<InputIterator>(first, last);
		}
		iterator lower_bound(const Key& k)
		{
			CRegisterReadingThread r(*this);
			auto resBase = ((Base*)this)->lower_bound(k);
			iterator res(resBase, this);
			return res;
		}
		const_iterator lower_bound(const Key& k) const
		{
			CRegisterReadingThread r(*this);
			auto resBase = ((Base*)this)->lower_bound(k);
			iterator res(resBase, this);
			return res;
		}
		iterator upper_bound(const Key& k)
		{
			CRegisterReadingThread r(*this);
			auto resBase = ((Base*)this)->upper_bound(k);
			iterator res(resBase, this);
			return res;
		}
		const_iterator upper_bound(const Key& k) const
		{
			CRegisterReadingThread r(*this);
			auto resBase =((Base*)this)->upper_bound(k);
			const_iterator res(resBase, this);
			return res;
		}
		std::pair<const_iterator, const_iterator> equal_range(const Key& k) const
		{
			CRegisterReadingThread r(*this);
			auto resBase = ((Base*)this)->equal_range(k);
			std::pair<const_iterator, const_iterator> res(const_iterator(resBase.first, this), const_iterator(resBase.second, this));
			return res;
		}
		std::pair<iterator, iterator> equal_range(const Key& k)
		{
			CRegisterReadingThread r(*this);
			auto resBase = ((Base*)this)->equal_range(k);
			std::pair<iterator, iterator> res(iterator(resBase.first, this), iterator(resBase.second, this));
			return res;
		}

		// Iterators:
		inline iterator begin()
		{
			CRegisterReadingThread r(*this);
			auto resBase = ((Base*)this)->begin();
			iterator res(resBase, this);
			return res;
		}
		inline const_iterator begin() const
		{
			CRegisterReadingThread r(*this);
			auto resBase = ((Base*)this)->begin();
			const_iterator res(resBase, this);
			return res;
		}
		inline const_iterator cbegin() const __NOEXCEPT__
		{
			CRegisterReadingThread r(*this);
			auto resBase = ((Base*)this)->cbegin();
			const_iterator res(resBase, this);
			return res;
		}
		inline reverse_iterator rbegin()
		{
			CRegisterReadingThread r(*this);
			auto resBase = ((Base*)this)->rbegin();
			reverse_iterator res(resBase, this);
			return res;
		}
		inline const_reverse_iterator rbegin() const
		{
			CRegisterReadingThread r(*this);
			auto resBase = ((Base*)this)->rbegin();
			const_reverse_iterator res(resBase, this);
			return res;
		}
		inline const_reverse_iterator crbegin() const __NOEXCEPT__
		{
			CRegisterReadingThread r(*this);
			auto resBase = ((Base*)this)->crbegin();
			const_reverse_iterator res(resBase, this);
			return res;
		}
		inline iterator end()
		{
			CRegisterReadingThread r(*this);
			auto resBase = ((Base*)this)->end();
			iterator res(resBase, this);
			return res;
		}
		inline const_iterator end() const
		{
			CRegisterReadingThread r(*this);
			auto resBase = ((Base*)this)->end();
			const_iterator res(resBase, this);
			return res;
		}
		inline const_iterator cend() const __NOEXCEPT__
		{
			CRegisterReadingThread r(*this);
			auto resBase = ((Base*)this)->cend();
			const_iterator res(resBase, this);
			return res;
		}
		inline reverse_iterator rend()
		{
			CRegisterReadingThread r(*this);
			auto resBase = ((Base*)this)->rend();
			reverse_iterator res(resBase, this);
			return res;
		}
		inline const_reverse_iterator rend() const
		{
			CRegisterReadingThread r(*this);
			auto resBase = ((Base*)this)->rend();
			const_reverse_iterator res(resBase, this);
			return res;
		}
		inline const_reverse_iterator crend() const __NOEXCEPT__
		{
			CRegisterReadingThread r(*this);
			auto resBase = ((Base*)this)->crend();
			const_reverse_iterator res(resBase, this);
			return res;
		}
	};

#endif

}

