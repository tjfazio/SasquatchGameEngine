#pragma once

#include <stdint.h>
#include <assert.h>

#include "MemoryHandle.h"

namespace Sasquatch
{
    template<class T>
    class ObjectHandle : MemoryHandle
    {
        public:
            ObjectHandle(const T* object, int32_t hashValue)
                : MemoryHandle(object, hashValue, (uint32_t)sizeof(T))
            {
            }

            inline T* GetObjectPtr() { return m_object; }

            inline T& GetObjectRef()
            {
                assert(IsValid());
                return *m_object; 
            }
    };
}