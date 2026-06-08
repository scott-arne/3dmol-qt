#pragma once

/// \file mol_adapter.h
/// \brief Converts OpenEye molecules into backend-neutral AtomData.

#include <type_traits>

#include "oe3dmol/atom_data.h"
#include "oe3dmol/style.h"

namespace OEChem {
class OEMolBase;
class OEGraphMol;
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

/// \brief Implementation helper for the OEGraphMol overload (see below).
AtomData build_atom_data_from_graphmol(const OEChem::OEGraphMol* mol,
                                       const SphereStyle& style);

/// \brief Build AtomData from a concrete OEGraphMol under a sphere style.
///
/// OEGraphMol is a value-type wrapper that does not derive from OEMolBase,
/// so a pointer to it does not implicitly convert to OEMolBase*. This
/// overload accepts the concrete graph-molecule pointer that callers
/// typically hold and forwards to the OEMolBase* implementation.
///
/// It is a constrained template so that a null pointer literal binds
/// unambiguously to the OEMolBase* overload rather than this one.
///
/// \param mol The source molecule (must be non-null with >0 atoms).
/// \param style Sphere style controlling radius and color scheme.
/// \returns Populated AtomData.
/// \throws DataError if mol is null or has no atoms.
template <typename T,
          typename = std::enable_if_t<std::is_same_v<T, OEChem::OEGraphMol>>>
AtomData build_atom_data(const T* mol, const SphereStyle& style) {
    return build_atom_data_from_graphmol(mol, style);
}

}  // namespace oe3dmol
