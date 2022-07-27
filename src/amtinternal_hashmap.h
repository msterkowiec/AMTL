//
// Assertive MultiThreading Library
//
//  Copyright Marcin Sterkowiec, Piotr Tracz, 2021. Use, modification and
//  distribution is subject to license (see accompanying file license.txt)
//

#pragma once

#include <stdint.h>
#include <atomic>
#include <thread>
#include <mutex>
#include <deque>
#include <vector>

#include "amt_types.h"
#include "amtinternal_utils.h"
#include "amt_cassert.h"

#if __AMT_IGNORE_UNREGISTERED_SCALARS__
#define AMT_VERIFY_SLOT(ptr) do {if(ptr==nullptr) return;} while(false)
#else
#define AMT_VERIFY_SLOT(ptr) do {AMT_CASSERT(ptr); if(ptr==nullptr) return;} while(false)
#endif

namespace amt
{
	//
	// Singleton AMTCountersHashMap
	//

	class __DLLEXPORT__ AMTCountersHashMap
	{		
		static const size_t HASH_SIZE = 65536 * 16;
		#ifdef _DEBUG
		static const size_t INITIAL_BUCKET_SIZE = 0; // don't initialize/resize in debug mode (it'd be too slow)
		#else
		static const size_t INITIAL_BUCKET_SIZE = 8; // in release mode we want run to be as smooth as possible, so we want to avoid additional memory allocation, at least for some time
		#endif

		struct AMTCounterHashMapElem
		{
			std::uint32_t m_ptrPart; // pointer to instance of scalar type; in 32 bits it's full pointer; in 64 bits it's bits 0-2 and 19-47 (bits 3-18 are in hash, bits 48-63 are not needed); this is to minimize size of data in 64 bits and use hash itself, having still quite a good hash (so bits 0-2 are skipped in hash, since typically data is 8-byte aligned)
			std::atomic<std::uint8_t> m_nPendingReadRequests;
			std::atomic<std::uint8_t> m_nPendingWriteRequests;
			std::atomic<std::uint8_t> m_nSlotUsed;  
			std::atomic<std::uint8_t> m_nSlotWanted; // slot is taken in two steps: if (++m_nSlotWanted == 1), then a thread can increment m_nSlotUsed (all this complication is due to the fact that m_ptrPart==nullptr means nothing because it is only the part of the pointer (rest is in the hash itself)

			__AMT_FORCEINLINE__ AMTCounterHashMapElem()
			{
				m_nPendingReadRequests = 0;
				m_nPendingWriteRequests = 0;
				m_nSlotUsed = 0;
				m_nSlotWanted = 0;
				m_ptrPart = 0;
			}
			__AMT_FORCEINLINE__ AMTCounterHashMapElem(AMTCounterHashMapElem&& o) __AMT_NOEXCEPT__
			{
				AMT_CASSERT(o.m_nSlotWanted.load() == 0); // != would mean missing thread synch - move constr should be called only during reorganization

				if (o.m_nSlotUsed)
				{
					m_ptrPart = o.m_ptrPart;
					m_nPendingReadRequests = o.m_nPendingReadRequests.load();
					m_nPendingWriteRequests = o.m_nPendingWriteRequests.load();
					m_nSlotUsed = o.m_nSlotUsed.load();					
					m_nSlotWanted = 0;
					o.m_nSlotUsed = 0;
				}
				else
				{
					m_ptrPart = 0;
					m_nPendingReadRequests = 0;
					m_nPendingWriteRequests = 0;
					m_nSlotUsed = 0;
					m_nSlotWanted = 0;
				}
			}
		};

		//
		// Structure that is intended to have fully safe multithreaded access even during resize (well, only if it is sure that resize() will be synchronized externally - not to be called from more that one thread at the same time)
		//
		// Note: some tests indicate that std::deque can give similar properties but it is platform and implementation dependent
		//
		template<typename T, size_t SLICE_SIZE>
		class CExpandableSlicedVector
		{
			std::atomic<size_t> m_nSize;
			std::atomic<std::vector<T*>*> m_pVecSlicesPtr;
			std::atomic<std::vector<T*>*> m_pScheduledToDelete;
			std::atomic<unsigned char> m_nRemoveScheduledToDelete;

		public:
			__AMT_FORCEINLINE__ CExpandableSlicedVector()
			{
				m_nSize = 0;
				m_pVecSlicesPtr = nullptr; // new std::vector<std::vector<T>*>();
				m_pScheduledToDelete = nullptr;
				m_nRemoveScheduledToDelete = 0;
			}
			void resize(size_t newSize)
			{
				if (newSize <= m_nSize)
					if (newSize == m_nSize)
						return;
					else
						throw std::runtime_error("Cannot shrink CExpandableSlicedVector");

				std::vector<T*>* pOldVecSlicedPtr = m_pVecSlicesPtr;
				std::vector<T*>* pNewVecSlicesPtr = new std::vector<T*>();
				size_t newVecSize = newSize / SLICE_SIZE + ((newSize % SLICE_SIZE) ? 1 : 0);
				pNewVecSlicesPtr->resize(newVecSize);
				size_t oldVecSize = 0;
				if (pOldVecSlicedPtr)
				{
					oldVecSize = pOldVecSlicedPtr->size();
					memcpy(&(*pNewVecSlicesPtr)[0], &(*pOldVecSlicedPtr)[0], oldVecSize * sizeof(T*));
				}
				for (size_t i = oldVecSize; i < newVecSize; ++i)
					(*pNewVecSlicesPtr)[i] = new T[SLICE_SIZE];
				m_pVecSlicesPtr = pNewVecSlicesPtr;
				m_nSize = newSize;
				if (pOldVecSlicedPtr)
				{
					if (m_pScheduledToDelete.load())
						RemoveScheduledToDelete();
					m_pScheduledToDelete = pOldVecSlicedPtr;
				}
			}
			__AMT_FORCEINLINE__ size_t size() const
			{
				return m_nSize;
			}
			__AMT_FORCEINLINE__ T& operator [] (size_t n)
			{
				AMT_DEBUG_CASSERT(n < m_nSize);
				if (m_pScheduledToDelete.load())
					if (m_pScheduledToDelete.load() != m_pVecSlicesPtr)
						RemoveScheduledToDelete();
				return (*m_pVecSlicesPtr)[n / SLICE_SIZE][n % SLICE_SIZE];
			}
			__AMT_FORCEINLINE__ void RemoveScheduledToDelete()
			{
				if (++m_nRemoveScheduledToDelete == 1)
				{
					if (m_pScheduledToDelete.load())
					{
						delete m_pScheduledToDelete.load();
						m_pScheduledToDelete = nullptr;
					}
				}
				--m_nRemoveScheduledToDelete;
			}
		};


		struct AMTCounterHashMapBucket
		{
			static_assert(sizeof(AMTCounterHashMapElem) == 8, "");
			
			//std::deque<AMTCounterHashMapElem> m_deqBucketElems; // std::deque is an interesting alternative to vector - this makes part of synch code not needed (at the price of slightly more costly access to elements by index)
			CExpandableSlicedVector<AMTCounterHashMapElem, 8> m_deqBucketElems;			
			std::atomic<AMTCounterType> m_nBeingResized; // e.g. true when underlying deque changes size; note that writing to an exisitng element is not considered reorganization (synch on m_nSlotUsed)
			
			__AMT_FORCEINLINE__ std::uint32_t GetPtrRest(void* ptr)
			{
				if (sizeof(size_t) == 4)
					return reinterpret_cast<uintptr_t>(ptr);
				else
					return ((std::uint32_t) (((uintptr_t)ptr) >> 16) & ~7) + (((uintptr_t)ptr) & 7); // bits 8-23 are in hash itself, the last 16 bits are not needed, so we need bits 0-7 and 24-47
			}
			__AMT_FORCEINLINE__ AMTCounterHashMapBucket()
			{
				m_nBeingResized = 0;			
				m_deqBucketElems.resize(INITIAL_BUCKET_SIZE); // let's initialize all at once to avoid slow downs when running - all slots will be marked unused
			}
			__AMT_FORCEINLINE__ void RegisterAddress(void* ptr)
			{
				do
				{
					size_t num = m_deqBucketElems.size();
					for (size_t i = 0; i < num; ++i)
					{
						AMTCounterHashMapElem& slot = m_deqBucketElems[i];
						if (slot.m_nSlotUsed == 0)
						{
							if ((++slot.m_nSlotWanted) == 1) // synch done this way; 1 means slot occupied but not ready to use
							{
								if (slot.m_nSlotUsed == 0)
								{
									slot.m_nPendingReadRequests = 0;
									slot.m_nPendingWriteRequests = 0;
									slot.m_ptrPart = GetPtrRest(ptr);
									++slot.m_nSlotUsed;
									--slot.m_nSlotWanted;
									return;
								}
								else
								{
									AMT_DEBUG_CASSERT(slot.m_nSlotWanted > 0);
									--slot.m_nSlotWanted; // another thread must have done it first, almost at the same time
								}
							}
							else
							{
								AMT_DEBUG_CASSERT(slot.m_nSlotWanted > 0);
								--slot.m_nSlotWanted; // another thread must have done it first, almost at the same time
							}
						}
					}

					// Check if another thread increased size of this bucket meanwhile:
					if (num < m_deqBucketElems.size())
						continue;

					// Increase size of deque:
					size_t oldSize = num;
						
					if ((++m_nBeingResized) == 1)
					{
						if (m_deqBucketElems.size() == oldSize) // no changes by other threads meanwhile?
						{
							size_t newSize = oldSize ? oldSize * 2 : 1;
							m_deqBucketElems.resize(newSize); // can throw bad_alloc!
						}
					}
					--m_nBeingResized;					
				} 
				while (true);
			}
			// Helper method for debugging:
			__AMT_FORCEINLINE__ size_t TryFindInOtherPlace(void* ptr, size_t idxToExclude)
			{
				std::uint32_t ptrRest = GetPtrRest(ptr);
				size_t num = m_deqBucketElems.size(); 
				for (size_t i = 0; i < num; ++i)
					if (i != idxToExclude)
					{
						AMTCounterHashMapElem& slot = m_deqBucketElems[i];
						if (slot.m_ptrPart == ptrRest)
							if (slot.m_nSlotUsed)
								if (slot.m_ptrPart == ptrRest)
									return i;
					}
				return (size_t)-1;
			}
			__AMT_FORCEINLINE__ void UnregisterAddress(void* ptr)
			{
				std::uint32_t ptrRest = GetPtrRest(ptr);

				size_t num = m_deqBucketElems.size(); 
				for (size_t i = 0; i < num; ++i)
				{
					AMTCounterHashMapElem& slot = m_deqBucketElems[i];
					if (slot.m_ptrPart == ptrRest)
						if (slot.m_nSlotUsed)
							if (slot.m_ptrPart == ptrRest)
							{				
								AMT_DEBUG_CASSERT(TryFindInOtherPlace(ptr, i) == (size_t) -1);
								AMT_DEBUG_CASSERT(slot.m_nSlotUsed == 1);
								//AMT_DEBUG_CASSERT(slot.m_nSlotWanted == 0); // commented out... actually in may occur that another thread still didn't decrement its will to use this slot
								slot.m_nSlotUsed = 0; // slot not occupied/used ny more
								return;
							}
				}
				AMT_DEBUG_CASSERT(false);
			}

			__AMT_FORCEINLINE__ AMTCounterHashMapElem* GetReadWriteCounters(void* ptr)
			{
				std::uint32_t ptrRest = GetPtrRest(ptr);
				AMT_DEBUG_CASSERT(sizeof(size_t) == 4 || (((size_t)(ptrRest >> 3)) << 19) + (((size_t)ptrRest)&7) + (((((size_t)ptr) >> 3) & 65535) <<3) == (size_t)ptr);

				size_t num = m_deqBucketElems.size(); 
				for (size_t i = 0; i < num; ++i)
				{
					AMTCounterHashMapElem& slot = m_deqBucketElems[i];
					if (slot.m_ptrPart == ptrRest)
						if (slot.m_nSlotUsed)
							if (slot.m_ptrPart == ptrRest)
							{
								AMT_DEBUG_CASSERT(TryFindInOtherPlace(ptr, i) == (size_t)-1);
								AMT_DEBUG_CASSERT(slot.m_nSlotUsed == 1);
								return &slot;
							}
				}				
				#if !__AMT_IGNORE_UNREGISTERED_SCALARS__
				AMT_DEBUG_CASSERT(false);
				#endif
				return nullptr;
			}
		};

		AMTCounterHashMapBucket m_aHashMap[HASH_SIZE];

		// Let constructor be private - access via GetCounterHashMap()
		AMTCountersHashMap()
		{
		}

		static_assert(HASH_SIZE >= 65536 && HASH_SIZE <= 16 * 1024 * 1024 && NumOfBits<HASH_SIZE>::value == 1, ""); // fow now don't even think of changing it : )		

		__AMT_FORCEINLINE__ std::uint32_t GetHash(void* ptr)
		{
			//size_t res = ((size_t)ptr) & (HASH_SIZE - 1);
			size_t res = (((size_t)ptr) >> 3) & (HASH_SIZE - 1); // lowest 3 bits and higher 29 bits of pointers will be contained in m_ptrPart (this is to minimize size of buckets, taking into consideration that variables are typically 8-byte aligned)
			return (std::uint32_t) res;
		}
		__AMT_FORCEINLINE__ AMTCounterHashMapBucket& GetBucket(void* ptr)
		{
			std::uint32_t hash = GetHash(ptr);
			AMT_DEBUG_CASSERT(hash < HASH_SIZE);
			return m_aHashMap[hash];
		}

	public:
		__AMT_FORCEINLINE__ static AMTCountersHashMap* GetCounterHashMap()
		{
			static volatile bool s_bCreated = false;
			static AMTCountersHashMap* s_pMap = nullptr;
			static std::mutex s_mtx;

			if (s_bCreated)
				return s_pMap;

			std::unique_lock<std::mutex> lock(s_mtx);
			if (s_bCreated)
				return s_pMap;

			s_pMap = new AMTCountersHashMap();
			s_bCreated = true;
			return s_pMap;
		}

		__AMT_FORCEINLINE__ void RegisterAddress(void* ptr)
		{
			AMTCounterHashMapBucket& bucket = GetBucket(ptr);
			bucket.RegisterAddress(ptr);
		}

		__AMT_FORCEINLINE__ void UnregisterAddress(void* ptr)
		{
			AMTCounterHashMapBucket& bucket = GetBucket(ptr);
			bucket.UnregisterAddress(ptr);
		}

		__AMT_FORCEINLINE__ AMTCountersHashMap::AMTCounterHashMapElem* GetReadWriteCounters(void* ptr)
		{
			AMTCounterHashMapBucket& bucket = GetBucket(ptr);
			return bucket.GetReadWriteCounters(ptr);
		}


	};

}

