//
// Assertive MultiThreading Library
//
//  Copyright Marcin Sterkowiec, Piotr Tracz, 2021. Use, modification and
//  distribution is subject to license (see accompanying file license.txt)
//

#pragma once

#include "amt_cassert.h"

// --------------------------------------------------------------
// Helper class to trace unwanted changes in object's raw data:

#if defined(_DEBUG) || defined(__AMT_RELEASE_WITH_ASSERTS__)
template<typename T>
class TObjectRawDataDebugChecker
{
	unsigned char m_arRawDataCopy[sizeof(T)];
	T* m_pObj;
	size_t byteToExclude; // for now there is possibility to exclude one single byte from check
	std::set<size_t> setBytesToExclude; // used when one byte is not enough

	size_t arByteOrderCanDifferAtPos[3];
	unsigned char numByteOrderCanDifferAtPos;
public:
	inline TObjectRawDataDebugChecker(T* pObj)
	{
		m_pObj = pObj;
		memcpy(m_arRawDataCopy, pObj, sizeof(T));
		byteToExclude = (size_t)-1;
		numByteOrderCanDifferAtPos = 0;
	}
	~TObjectRawDataDebugChecker()
	{
		for (unsigned char i = 0; i < numByteOrderCanDifferAtPos; ++i)
		{
			size_t pos = arByteOrderCanDifferAtPos[i];
			if (m_arRawDataCopy[pos] != ((unsigned char*)m_pObj)[pos])
			{
				unsigned char b = m_arRawDataCopy[pos];
				m_arRawDataCopy[pos] = m_arRawDataCopy[pos + 1];
				m_arRawDataCopy[pos + 1] = b; // if order can differ, and something doesn't match, try with different order of bytes...
			}
		}

		size_t nPosThatDiffers = (size_t)-1;
		if (setBytesToExclude.size())
		{
			for (size_t i = 0; i < sizeof(T); ++i)
				if (setBytesToExclude.find(i) == setBytesToExclude.end())
				{
					bool bDataHasChanged = (m_arRawDataCopy[i] != ((unsigned char*)m_pObj)[i]);
					AMT_CASSERT(!bDataHasChanged);
					if (bDataHasChanged)
						nPosThatDiffers = i;
				}
		}
		else
			if (byteToExclude == (size_t)-1)
			{
				bool bDataHasChanged = memcmp(m_arRawDataCopy, m_pObj, sizeof(T)) != 0;
				AMT_CASSERT(!bDataHasChanged);
				if (bDataHasChanged)
					nPosThatDiffers = GetPosThatDiffers(m_arRawDataCopy, (unsigned char*)m_pObj, sizeof(T));
			}
			else
				if (sizeof(T) > 1)
				{
					if (byteToExclude == 0)
					{
						bool bDataHasChanged = memcmp(&(m_arRawDataCopy[1]), ((unsigned char*)m_pObj) + 1, sizeof(T) - 1) != 0;
						AMT_CASSERT(!bDataHasChanged);
						if (bDataHasChanged)
							nPosThatDiffers = GetPosThatDiffers(m_arRawDataCopy + 1, ((unsigned char*)m_pObj) + 1, sizeof(T) - 1);
					}
					else
						if (byteToExclude == sizeof(T) - 1)
						{
							bool bDataHasChanged = memcmp(m_arRawDataCopy, m_pObj, sizeof(T) - 1) != 0;
							AMT_CASSERT(!bDataHasChanged);
							if (bDataHasChanged)
								nPosThatDiffers = GetPosThatDiffers(m_arRawDataCopy, (unsigned char*)m_pObj, sizeof(T) - 1);
						}
						else
						{
							bool bDataHasChanged = memcmp(m_arRawDataCopy, m_pObj, byteToExclude) != 0;
							AMT_CASSERT(!bDataHasChanged);
							if (bDataHasChanged)
								nPosThatDiffers = GetPosThatDiffers(m_arRawDataCopy, (unsigned char*)m_pObj, byteToExclude);
							else
							{
								bDataHasChanged = memcmp(&(m_arRawDataCopy[byteToExclude + 1]), ((unsigned char*)m_pObj) + byteToExclude + 1, sizeof(T) - byteToExclude - 1) != 0;
								AMT_CASSERT(!bDataHasChanged);
								if (bDataHasChanged)
									nPosThatDiffers = GetPosThatDiffers(&(m_arRawDataCopy[byteToExclude + 1]), ((unsigned char*)m_pObj) + byteToExclude + 1, sizeof(T) - byteToExclude - 1);
							}
						}
				}
	}
	inline void ExcludeByte(size_t num)
	{
		if (byteToExclude != (size_t)-1 && byteToExclude != num)
		{
			setBytesToExclude.insert(byteToExclude);
			setBytesToExclude.insert(num);
		}
		else
			byteToExclude = num;
	}
	// Returns -1 on non-diff:
	inline size_t GetPosThatDiffers(unsigned char* p1, unsigned char* p2, size_t len)
	{
		for (size_t i = 0; i < len; ++i)
			if (*p1++ != *p2++)
				return i;

		return (size_t)-1;
	}
	void ByteOrderCanDifferAtPos(size_t idx)
	{
		AMT_CASSERT(numByteOrderCanDifferAtPos < 3);
		arByteOrderCanDifferAtPos[numByteOrderCanDifferAtPos++] = idx;
	}
};
#else
template<typename T>
class TObjectRawDataDebugChecker
{
public:
	__forceinline TObjectRawDataDebugChecker(T*){} // object to be optimized out in release
	__forceinline void ExcludeByte(size_t) {}
	__forceinline void ByteOrderCanDifferAtPos(size_t){}
};
#endif

