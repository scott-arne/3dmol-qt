#pragma once

/// \file cylinder_builder.h
/// \brief Builds the shared unit cylinder mesh and per-bond instance buffers.

#include <vector>

#include "oe3dmol/bond_data.h"
#include "oe3dmol/mesh_buffer.h"

namespace oe3dmol {

/// \brief Build the shared billboard base primitive for cylinder imposters.
///
/// A 4-vertex quad (6 indices). Each vertex carries (endpoint_sel, corner_sign)
/// per CylinderBillboardLayout; the vertex shader expands it around the bond
/// using the per-instance start/end/radius. Built once, shared by all bonds.
///
/// \returns A MeshBuffer of 4 vertices (2 floats each) and 6 indices.
MeshBuffer build_cylinder_billboard();

/// \brief Pack half-bond records into a per-instance float buffer.
///
/// Layout per instance (CylinderInstanceLayout): start.xyz, end.xyz, radius,
/// color.rgba = 11 floats. One instance per half-bond.
///
/// \param bonds Half-bond data.
/// \returns Interleaved instance buffer (11 * bonds.size() floats); empty if no bonds.
std::vector<float> build_cylinder_instances(const BondData& bonds);

}  // namespace oe3dmol
