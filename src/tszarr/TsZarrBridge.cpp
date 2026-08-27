#include "TsZarrBridge.h"

#include "absl/status/status.h"
#include <nlohmann/json.hpp>
#include "tensorstore/array.h"
#include "tensorstore/context.h"
#include "tensorstore/index.h"
#include "tensorstore/index_space/dim_expression.h"
#include "tensorstore/open.h"
#include "tensorstore/open_mode.h"
#include "tensorstore/resize_options.h"
#include "tensorstore/tensorstore.h"
#include "tensorstore/util/future.h"
#include "tensorstore/util/span.h"

#include <algorithm>
#include <cstring>
#include <deque>
#include <exception>
#include <mutex>
#include <string>
#include <utility>

namespace {

using Store = tensorstore::TensorStore<std::uint8_t, 3, tensorstore::ReadWriteMode::read_write>;

constexpr std::int64_t kDefaultInitialCapacity = 256;
constexpr std::int64_t kDefaultMaxPending = 8;

struct WriterHandle {
    std::mutex mutex;
    Store store;
    std::int64_t height = 0;
    std::int64_t width = 0;
    std::int64_t capacity = 0;
    std::int64_t max_pending = kDefaultMaxPending;
    std::deque<tensorstore::Future<const void>> pending;
    absl::Status first_error;  // 后台写盘的延迟错误，粘滞上报

    // 调用方持锁。等待最早的一笔在途写，把失败记入 first_error。
    void WaitOldestLocked()
    {
        if (pending.empty()) return;
        auto future = std::move(pending.front());
        pending.pop_front();
        const absl::Status status = future.status();
        if (!status.ok() && first_error.ok()) {
            first_error = status;
        }
    }
};

void setError(const std::string& message, char* error, std::size_t errorSize)
{
    if (!error || errorSize == 0) return;
    const std::size_t size = std::min(message.size(), errorSize - 1);
    std::memcpy(error, message.data(), size);
    error[size] = '\0';
}

void clearError(char* error, std::size_t errorSize)
{
    if (error && errorSize > 0) error[0] = '\0';
}

std::string normalizedPath(const char* path)
{
    std::string result = path ? path : "";
    std::replace(result.begin(), result.end(), '\\', '/');
    if (!result.empty() && result.back() != '/') result.push_back('/');
    return result;
}

nlohmann::json makeArraySpec(const std::string& path, std::int64_t capacity,
                             std::int64_t height, std::int64_t width,
                             const char* attributesJson)
{
    nlohmann::json attributes = {{"creator", "TunnelDetection"}};
    if (attributesJson && attributesJson[0] != '\0') {
        nlohmann::json extra = nlohmann::json::parse(attributesJson);  // 失败抛出，由调用方 catch
        if (extra.is_object()) {
            for (auto& item : extra.items()) attributes[item.key()] = item.value();
        }
    }
    return {
        {"driver", "zarr3"},
        // file_io_sync=false：不逐文件 fsync，与现方案 fwrite（OS 缓存）同语义；
        // 掉电丢失窗口与旧方案一致，由 OS 刷盘节奏决定。
        {"kvstore", {{"driver", "file"}, {"path", path}, {"file_io_sync", false}}},
        {"metadata",
         {{"shape", {capacity, height, width}},
          {"data_type", "uint8"},
          {"chunk_grid",
           {{"name", "regular"},
            {"configuration", {{"chunk_shape", {1, height, width}}}}}},
          {"chunk_key_encoding", {{"name", "default"}}},
          {"codecs", nlohmann::json::array({{{"name", "bytes"}}})},
          {"fill_value", 0},
          {"dimension_names", {"frame", "y", "x"}},
          {"attributes", attributes}}}};
}

// 调用方持锁。把 packet 维扩容/收缩到 newCapacity。
absl::Status resizePacketDimLocked(WriterHandle& handle, std::int64_t newCapacity,
                                   tensorstore::ResizeMode mode)
{
    const tensorstore::Index newMin[3] = {tensorstore::kImplicit, tensorstore::kImplicit,
                                          tensorstore::kImplicit};
    const tensorstore::Index newMax[3] = {newCapacity, tensorstore::kImplicit,
                                          tensorstore::kImplicit};
    auto resized = tensorstore::Resize(handle.store, tensorstore::span(newMin),
                                       tensorstore::span(newMax), mode)
                       .result();
    if (!resized.ok()) return resized.status();
    handle.store = *std::move(resized);
    handle.capacity = newCapacity;
    return absl::OkStatus();
}

}  // namespace

void* tszarr_writer_create(const char* array_path_utf8, const TsZarrWriterOptions* options,
                           char* error, std::size_t error_size)
{
    clearError(error, error_size);
    try {
        if (!options || options->packet_height <= 0 || options->packet_width <= 0) {
            setError("Invalid TsZarrWriterOptions (packet_height/packet_width).", error, error_size);
            return nullptr;
        }
        const std::int64_t capacity =
            options->initial_capacity > 0 ? options->initial_capacity : kDefaultInitialCapacity;

        auto spec = makeArraySpec(normalizedPath(array_path_utf8), capacity,
                                  options->packet_height, options->packet_width,
                                  options->attributes_json_utf8);
        tensorstore::OpenMode openMode = tensorstore::OpenMode::create;
        if (options->overwrite_existing != 0) {
            openMode = openMode | tensorstore::OpenMode::delete_existing;
        }
        auto result = tensorstore::Open<std::uint8_t, 3, tensorstore::ReadWriteMode::read_write>(
                          std::move(spec), tensorstore::Context::Default(), openMode).result();
        if (!result.ok()) {
            setError(result.status().ToString(), error, error_size);
            return nullptr;
        }

        auto* handle = new WriterHandle;
        handle->store = *std::move(result);
        handle->height = options->packet_height;
        handle->width = options->packet_width;
        handle->capacity = capacity;
        handle->max_pending = options->max_pending > 0 ? options->max_pending : kDefaultMaxPending;
        return handle;
    } catch (const std::exception& ex) {
        setError(ex.what(), error, error_size);
        return nullptr;
    }
}

bool tszarr_writer_write_packet(void* opaque, std::int64_t packet_index,
                                const std::uint8_t* data, char* error, std::size_t error_size)
{
    clearError(error, error_size);
    auto* handle = static_cast<WriterHandle*>(opaque);
    if (!handle || !data || packet_index < 0) {
        setError("Invalid TsZarr writer handle or arguments.", error, error_size);
        return false;
    }
    try {
        std::lock_guard<std::mutex> lock(handle->mutex);
        if (!handle->first_error.ok()) {
            setError("Deferred write error: " + handle->first_error.ToString(), error, error_size);
            return false;
        }
        if (packet_index >= handle->capacity) {
            const std::int64_t newCapacity = std::max(handle->capacity * 2, packet_index + 1);
            const absl::Status status =
                resizePacketDimLocked(*handle, newCapacity, tensorstore::expand_only);
            if (!status.ok()) {
                setError(status.ToString(), error, error_size);
                return false;
            }
        }

        auto slice = handle->store | tensorstore::Dims(0).IndexSlice(packet_index);
        if (!slice.ok()) {
            setError(slice.status().ToString(), error, error_size);
            return false;
        }
        const tensorstore::Index shape[2] = {handle->height, handle->width};
        const tensorstore::Array<const std::uint8_t, 2> view(data, tensorstore::span(shape));
        auto futures = tensorstore::Write(tensorstore::UnownedToShared(view), *std::move(slice));

        // 等数据拷入 chunk 缓存（毫秒级），之后调用方即可复用 data 缓冲区。
        const absl::Status copyStatus = futures.copy_future.status();
        if (!copyStatus.ok()) {
            setError(copyStatus.ToString(), error, error_size);
            return false;
        }
        futures.commit_future.Force();  // 非阻塞：触发后台落盘
        handle->pending.emplace_back(std::move(futures.commit_future));

        // 背压：在途超限时等最早一笔完成，防止磁盘抖动导致内存膨胀。
        while (static_cast<std::int64_t>(handle->pending.size()) > handle->max_pending) {
            handle->WaitOldestLocked();
        }
        if (!handle->first_error.ok()) {
            setError("Deferred write error: " + handle->first_error.ToString(), error, error_size);
            return false;
        }
        return true;
    } catch (const std::exception& ex) {
        setError(ex.what(), error, error_size);
        return false;
    }
}

bool tszarr_writer_flush(void* opaque, char* error, std::size_t error_size)
{
    clearError(error, error_size);
    auto* handle = static_cast<WriterHandle*>(opaque);
    if (!handle) {
        setError("Invalid TsZarr writer handle.", error, error_size);
        return false;
    }
    try {
        std::lock_guard<std::mutex> lock(handle->mutex);
        while (!handle->pending.empty()) handle->WaitOldestLocked();
        if (!handle->first_error.ok()) {
            setError(handle->first_error.ToString(), error, error_size);
            return false;
        }
        return true;
    } catch (const std::exception& ex) {
        setError(ex.what(), error, error_size);
        return false;
    }
}

std::int64_t tszarr_writer_pending(void* opaque)
{
    auto* handle = static_cast<WriterHandle*>(opaque);
    if (!handle) return -1;
    std::lock_guard<std::mutex> lock(handle->mutex);
    return static_cast<std::int64_t>(handle->pending.size());
}

bool tszarr_writer_close(void* opaque, std::int64_t final_packet_count,
                         char* error, std::size_t error_size)
{
    clearError(error, error_size);
    auto* handle = static_cast<WriterHandle*>(opaque);
    if (!handle) {
        setError("Invalid TsZarr writer handle.", error, error_size);
        return false;
    }
    bool ok = false;
    try {
        std::lock_guard<std::mutex> lock(handle->mutex);
        while (!handle->pending.empty()) handle->WaitOldestLocked();
        absl::Status status = handle->first_error;
        if (status.ok() && final_packet_count >= 0 && final_packet_count != handle->capacity) {
            status = resizePacketDimLocked(*handle, final_packet_count,
                                           tensorstore::shrink_only);
        }
        if (!status.ok()) {
            setError(status.ToString(), error, error_size);
        } else {
            ok = true;
        }
    } catch (const std::exception& ex) {
        setError(ex.what(), error, error_size);
    }
    delete handle;
    return ok;
}

bool tszarr_read_packet(const char* array_path_utf8, std::int64_t packet_index,
                        std::uint8_t* out, std::int64_t packet_height,
                        std::int64_t packet_width, char* error, std::size_t error_size)
{
    clearError(error, error_size);
    if (!out || packet_height <= 0 || packet_width <= 0 || packet_index < 0) {
        setError("Invalid tszarr_read_packet arguments.", error, error_size);
        return false;
    }
    try {
        nlohmann::json spec = {
            {"driver", "zarr3"},
            {"kvstore", {{"driver", "file"}, {"path", normalizedPath(array_path_utf8)}}},
        };
        auto result = tensorstore::Open<std::uint8_t, 3, tensorstore::ReadWriteMode::read>(
                          std::move(spec), tensorstore::Context::Default(),
                          tensorstore::OpenMode::open)
                          .result();
        if (!result.ok()) {
            setError(result.status().ToString(), error, error_size);
            return false;
        }
        auto slice = *std::move(result) | tensorstore::Dims(0).IndexSlice(packet_index);
        if (!slice.ok()) {
            setError(slice.status().ToString(), error, error_size);
            return false;
        }
        const tensorstore::Index shape[2] = {packet_height, packet_width};
        tensorstore::Array<std::uint8_t, 2> view(out, tensorstore::span(shape));
        const absl::Status status =
            tensorstore::Read(*std::move(slice), tensorstore::UnownedToShared(view)).status();
        if (!status.ok()) {
            setError(status.ToString(), error, error_size);
            return false;
        }
        return true;
    } catch (const std::exception& ex) {
        setError(ex.what(), error, error_size);
        return false;
    }
}
