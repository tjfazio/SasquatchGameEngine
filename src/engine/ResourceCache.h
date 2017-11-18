#pragma once

#include <stdint.h>

#include "ResourceCache.h"
#include "ResourceHandle.h"
#include "common.h"
#include "file.h"

namespace Sasquatch { namespace Resources
{
    using ReadFileCallbackArgs = Sasquatch::Platform::ReadFileCallbackArgs;

    const int32_t MaxResourceCount = 10000;
    const int32_t MaxResourceMemorySize = GIGABYTES(1);

    struct ResourceCacheNode
    {
        ResourceHandle Resource;
        ResourceCacheNode *Next;
    };

    class ResourceCache
    {
        private:
            uint32_t m_nextResourceIndex;            
            uint32_t m_maxResourceSize;
            uint32_t m_loadedMemorySize;
            int32_t m_isReady;

            ResourceHandle m_resourceCacheHandles[MaxResourceCount];
            ResourceCacheNode m_resourceCacheMap[3 * MaxResourceCount];
            void *m_resourceMemory;

            void Reset();

        public:        
            ResourceCache();

            void Initialize(uint32_t resourceMemorySize);
            
            void LoadResourcePacakage(const char *resourcePackagePath);

            static void LoadResourcePackageCallback(ReadFileCallbackArgs callbackArgs);

            ResourceHandle *GetResource(const char *resourceName);

            bool IsReady() { return m_isReady; }
    };

    extern ResourceCache g_ResourceCache;
}}