#pragma once

/// \file style_spec.h
/// \brief Variant over the supported representation styles.

#include <variant>

#include "oe3dmol/stick_style.h"
#include "oe3dmol/style.h"

namespace oe3dmol {

/// \brief A single representation style applied to a model.
///
/// A model may carry several (e.g. a scaled SphereStyle + a StickStyle for
/// ball-and-stick). New representations extend this variant.
using StyleSpec = std::variant<SphereStyle, StickStyle>;

}  // namespace oe3dmol
