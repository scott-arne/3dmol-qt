#pragma once

/// \file vdw_radii.h
/// \brief Van der Waals radii table ported from 3dmol.js GLModel.ts.

#include <string>

namespace oe3dmol {

/// \brief Default sphere radius for elements absent from the table (Angstrom).
constexpr double DEFAULT_SPHERE_RADIUS = 1.5;

/// \brief Look up the VDW radius for an element symbol.
///
/// Mirrors 3dmol's lookup including the case-normalization retry for
/// multi-letter symbols. Returns DEFAULT_SPHERE_RADIUS if unknown.
///
/// \param element Element symbol (e.g. "C", "Fe").
/// \returns VDW radius in Angstrom.
double vdw_radius(const std::string& element);

}  // namespace oe3dmol
