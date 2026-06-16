#pragma once

/// \file detail/shader_resources.h
/// \brief Internal: register the embedded shader .qsb resources once. Not public.

/// \brief Force registration of the :/shaders/*.qsb resources (Q_INIT_RESOURCE).
/// Declared at global scope (no namespace) so it can call Q_INIT_RESOURCE, whose
/// generated symbol is global. Both pipelines call this from their constructors.
void oe3dmol_ensure_shader_resources();
