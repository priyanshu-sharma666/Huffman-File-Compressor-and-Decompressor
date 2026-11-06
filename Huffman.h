#pragma once

#include <string>
#include <cstdint>

struct CompressionResult {
    bool success = false;
    uint64_t originalSize = 0;
    uint64_t compressedSize = 0;
    double spaceSaved = 0.0;
    std::string outputPath;
    std::string error;
};

// Compresses any file and saves it to outputPath
CompressionResult CompressFile(const std::string& inputPath, const std::string& outputPath);

// Decompresses a .huff file back to its original form
CompressionResult DecompressFile(const std::string& inputPath, const std::string& outputPath);
