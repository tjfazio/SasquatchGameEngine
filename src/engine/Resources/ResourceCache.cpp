#include "ResourceCache.h"
#include "../file.h"
#include "../debug.h"

namespace
{
    using namespace Sasquatch::Debug;
}

namespace Sasquatch { namespace Resources 
{
    using namespace Sasquatch::Debug;

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
        m_maxResourceSize = resourceMemorySize;
        m_resourceMemory = VirtualAlloc(NULL, m_maxResourceSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
        assert(m_resourceMemory != nullptr);
        if (m_resourceMemory == nullptr)
        {
            Log(LogLevel::Critical, L"Unable to allocate memory for resource cache!");            
        }
    }

    void ResourceCache::LoadResourcePacakage(const char *resourcePackagePath)
    {
        Reset();
        Sasquatch::Platform::ReadFile(resourcePackagePath, m_resourceMemory, m_maxResourceSize, LoadResourcePackageCallback);
    }    

    void LoadResourcePackageCallback(ReadFileCallbackArgs callbackArgs)
    {
        if (!callbackArgs.Success)
        {
            // TODO: more descriptive error message
            Log(LogLevel::Error, L"Failed to load resource package");
            return;
        }

        g_ResourceCache.m_loadedMemorySize = (uint32_t)callbackArgs.BytesRead;
        for (uint32_t i = 0; i < g_ResourceCache.m_loadedMemorySize; i++)
        {
            // add header reading logic
        }
        g_ResourceCache.m_isReady = 1;
    }

    ResourceHandle *ResourceCache::GetResource(const char *resourceName)
    {
        return nullptr;
    }
}}