#pragma once

#include <stdint.h>
#include <assert.h>

namespace Sasquatch
{
    class MemoryHandle
    {        
        protected:
            void *m_memory;
            int32_t m_hashValue;
            uint32_t m_memorySize;
        public:
            MemoryHandle(void* memory, int32_t hashValue, uint32_t memorySize)
            {
                m_memory = memory;
                m_hashValue = hashValue;
                m_memorySize = memorySize;
            }
            
            int32_t GetHashValue() { return m_hashValue; }

            uint32_t GetMemorySize() { return m_memorySize; }
            
            bool IsValid() { return m_memory != nullptr; }
    };
}