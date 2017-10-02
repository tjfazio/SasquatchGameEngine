using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;

namespace ResourcePack
{
    class Program
    {
        static void Main(string[] args)
        {
            string inputDirectory = ".";
            string outputFilePath = "Resources";
            
            for (int i = 0; i < args.Length - 1; i += 2)
            {
                string key = args[i];
                string value = args[i + 1];
                switch (key)
                {
                    case "--inputDirectory":
                        inputDirectory = value;
                        break;
                    case "--outputFilePath":
                        outputFilePath = value;
                        break;
                }
            }

            using (var packer = new ResourcePacker(inputDirectory, outputFilePath))
            {
                foreach (var filePath in Directory.EnumerateFiles(inputDirectory, "*", SearchOption.AllDirectories))
                {
                    packer.PackFile(filePath);
                }
            }

        }
    }
}
