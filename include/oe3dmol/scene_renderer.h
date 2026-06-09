#pragma once

/// \file scene_renderer.h
/// \brief Persistent renderer owning the sphere + cylinder pipelines.
///
/// Built once against a caller-provided QRhi + render pass; does NOT own the
/// device or context. Shared by the offscreen Viewer and the interactive
/// QRhiWidget so model->geometry->pipeline orchestration lives in one place.

#include <memory>
#include <vector>

#include <QSize>

#include "oe3dmol/atom_data.h"
#include "oe3dmol/bond_data.h"
#include "oe3dmol/camera.h"
#include "oe3dmol/mesh_buffer.h"

QT_BEGIN_NAMESPACE
class QRhi;
class QRhiCommandBuffer;
class QRhiResourceUpdateBatch;
class QRhiRenderPassDescriptor;
QT_END_NAMESPACE

namespace oe3dmol {

class SphereImposterPipeline;
class CylinderPipeline;

/// \brief Owns both representation pipelines and the scene's GPU geometry.
class SceneRenderer {
public:
    /// \brief Build both pipelines against a device + render pass.
    /// \throws RenderError on pipeline/shader failure.
    SceneRenderer(QRhi* rhi, QRhiRenderPassDescriptor* rp);
    ~SceneRenderer();

    SceneRenderer(const SceneRenderer&) = delete;
    SceneRenderer& operator=(const SceneRenderer&) = delete;

    /// \brief Set the scene geometry (CPU side); rebuilds GPU buffers on next draw.
    void set_geometry(const AtomData& atoms, const BondData& bonds);

    /// \brief Record a frame: upload (if dirty), set camera uniforms, draw both.
    void draw(QRhiCommandBuffer* cb, const Camera& camera, const QSize& viewport);

private:
    QRhi* rhi_ = nullptr;
    std::unique_ptr<SphereImposterPipeline> spheres_;
    std::unique_ptr<CylinderPipeline> cylinders_;
    MeshBuffer sphere_mesh_;
    MeshBuffer unit_cylinder_;
    std::vector<float> instances_;
    bool geometry_dirty_ = false;
    bool has_spheres_ = false;
    bool has_sticks_ = false;
};

}  // namespace oe3dmol
