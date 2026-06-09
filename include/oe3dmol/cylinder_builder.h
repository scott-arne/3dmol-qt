#pragma once

/// \file cylinder_builder.h
/// \brief Builds the shared unit cylinder mesh and per-bond instance buffers.

#include <vector>

#include "oe3dmol/bond_data.h"
#include "oe3dmol/mesh_buffer.h"

namespace oe3dmol {

/// \brief Build a tessellated unit cylinder (axis +Z, z in [0,1], unit radius).
///
/// The side surface has `segments` radial divisions with outward radial normals;
/// flat end caps are included (center-fan triangles at z=0 and z=1). Uses
/// CylinderVertexLayout (position + normal, 6 floats/vertex). Built once and
/// shared across all bond instances.
///
/// \param segments Radial divisions (>= 3).
/// \returns Unit-cylinder mesh.
/// \throws GeometryError if segments < 3.
MeshBuffer build_unit_cylinder(int segments = 16);

/// \brief Pack half-bond records into a per-instance float buffer.
///
/// Layout per instance (CylinderInstanceLayout): start.xyz, end.xyz, radius,
/// color.rgba = 11 floats. One instance per half-bond.
///
/// \param bonds Half-bond data.
/// \returns Interleaved instance buffer (11 * bonds.size() floats); empty if no bonds.
std::vector<float> build_cylinder_instances(const BondData& bonds);

}  // namespace oe3dmol
