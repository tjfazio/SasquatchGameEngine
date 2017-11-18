#include "ResourceCache.h"

#include <Windows.h>

#include "file.h"
#include "debug.h"

namespace
{
    using namespace Sasquatch::Debug;
}

namespace Sasquatch { namespace Resources 
{
    using namespace Sasquatch::Debug;

    ResourceCache *s_resourceCachePtr;

    ResourceCache::ResourceCache()
    {
        Reset();
    }

    void ResourceCache::Reset()
    {
        m_isReady = 0;
        m_nextResourceIndex = 0;
        m_loadedMemorySize = 0;
    }

    void ResourceCache::Initialize(uint32_t resourceMemorySize)
    {
        assert(s_resourceCachePtr == nullptr);

        m_maxResourceSize = resourceMemorySize;
        m_resourceMemory = VirtualAlloc(NULL, m_maxResourceSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
        assert(m_resourceMemory != nullptr);
        if (m_resourceMemory == nullptr)
        {
            Log(LogLevel::Critical,  "Unable to allocate memory for resource cache!");  
            return;
        }

        s_resourceCachePtr = this;
    }

    const uint8_t ResourceHeaderStart[2] = { (uint8_t)0xFE, (uint8_t)0x01 };

    void ResourceCache::LoadResourcePackageCallback(ReadFileCallbackArgs callbackArgs)
    {
        if (!callbackArgs.Success || callbackArgs.BytesRead < 0)
        {
            // TODO: more descriptive error message
            Log(LogLevel::Error, "Failed to load resource package");
            return;
        }

        s_resourceCachePtr->m_loadedMemorySize = (uint32_t)callbackArgs.BytesRead;
        uint8_t *memory = (uint8_t*)s_resourceCachePtr->m_resourceMemory;
        bool startOfBlock = true;
        while (memory < (uint8_t*)s_resourceCachePtr->m_resourceMemory + s_resourceCachePtr->m_loadedMemorySize)
        {
            // add header reading logic
            if (*memory == ResourceHeaderStart[0] && *(memory + 1) == ResourceHeaderStart[1])
            {
                // start of header
                memory += sizeof(ResourceHeaderStart);

                int32_t resourceIndex = *(int32_t *)memory;
                memory += sizeof(int32_t);
                assert(resourceIndex < MaxResourceCount);

                int32_t packedSize = *(int32_t*)memory;
                memory += sizeof(int32_t);

                int32_t unpackedSize = *(int32_t*)memory;
                memory += sizeof(int32_t);

                int32_t resourceNameLength = *(int32_t*)memory;
                memory += sizeof(int32_t);

                const char *resourceNamePtr = (const char *)memory;
                memory += resourceNameLength + 1; // add one to include null byte
                assert(resourceNamePtr[resourceNameLength] == NULL);

                StringHandle name(resourceNamePtr, resourceNameLength + 1, resourceNameLength);
                s_resourceCachePtr->m_resourceCacheHandles[resourceIndex] = ResourceHandle(name, unpackedSize, (void *)memory);
                memory += unpackedSize;
                
                startOfBlock = true;
            }
            else
            {
                // TODO: fix logging
                if (startOfBlock)
                {
                    Log(LogLevel::Error, "Resource memory is corrupted");
                    startOfBlock = false;
                }
                memory++;
            }
        }
        s_resourceCachePtr->m_isReady = 1;
    }

    void ResourceCache::LoadResourcePacakage(const char *resourcePackagePath)
    {
        Reset();
        Sasquatch::Platform::ReadFile(resourcePackagePath, m_resourceMemory, m_maxResourceSize, LoadResourcePackageCallback);
    }    


    ResourceHandle *ResourceCache::GetResource(const char *resourceName)
    {
        return nullptr;
    }
}}