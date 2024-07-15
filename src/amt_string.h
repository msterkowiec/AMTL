#pragma once

#include <string>
#include <atomic>
#include "amt_cassert.h"
#include "amt_types.h"

namespace amt
{

#if !defined(__AMTL_ASSERTS_ARE_ON__)
	using string = std::string;
#else

class string : public std::string
{
	typedef std::string Base;

public:
	using value_type = char;
	using traits_type = std::char_traits<char>;
	using allocator_type = std::allocator<char>;
	using reference = char&;
	using const_reference = const char&;
	using pointer = char*;
	using const_pointer = const char*;
	using difference_type = ptrdiff_t;
	using size_type = size_t;

private:

	// Partial read is a read of size/length only
	// Partial write is a write that does not change the size of the string
	mutable std::atomic<AMTCounterType> m_nPendingReadRequests;
	mutable std::atomic<AMTCounterType> m_nPendingPartialReadRequests;
	mutable std::atomic<AMTCounterType> m_nPendingWriteRequests;
	mutable std::atomic<AMTCounterType> m_nPendingPartialWriteRequests;
	mutable std::atomic<std::uint64_t> m_nCountOperInvalidateIter; // count of operations that invalidate iterators - this single count seems good enough

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
		AMT_CASSERT(m_nPendingWriteRequests == 0 && m_nPendingPartialWriteRequests == 0);
	}
	__AMT_FORCEINLINE__ void UnregisterReadingThread() const
	{
		AMT_CASSERT(m_nPendingWriteRequests == 0 && m_nPendingPartialWriteRequests == 0);
		--m_nPendingReadRequests;
	}
	__AMT_FORCEINLINE__ void RegisterPartiallyReadingThread() const
	{
		++m_nPendingPartialReadRequests;
		AMT_CASSERT(m_nPendingWriteRequests == 0);
	}
	__AMT_FORCEINLINE__ void UnregisterPartiallyReadingThread() const
	{
		AMT_CASSERT(m_nPendingWriteRequests == 0 && m_nPendingPartialWriteRequests == 0); // + ?
		--m_nPendingPartialReadRequests;
	}
	__AMT_FORCEINLINE__ void RegisterWritingThread() const
	{
		++m_nPendingWriteRequests;
		AMT_CASSERT(m_nPendingWriteRequests == 1 && m_nPendingPartialWriteRequests == 0);
		AMT_CASSERT(m_nPendingReadRequests == 0 && m_nPendingPartialReadRequests == 0);
	}
	__AMT_FORCEINLINE__ void UnregisterWritingThread() const
	{
		AMT_CASSERT(m_nPendingWriteRequests == 1 && m_nPendingPartialWriteRequests == 0);
		AMT_CASSERT(m_nPendingReadRequests == 0 && m_nPendingPartialReadRequests == 0);
		--m_nPendingWriteRequests;
	}
	__AMT_FORCEINLINE__ void RegisterPartiallyWritingThread() const
	{
		++m_nPendingPartialWriteRequests;
		AMT_CASSERT(m_nPendingWriteRequests == 0 && m_nPendingPartialWriteRequests == 1);
		AMT_CASSERT(m_nPendingReadRequests == 0);
	}
	__AMT_FORCEINLINE__ void UnregisterPartiallyWritingThread() const
	{
		AMT_CASSERT(m_nPendingWriteRequests == 0 && m_nPendingPartialWriteRequests == 1);
		AMT_CASSERT(m_nPendingReadRequests == 0);
		--m_nPendingPartialWriteRequests;
	}

	class CRegisterReadingThread
	{
		const string& s;

	public:
		inline CRegisterReadingThread(const string& s) : s(s)
		{
			s.RegisterReadingThread();
		}
		inline ~CRegisterReadingThread() __AMT_CAN_THROW__
		{
			s.UnregisterReadingThread();
		}
	};
	class CRegisterPartiallyReadingThread
	{
		const string& s;

	public:
		inline CRegisterPartiallyReadingThread(const string& s) : s(s)
		{
			s.RegisterPartiallyReadingThread();
		}
		inline ~CRegisterPartiallyReadingThread() __AMT_CAN_THROW__
		{
			s.UnregisterPartiallyReadingThread();
		}
	};
	class CRegisterWritingThread
	{
		const string& s;

	public:
		inline CRegisterWritingThread(const string& s) : s(s)
		{
			s.RegisterWritingThread();
		}
		inline ~CRegisterWritingThread() __AMT_CAN_THROW__
		{
			s.UnregisterWritingThread();
		}
	};
	class CRegisterPartiallyWritingThread
	{
		const string& s;

	public:
		inline CRegisterPartiallyWritingThread(const string& s) : s(s)
		{
			s.RegisterPartiallyWritingThread();
		}
		inline ~CRegisterPartiallyWritingThread() __AMT_CAN_THROW__
		{
			s.UnregisterPartiallyWritingThread();
		}
	};

	// ITER can be iterator, const_iterator etc.
	template<class ITER>
	class IteratorBase : public ITER
	{
		std::int64_t m_nCountOperInvalidateIter; // the counter that container had, while creating iterator
		const string* m_pStr;

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
			static inline void RunCheck(const ITER_TYPE& it, const string& str)
			{
				AMT_CASSERT(it != ((Base*)&str)->begin());
			}
		};
		template<typename V>
		struct SAssertNotBegin<true, V>
		{
			template<typename ITER_TYPE>
			static inline void RunCheck(const ITER_TYPE& it, const string& str)
			{
				AMT_CASSERT(it != ((Base*)&str)->rbegin());
			}
		};
		template<typename V>
		struct SAssertNotBegin<2, V>
		{
			template<typename ITER_TYPE>
			static inline void RunCheck(const ITER_TYPE& it, const string& str)
			{
				AMT_CASSERT(it != ((Base*)&str)->crbegin());
			}
		};
		template<char IS_REVERSE_ITER, typename = void>
		struct SAssertNotEnd
		{
			template<typename ITER_TYPE>
			static inline void RunCheck(const ITER_TYPE& it, const string& str)
			{
				AMT_CASSERT(it != ((Base*)&str)->end());
			}
		};
		template<typename V>
		struct SAssertNotEnd<true, V>
		{
			template<typename ITER_TYPE>
			static inline void RunCheck(const ITER_TYPE& it, const string& str)
			{
				AMT_CASSERT(it != ((Base*)&str)->rend());
			}
		};
		template<typename V>
		struct SAssertNotEnd<2, V>
		{
			template<typename ITER_TYPE>
			static inline void RunCheck(const ITER_TYPE& it, const string& str)
			{
				AMT_CASSERT(it != ((Base*)&str)->crend());
			}
		};

	public:
		__AMT_FORCEINLINE__ IteratorBase()
		{
			m_pStr = nullptr;
			m_nCountOperInvalidateIter = (size_t) -1;
			#if __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__
			m_nPendingReadRequests = 0;
			m_nPendingWriteRequests = 0;
			#endif
		}		
		__AMT_FORCEINLINE__ IteratorBase(ITER it, const string* pStr) : ITER()
		{
			*((ITER*)this) = it;
			m_pStr = pStr;
			m_nCountOperInvalidateIter = m_pStr->m_nCountOperInvalidateIter;
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
			m_pStr = o.m_pStr;
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
				m_pStr = o.m_pStr;
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
				m_pStr = o.m_pStr;
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
				m_pStr = o.m_pStr;
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
				m_pStr = o.m_pStr;
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
			AMT_CASSERT(it1.m_pStr == it2.m_pStr);
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
			AMT_CASSERT(it1.m_pStr == it2.m_pStr);
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
			return m_nCountOperInvalidateIter == m_pStr->m_nCountOperInvalidateIter; // good enough
			#else
			return true;
			#endif
		}
		__AMT_FORCEINLINE__ void AssertIsValid(const string* pStr = nullptr) const
		{
			AMT_CASSERT(m_pStr != nullptr);
			AMT_CASSERT(m_pStr == pStr || pStr == nullptr); // passing string is not mandatory but lets make sure that iterator is not used versus wrong object
			#if __AMT_CHECK_ITERATORS_VALIDITY__
			AMT_CASSERT(IsIteratorValid());
			#endif
		}
		__AMT_FORCEINLINE__ void AssertNotBegin() const
		{			
			SAssertNotBegin<IsRevIter<Base, ITER>::value + IsConstRevIter<Base, ITER>::value>::template RunCheck<ITER>(*((ITER*)this), *m_pStr);
		}
		__AMT_FORCEINLINE__ void AssertNotEnd() const
		{
			SAssertNotEnd<IsRevIter<Base, ITER>::value + IsConstRevIter<Base, ITER>::value>::template RunCheck<ITER>(*((ITER*)this), *m_pStr);
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
			IteratorBase resIter(baseIter, m_pStr);
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
			IteratorBase resIter(baseIter, m_pStr);
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
		string& operator[](const difference_type n) const __AMT_NOEXCEPT__
		{
			#if __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__
			CRegisterReadingThread r(*this);
			#endif
			#if __AMT_CHECK_ITERATORS_VALIDITY__
			AssertIsValid();
			AssertNotEnd();
			#endif
			size_t curIdx = *((ITER*)this) - m_pStr->begin();
			AMT_CASSERT(curIdx + n < m_pStr->size());
			#if __AMT_LET_DESTRUCTORS_THROW__ // this macro means we are in UTs... let the UT complete smoothly in this error condition...
			if (curIdx + n >= m_pStr->size())
			{
				static string val{}; // workaround for UTs...
				return val; 
			}
			#endif
			return ITER::operator[](n);
		}

	};		

public:

	using iterator = IteratorBase<typename Base::iterator>;
	using const_iterator = IteratorBase<typename Base::const_iterator>;
	using reverse_iterator = IteratorBase<typename Base::reverse_iterator>;
	using const_reverse_iterator = IteratorBase<typename Base::const_reverse_iterator>;

	string() : Base()
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		Init();
		#endif
	}
	~string()
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterWritingThread r(*this);
		AMT_CASSERT(m_nPendingReadRequests == 0);
		AMT_CASSERT(m_nPendingWriteRequests == 1);
		AMT_CASSERT(m_nPendingPartialReadRequests == 0);
		AMT_CASSERT(m_nPendingPartialWriteRequests == 0);
		#endif
		++m_nCountOperInvalidateIter;
	}
	string(const std::string& o) : Base(o)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		Init();
		#endif
	}
	string(const amt::string& o) : Base(o)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterReadingThread r(o);
		Init();
		CRegisterWritingThread r2(*this);
		#endif		
	}
	string(string&& o) __AMT_NOEXCEPT__
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterWritingThread r(o);
		Init();
		CRegisterWritingThread r2(*this);
		#endif
		*((Base*)this) = std::move(*((Base*)&o));
	}
	string(const char* str) : Base(str)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		Init();
		CRegisterWritingThread r(*this);
		#endif		
	}
	string(const char* str, size_t n) : Base(str, n)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		Init();
		CRegisterWritingThread r(*this);
		#endif		
	}
	string(size_t n, char ch) : Base(n, ch)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		Init();
		CRegisterWritingThread r(*this);
		#endif		
	}
	string(std::initializer_list<char> list)
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
	template< class InputIt, std::enable_if_t<amt::is_iterator<InputIt>::value, int> = 0 >
	inline string(InputIt begin, InputIt end)
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
	string& operator= (const string& o)
	{
		if (&o != this)
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
	string& operator = (string&& o) __AMT_NOEXCEPT__
	{
		if (&o != this)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(o);
			CRegisterWritingThread r2(*this);
			#endif
			++m_nCountOperInvalidateIter;
			*((Base*)this) = std::move(*((Base*)&o));
			++o.m_nCountOperInvalidateIter;
		}
		return *this;
	}
	string& operator= (const char* str)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterWritingThread r2(*this);
		#endif
		++m_nCountOperInvalidateIter;
		* ((Base*)this) = str;
		return *this;
	}
	string& operator= (char ch)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterWritingThread r2(*this);
		#endif
		++m_nCountOperInvalidateIter;
		* ((Base*)this) = ch;
		return *this;
	}
	string& operator = (std::initializer_list<char> list)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterWritingThread r(*this);
		#endif
		++m_nCountOperInvalidateIter;
		clear();
		reserve(list.size());
		for (auto it = list.begin(); it != list.end(); ++it)
			push_back(*it);
		return *this;
	}
	size_t size() const __AMT_NOEXCEPT__
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterPartiallyReadingThread r(*this);
		#endif
		return Base::size();
	}
	size_t length() const __AMT_NOEXCEPT__
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterPartiallyReadingThread r(*this);
		#endif
		return Base::length();
	}
	bool empty() const __AMT_NOEXCEPT__
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterPartiallyReadingThread r(*this);
		#endif
		return Base::empty();
	}
	void clear() __AMT_NOEXCEPT__
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterWritingThread r(*this);
		#endif
		++m_nCountOperInvalidateIter;
		Base::clear();
	}
	void shrink_to_fit()
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterWritingThread r(*this);
		#endif
		++m_nCountOperInvalidateIter;
		Base::shrink_to_fit();
	}
	size_t capacity() const __AMT_NOEXCEPT__
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterPartiallyReadingThread r(*this);
		#endif
		return Base::capacity();
	}
	void reserve(size_t n = 0)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterPartiallyWritingThread r(*this);
		#endif
		++m_nCountOperInvalidateIter;
		Base::reserve(n);
	}
	void resize(size_t n)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterWritingThread r(*this);
		#endif
		++m_nCountOperInvalidateIter;
		Base::resize(n);
	}
	void resize(size_t n, char c)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterWritingThread r(*this);
		#endif
		++m_nCountOperInvalidateIter;
		Base::resize(n, c);
	}
	size_t max_size() const __AMT_NOEXCEPT__
	{
		// no potential multithreading issues here
		return Base::max_size();
	}
	char& operator[] (size_t pos)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterPartiallyWritingThread r(*this); // potential write when using this version of operator []
		#endif
		return ((Base*)this)->operator[](pos);
	}
	const char& operator[] (size_t pos) const
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterReadingThread r(*this);
		#endif
		return ((Base*)this)->operator[](pos);
	}
	char& at(size_t pos)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterPartiallyWritingThread r(*this); // potential write when using this version of operator []
		#endif
		return ((Base*)this)->at(pos);
	}
	const char& at(size_t pos) const
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterReadingThread r(*this);
		#endif
		return ((Base*)this)->at(pos);
	}
	char& back()
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterPartiallyWritingThread r(*this); // potential write when using this version of operator []
		#endif
		return ((Base*)this)->back();
	}
	const char& back() const
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterReadingThread r(*this); 
		#endif
		return ((Base*)this)->back();
	}
	char& front()
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterPartiallyWritingThread r(*this); // potential write when using this version of operator []
		#endif
		return ((Base*)this)->front();
	}
	const char& front() const
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterReadingThread r(*this); 
		#endif
		return ((Base*)this)->front();
	}
	string& append(const string& o)
	{
		if (&o != this)
		{		
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(o); 
			CRegisterWritingThread r2(*this);
			#endif
			++m_nCountOperInvalidateIter;
			((Base*)this)->append(o);
			return *this;
		}
		else
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			#endif
			++m_nCountOperInvalidateIter;
			((Base*)this)->append(o);
			return *this;

		}
	}
	string& append(const string& o, size_t subpos, size_t sublen)
	{
		if (&o != this)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(o); 
			CRegisterWritingThread r2(*this);
			#endif
			++m_nCountOperInvalidateIter;
			((Base*)this)->append(o, subpos, sublen);
			return *this;
		}
		else
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			#endif
			++m_nCountOperInvalidateIter;
			((Base*)this)->append(o, subpos, sublen);
			return *this;

		}
	}
	string& append(const char* str)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterWritingThread r(*this);
		#endif
		++m_nCountOperInvalidateIter;
		((Base*)this)->append(str);
		return *this;
	}
	string& append(const char* str, size_t n)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterWritingThread r(*this);
		#endif
		++m_nCountOperInvalidateIter;
		((Base*)this)->append(str, n);
		return *this;
	}
	string& append(size_t n, char ch)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterWritingThread r(*this);
		#endif
		++m_nCountOperInvalidateIter;
		((Base*)this)->append(n, ch);
		return *this;
	}
	template< class InputIterator, std::enable_if_t<amt::is_iterator<InputIterator>::value, int> = 0 >
	string& append(InputIterator first, InputIterator last)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterWritingThread r(*this);
		#endif
		++m_nCountOperInvalidateIter;
		((Base*)this)->append(first, last);
		return *this;
	}
	string& append(std::initializer_list<char> il)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterWritingThread r(*this);
		#endif
		++m_nCountOperInvalidateIter;
		((Base*)this)->append(il);
		return *this;
	}
	string& operator+= (const string& o)
	{
		if (&o != this)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(o); 
			CRegisterWritingThread r2(*this);
			#endif
			++m_nCountOperInvalidateIter;
			((Base*)this)->append(o);
			return *this;
		}
		else
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			#endif
			++m_nCountOperInvalidateIter;
			((Base*)this)->append(o);
			return *this;
		}
	}
	string& operator+= (const char* str)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterWritingThread r(*this);
		#endif
		++m_nCountOperInvalidateIter;
		((Base*)this)->append(str);
		return *this;
	}
	string& operator+= (char ch)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterWritingThread r(*this);
		#endif
		++m_nCountOperInvalidateIter;
		*((Base*)this) += ch;
		return *this;
	}
	string& operator+=(std::initializer_list<char> il)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterWritingThread r(*this);
		#endif
		++m_nCountOperInvalidateIter;
		((Base*)this)->append(il);
		return *this;
	}
	void push_back(char ch)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterWritingThread r(*this);
		#endif
		++m_nCountOperInvalidateIter;
		((Base*)this)->push_back(ch);
	}
	string& assign(const string& o)
	{
		if (&o != this)
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
	string& assign(string&& o) __AMT_NOEXCEPT__
	{
		if (&o != this)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(o);
			CRegisterWritingThread r2(*this);
			#endif
			++m_nCountOperInvalidateIter;
			*((Base*)this) = std::move(*((Base*)&o));
		}
		return *this;
	}
	string& assign(const string& o, size_t subpos, size_t sublen)
	{
		if (&o != this)
		{ 
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(o);
			CRegisterWritingThread r2(*this);
			#endif
			++m_nCountOperInvalidateIter;
			((Base*)this)->assign(o, subpos, sublen);
			return *this;
		}
		else
		{
			// TODO: make sure this version is ok
			#if __AMT_CHECK_MULTITHREADED_ISSUES__				
			CRegisterWritingThread r(*this);
			#endif
			++m_nCountOperInvalidateIter;
			((Base*)this)->assign(o, subpos, sublen);
			return *this;
		}
	}
	string& assign (const char* str)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterWritingThread r2(*this);
		#endif
		++m_nCountOperInvalidateIter;
		((Base*)this)->assign(str);
		return *this;
	}
	string& assign (const char* str, size_t n)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterWritingThread r2(*this);
		#endif
		++m_nCountOperInvalidateIter;
		((Base*)this)->assign(str, n);
		return *this;
	}
	string& assign (size_t n, char ch)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterWritingThread r2(*this);
		#endif
		++m_nCountOperInvalidateIter;
		((Base*)this)->assign(n, ch);
		return *this;
	}
	template< class InputIterator, std::enable_if_t<amt::is_iterator<InputIterator>::value, int> = 0 >
	string& assign(InputIterator first, InputIterator last)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterWritingThread r2(*this);
		#endif
		++m_nCountOperInvalidateIter;
		((Base*)this)->assign(first, last);
		return *this;
	}
	string& assign(std::initializer_list<char> il)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterWritingThread r2(*this);
		#endif
		++m_nCountOperInvalidateIter;
		((Base*)this)->assign(il);
		return *this;
	}
	string& insert(size_t pos, const string& o)
	{
		if (&o != this)
		{ 
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(o);
			CRegisterWritingThread r2(*this);
			#endif
			++m_nCountOperInvalidateIter;
			((Base*)this)->insert(pos, o);
			return *this;
		}
		else
		{
			// TODO: make sure this version is ok
			#if __AMT_CHECK_MULTITHREADED_ISSUES__				
			CRegisterWritingThread r(*this);
			#endif
			++m_nCountOperInvalidateIter;
			((Base*)this)->insert(pos, o);
			return *this;
		}
	}
	string& insert(size_t pos, const string& o, size_t subpos, size_t sublen)
	{
		if (&o != this)
		{ 
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterReadingThread r(o);
			CRegisterWritingThread r2(*this);
			#endif
			++m_nCountOperInvalidateIter;
			((Base*)this)->insert(pos, o, subpos, sublen);
			return *this;
		}
		else
		{
			// TODO: make sure this version is ok
			#if __AMT_CHECK_MULTITHREADED_ISSUES__				
			CRegisterWritingThread r(*this);
			#endif
			++m_nCountOperInvalidateIter;
			((Base*)this)->insert(pos, o, subpos, sublen);
			return *this;
		}
	}
	string& insert(size_t pos, const char* str)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterWritingThread r(*this);
		#endif
		++m_nCountOperInvalidateIter;
		((Base*)this)->insert(pos, str);
		return *this;
	}
	string& insert(size_t pos, const char* str, size_t n)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterWritingThread r(*this);
		#endif
		++m_nCountOperInvalidateIter;
		((Base*)this)->insert(pos, str, n);
		return *this;
	}
	string& insert(size_t pos, size_t n, char c)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterWritingThread r(*this);
		#endif
		++m_nCountOperInvalidateIter;
		((Base*)this)->insert(pos, n, c);
		return *this;
	}
	iterator insert(const_iterator p, size_t n, char c)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterWritingThread r(*this);
		#endif
		++m_nCountOperInvalidateIter;
		auto iterBase = ((Base*)this)->insert(p, n, c);
		amt::string::iterator res(iterBase, this);
		return res;
	}
	iterator insert(const_iterator p, char c)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterWritingThread r(*this);
		#endif
		++m_nCountOperInvalidateIter;
		auto iterBase = ((Base*)this)->insert(p, c);
		amt::string::iterator res(iterBase, this);
		return res;
	}
	template< class InputIterator, std::enable_if_t<amt::is_iterator<InputIterator>::value, int> = 0 >
	iterator insert(iterator p, InputIterator first, InputIterator last)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterWritingThread r(*this);
		#endif
		++m_nCountOperInvalidateIter;
		auto iterBase = ((Base*)this)->insert(p, first, last);
		amt::string::iterator res(iterBase, this);
		return res;
	}
	string& insert(const_iterator p, std::initializer_list<char> il)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterWritingThread r(*this);
		#endif
		++m_nCountOperInvalidateIter;
		((Base*)this)->insert(p, il);
		return *this;
	}
	string& erase(size_t pos = 0, size_t len = npos)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterWritingThread r(*this);
		#endif
		++m_nCountOperInvalidateIter;
		((Base*)this)->erase(pos, len);
		return *this;
	}
	iterator erase(const_iterator p)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterWritingThread r(*this);
		#endif
		++m_nCountOperInvalidateIter;
		auto iterBase = ((Base*)this)->erase(p);
		iterator res(iterBase, this);
		return res;
	}
	iterator erase(const_iterator first, const_iterator last)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterWritingThread r(*this);
		#endif
		++m_nCountOperInvalidateIter;
		auto iterBase = ((Base*)this)->erase(first, last);
		iterator res(iterBase, this);
		return res;
	}
	string& replace(size_t pos, size_t len, const string& o)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterWritingThread r(*this);
		CRegisterReadingThread r2(o);
		#endif
		++m_nCountOperInvalidateIter;
		// TODO: if (this == &o)
		((Base*)this)->replace(pos, len, o);
		return *this;
	}
		
	string& replace(const_iterator i1, const_iterator i2, const string& o)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterWritingThread r(*this);
		CRegisterReadingThread r2(o);
		#endif
		++m_nCountOperInvalidateIter;
		// TODO: if (this == &o)
		((Base*)this)->replace(i1, i2, o);
		return *this;		
	}
	string& replace(size_t pos, size_t len, const string& o, size_t subpos, size_t sublen)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterWritingThread r(*this);
		CRegisterReadingThread r2(o);
		#endif
		++m_nCountOperInvalidateIter;
		// TODO: if (this == &o)
		((Base*)this)->replace(pos, len, o, subpos, sublen);
		return *this;
	}
	string& replace(size_t pos, size_t len, const char* s)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterWritingThread r(*this);
		#endif
		++m_nCountOperInvalidateIter;
		((Base*)this)->replace(pos, len, s);
		return *this;
	}
	string& replace(const_iterator i1, const_iterator i2, const char* s)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterWritingThread r(*this);
		#endif
		++m_nCountOperInvalidateIter;
		((Base*)this)->replace(i1, i2, s);
		return *this;
	}
	string& replace(size_t pos, size_t len, const char* s, size_t n)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterWritingThread r(*this);
		#endif
		++m_nCountOperInvalidateIter;
		((Base*)this)->replace(pos, len, s, n);
		return *this;
	}
	string& replace(const_iterator i1, const_iterator i2, const char* s, size_t n)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterWritingThread r(*this);
		#endif
		++m_nCountOperInvalidateIter;
		((Base*)this)->replace(i1, i2, s, n);
		return *this;
	}
	string& replace(size_t pos, size_t len, size_t n, char c)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterWritingThread r(*this);
		#endif
		++m_nCountOperInvalidateIter;
		((Base*)this)->replace(pos, len, n, c);
		return *this;
	}
	string& replace(const_iterator i1, const_iterator i2, size_t n, char c)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterWritingThread r(*this);
		#endif
		++m_nCountOperInvalidateIter;
		((Base*)this)->replace(i1, i2, n, c);
		return *this;
	}
	template< class InputIterator, std::enable_if_t<amt::is_iterator<InputIterator>::value, int> = 0 >
	string& replace(const_iterator i1, const_iterator i2, InputIterator first, InputIterator last)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterWritingThread r(*this);
		// TODO: handle iterators
		#endif
		++m_nCountOperInvalidateIter;
		((Base*)this)->replace(i1, i2, first, last);
		return *this;
	}
	string& replace(const_iterator i1, const_iterator i2, std::initializer_list<char> il)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterWritingThread r(*this);
		// TODO: handle iterators
		#endif
		++m_nCountOperInvalidateIter;
		((Base*)this)->replace(i1, i2, il);
		return *this;
	}
	void swap(string& str)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterWritingThread r(*this);
		CRegisterWritingThread r2(str);
		#endif
		++m_nCountOperInvalidateIter;
		++str.m_nCountOperInvalidateIter;
		((Base*)this)->swap(*(Base*) &str);
	}
	void pop_back()
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterWritingThread r(*this);
		#endif
		++m_nCountOperInvalidateIter;
		((Base*)this)->pop_back();
	}
	const char* c_str() const __AMT_NOEXCEPT__
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterReadingThread r(*this); // it is only in older versions of C++ that c_str() could write or even reallocate
		#endif
		// ++m_nCountOperInvalidateIter; // commented out (see also the comment above)
		return ((Base*)this)->c_str();
	}
	const char* data() const noexcept
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterReadingThread r(*this); 
		#endif
		return ((Base*)this)->c_str();
	}
	size_t copy(char* s, size_t len, size_t pos = 0) const
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterReadingThread r(*this); 
		#endif
		++m_nCountOperInvalidateIter;
		return ((Base*)this)->copy(s, len, pos);
	}
	size_t find(const string& o, size_t pos = 0) const __AMT_NOEXCEPT__
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterReadingThread r(*this); 
		CRegisterReadingThread r2(o);
		#endif
		return ((Base*)this)->find(o, pos);
	}
	size_t find(const char* s, size_t pos = 0) const
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterReadingThread r(*this); 
		#endif
		return ((Base*)this)->find(s, pos);
	}
	size_t find(const char* s, size_t pos, size_type n) const
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterReadingThread r(*this); 
		#endif
		return ((Base*)this)->find(s, pos, n);
	}
	size_t find(char c, size_t pos = 0) const __AMT_NOEXCEPT__
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterReadingThread r(*this); 
		#endif
		return ((Base*)this)->find(c, pos);
	}
	size_t rfind(const string& o, size_t pos = 0) const __AMT_NOEXCEPT__
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterReadingThread r(*this); 
		CRegisterReadingThread r2(o);
		#endif
		return ((Base*)this)->rfind(o, pos);
	}
	size_t rfind(const char* s, size_t pos = 0) const
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterReadingThread r(*this); 
		#endif
		return ((Base*)this)->rfind(s, pos);
	}
	size_t rfind(const char* s, size_t pos, size_type n) const
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterReadingThread r(*this); 
		#endif
		return ((Base*)this)->rfind(s, pos, n);
	}
	size_t rfind(char c, size_t pos = 0) const __AMT_NOEXCEPT__
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterReadingThread r(*this); 
		#endif
		return ((Base*)this)->rfind(c, pos);
	}
	size_t find_first_of(const string& o, size_t pos = 0) const __AMT_NOEXCEPT__
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterReadingThread r(*this); 
		CRegisterReadingThread r2(o);
		#endif
		return ((Base*)this)->find_first_of(o, pos);
	}
	size_t find_first_of(const char* s, size_t pos = 0) const
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterReadingThread r(*this); 
		#endif
		return ((Base*)this)->find_first_of(s, pos);
	}
	size_t find_first_of(const char* s, size_t pos, size_type n) const
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterReadingThread r(*this); 
		#endif
		return ((Base*)this)->find_first_of(s, pos, n);
	}
	size_t find_first_of(char c, size_t pos = 0) const __AMT_NOEXCEPT__
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterReadingThread r(*this); 
		#endif
		return ((Base*)this)->find_first_of(c, pos);
	}
	size_t find_last_of(const string& o, size_t pos = 0) const __AMT_NOEXCEPT__
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterReadingThread r(*this); 
		CRegisterReadingThread r2(o);
		#endif
		return ((Base*)this)->find_last_of(o, pos);
	}
	size_t find_last_of(const char* s, size_t pos = 0) const
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterReadingThread r(*this); 
		#endif
		return ((Base*)this)->find_last_of(s, pos);
	}
	size_t find_last_of(const char* s, size_t pos, size_type n) const
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterReadingThread r(*this); 
		#endif
		return ((Base*)this)->find_last_of(s, pos, n);
	}
	size_t find_last_of(char c, size_t pos = 0) const __AMT_NOEXCEPT__
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterReadingThread r(*this); 
		#endif
		return ((Base*)this)->find_last_of(c, pos);
	}
	size_t find_first_not_of(const string& o, size_t pos = 0) const __AMT_NOEXCEPT__
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterReadingThread r(*this); 
		CRegisterReadingThread r2(o);
		#endif
		return ((Base*)this)->find_first_not_of(o, pos);
	}
	size_t find_first_not_of(const char* s, size_t pos = 0) const
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterReadingThread r(*this); 
		#endif
		return ((Base*)this)->find_first_not_of(s, pos);
	}
	size_t find_first_not_of(const char* s, size_t pos, size_type n) const
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterReadingThread r(*this); 
		#endif
		return ((Base*)this)->find_first_not_of(s, pos, n);
	}
	size_t find_first_not_of(char c, size_t pos = 0) const __AMT_NOEXCEPT__
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterReadingThread r(*this); 
		#endif
		return ((Base*)this)->find_first_not_of(c, pos);
	}
	size_t find_last_not_of(const string& o, size_t pos = 0) const __AMT_NOEXCEPT__
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterReadingThread r(*this); 
		CRegisterReadingThread r2(o);
		#endif
		return ((Base*)this)->find_last_not_of(o, pos);
	}
	size_t find_last_not_of(const char* s, size_t pos = 0) const
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterReadingThread r(*this); 
		#endif
		return ((Base*)this)->find_last_not_of(s, pos);
	}
	size_t find_last_not_of(const char* s, size_t pos, size_type n) const
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterReadingThread r(*this); 
		#endif
		return ((Base*)this)->find_last_not_of(s, pos, n);
	}
	size_t find_last_not_of(char c, size_t pos = 0) const __AMT_NOEXCEPT__
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterReadingThread r(*this); 
		#endif
		return ((Base*)this)->find_last_not_of(c, pos);
	}
	string substr(size_t pos = 0, size_t len = npos) const
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterReadingThread r(*this); 
		#endif
		return ((Base*)this)->substr(pos, len);
	}
	int compare(const string& o) const __AMT_NOEXCEPT__
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterReadingThread r(*this); 
		CRegisterReadingThread r2(o);
		#endif
		return ((Base*)this)->compare(o);
	}
	int compare(size_t pos, size_t len, const string& o) const
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterReadingThread r(*this); 
		CRegisterReadingThread r2(o);
		#endif
		return ((Base*)this)->compare(pos, len, o);
	}
	int compare(size_t pos, size_t len, const string& o, size_t subpos, size_t sublen) const
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterReadingThread r(*this); 
		CRegisterReadingThread r2(o);
		#endif
		return ((Base*)this)->compare(pos, len, o, subpos, sublen);
	}
	int compare(const char* s) const
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterReadingThread r(*this); 
		#endif
		return ((Base*)this)->compare(s);
	}
	int compare(size_t pos, size_t len, const char* s) const
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterReadingThread r(*this); 
		#endif
		return ((Base*)this)->compare(pos, len, s);
	}
	int compare(size_t pos, size_t len, const char* s, size_t n) const
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterReadingThread r(*this); 
		#endif
		return ((Base*)this)->compare(pos, len, s, n);
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

	// TODO: non-member function overloads
};

#endif

} // end of namespace amt
