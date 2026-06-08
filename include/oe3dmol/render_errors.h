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

}  // namespace oe3dmol
