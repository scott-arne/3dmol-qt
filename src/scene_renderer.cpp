#include "oe3dmol/scene_renderer.h"

#include "oe3dmol/cylinder_builder.h"
#include "oe3dmol/cylinder_pipeline.h"
#include "oe3dmol/sphere_imposter_builder.h"
#include "oe3dmol/sphere_imposter_pipeline.h"

#include <rhi/qrhi.h>

namespace oe3dmol {

SceneRenderer::SceneRenderer(QRhi* rhi, QRhiRenderPassDescriptor* rp) : rhi_(rhi) {
    spheres_ = std::make_unique<SphereImposterPipeline>(rhi, rp);
    cylinders_ = std::make_unique<CylinderPipeline>(rhi, rp);
    cylinder_billboard_ = build_cylinder_billboard();
}

SceneRenderer::~SceneRenderer() = default;

void SceneRenderer::set_geometry(const AtomData& atoms, const BondData& bonds) {
    sphere_mesh_ = build_sphere_imposters(atoms);
    instances_ = build_cylinder_instances(bonds);
    has_spheres_ = sphere_mesh_.index_count() > 0;
    has_sticks_ = !instances_.empty();
    geometry_dirty_ = true;
}

void SceneRenderer::draw(QRhiCommandBuffer* cb, const Camera& camera, const QSize& viewport) {
    ImposterUniforms u;
    u.view = camera.view_matrix();
    u.projection = camera.projection_matrix(
        viewport.height() > 0
            ? static_cast<float>(viewport.width()) / static_cast<float>(viewport.height())
            : 1.0f);
    u.model_view = u.view;

    QRhiResourceUpdateBatch* batch = rhi_->nextResourceUpdateBatch();
    if (geometry_dirty_) {
        if (has_spheres_) {
            spheres_->upload(sphere_mesh_, batch);
        }
        cylinders_->upload(cylinder_billboard_, instances_, batch);
        geometry_dirty_ = false;
    }
    if (has_spheres_) {
        spheres_->set_uniforms(u, batch);
    }
    cylinders_->set_uniforms(u, batch);
    cb->resourceUpdate(batch);

    if (has_spheres_) {
        spheres_->draw(cb, viewport);
    }
    if (has_sticks_) {
        cylinders_->draw(cb, viewport);
    }
}

}  // namespace oe3dmol
