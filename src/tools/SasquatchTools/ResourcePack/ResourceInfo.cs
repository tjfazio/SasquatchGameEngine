using System;

namespace ResourcePack
{
    internal sealed class ResourceInfo
    {
        public string Name { get; set; }
        public UInt32 UnpackedResourceSize { get; set; }
        public UInt32 PackedResourceSize { get; set; }
    }
}
