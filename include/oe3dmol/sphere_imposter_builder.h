#pragma once

/// \file sphere_imposter_builder.h
/// \brief Builds 3dmol.js-style sphere imposter geometry from AtomData.

#include "oe3dmol/atom_data.h"
#include "oe3dmol/mesh_buffer.h"

namespace oe3dmol {

/// \brief Construct billboard-quad imposter geometry for spheres.
///
/// Reproduces 3dmol.js GLModel.drawSphereImposter:
///   - 4 vertices per atom, all at the atom center,
///   - per-vertex normal encodes the corner offset (-r,r),(-r,-r),(r,-r),(r,r),
///     with z = 0,
///   - per-vertex color equals the atom color,
///   - two triangles per atom with indices [v,v+1,v+2, v+2,v+3,v].
///
/// \param atoms Atom render data.
/// \returns A MeshBuffer with 4*N vertices and 6*N indices.
///
/// \note Indices are 32-bit (MeshBuffer::indices is uint32_t), so the largest
///       emitted index 4*N-1 must fit in uint32_t. This caps a single mesh at
///       roughly 1.07 billion atoms — far beyond any real molecular system, but
///       a hard ceiling of the 32-bit index format.
MeshBuffer build_sphere_imposters(const AtomData& atoms);

}  // namespace oe3dmol
