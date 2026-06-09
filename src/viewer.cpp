#include "oe3dmol/viewer.h"

#include "oe3dmol/bond_adapter.h"
#include "oe3dmol/camera.h"
#include "oe3dmol/cylinder_builder.h"
#include "oe3dmol/cylinder_pipeline.h"
#include "oe3dmol/mesh_buffer.h"
#include "oe3dmol/mol_adapter.h"
#include "oe3dmol/render_errors.h"
#include "oe3dmol/rhi_context.h"
#include "oe3dmol/sphere_imposter_builder.h"
#include "oe3dmol/sphere_imposter_pipeline.h"

#include <variant>

#include <rhi/qrhi.h>

namespace oe3dmol {

Viewer::Viewer(int width, int height) : size_(width, height) {
    if (width <= 0 || height <= 0) {
        throw RenderError("Viewer: width and height must be positive");
    }
}

Viewer::~Viewer() = default;

int Viewer::add_model(const OEChem::OEMolBase* mol) {
    models_.push_back(Model{mol, {SphereStyle{}}});
    return static_cast<int>(models_.size()) - 1;
}

void Viewer::set_style(int model_index, const SphereStyle& style) {
    set_style(model_index, std::vector<StyleSpec>{style});
}

void Viewer::set_style(int model_index, const std::vector<StyleSpec>& styles) {
    if (model_index < 0 || model_index >= static_cast<int>(models_.size())) {
        throw RenderError("Viewer: set_style model index out of range");
    }
    models_[static_cast<std::size_t>(model_index)].styles = styles;
}

void Viewer::zoom_to() {
    // Camera is fit at render time from the combined scene bounds.
}

QImage Viewer::render() {
    if (models_.empty()) {
        throw RenderError("Viewer: no models to render");
    }

    AtomData atoms;
    BondData bonds;
    BoundingBox scene_bounds;

    for (const Model& m : models_) {
        for (const StyleSpec& spec : m.styles) {
            if (std::holds_alternative<SphereStyle>(spec)) {
                AtomData d = build_atom_data(m.mol, std::get<SphereStyle>(spec));
                for (std::size_t i = 0; i < d.size(); ++i) {
                    atoms.add(d.positions[i], d.radii[i], d.colors[i], d.source_idx[i]);
                    scene_bounds.include(d.positions[i], d.radii[i]);
                }
            } else {
                BondData b = build_bond_data(m.mol, std::get<StickStyle>(spec));
                for (std::size_t i = 0; i < b.size(); ++i) {
                    bonds.add(b.starts[i], b.ends[i], b.radii[i], b.colors[i],
                              b.source_bond_idx[i]);
                    scene_bounds.include(b.starts[i], b.radii[i]);
                    scene_bounds.include(b.ends[i], b.radii[i]);
                }
            }
        }
    }

    MeshBuffer sphere_mesh = build_sphere_imposters(atoms);
    MeshBuffer unit_cyl = build_unit_cylinder(16);
    std::vector<float> instances = build_cylinder_instances(bonds);

    Camera cam;
    cam.zoom_to(scene_bounds.valid() ? scene_bounds : atoms.bounds);

    RhiContext ctx(size_);
    SphereImposterPipeline sphere_pipeline(ctx.rhi(), ctx.render_pass_descriptor());
    CylinderPipeline cyl_pipeline(ctx.rhi(), ctx.render_pass_descriptor());

    ImposterUniforms u;
    u.view = cam.view_matrix();
    u.projection = cam.projection_matrix(
        static_cast<float>(size_.width()) / static_cast<float>(size_.height()));
    u.model_view = u.view;

    const QSize sz = size_;
    const bool has_spheres = sphere_mesh.index_count() > 0;
    const bool has_sticks = !instances.empty();

    return ctx.render_frame(background_, [&](QRhiCommandBuffer* cb) {
        QRhiResourceUpdateBatch* batch = ctx.rhi()->nextResourceUpdateBatch();
        if (has_spheres) {
            sphere_pipeline.upload(sphere_mesh, batch);
            sphere_pipeline.set_uniforms(u, batch);
        }
        cyl_pipeline.upload(unit_cyl, instances, batch);
        cyl_pipeline.set_uniforms(u, batch);
        cb->resourceUpdate(batch);
        if (has_spheres) {
            sphere_pipeline.draw(cb, sz);
        }
        if (has_sticks) {
            cyl_pipeline.draw(cb, sz);
        }
    });
}

bool Viewer::render_to_png(const std::string& path) {
    QImage img = render();
    if (img.isNull()) {
        return false;
    }
    return img.save(QString::fromStdString(path), "PNG");
}

}  // namespace oe3dmol
