#pragma once

#include <string>
#include <atomic>
#include "amt_cassert.h"
#include "amt_types.h"

namespace amt
{

class string : public std::string
{
	typedef std::string Base;

public:
	typedef Base::iterator iterator; // TODO: create custom iterator
	typedef Base::const_iterator const_iterator; // TODO: create custom const_iterator
	typedef Base::reverse_iterator reverse_iterator; // TODO: create custom reverse_iterator
	typedef Base::const_reverse_iterator const_reverse_iterator; // TODO: create custom const_reverse_iterator

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
		AMT_CASSERT(m_nPendingWriteRequests == 0 && m_nPendingPartialWriteRequests == 0);
	}
	__AMT_FORCEINLINE__ void UnregisterReadingThread() const
	{
		AMT_CASSERT(m_nPendingWriteRequests == 0 && m_nPendingPartialWriteRequests == 0);
		--m_nPendingReadRequests;
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

public:
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
	string& operator = (string&& o) __AMT_NOEXCEPT__
	{
		if (&o != this)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(o);
			CRegisterWritingThread r2(*this);
			#endif
			*((Base*)this) = std::move(*((Base*)&o));
		}
		return *this;
	}
	string& operator= (const char* str)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterWritingThread r2(*this);
		#endif
		* ((Base*)this) = str;
		return *this;
	}
	string& operator= (char ch)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterWritingThread r2(*this);
		#endif
		* ((Base*)this) = ch;
		return *this;
	}
	string& operator = (std::initializer_list<char> list)
	{
		CRegisterWritingThread r(*this);
		clear();
		reserve(list.size());
		for (auto it = list.begin(); it != list.end(); ++it)
			push_back(*it);
		return *this;
	}
	size_t size() const __AMT_NOEXCEPT__
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterReadingThread r(*this);
		#endif
		return Base::size();
	}
	size_t length() const __AMT_NOEXCEPT__
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterReadingThread r(*this);
		#endif
		return Base::length();
	}
	bool empty() const __AMT_NOEXCEPT__
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterReadingThread r(*this);
		#endif
		return Base::empty();
	}
	void clear() __AMT_NOEXCEPT__
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterWritingThread r(*this);
		#endif
		Base::clear();
	}
	void shrink_to_fit()
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterWritingThread r(*this);
		#endif
		Base::shrink_to_fit();
	}
	size_t capacity() const __AMT_NOEXCEPT__
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterReadingThread r(*this);
		#endif
		return Base::capacity();
	}
	void reserve(size_t n = 0)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterWritingThread r(*this);
		#endif
		Base::reserve(n);
	}
	void resize(size_t n)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterWritingThread r(*this);
		#endif
		Base::resize(n);
	}
	void resize(size_t n, char c)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterWritingThread r(*this);
		#endif
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
		CRegisterWritingThread r(*this); // potential write when using this version of operator []
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
		CRegisterWritingThread r(*this); // potential write when using this version of operator []
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
		CRegisterWritingThread r(*this); // potential write when using this version of operator []
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
		CRegisterWritingThread r(*this); // potential write when using this version of operator []
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
			((Base*)this)->append(o);
			return *this;
		}
		else
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			#endif
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
			((Base*)this)->append(o, subpos, sublen);
			return *this;
		}
		else
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			#endif
			((Base*)this)->append(o, subpos, sublen);
			return *this;

		}
	}
	string& append(const char* str)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterWritingThread r(*this);
		#endif
		((Base*)this)->append(str);
		return *this;
	}
	string& append(const char* str, size_t n)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterWritingThread r(*this);
		#endif
		((Base*)this)->append(str, n);
		return *this;
	}
	string& append(size_t n, char ch)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterWritingThread r(*this);
		#endif
		((Base*)this)->append(n, ch);
		return *this;
	}
	template< class InputIterator, std::enable_if_t<amt::is_iterator<InputIterator>::value, int> = 0 >
	string& append(InputIterator first, InputIterator last)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterWritingThread r(*this);
		#endif
		((Base*)this)->append(first, last);
		return *this;
	}
	string& append(std::initializer_list<char> il)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterWritingThread r(*this);
		#endif
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
			((Base*)this)->append(o);
			return *this;
		}
		else
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(*this);
			#endif
			((Base*)this)->append(o);
			return *this;
		}
	}
	string& operator+= (const char* str)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterWritingThread r(*this);
		#endif
		((Base*)this)->append(str);
		return *this;
	}
	string& operator+= (char ch)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterWritingThread r(*this);
		#endif
		*((Base*)this) += ch;
		return *this;
	}
	string& operator+=(std::initializer_list<char> il)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterWritingThread r(*this);
		#endif
		((Base*)this)->append(il);
		return *this;
	}
	void push_back(char ch)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterWritingThread r(*this);
		#endif
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
	string& assign(string&& o) __AMT_NOEXCEPT__
	{
		if (&o != this)
		{
			#if __AMT_CHECK_MULTITHREADED_ISSUES__
			CRegisterWritingThread r(o);
			CRegisterWritingThread r2(*this);
			#endif
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
			((Base*)this)->assign(o, subpos, sublen);
			return *this;
		}
		else
		{
			// TODO: make sure this version is ok
			#if __AMT_CHECK_MULTITHREADED_ISSUES__				
			CRegisterWritingThread r(*this);
			#endif
			((Base*)this)->assign(o, subpos, sublen);
			return *this;
		}
	}
	string& assign (const char* str)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterWritingThread r2(*this);
		#endif
		((Base*)this)->assign(str);
		return *this;
	}
	string& assign (const char* str, size_t n)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterWritingThread r2(*this);
		#endif
		((Base*)this)->assign(str, n);
		return *this;
	}
	string& assign (size_t n, char ch)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterWritingThread r2(*this);
		#endif
		((Base*)this)->assign(n, ch);
		return *this;
	}
	template< class InputIterator, std::enable_if_t<amt::is_iterator<InputIterator>::value, int> = 0 >
	string& assign(InputIterator first, InputIterator last)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterWritingThread r2(*this);
		#endif
		((Base*)this)->assign(first, last);
		return *this;
	}
	string& assign(std::initializer_list<char> il)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__
		CRegisterWritingThread r2(*this);
		#endif
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
			((Base*)this)->insert(pos, o);
			return *this;
		}
		else
		{
			// TODO: make sure this version is ok
			#if __AMT_CHECK_MULTITHREADED_ISSUES__				
			CRegisterWritingThread r(*this);
			#endif
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
			((Base*)this)->insert(pos, o, subpos, sublen);
			return *this;
		}
		else
		{
			// TODO: make sure this version is ok
			#if __AMT_CHECK_MULTITHREADED_ISSUES__				
			CRegisterWritingThread r(*this);
			#endif
			((Base*)this)->insert(pos, o, subpos, sublen);
			return *this;
		}
	}
	string& insert(size_t pos, const char* str)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterWritingThread r(*this);
		#endif
		((Base*)this)->insert(pos, str);
		return *this;
	}
	string& insert(size_t pos, const char* str, size_t n)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterWritingThread r(*this);
		#endif
		((Base*)this)->insert(pos, str, n);
		return *this;
	}
	string& insert(size_t pos, size_t n, char c)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterWritingThread r(*this);
		#endif
		((Base*)this)->insert(pos, n, c);
		return *this;
	}
	iterator insert(const_iterator p, size_t n, char c)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterWritingThread r(*this);
		#endif
		return ((Base*)this)->insert(p, n, c);
	}
	iterator insert(const_iterator p, char c)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterWritingThread r(*this);
		#endif
		return ((Base*)this)->insert(p, c);
	}
	template< class InputIterator, std::enable_if_t<amt::is_iterator<InputIterator>::value, int> = 0 >
	iterator insert(iterator p, InputIterator first, InputIterator last)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterWritingThread r(*this);
		#endif
		return ((Base*)this)->insert(p, first, last);
	}
	string& insert(const_iterator p, std::initializer_list<char> il)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterWritingThread r(*this);
		#endif
		((Base*)this)->insert(p, il);
		return *this;
	}
	string& erase(size_t pos = 0, size_t len = npos)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterWritingThread r(*this);
		#endif
		((Base*)this)->erase(pos, len);
		return *this;
	}
	iterator erase(const_iterator p)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterWritingThread r(*this);
		#endif
		return ((Base*)this)->erase(p);
	}
	iterator erase(const_iterator first, const_iterator last)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterWritingThread r(*this);
		#endif
		return ((Base*)this)->erase(first, last);
	}
	string& replace(size_t pos, size_t len, const string& o)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterWritingThread r(*this);
		CRegisterReadingThread r2(o);
		#endif
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
		// TODO: if (this == &o)
		((Base*)this)->replace(pos, len, o, subpos, sublen);
		return *this;
	}
	string& replace(size_t pos, size_t len, const char* s)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterWritingThread r(*this);
		#endif
		((Base*)this)->replace(pos, len, s);
		return *this;
	}
	string& replace(const_iterator i1, const_iterator i2, const char* s)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterWritingThread r(*this);
		#endif
		((Base*)this)->replace(i1, i2, s);
		return *this;
	}
	string& replace(size_t pos, size_t len, const char* s, size_t n)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterWritingThread r(*this);
		#endif
		((Base*)this)->replace(pos, len, s, n);
		return *this;
	}
	string& replace(const_iterator i1, const_iterator i2, const char* s, size_t n)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterWritingThread r(*this);
		#endif
		((Base*)this)->replace(i1, i2, s, n);
		return *this;
	}
	string& replace(size_t pos, size_t len, size_t n, char c)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterWritingThread r(*this);
		#endif
		((Base*)this)->replace(pos, len, n, c);
		return *this;
	}
	string& replace(const_iterator i1, const_iterator i2, size_t n, char c)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterWritingThread r(*this);
		#endif
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
		((Base*)this)->replace(i1, i2, first, last);
		return *this;
	}
	string& replace(const_iterator i1, const_iterator i2, std::initializer_list<char> il)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterWritingThread r(*this);
		// TODO: handle iterators
		#endif
		((Base*)this)->replace(i1, i2, il);
		return *this;
	}
	void swap(string& str)
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterWritingThread r(*this);
		CRegisterWritingThread r2(str);
		#endif
		((Base*)this)->swap(*(Base*) &str);
	}
	void pop_back()
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterWritingThread r(*this);
		#endif
		((Base*)this)->pop_back();
	}
	const char* c_str() const __AMT_NOEXCEPT__
	{
		#if __AMT_CHECK_MULTITHREADED_ISSUES__				
		CRegisterWritingThread r(*this); // this is highly debatable, but in general c_str() can not only add a terminating null character, but also reallocate buffer, if needed
		#endif
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
	// TODO: iterators + non-member function overloads
};

} // end of namespace amt
