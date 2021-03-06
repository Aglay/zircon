// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <zx/handle.h>
#include <zx/object.h>

namespace zx {

class channel : public object<channel> {
public:
    static constexpr zx_obj_type_t TYPE = ZX_OBJ_TYPE_CHANNEL;

    constexpr channel() = default;

    explicit channel(zx_handle_t value) : object(value) {}

    explicit channel(handle&& h) : object(h.release()) {}

    channel(channel&& other) : object(other.release()) {}

    channel& operator=(channel&& other) {
        reset(other.release());
        return *this;
    }

    static zx_status_t create(uint32_t flags, channel* endpoint0,
                              channel* endpoint1);

    zx_status_t read(uint32_t flags, void* bytes, uint32_t num_bytes,
                     uint32_t* actual_bytes, zx_handle_t* handles,
                     uint32_t num_handles, uint32_t* actual_handles) const {
        return zx_channel_read(get(), flags, bytes, handles, num_bytes,
                               num_handles, actual_bytes, actual_handles);
    }

    zx_status_t write(uint32_t flags, const void* bytes, uint32_t num_bytes,
                      const zx_handle_t* handles, uint32_t num_handles) const {
        return zx_channel_write(get(), flags, bytes, num_bytes, handles,
                                num_handles);
    }

    zx_status_t call(uint32_t flags, zx_time_t deadline,
                     const zx_channel_call_args_t* args,
                     uint32_t* actual_bytes, uint32_t* actual_handles,
                     zx_status_t* read_status) const {
        return zx_channel_call(get(), flags, deadline, args, actual_bytes,
                               actual_handles, read_status);
    }
};

using unowned_channel = const unowned<channel>;

} // namespace zx
