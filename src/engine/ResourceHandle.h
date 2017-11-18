#pragma once

#include <stdint.h>
#include <assert.h>

#include "StringHandle.h"

namespace Sasquatch { namespace Resources
{
    class ResourceHandle final
    {
        private:           
            StringHandle m_resourceName;
            uint32_t m_resourceSize;
            void *m_resourceMemory;
        public:
            ResourceHandle() : m_resourceSize(0), m_resourceMemory(nullptr)
            {
            }

            ResourceHandle(StringHandle resourceName, uint32_t resourceSize, void *resourceMemory)
                : m_resourceName(resourceName),
                m_resourceSize(resourceSize),
                m_resourceMemory(resourceMemory)
            {
            }

            ResourceHandle(const ResourceHandle& other)
                : m_resourceName(other.m_resourceName),
                m_resourceSize(other.m_resourceSize),
                m_resourceMemory(other.m_resourceMemory)
            {
            }

            inline StringHandle GetName() { return m_resourceName; }

            inline uint32_t GetSize() { return m_resourceSize; }

            inline void *GetMemory() 
            { 
                assert(IsValid());
                return m_resourceMemory; 
            }
            
            inline bool IsValid() { return m_resourceMemory != nullptr; }
    };
}}