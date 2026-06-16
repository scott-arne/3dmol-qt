#pragma once

/// \file detail/validate_molecule.h
/// \brief Internal: reject null/empty molecules before they enter a model list.
///        Not public API.

#include <string>

#include <oechem.h>

#include "oe3dmol/data_errors.h"

namespace oe3dmol {
namespace detail {

/// \brief Throw DataError if `mol` is null or has zero atoms. Mirrors the
/// preconditions build_atom_data/build_bond_data enforce, so a model that
/// passes this cannot poison a later rebuild.
/// \param mol The candidate molecule (non-owning).
/// \param what Caller name for the error message (e.g. "Viewer::add_model").
inline void require_renderable_molecule(const OEChem::OEMolBase* mol, const char* what) {
    if (mol == nullptr) {
        throw DataError(std::string(what) + ": molecule is null");
    }
    if (mol->NumAtoms() == 0) {
        throw DataError(std::string(what) + ": molecule has no atoms");
    }
}

}  // namespace detail
}  // namespace oe3dmol
