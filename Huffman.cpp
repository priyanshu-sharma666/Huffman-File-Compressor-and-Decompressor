#include "Huffman.h"

#include <fstream>
#include <vector>
#include <queue>
#include <array>
#include <memory>
#include <filesystem>
#include <algorithm>

namespace {

struct Node {
    uint8_t ch = 0;
    uint64_t freq = 0;
    std::unique_ptr<Node> left;
    std::unique_ptr<Node> right;

    Node(uint8_t c, uint64_t f) : ch(c), freq(f) {}
    Node(uint64_t f, std::unique_ptr<Node> l, std::unique_ptr<Node> r)
        : freq(f), left(std::move(l)), right(std::move(r)) {}

    bool isLeaf() const { return !left && !right; }
};

struct CompareNode {
    bool operator()(const std::unique_ptr<Node>& a, const std::unique_ptr<Node>& b) const {
        return a->freq > b->freq;
    }
};

struct Code {
    uint64_t bits = 0;
    int len = 0;
};

class BitWriter {
    std::ostream& out;
    std::vector<uint8_t> buffer;
    uint64_t bitBuf = 0;
    int bitCount = 0;

public:
    explicit BitWriter(std::ostream& os) : out(os) {
        buffer.reserve(65536);
    }

    void write(uint64_t bits, int count) {
        bitBuf = (bitBuf << count) | (bits & ((count == 64) ? ~0ULL : ((1ULL << count) - 1)));
        bitCount += count;
        while (bitCount >= 8) {
            buffer.push_back((bitBuf >> (bitCount - 8)) & 0xFF);
            bitCount -= 8;
            if (buffer.size() >= 65536) {
                out.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
                buffer.clear();
            }
        }
    }

    void flush() {
        if (bitCount > 0) {
            buffer.push_back((bitBuf << (8 - bitCount)) & 0xFF);
            bitCount = 0;
        }
        if (!buffer.empty()) {
            out.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
            buffer.clear();
        }
    }
};

class BitReader {
    std::istream& in;
    std::vector<uint8_t> buffer;
    size_t pos = 0;
    size_t size = 0;
    uint64_t bitBuf = 0;
    int bitCount = 0;

    bool refill() {
        if (pos >= size) {
            buffer.resize(65536);
            in.read(reinterpret_cast<char*>(buffer.data()), 65536);
            size = in.gcount();
            pos = 0;
            if (size == 0) return false;
        }
        return true;
    }

public:
    explicit BitReader(std::istream& is) : in(is) {
        buffer.resize(65536);
    }

    int readBit() {
        if (bitCount == 0) {
            if (!refill()) return -1;
            bitBuf = buffer[pos++];
            bitCount = 8;
        }
        bitCount--;
        return (bitBuf >> bitCount) & 1;
    }
};

void buildCodeMap(const Node* node, uint64_t bits, int len, std::array<Code, 256>& map) {
    if (!node) return;
    if (node->isLeaf()) {
        map[node->ch] = { bits, len == 0 ? 1 : len };
        return;
    }
    buildCodeMap(node->left.get(), (bits << 1) | 0, len + 1, map);
    buildCodeMap(node->right.get(), (bits << 1) | 1, len + 1, map);
}

} // namespace

CompressionResult CompressFile(const std::string& inputPath, const std::string& outputPath) {
    CompressionResult result;
    result.outputPath = outputPath;

    std::ifstream in(inputPath, std::ios::binary);
    if (!in) {
        result.error = "Cannot open source file: " + inputPath;
        return result;
    }

    // Count byte frequencies
    std::array<uint64_t, 256> freqs{};
    std::vector<uint8_t> block(65536);
    uint64_t totalBytes = 0;

    while (in.read(reinterpret_cast<char*>(block.data()), block.size()) || in.gcount() > 0) {
        size_t count = in.gcount();
        for (size_t i = 0; i < count; ++i) {
            freqs[block[i]]++;
        }
        totalBytes += count;
    }
    result.originalSize = totalBytes;

    if (totalBytes == 0) {
        std::ofstream out(outputPath, std::ios::binary);
        if (!out) {
            result.error = "Cannot create output file: " + outputPath;
            return result;
        }
        out.write("HUFF", 4);
        uint64_t zero = 0;
        uint16_t zeroSymbols = 0;
        out.write(reinterpret_cast<const char*>(&zero), 8);
        out.write(reinterpret_cast<const char*>(&zeroSymbols), 2);
        result.compressedSize = std::filesystem::file_size(outputPath);
        result.success = true;
        return result;
    }

    // Build tree via min-heap
    std::priority_queue<std::unique_ptr<Node>, std::vector<std::unique_ptr<Node>>, CompareNode> pq;
    uint16_t uniqueSymbols = 0;
    for (int i = 0; i < 256; ++i) {
        if (freqs[i] > 0) {
            pq.push(std::make_unique<Node>(static_cast<uint8_t>(i), freqs[i]));
            uniqueSymbols++;
        }
    }

    while (pq.size() > 1) {
        auto l = std::move(const_cast<std::unique_ptr<Node>&>(pq.top())); pq.pop();
        auto r = std::move(const_cast<std::unique_ptr<Node>&>(pq.top())); pq.pop();
        pq.push(std::make_unique<Node>(l->freq + r->freq, std::move(l), std::move(r)));
    }
    auto root = std::move(const_cast<std::unique_ptr<Node>&>(pq.top())); pq.pop();

    // Generate prefix codes
    std::array<Code, 256> codeMap{};
    buildCodeMap(root.get(), 0, 0, codeMap);

    // Write header and encoded bits
    std::ofstream out(outputPath, std::ios::binary);
    if (!out) {
        result.error = "Cannot create output file: " + outputPath;
        return result;
    }

    out.write("HUFF", 4);
    out.write(reinterpret_cast<const char*>(&totalBytes), 8);
    out.write(reinterpret_cast<const char*>(&uniqueSymbols), 2);

    for (int i = 0; i < 256; ++i) {
        if (freqs[i] > 0) {
            uint8_t sym = static_cast<uint8_t>(i);
            out.write(reinterpret_cast<const char*>(&sym), 1);
            out.write(reinterpret_cast<const char*>(&freqs[i]), 8);
        }
    }

    in.clear();
    in.seekg(0);

    BitWriter writer(out);
    while (in.read(reinterpret_cast<char*>(block.data()), block.size()) || in.gcount() > 0) {
        size_t count = in.gcount();
        for (size_t i = 0; i < count; ++i) {
            const auto& c = codeMap[block[i]];
            writer.write(c.bits, c.len);
        }
    }
    writer.flush();

    out.close();
    in.close();

    result.compressedSize = std::filesystem::file_size(outputPath);
    result.spaceSaved = (totalBytes > 0) ? (1.0 - static_cast<double>(result.compressedSize) / totalBytes) * 100.0 : 0.0;
    result.success = true;
    return result;
}

CompressionResult DecompressFile(const std::string& inputPath, const std::string& outputPath) {
    CompressionResult result;
    result.outputPath = outputPath;

    std::ifstream in(inputPath, std::ios::binary);
    if (!in) {
        result.error = "Cannot open compressed file: " + inputPath;
        return result;
    }

    char magic[4];
    if (!in.read(magic, 4) || std::string(magic, 4) != "HUFF") {
        result.error = "Invalid file: Missing HUFF header identifier";
        return result;
    }

    uint64_t totalBytes = 0;
    uint16_t uniqueSymbols = 0;
    in.read(reinterpret_cast<char*>(&totalBytes), 8);
    in.read(reinterpret_cast<char*>(&uniqueSymbols), 2);

    result.originalSize = totalBytes;
    result.compressedSize = std::filesystem::file_size(inputPath);

    std::ofstream out(outputPath, std::ios::binary);
    if (!out) {
        result.error = "Cannot create output file: " + outputPath;
        return result;
    }

    if (totalBytes == 0) {
        result.success = true;
        return result;
    }

    std::array<uint64_t, 256> freqs{};
    std::priority_queue<std::unique_ptr<Node>, std::vector<std::unique_ptr<Node>>, CompareNode> pq;

    for (uint16_t i = 0; i < uniqueSymbols; ++i) {
        uint8_t sym = 0;
        uint64_t freq = 0;
        in.read(reinterpret_cast<char*>(&sym), 1);
        in.read(reinterpret_cast<char*>(&freq), 8);
        freqs[sym] = freq;
        pq.push(std::make_unique<Node>(sym, freq));
    }

    while (pq.size() > 1) {
        auto l = std::move(const_cast<std::unique_ptr<Node>&>(pq.top())); pq.pop();
        auto r = std::move(const_cast<std::unique_ptr<Node>&>(pq.top())); pq.pop();
        pq.push(std::make_unique<Node>(l->freq + r->freq, std::move(l), std::move(r)));
    }
    auto root = std::move(const_cast<std::unique_ptr<Node>&>(pq.top())); pq.pop();

    BitReader reader(in);
    uint64_t written = 0;
    std::vector<uint8_t> outBuf;
    outBuf.reserve(65536);

    if (root->isLeaf()) {
        uint8_t ch = root->ch;
        while (written < totalBytes) {
            size_t chunk = std::min<uint64_t>(totalBytes - written, 65536);
            std::vector<uint8_t> blk(chunk, ch);
            out.write(reinterpret_cast<const char*>(blk.data()), chunk);
            written += chunk;
        }
    } else {
        const Node* curr = root.get();
        while (written < totalBytes) {
            int bit = reader.readBit();
            if (bit == -1) {
                result.error = "Corrupted archive: Unexpected end of stream";
                return result;
            }

            curr = (bit == 1) ? curr->right.get() : curr->left.get();
            if (!curr) {
                result.error = "Corrupted archive: Invalid tree traversal";
                return result;
            }

            if (curr->isLeaf()) {
                outBuf.push_back(curr->ch);
                written++;
                curr = root.get();

                if (outBuf.size() >= 65536) {
                    out.write(reinterpret_cast<const char*>(outBuf.data()), outBuf.size());
                    outBuf.clear();
                }
            }
        }

        if (!outBuf.empty()) {
            out.write(reinterpret_cast<const char*>(outBuf.data()), outBuf.size());
            outBuf.clear();
        }
    }

    result.success = true;
    return result;
}