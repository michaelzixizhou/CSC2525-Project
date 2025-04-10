#include "compression_encodings.hpp"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <sstream>

// -----------------------------------------------------------------------------
// BitVector Implementation
// -----------------------------------------------------------------------------
void BitVector::push_back(bool bit) {
    // If we are at a new block, add a new 64-bit word.
    if (numBits % 64 == 0)
        data.push_back(0ULL);
    size_t block = numBits / 64;
    size_t offset = numBits % 64; // 0 is most significant in our 64-bit word.
    if (bit) {
        // Set bit at (63 - offset) in big-endian order.
        data[block] |= (1ULL << (63 - offset));
    }
    numBits++;
}

size_t BitVector::size() const {
    return numBits;
}

void BitVector::build_rank_metadata() {
    size_t blocks = data.size();
    rank_table.resize(blocks);
    size_t sum = 0;
    for (size_t i = 0; i < blocks; ++i) {
        rank_table[i] = sum;
        // Use built-in popcount for 64-bit word.
        sum += __builtin_popcountll(data[i]);
    }
}

size_t BitVector::rank(size_t pos) const {
    assert(pos <= numBits);
    size_t block = pos / 64;
    size_t offset = pos % 64;
    size_t r = (block < rank_table.size() ? rank_table[block] : 0);
    if (block < data.size()) {
        // Create mask: count bits from the start of the block up to offset.
        uint64_t mask = (1ULL << (64 - offset)) - 1;
        r += __builtin_popcountll(data[block] & mask);
    }
    return r;
}

size_t BitVector::select(size_t i) const {
    // Binary search in rank_table to find the block containing the i-th one.
    size_t left = 0, right = rank_table.size();
    while (left < right) {
        size_t mid = (left + right) / 2;
        // Calculate ones up to the end of block mid.
        size_t ones = (mid < rank_table.size() ? rank_table[mid] : 0);
        if (ones <= i)
            left = mid + 1;
        else
            right = mid;
    }
    size_t block = left - 1;
    size_t offset_in_block = i - rank_table[block];
    uint64_t word = data[block];
    // Scan bits in the word in big-endian order.
    for (size_t j = 0; j < 64; j++) {
        if ((word >> (63 - j)) & 1ULL) {
            if (offset_in_block == 0)
                return block * 64 + j;
            offset_in_block--;
        }
    }
    assert(false && "select: Not found");
    return 0;
}

// Helper to get a bit at a given index using our get logic.
bool BitVector::get(size_t index) const {
    assert(index < numBits);
    size_t block = index / 64;
    size_t offset = index % 64;
    return (data[block] >> (63 - offset)) & 1ULL;
}

std::string BitVector::to_string() const {
    std::ostringstream oss;
    for (size_t i = 0; i < numBits; ++i)
        oss << (get(i) ? '1' : '0');
    return oss.str();
}

// -----------------------------------------------------------------------------
// EliasFano Implementation
// -----------------------------------------------------------------------------
EliasFano::EliasFano(const std::vector<uint64_t> &values) {
    n = values.size();
    if (n == 0) {
        u = 0;
        lower_bits = 0;
        return;
    }
    u = values.back();
    if (n > 0 && u / n > 0)
        lower_bits =
            static_cast<uint32_t>(std::floor(std::log2(u / double(n))));
    else
        lower_bits = 0;

    lower.resize(n);
    uint64_t U = (u >> lower_bits) + n;
    // Reserve space for upper bits.
    upper.data.resize((U + 63) / 64, 0ULL);
    upper.numBits = U;

    for (size_t i = 0; i < n; ++i) {
        uint64_t x = values[i];
        uint32_t low = static_cast<uint32_t>(x & ((1ULL << lower_bits) - 1));
        lower[i] = low;
        uint64_t high = x >> lower_bits;
        size_t pos = high + i;
        // Set the bit in the upper BitVector.
        size_t block = pos / 64;
        size_t offset = pos % 64;
        upper.data[block] |= (1ULL << (63 - offset));
    }
    // After construction, build metadata for constant-time queries.
    upper.build_rank_metadata();
}

std::vector<uint64_t> EliasFano::decode() const {
    std::vector<uint64_t> output(n);
    for (size_t i = 0; i < n; ++i) {
        size_t pos = upper.select(i);
        uint64_t high = pos - i;
        output[i] = (high << lower_bits) | lower[i];
    }
    return output;
}

uint64_t EliasFano::access(size_t i) const {
    assert(i < n);
    size_t pos = upper.select(i);
    uint64_t high = pos - i;
    return (high << lower_bits) | lower[i];
}

size_t EliasFano::size_in_bits() const {
    return n * lower_bits + upper.size();
}

// -----------------------------------------------------------------------------
// GolombDelta Implementation
// -----------------------------------------------------------------------------
uint64_t GolombDelta::compute_M(const std::vector<uint64_t> &gaps) {
    uint64_t sum = 0;
    for (uint64_t g : gaps)
        sum += g;
    double avg = (gaps.empty() ? 1.0 : sum / static_cast<double>(gaps.size()));
    return std::max(uint64_t(1), static_cast<uint64_t>(std::ceil(avg)));
}

GolombDelta::GolombDelta(const std::vector<uint64_t> &values) {
    if (values.empty()) {
        M = 1;
        return;
    }
    std::vector<uint64_t> gaps;
    gaps.reserve(values.size());
    // First gap: value[0] + 1.
    gaps.push_back(values[0] + 1);
    for (size_t i = 1; i < values.size(); ++i) {
        uint64_t gap = values[i] - values[i - 1];
        gaps.push_back(gap);
    }
    M = compute_M(gaps);
    uint32_t b = (M > 1 ? static_cast<uint32_t>(std::ceil(std::log2(M))) : 1);
    uint64_t threshold = (1ULL << b) - M;

    for (uint64_t gap : gaps) {
        uint64_t q = gap / M;
        uint64_t r = gap % M;
        // Write quotient in unary: q zeros then one.
        for (uint64_t i = 0; i < q; ++i)
            encoded.push_back(false);
        encoded.push_back(true);
        // Write remainder in truncated binary.
        if (r < threshold) {
            for (int i = b - 2; i >= 0; --i)
                encoded.push_back((r >> i) & 1);
        } else {
            r += threshold;
            for (int i = b - 1; i >= 0; --i)
                encoded.push_back((r >> i) & 1);
        }
    }
}

std::vector<uint64_t> GolombDelta::decode(size_t numValues) const {
    std::vector<uint64_t> gaps;
    size_t pos = 0;
    uint32_t b = (M > 1 ? static_cast<uint32_t>(std::ceil(std::log2(M))) : 1);
    uint64_t threshold = (1ULL << b) - M;
    while (pos < encoded.size() && gaps.size() < numValues) {
        uint64_t q = 0;
        while (pos < encoded.size() && !encoded.get(pos)) {
            ++q;
            ++pos;
        }
        if (pos < encoded.size())
            ++pos; // Skip the terminating one.
        uint64_t r = 0;
        if (b - 1 > 0 && pos + (b - 1) <= encoded.size()) {
            for (uint32_t i = 0; i < b - 1; ++i) {
                r = (r << 1) | (encoded.get(pos + i) ? 1ULL : 0ULL);
            }
            if (r < threshold) {
                pos += (b - 1);
                gaps.push_back(q * M + r);
                continue;
            }
        }
        r = 0;
        if (pos + b <= encoded.size()) {
            for (uint32_t i = 0; i < b; ++i) {
                r = (r << 1) | (encoded.get(pos + i) ? 1ULL : 0ULL);
            }
            r -= threshold;
            pos += b;
        }
        gaps.push_back(q * M + r);
    }
    // Reconstruct the original sorted sequence.
    std::vector<uint64_t> values;
    values.reserve(numValues);
    uint64_t last = 0;
    for (size_t i = 0; i < gaps.size(); ++i) {
        if (i == 0)
            last = gaps[i] - 1; // First value encoded as value+1.
        else
            last += gaps[i];
        values.push_back(last);
    }
    return values;
}

uint64_t GolombDelta::access(size_t index) const {
    // Access the index-th value by sequentially decoding gaps until we reach
    // that index.
    size_t pos = 0, count = 0;
    uint32_t b = (M > 1 ? static_cast<uint32_t>(std::ceil(std::log2(M))) : 1);
    uint64_t threshold = (1ULL << b) - M;
    uint64_t value = 0;
    while (pos < encoded.size() && count <= index) {
        uint64_t q = 0;
        while (pos < encoded.size() && !encoded.get(pos)) {
            ++q;
            ++pos;
        }
        if (pos < encoded.size())
            ++pos; // skip the terminating one.
        uint64_t r = 0;
        if (b - 1 > 0 && pos + (b - 1) <= encoded.size()) {
            for (uint32_t i = 0; i < b - 1; ++i)
                r = (r << 1) | (encoded.get(pos + i) ? 1ULL : 0ULL);
            if (r < threshold) {
                pos += (b - 1);
                // For index 0, the value is gap-1.
                uint64_t gap = q * M + r;
                if (count == 0)
                    value = gap - 1;
                else
                    value += gap;
                count++;
                continue;
            }
        }
        r = 0;
        if (pos + b <= encoded.size()) {
            for (uint32_t i = 0; i < b; ++i)
                r = (r << 1) | (encoded.get(pos + i) ? 1ULL : 0ULL);
            r -= threshold;
            pos += b;
        }
        uint64_t gap = q * M + r;
        if (count == 0)
            value = gap - 1;
        else
            value += gap;
        count++;
    }
    return value;
}

size_t GolombDelta::size_in_bits() const {
    return encoded.size();
}
