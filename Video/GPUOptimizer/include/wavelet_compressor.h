#pragma once
#include <vector>

class WaveletCompressor {
public:
    static void compress(const std::vector<float>& input, std::vector<float>& output, int levels);
    static void decompress(const std::vector<float>& input, std::vector<float>& output, int levels);
}; 