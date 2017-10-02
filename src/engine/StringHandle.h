#pragma once

#include <stdint.h>
#include <assert.h>
#include "ObjectHandle.h"

namespace Sasquatch
{
    template<class T>
    class GenericStringHandle : MemoryHandle
    {
        private:
            uint32_t m_length;

            int32_t CalculateHashValue(T* charArray)
            {
                return 0;
            }

        public:
            {
                m_length = 0;
            }

            GenericStringHandle(T* charArray, uint32_t memorySize, uint32_t length) 
                : MemoryHandle(charArray, CalculateHashValue(charArray), memorySize)
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

            GenericStringHandle(const GenericStringHandle<T> &stringHandle)
            {
            }

            uint32_t GetLength() { return m_length; }

            T* GetCString() { return GetObjectPtr(); }
    };

    typedef GenericStringHandle<char> StringHandle;
    typedef GenericStringHandle<wchar_t> WStringHandle;
}
