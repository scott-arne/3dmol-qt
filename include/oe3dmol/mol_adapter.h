#pragma once

/// \file mol_adapter.h
/// \brief Converts OpenEye molecules into backend-neutral AtomData.

#include "oe3dmol/atom_data.h"
#include "oe3dmol/style.h"

namespace OEChem {
class OEMolBase;
}

namespace oe3dmol {

/// \brief Build AtomData from an OpenEye molecule under a sphere style.
///
/// Extracts each atom's coordinates and element, resolves radius via
/// resolve_radius() and color via the style's color scheme. The molecule
/// is a non-owning input view.
///
/// \param mol The source molecule (must be non-null with >0 atoms).
/// \param style Sphere style controlling radius and color scheme.
/// \returns Populated AtomData.
/// \throws DataError if mol is null or has no atoms.
AtomData build_atom_data(const OEChem::OEMolBase* mol, const SphereStyle& style);

}  // namespace oe3dmol
