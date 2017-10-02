#pragma once

#include <stdint.h>
#include "../StringHandle.h"

namespace Sasquatch { namespace Resources
{
    class ResourceHandle
    {
        private:           
            StringHandle m_resourceName;
            uint32_t m_resourceSize;
            void *m_resourceMemory;
        public:
            ResourceHandle()
            {
                m_resourceSize = 0;
                m_resourceMemory = nullptr;
            }

            ResourceHandle(StringHandle resourceName, uint32_t resourceSize, void *resourceMemory)
                : m_resourceName(resourceName)
            {
                m_resourceSize = resourceSize;
                m_resourceMemory = resourceMemory;
            }

            StringHandle GetName() { return m_resourceName; }
            uint32_t GetSize() { return m_resourceSize; }
            void *GetMemory() { return m_resourceMemory; }
            bool IsValid() { return m_resourceMemory != nullptr; }
    };
}}