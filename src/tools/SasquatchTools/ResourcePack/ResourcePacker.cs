using System;
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
        
        public ResourcePacker(string inputDirectory, string outputFilePath)
        {
            this.inputDirectory = inputDirectory;
            this.outputFile = File.Open(outputFilePath, FileMode.Create, FileAccess.Write);
            this.tempFile = File.Open(Path.GetTempFileName(), FileMode.OpenOrCreate, FileAccess.ReadWrite);
        }
        
        public void PackFile(string inputFilePath)
        {
            this.tempFile.Position = 0;
            using (FileStream inputFileStream = File.Open(inputFilePath, FileMode.Open, FileAccess.Read))
            {
                inputFileStream.CopyTo(this.tempFile);
            }
            long resourceSize = this.tempFile.Position;

            if (resourceSize > UInt32.MaxValue)
            {
                throw new InvalidOperationException($"Resource {inputFilePath} is larger than maximum size. Resource size: {resourceSize}, Maximum size: {UInt32.MaxValue}");
            }

            this.WriteHeader(GetResourceName(inputFilePath), (UInt32)resourceSize, (UInt32)resourceSize);
            
            this.tempFile.Position = 0;
            this.tempFile.CopyTo(this.outputFile);

            this.outputFile.Flush();
        }

        private void WriteHeader(string resourceName, UInt32 packedResourceSize, UInt32 unpackedResourceSize)
        {
            byte[] magicValue = BitConverter.GetBytes(MagicValue);

            byte[] asciiResourceName = Encoding.ASCII.GetBytes(resourceName);
            byte[] resourceNameLength = BitConverter.GetBytes(resourceName.Length);

            byte[] packedSize = BitConverter.GetBytes(packedResourceSize);
            byte[] unpackedSize = BitConverter.GetBytes(unpackedResourceSize);

            if (!BitConverter.IsLittleEndian)
            {
                magicValue = magicValue.Reverse().ToArray();
                packedSize = packedSize.Reverse().ToArray();
                unpackedSize = unpackedSize.Reverse().ToArray();
                resourceNameLength = resourceNameLength.Reverse().ToArray();
            }

            this.outputFile.Write(magicValue, 0, magicValue.Length);
            this.outputFile.Write(packedSize, 0, packedSize.Length);
            this.outputFile.Write(unpackedSize, 0, unpackedSize.Length);
            this.outputFile.Write(resourceNameLength, 0, resourceNameLength.Length);

            this.outputFile.Write(asciiResourceName, 0, asciiResourceName.Length);
            // null-terminate string to make it play nice with C strings
            this.outputFile.WriteByte(0);
        }

        private string GetResourceName(string inputFilePath)
        {
            return inputFilePath.Substring(this.inputDirectory.Length).TrimStart('/', '\\');
        }

        public void Dispose()
        {
            this.outputFile?.Flush();
            this.tempFile?.Close();
            this.outputFile?.Close();
        }
    }
}
