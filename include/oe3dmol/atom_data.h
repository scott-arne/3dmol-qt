#pragma once

/// \file atom_data.h
/// \brief Backend-neutral structure-of-arrays render buffer for atoms.
///
/// Lives in oe3dmol_core (NOT oe3dmol_data) so that geometry construction
/// depends only on core and stays free of OpenEye and the GPU. oe3dmol_data
/// produces AtomData from OEMolBase*, but does not own this definition.

#include <vector>

#include <QVector3D>

#include "oe3dmol/bounding_box.h"
#include "oe3dmol/color.h"

namespace oe3dmol {

/// \brief Per-atom render attributes in structure-of-arrays form.
struct AtomData {
    std::vector<QVector3D> positions;   ///< Atom centers (Angstrom).
    std::vector<float> radii;           ///< Rendered radii (Angstrom).
    std::vector<Color> colors;          ///< Per-atom colors.
    std::vector<unsigned> source_idx;   ///< Index back to the source atom (picking).
    BoundingBox bounds;                 ///< Accumulated bounds (radius-aware).

    /// \brief Number of atoms held.
    std::size_t size() const { return positions.size(); }

    /// \brief Append one atom and grow the bounding box.
    void add(const QVector3D& position, float radius, const Color& color,
             unsigned source_index) {
        positions.push_back(position);
        radii.push_back(radius);
        colors.push_back(color);
        source_idx.push_back(source_index);
        bounds.include(position, radius);
    }
};

}  // namespace oe3dmol
