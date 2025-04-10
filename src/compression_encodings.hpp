#ifndef COMPRESSION_ENCODINGS_HPP
#define COMPRESSION_ENCODINGS_HPP

#include <cstdint>
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
// BitVector: A minimal helper class for bit-level operations.
// -----------------------------------------------------------------------------

class BitVector {
    public:
        std::vector<uint64_t> data;     // Array of 64-bit words.
        size_t numBits;                 // Total number of bits stored.
        std::vector<size_t> rank_table; // rank_table[i] holds the total number
                                        // of ones in blocks [0, i).

        BitVector() : numBits(0) {}

        // Bits are stored in big-endian order within each 64-bit word.
        void push_back(bool bit);

        // Returns the total number of bits.
        size_t size() const;

        // Build the rank metadata table. Must be called after finishing bit
        // appends.
        void build_rank_metadata();

        // Constant time (O(1)) rank: number of 1s in positions [0, pos).
        size_t rank(size_t pos) const;

        // Select the index of the i-th set bit (0-indexed).
        size_t select(size_t i) const;

        // Helper to get a bit at a given index using our get logic.
        bool get(size_t index) const;

        // Convert the BitVector to a string of '0' and '1' for debugging.
        std::string to_string() const;
};

// -----------------------------------------------------------------------------
// EliasFano: Class for Elias-Fano encoding of a sorted vector of 64-bit
// integers.
// -----------------------------------------------------------------------------
class EliasFano {
    public:
        uint64_t n;          // Number of values
        uint64_t u;          // Maximum value (universe)
        uint32_t lower_bits; // Number of lower bits stored explicitly

        // Encoded components (lower bits and upper bits)
        std::vector<uint32_t> lower;
        BitVector upper;

        // Constructor: encodes the sorted vector "values"
        EliasFano(const std::vector<uint64_t> &values);

        // Decode the encoded sequence, returning the original sorted vector.
        std::vector<uint64_t> decode() const;

        // Constant time random access: return the i-th value.
        uint64_t access(size_t i) const;

        // Returns the total encoded size in bits.
        size_t size_in_bits() const;
};

// -----------------------------------------------------------------------------
// GolombDelta: Class for Golomb-delta encoding of a sorted vector (bitmap)
// -----------------------------------------------------------------------------
class GolombDelta {
    public:
        uint64_t M;        // Golomb parameter (divisor)
        BitVector encoded; // Encoded output bits

        // Constructor: encode the sorted vector "values" using Golomb-delta
        // coding.
        GolombDelta(const std::vector<uint64_t> &values);

        // Decode the encoded bitvector to retrieve the original sorted
        // sequence. "numValues" indicates the number of values that were
        // encoded.
        std::vector<uint64_t> decode(size_t numValues) const;

        // Random access: decode to get the i-th value (by sequentially decoding
        // until index i).
        uint64_t access(size_t i) const;

        // Returns the size in bits of the encoded bitvector.
        size_t size_in_bits() const;

    private:
        // Helper: compute the Golomb parameter M from a vector of gap values.
        static uint64_t compute_M(const std::vector<uint64_t> &gaps);
};

#endif // COMPRESSION_ENCODINGS_HPP
