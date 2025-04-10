#include "compression_encodings.hpp"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <random>
#include <unordered_set>
#include <vector>

// Utility: Generate a sorted bitmap (vector of positions) with given universe
// size and number of set bits.
std::vector<uint64_t> generate_sorted_bitmap(uint64_t universe,
                                             size_t numElements) {
    std::vector<uint64_t> result;
    result.reserve(numElements);
    std::unordered_set<uint64_t> chosen;
    std::mt19937_64 rng(42);
    while (result.size() < numElements) {
        uint64_t candidate = rng() % universe;
        if (chosen.find(candidate) == chosen.end()) {
            chosen.insert(candidate);
            result.push_back(candidate);
        }
    }
    std::sort(result.begin(), result.end());
    return result;
}

// Utility: Measure elapsed time in microseconds for a callable.
template <typename F, typename... Args>
uint64_t measure_time(F func, Args &&...args) {
    auto start = std::chrono::high_resolution_clock::now();
    func(std::forward<Args>(args)...);
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(end - start)
        .count();
}

int main() {
    // Open CSV file for writing results.
    std::ofstream csvFile("results.csv");
    if (!csvFile.is_open()) {
        std::cerr << "Error opening results.csv for writing.\n";
        return 1;
    }
    // Write header.
    csvFile << "Universe,Density,NumElements,Method,EncodedSizeBits,"
               "EncodingTimeUs,CompressionRatio,RandomAccessTimeUs\n";

    // Experiment: vary universe sizes and densities.
    std::vector<uint64_t> universes = {1000000ULL, 10000000ULL};
    std::vector<double> densities = {0.01, 0.05, 0.1};

    for (uint64_t universe : universes) {
        std::cout << "Universe size: " << universe << "\n";
        for (double d : densities) {
            size_t numElements = static_cast<size_t>(universe * d);
            std::cout << "  Density: " << d << " (" << numElements
                      << " elements)\n";
            auto bitmap = generate_sorted_bitmap(universe, numElements);

            // Compute uncompressed size (assume 64 bits per element)
            uint64_t uncompressed_bits = numElements * 64;

            // --- Elias-Fano Encoding ---
            uint64_t t_ef = measure_time([&]() { EliasFano ef(bitmap); });
            EliasFano ef(bitmap);
            auto decodedEF = ef.decode();
            assert(decodedEF == bitmap);
            size_t ef_bits = ef.size_in_bits();
            double ef_ratio = (double)ef_bits / uncompressed_bits * 100.0;
            double ef_savings = 100.0 - ef_ratio;
            std::cout << "    Elias-Fano: Encoded size = " << ef_bits
                      << " bits, encoding time ~ " << t_ef
                      << " us, compression ratio = " << ef_ratio
                      << "%, savings = " << ef_savings << "%\n";

            size_t test_index = numElements / 2;
            uint64_t access_time_ef = measure_time([&]() {
                volatile uint64_t dummy = ef.access(test_index);
                (void)dummy; // prevent optimization
            });
            std::cout << "      Elias-Fano: Random access at index "
                      << test_index << " took ~ " << access_time_ef << " us\n";

            // Write Elias-Fano result to CSV.
            csvFile << universe << "," << d << "," << numElements
                    << ",EliasFano," << ef_bits << "," << t_ef << ","
                    << ef_ratio << "," << access_time_ef << "\n";

            // --- Golomb Delta Encoding ---
            uint64_t t_gd = measure_time([&]() { GolombDelta gd(bitmap); });
            GolombDelta gd(bitmap);
            auto decodedGD = gd.decode(numElements);
            assert(decodedGD == bitmap);
            size_t gd_bits = gd.size_in_bits();
            double gd_ratio = (double)gd_bits / uncompressed_bits * 100.0;
            double gd_savings = 100.0 - gd_ratio;
            std::cout << "    Golomb Delta: Encoded size = " << gd_bits
                      << " bits, encoding time ~ " << t_gd
                      << " us, compression ratio = " << gd_ratio
                      << "%, savings = " << gd_savings << "%\n";

            uint64_t access_time_gd = measure_time([&]() {
                volatile uint64_t dummy = gd.access(test_index);
                (void)dummy;
            });
            std::cout << "      Golomb Delta: Random access at index "
                      << test_index << " took ~ " << access_time_gd << " us\n";

            // Write Golomb Delta results to CSV.
            csvFile << universe << "," << d << "," << numElements
                    << ",GolombDelta," << gd_bits << "," << t_gd << ","
                    << gd_ratio << "," << access_time_gd << "\n";
        }
        std::cout << "\n";
    }

    csvFile.close();
    return 0;
}
