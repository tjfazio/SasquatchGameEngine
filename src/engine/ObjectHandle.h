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
            {
            }

            T* GetObjectPtr() { return m_object; }

            T& GetObjectRef()
            {
                assert(IsValid());
                return *m_object; 
            }
    };
}
