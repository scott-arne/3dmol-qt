#pragma once

/// \file bond_adapter.h
/// \brief Converts OpenEye molecule bonds into backend-neutral BondData.

#include "oe3dmol/bond_data.h"
#include "oe3dmol/stick_style.h"

namespace OEChem {
class OEMolBase;
}

namespace oe3dmol {

/// \brief Build half-bond BondData from an OpenEye molecule under a stick style.
///
/// Iterates the molecule's existing bonds (no perception). Each bond is split at
/// its midpoint into two half-bonds; each half takes its near atom's element
/// color and the style's radius. Zero-length (coincident-atom) bonds are skipped.
///
/// \param mol The source molecule (non-owning; must be non-null).
/// \param style Stick style controlling radius and color scheme.
/// \returns BondData with 2 entries per (non-degenerate) bond; empty if no bonds.
/// \throws DataError if mol is null.
BondData build_bond_data(const OEChem::OEMolBase* mol, const StickStyle& style);

}  // namespace oe3dmol
