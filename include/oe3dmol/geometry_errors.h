#pragma once

/// \file geometry_errors.h
/// \brief Exception type for the geometry layer.

#include <stdexcept>
#include <string>

namespace oe3dmol {

/// \brief Thrown on malformed geometry-construction input.
class GeometryError : public std::runtime_error {
public:
    explicit GeometryError(const std::string& message) : std::runtime_error(message) {}
};

}  // namespace oe3dmol
