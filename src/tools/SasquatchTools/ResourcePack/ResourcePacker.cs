using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

namespace ResourcePack
{
    internal sealed class ResourcePacker : IDisposable
    {
        private const ushort MagicValue = 0xFE01;

        private string inputDirectory;
        private FileStream outputFile;
        private FileStream tempFile;

        private readonly List<ResourceInfo> packedResources;

        public ResourcePacker(string inputDirectory, string outputFilePath)
        {
            this.inputDirectory = inputDirectory;
            this.outputFile = File.Open(outputFilePath, FileMode.Create, FileAccess.Write);
            this.tempFile = File.Open(Path.GetTempFileName(), FileMode.OpenOrCreate, FileAccess.ReadWrite);
            this.packedResources = new List<ResourceInfo>();
        }
        
        public void PackFile(string inputFilePath)
        {
            long originalPosition = this.tempFile.Position;
            using (FileStream inputFileStream = File.Open(inputFilePath, FileMode.Open, FileAccess.Read))
            {
                inputFileStream.CopyTo(this.tempFile);
            }
            long resourceSize = this.tempFile.Position - originalPosition;

            if (resourceSize > UInt32.MaxValue)
            {
                throw new InvalidOperationException($"Resource {inputFilePath} is larger than maximum size. Resource size: {resourceSize}, Maximum size: {UInt32.MaxValue}");
            }

            this.packedResources.Add(new ResourceInfo()
            {
                Name = GetResourceName(inputFilePath),
                PackedResourceSize = (UInt32)resourceSize,
                UnpackedResourceSize = (UInt32)resourceSize
            });
        }

        private string GetResourceName(string inputFilePath)
        {
            return inputFilePath.Substring(this.inputDirectory.Length).TrimStart('/', '\\');
        }

        private void FinalizePackage()
        {
            this.tempFile.Position = 0;

            this.WriteHeader();
            
            this.tempFile.CopyTo(this.outputFile);

            this.outputFile.Flush();
        }

        private void WriteHeader()
        {
            for (int i = 0; i < this.packedResources.Count; i++)
            {
                ResourceInfo resource = this.packedResources[i];

                byte[] magicValue = BitConverter.GetBytes(MagicValue);
                byte[] resourceIndex = BitConverter.GetBytes(i);

                byte[] asciiResourceName = Encoding.ASCII.GetBytes(resource.Name);
                byte[] resourceNameLength = BitConverter.GetBytes(resource.Name.Length);

                byte[] packedSize = BitConverter.GetBytes(resource.PackedResourceSize);
                byte[] unpackedSize = BitConverter.GetBytes(resource.UnpackedResourceSize);

                if (!BitConverter.IsLittleEndian)
                {
                    magicValue = magicValue.Reverse().ToArray();
                    resourceIndex = resourceIndex.Reverse().ToArray();
                    packedSize = packedSize.Reverse().ToArray();
                    unpackedSize = unpackedSize.Reverse().ToArray();
                    resourceNameLength = resourceNameLength.Reverse().ToArray();
                }

                this.outputFile.Write(magicValue, 0, magicValue.Length);
                this.outputFile.Write(resourceIndex, 0, resourceIndex.Length);
                this.outputFile.Write(packedSize, 0, packedSize.Length);
                this.outputFile.Write(unpackedSize, 0, unpackedSize.Length);
                this.outputFile.Write(resourceNameLength, 0, resourceNameLength.Length);

                this.outputFile.Write(asciiResourceName, 0, asciiResourceName.Length);
                // null-terminate string to make it play nice with C strings
                this.outputFile.WriteByte(0);
            }
        }

        public void Dispose()
        {
            this.FinalizePackage();
            this.tempFile?.Close();
            this.outputFile?.Close();
        }
    }
}
