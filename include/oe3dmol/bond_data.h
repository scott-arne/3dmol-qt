#pragma once

/// \file bond_data.h
/// \brief Backend-neutral structure-of-arrays buffer for bond half-cylinders.
///
/// Each chemical bond is split at its midpoint into two half-bonds so that each
/// half can take its near atom's color (3dmol's half-and-half stick coloring).
/// Lives in oe3dmol_core so cylinder geometry stays OpenEye-free.

#include <vector>

#include <QVector3D>

#include "oe3dmol/bounding_box.h"
#include "oe3dmol/color.h"

namespace oe3dmol {

/// \brief Per-half-bond render attributes in structure-of-arrays form.
struct BondData {
    std::vector<QVector3D> starts;          ///< Half-bond start (atom end).
    std::vector<QVector3D> ends;            ///< Half-bond end (bond midpoint).
    std::vector<float> radii;               ///< Cylinder radius per half-bond.
    std::vector<Color> colors;              ///< Near-atom color per half-bond.
    std::vector<unsigned> source_bond_idx;  ///< Source OE bond index (picking).
    BoundingBox bounds;                     ///< Accumulated bounds.

    /// \brief Number of half-bonds held.
    std::size_t size() const { return starts.size(); }

    /// \brief Reserve capacity for `n` half-bonds across all arrays.
    void reserve(std::size_t n) {
        starts.reserve(n);
        ends.reserve(n);
        radii.reserve(n);
        colors.reserve(n);
        source_bond_idx.reserve(n);
    }

    /// \brief Append one half-bond and grow the bounding box (endpoints, no radius).
    void add(const QVector3D& start, const QVector3D& end, float radius,
             const Color& color, unsigned bond_index) {
        starts.push_back(start);
        ends.push_back(end);
        radii.push_back(radius);
        colors.push_back(color);
        source_bond_idx.push_back(bond_index);
        bounds.include(start, radius);
        bounds.include(end, radius);
    }
};

}  // namespace oe3dmol
