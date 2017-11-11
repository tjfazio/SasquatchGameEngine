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

    void ResourceCache::LoadResourcePackageCallback(ReadFileCallbackArgs callbackArgs)
    {
        if (!callbackArgs.Success)
        {
            // TODO: more descriptive error message
            Log(LogLevel::Error, "Failed to load resource package");
            return;
        }

        s_resourceCachePtr->m_loadedMemorySize = (uint32_t)callbackArgs.BytesRead;
        for (uint32_t i = 0; i < s_resourceCachePtr->m_loadedMemorySize; i++)
        {
            // add header reading logic
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