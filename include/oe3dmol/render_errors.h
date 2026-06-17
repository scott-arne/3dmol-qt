#pragma once

/// \file render_errors.h
/// \brief Exception type for the render layer.

#include <stdexcept>
#include <string>

namespace oe3dmol {

/// \brief Thrown on RHI initialization, pipeline, or readback failure.
class RenderError : public std::runtime_error {
public:
    explicit RenderError(const std::string& message) : std::runtime_error(message) {}
};

/// \brief Thrown specifically when the RHI reports device loss (QRhi::FrameOpDeviceLost).
/// Derives from RenderError so existing catch(RenderError) sites still catch it;
/// a persistent renderer (OffscreenSession) catches this narrower type to rebuild
/// the device and retry.
class DeviceLostError : public RenderError {
public:
    explicit DeviceLostError(const std::string& message) : RenderError(message) {}
};

/// \brief Throw RenderError(message) when cond is false. Terse guard for the
/// many RHI "create returned null / failed" checks in the render layer.
inline void render_require(bool cond, const std::string& message) {
    if (!cond) {
        throw RenderError(message);
    }
}

}  // namespace oe3dmol
