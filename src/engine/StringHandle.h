#pragma once

#include <stdint.h>
#include <assert.h>

#include "ObjectHandle.h"

namespace Sasquatch
{
    template<class T>
    class GenericStringHandle final : MemoryHandle
    {
        private:
            uint32_t m_length;

            int32_t CalculateHashValue(const T* charArray)
            {
                // TODO: implement
                return 0;
            }

        public:
            GenericStringHandle() : MemoryHandle(nullptr, 0, 0)
            {
                m_length = 0;
            }

            GenericStringHandle(const T* charArray, uint32_t memorySize, uint32_t length) 
                : MemoryHandle((void*)charArray, CalculateHashValue(charArray), memorySize)
            {
                #ifdef SASQUATCH_NONRELEASE
                if (sizeof(T) == sizeof(char))
                {
                    assert(strlen((char const *)*charArray) == length);
                }
                else
                {
                    assert(strlen((char wchar_t *)*charArray) == length);
                }
                #endif
            
                m_length = length; 
            }

            GenericStringHandle(const GenericStringHandle<T> &other)
                : MemoryHandle(other.m_memory, other.m_hashValue, other.m_memorySize),
                m_length(other.m_length)
            {
            }

            inline uint32_t GetLength() { return m_length; }

            inline T* GetCString() { return GetObjectPtr(); }
    };

    typedef GenericStringHandle<char> StringHandle;
    typedef GenericStringHandle<wchar_t> WStringHandle;
}