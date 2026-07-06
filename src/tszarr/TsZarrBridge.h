#pragma once

// TensorStore Zarr v3 采集写盘桥接层（C ABI，跨 DLL 边界安全）。
//
// 设计约束：
//  - 只允许 C 兼容类型过边界：裸指针 + 长度、int64、bool、UTF-8 字符串、错误缓冲区。
//  - 所有异常在 DLL 内 catch 干净，以返回值 + 错误缓冲区上报。
//  - 写入为异步提交：write_packet 仅等待"数据拷入缓存"（毫秒级），落盘由
//    TensorStore 内部线程池后台执行；内部维持有界在途队列（背压），
//    flush/close 是同步收口点。
//  - 数组布局：uint8 [packet, y, x]，chunk = [1, packet_height, packet_width]，
//    即"一包一 chunk 一文件"，顺序大块落盘；packet 维按需自动扩容，close 时
//    收缩为实际包数。codec 固定为 bytes（不压缩）。

#include <cstddef>
#include <cstdint>

#if defined(_WIN32)
#if defined(TSZARR_BRIDGE_EXPORTS)
#define TSZARR_API __declspec(dllexport)
#else
#define TSZARR_API __declspec(dllimport)
#endif
#else
#define TSZARR_API
#endif

extern "C" {

struct TsZarrWriterOptions {
    std::int64_t packet_height;      // 一包的行数（变换后）
    std::int64_t packet_width;       // 一包的行字节数（变换后；彩色相机 = 像素宽*3）
    std::int64_t initial_capacity;   // packet 维初始容量（包数），<=0 取默认 256
    std::int64_t max_pending;        // 在途异步写上限（包数），<=0 取默认 8
    const char* attributes_json_utf8; // 可选：合入 zarr attributes 的 JSON 对象字符串，可为 null
};

// 创建 Zarr v3 数组并返回写句柄；失败返回 null 并填充 error。
// array_path_utf8 为数组根目录；若已存在则整体覆盖（与 fopen "wb" 语义一致）。
TSZARR_API void* tszarr_writer_create(const char* array_path_utf8,
                                      const TsZarrWriterOptions* options,
                                      char* error, std::size_t error_size);

// 异步写入第 packet_index 包（packet_height*packet_width 字节）。
// 返回后 data 缓冲区即可复用。失败（含此前后台写盘的延迟错误）返回 false。
TSZARR_API bool tszarr_writer_write_packet(void* handle, std::int64_t packet_index,
                                           const std::uint8_t* data,
                                           char* error, std::size_t error_size);

// 等待所有在途写完成；返回累计状态。
TSZARR_API bool tszarr_writer_flush(void* handle, char* error, std::size_t error_size);

// 当前在途（尚未确认落盘）的包数，句柄无效返回 -1。
TSZARR_API std::int64_t tszarr_writer_pending(void* handle);

// 收口：等待全部落盘，final_packet_count >= 0 时把 packet 维收缩为该值，
// 然后释放句柄（无论成败句柄都会失效）。
TSZARR_API bool tszarr_writer_close(void* handle, std::int64_t final_packet_count,
                                    char* error, std::size_t error_size);

// 诊断/校验用：打开既有数组同步读回一包。out 缓冲区需容纳
// packet_height*packet_width 字节（与创建时一致）。
TSZARR_API bool tszarr_read_packet(const char* array_path_utf8, std::int64_t packet_index,
                                   std::uint8_t* out, std::int64_t packet_height,
                                   std::int64_t packet_width,
                                   char* error, std::size_t error_size);

}
