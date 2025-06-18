#include <vector>
#include <cstdint>
#include <cmath>

// WaveletCompressor: компрессия и декомпрессия текстур
class WaveletCompressor {
public:
    // Компрессия текстуры (1D float)
    static void compress(const std::vector<float>& input, std::vector<float>& output, int levels) {
        std::vector<float> temp = input;
        int n = temp.size();
        for (int l = 0; l < levels; ++l) {
            int half = n / 2;
            for (int i = 0; i < half; ++i) {
                float avg = (temp[2 * i] + temp[2 * i + 1]) * 0.5f;
                float diff = (temp[2 * i] - temp[2 * i + 1]) * 0.5f;
                output[i] = avg;
                output[half + i] = diff;
            }
            for (int i = 0; i < n; ++i) temp[i] = output[i];
            n /= 2;
        }
    }
    // Декомпрессия текстуры (1D float)
    static void decompress(const std::vector<float>& input, std::vector<float>& output, int levels) {
        int n = input.size() / (1 << (levels - 1));
        output = input;
        for (int l = 0; l < levels; ++l) {
            int half = n / 2;
            for (int i = 0; i < half; ++i) {
                float avg = output[i];
                float diff = output[half + i];
                output[2 * i] = avg + diff;
                output[2 * i + 1] = avg - diff;
            }
            n *= 2;
        }
    }
}; 