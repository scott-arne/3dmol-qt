#pragma once

/// \file data_errors.h
/// \brief Exception types for the data-adapter layer.

#include <stdexcept>
#include <string>

namespace oe3dmol {

/// \brief Thrown when OpenEye input cannot be converted to AtomData.
class DataError : public std::runtime_error {
public:
    explicit DataError(const std::string& message) : std::runtime_error(message) {}
};

}  // namespace oe3dmol
