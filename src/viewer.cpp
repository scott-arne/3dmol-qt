#include "oe3dmol/viewer.h"

#include "oe3dmol/bond_adapter.h"
#include "oe3dmol/camera.h"
#include "oe3dmol/mol_adapter.h"
#include "oe3dmol/render_errors.h"
#include "oe3dmol/rhi_context.h"
#include "oe3dmol/scene_renderer.h"

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
                    scene_bounds.include(d.positions[i], d.radii[i]);
                }
                if (atoms.size() == 0) {
                    atoms = std::move(d);
                } else {
                    for (std::size_t i = 0; i < d.size(); ++i) {
                        atoms.add(d.positions[i], d.radii[i], d.colors[i],
                                  d.source_idx[i]);
                    }
                }
            } else if (std::holds_alternative<StickStyle>(spec)) {
                BondData b = build_bond_data(m.mol, std::get<StickStyle>(spec));
                for (std::size_t i = 0; i < b.size(); ++i) {
                    scene_bounds.include(b.starts[i], b.radii[i]);
                    scene_bounds.include(b.ends[i], b.radii[i]);
                }
                if (bonds.size() == 0) {
                    bonds = std::move(b);
                } else {
                    for (std::size_t i = 0; i < b.size(); ++i) {
                        bonds.add(b.starts[i], b.ends[i], b.radii[i], b.colors[i],
                                  b.source_bond_idx[i]);
                    }
                }
            }
        }
    }

    Camera cam;
    cam.zoom_to(scene_bounds.valid() ? scene_bounds : atoms.bounds);

    RhiContext ctx(size_);
    SceneRenderer renderer(ctx.rhi(), ctx.render_pass_descriptor());
    renderer.set_geometry(atoms, bonds);

    const QSize sz = size_;
    return ctx.render_frame(background_, [&](QRhiCommandBuffer* cb) {
        renderer.draw(cb, cam, sz);
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
