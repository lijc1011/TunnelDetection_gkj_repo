// 写盘基准：现方案（LUT 加密 + fwrite 单文件）对比 zarr v3（LUT 加密 + TensorStore 异步写）。
// 用法：TsZarrBench <输出目录> [包数=64] [包高=4096] [包宽=4096]
// 两条路径写完全相同的数据（逐包 memcpy 基准图 + 盖章包号 + LUT 加密），
// 结束后读回首末两包做逐字节校验。注意：两条路径同样受 OS 页缓存影响，
// 想测穿透盘面的稳态吞吐请把包数加大到内存容不下的量级。

#include "TsZarrBridge.h"
#include "../image_encryptor.hpp"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

namespace {

// Windows 下 main 的 argv 是本地代码页(GBK)编码；桥接口要求 UTF-8。
std::string argvToUtf8(const char* s)
{
#if defined(_WIN32)
    const int wlen = MultiByteToWideChar(CP_ACP, 0, s, -1, nullptr, 0);
    std::wstring wide(static_cast<size_t>(wlen), L'\0');
    MultiByteToWideChar(CP_ACP, 0, s, -1, wide.data(), wlen);
    const int ulen = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string utf8(static_cast<size_t>(ulen), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, utf8.data(), ulen, nullptr, nullptr);
    utf8.resize(std::strlen(utf8.c_str()));
    return utf8;
#else
    return s;
#endif
}

using Clock = std::chrono::steady_clock;

double msSince(Clock::time_point start)
{
    return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

// 模拟隧道衬砌灰度纹理：纵向渐变 + 确定性伪随机噪声
void fillBasePacket(std::vector<std::uint8_t>& buf, std::int64_t height, std::int64_t width)
{
    std::uint32_t state = 20260703u;
    for (std::int64_t y = 0; y < height; ++y) {
        std::uint8_t* row = buf.data() + y * width;
        const std::uint8_t base = static_cast<std::uint8_t>(96 + (y * 64) / height);
        for (std::int64_t x = 0; x < width; ++x) {
            state = state * 1664525u + 1013904223u;
            row[x] = static_cast<std::uint8_t>(base + (state >> 27));
        }
    }
}

void preparePacket(std::vector<std::uint8_t>& packet, const std::vector<std::uint8_t>& base,
                   std::int64_t index, const ImageEncryptor& enc)
{
    std::memcpy(packet.data(), base.data(), base.size());
    std::memcpy(packet.data(), &index, sizeof(index));  // 盖章包号，保证每包内容不同且可复现
    enc.encrypt_data(reinterpret_cast<char*>(packet.data()), packet.size());
}

}  // namespace

int main(int argc, char** argv)
{
    const bool selfTest = argc == 2 && std::strcmp(argv[1], "--self-test") == 0;
    if (argc < 2) {
        std::printf("usage: TsZarrBench --self-test\n"
                    "       TsZarrBench <out_dir> [packets=64] [height=4096] [width=4096]\n");
        return 1;
    }
    std::filesystem::path selfTestDir;
    std::string outDir;
    std::string outDirUtf8;
    std::int64_t packets = argc > 2 ? std::atoll(argv[2]) : 64;
    std::int64_t height = argc > 3 ? std::atoll(argv[3]) : 4096;
    std::int64_t width = argc > 4 ? std::atoll(argv[4]) : 4096;
    if (selfTest) {
        selfTestDir = std::filesystem::temp_directory_path()
                      / ("TunnelDetection_ZarrSelfTest_" + std::to_string(GetCurrentProcessId()));
        std::error_code ec;
        std::filesystem::remove_all(selfTestDir, ec);
        if (!std::filesystem::create_directories(selfTestDir, ec) || ec) {
            std::printf("SELF-TEST FAIL: cannot create %s\n", selfTestDir.string().c_str());
            return 1;
        }
        outDir = selfTestDir.string();
        outDirUtf8 = outDir;
        packets = 5;
        height = 64;
        width = 96;
    } else {
        outDir = argv[1];                       // 本地编码：给 fopen 用
        outDirUtf8 = argvToUtf8(argv[1]);       // UTF-8：给桥用
    }
    const std::int64_t packetBytes = height * width;
    const double totalMB = static_cast<double>(packetBytes) * packets / (1024.0 * 1024.0);

    std::printf("packets=%lld  packet=%lldx%lld (%.1f MB)  total=%.1f MB\n",
                static_cast<long long>(packets), static_cast<long long>(height),
                static_cast<long long>(width), packetBytes / (1024.0 * 1024.0), totalMB);

    const ImageEncryptor enc("20260703120000-1Cam_BENCH");
    std::vector<std::uint8_t> base(static_cast<std::size_t>(packetBytes));
    std::vector<std::uint8_t> packet(static_cast<std::size_t>(packetBytes));
    fillBasePacket(base, height, width);

    const std::filesystem::path rawOutput = std::filesystem::path(outDir) / "bench_raw.camdatx";
    const std::filesystem::path zarrOutput = std::filesystem::path(outDir) / "bench_zarr.zarr";
    if (std::filesystem::exists(rawOutput) || std::filesystem::exists(zarrOutput)) {
        std::printf("refusing to overwrite existing benchmark output in %s\n", outDir.c_str());
        return 2;
    }

    // ---- A. 现方案：LUT 加密 + fwrite 单文件 ----
    {
        const std::string rawPath = outDir + "\\bench_raw.camdatx";
        FILE* fp = nullptr;
        if (fopen_s(&fp, rawPath.c_str(), "wb") != 0 || !fp) {
            std::printf("[raw ] cannot open %s\n", rawPath.c_str());
            return 1;
        }
        const auto start = Clock::now();
        for (std::int64_t i = 0; i < packets; ++i) {
            preparePacket(packet, base, i, enc);
            if (fwrite(packet.data(), static_cast<std::size_t>(packetBytes), 1, fp) != 1) {
                std::printf("[raw ] fwrite failed at packet %lld\n", static_cast<long long>(i));
                fclose(fp);
                return 1;
            }
        }
        fclose(fp);
        const double ms = msSince(start);
        std::printf("[raw ] total %.0f ms  ->  %.1f MB/s\n", ms, totalMB / (ms / 1000.0));
    }

    // ---- B. 新方案：LUT 加密 + TensorStore zarr v3 异步写 ----
    {
        const std::string zarrPath = outDirUtf8 + "\\bench_zarr.zarr";
        char err[512] = {0};
        TsZarrWriterOptions opts = {};
        opts.packet_height = height;
        opts.packet_width = width;
        opts.initial_capacity = 16;  // 故意偏小，顺带覆盖运行中扩容路径
        opts.max_pending = 8;
        opts.overwrite_existing = 0;
        opts.attributes_json_utf8 = "{\"encrypted\":true,\"source\":\"TsZarrBench\"}";

        void* writer = tszarr_writer_create(zarrPath.c_str(), &opts, err, sizeof(err));
        if (!writer) {
            std::printf("[zarr] create failed: %s\n", err);
            return 1;
        }
        double maxSubmitMs = 0, sumSubmitMs = 0;
        const auto start = Clock::now();
        for (std::int64_t i = 0; i < packets; ++i) {
            preparePacket(packet, base, i, enc);
            const auto submitStart = Clock::now();
            if (!tszarr_writer_write_packet(writer, i, packet.data(), err, sizeof(err))) {
                std::printf("[zarr] write failed at packet %lld: %s\n",
                            static_cast<long long>(i), err);
                tszarr_writer_close(writer, -1, err, sizeof(err));
                return 1;
            }
            const double submitMs = msSince(submitStart);
            sumSubmitMs += submitMs;
            if (submitMs > maxSubmitMs) maxSubmitMs = submitMs;
        }
        const double submitPhaseMs = msSince(start);
        if (!tszarr_writer_close(writer, packets, err, sizeof(err))) {
            std::printf("[zarr] close failed: %s\n", err);
            return 1;
        }
        const double ms = msSince(start);
        std::printf("[zarr] total %.0f ms  ->  %.1f MB/s   (submit phase %.0f ms, "
                    "avg submit %.1f ms, max submit %.1f ms)\n",
                    ms, totalMB / (ms / 1000.0), submitPhaseMs, sumSubmitMs / packets,
                    maxSubmitMs);

        // ---- 校验：读回首末两包与预期逐字节比对 ----
        std::vector<std::uint8_t> readBack(static_cast<std::size_t>(packetBytes));
        std::vector<std::uint8_t> expected(static_cast<std::size_t>(packetBytes));
        const std::int64_t checks[2] = {0, packets - 1};
        for (const std::int64_t idx : checks) {
            if (!tszarr_read_packet(zarrPath.c_str(), idx, readBack.data(), height, width, err,
                                    sizeof(err))) {
                std::printf("[zarr] verify read failed at packet %lld: %s\n",
                            static_cast<long long>(idx), err);
                return 1;
            }
            preparePacket(expected, base, idx, enc);
            if (std::memcmp(readBack.data(), expected.data(),
                            static_cast<std::size_t>(packetBytes)) != 0) {
                std::printf("[zarr] VERIFY MISMATCH at packet %lld\n",
                            static_cast<long long>(idx));
                return 1;
            }
        }
        std::printf("[zarr] verify OK (packet 0 and %lld round-trip identical)\n",
                    static_cast<long long>(packets - 1));

        // 默认覆盖保护必须生效；这是现场安全写入的回归测试。
        void* duplicate = tszarr_writer_create(zarrPath.c_str(), &opts, err, sizeof(err));
        if (duplicate != nullptr) {
            tszarr_writer_close(duplicate, 0, err, sizeof(err));
            std::printf("[zarr] SAFE-OVERWRITE TEST FAIL\n");
            return 1;
        }
        std::printf("[zarr] safe-overwrite refusal OK\n");
    }
    if (selfTest) {
        std::error_code ec;
        std::filesystem::remove_all(selfTestDir, ec);
        if (ec) {
            std::printf("SELF-TEST PASS (temporary output retained: %s)\n",
                        selfTestDir.string().c_str());
        } else {
            std::printf("FIELD SELF-TEST PASS\n");
        }
    }
    return 0;
}
