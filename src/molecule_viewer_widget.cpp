#include "oe3dmol/molecule_viewer_widget.h"

#include <algorithm>
#include <cmath>
#include <variant>

#include <QMouseEvent>
#include <QWheelEvent>

#include <rhi/qrhi.h>

#include "oe3dmol/arcball.h"
#include "oe3dmol/bond_adapter.h"
#include "oe3dmol/mol_adapter.h"
#include "oe3dmol/scene_renderer.h"

namespace oe3dmol {

MoleculeViewerWidget::MoleculeViewerWidget(QWidget* parent) : QRhiWidget(parent) {}

MoleculeViewerWidget::~MoleculeViewerWidget() = default;

int MoleculeViewerWidget::add_model(const OEChem::OEMolBase* mol) {
    models_.push_back(Model{mol, {SphereStyle{}}});
    return static_cast<int>(models_.size()) - 1;
}

void MoleculeViewerWidget::set_style(int model_index, const SphereStyle& style) {
    set_style(model_index, std::vector<StyleSpec>{style});
}

void MoleculeViewerWidget::set_style(int model_index, const std::vector<StyleSpec>& styles) {
    if (model_index < 0 || model_index >= static_cast<int>(models_.size())) {
        return;
    }
    models_[static_cast<std::size_t>(model_index)].styles = styles;
    rebuild_geometry();
    if (renderer_) {
        renderer_->set_geometry(atoms_, bonds_);
    }
    update();
}

void MoleculeViewerWidget::set_background_color(const QColor& color) {
    background_ = color;
    update();
}

void MoleculeViewerWidget::rebuild_geometry() {
    atoms_ = AtomData{};
    bonds_ = BondData{};
    BoundingBox scene_bounds;
    for (const Model& m : models_) {
        for (const StyleSpec& spec : m.styles) {
            if (std::holds_alternative<SphereStyle>(spec)) {
                AtomData d = build_atom_data(m.mol, std::get<SphereStyle>(spec));
                for (std::size_t i = 0; i < d.size(); ++i) {
                    atoms_.add(d.positions[i], d.radii[i], d.colors[i], d.source_idx[i]);
                    scene_bounds.include(d.positions[i], d.radii[i]);
                }
            } else if (std::holds_alternative<StickStyle>(spec)) {
                BondData b = build_bond_data(m.mol, std::get<StickStyle>(spec));
                for (std::size_t i = 0; i < b.size(); ++i) {
                    bonds_.add(b.starts[i], b.ends[i], b.radii[i], b.colors[i],
                               b.source_bond_idx[i]);
                    scene_bounds.include(b.starts[i], b.radii[i]);
                    scene_bounds.include(b.ends[i], b.radii[i]);
                }
            }
        }
    }
    if (!camera_initialized_ && scene_bounds.valid()) {
        camera_.zoom_to(scene_bounds);
        camera_initialized_ = true;
    }
}

void MoleculeViewerWidget::initialize(QRhiCommandBuffer*) {
    // May be re-invoked on device loss; rebuild from scratch (idempotent).
    renderer_ = std::make_unique<SceneRenderer>(rhi(),
        renderTarget()->renderPassDescriptor());
    rebuild_geometry();
    renderer_->set_geometry(atoms_, bonds_);
}

void MoleculeViewerWidget::render(QRhiCommandBuffer* cb) {
    if (!renderer_) {
        return;
    }
    const QSize px = renderTarget()->pixelSize();
    const QColor bg = background_.toRgb();
    cb->beginPass(renderTarget(), bg, {1.0f, 0});
    renderer_->draw(cb, camera_, px);
    cb->endPass();
}

void MoleculeViewerWidget::releaseResources() {
    renderer_.reset();
}

void MoleculeViewerWidget::mousePressEvent(QMouseEvent* e) {
    last_pos_ = e->position().toPoint();
}

void MoleculeViewerWidget::mouseMoveEvent(QMouseEvent* e) {
    const QPoint pos = e->position().toPoint();
    const bool pan = (e->buttons() & Qt::RightButton) ||
                     ((e->buttons() & Qt::LeftButton) && (e->modifiers() & Qt::ShiftModifier));
    if (pan) {
        const float dx = static_cast<float>(pos.x() - last_pos_.x()) /
                         static_cast<float>(std::max(width(), 1));
        const float dy = static_cast<float>(pos.y() - last_pos_.y()) /
                         static_cast<float>(std::max(height(), 1));
        camera_.pan(dx, -dy);  // screen-down is -world-up
        update();
    } else if (e->buttons() & Qt::LeftButton) {
        camera_.orbit(arcball_rotation(QPointF(last_pos_), QPointF(pos), size()));
        update();
    }
    last_pos_ = pos;
}

void MoleculeViewerWidget::wheelEvent(QWheelEvent* e) {
    const float steps = static_cast<float>(e->angleDelta().y()) / 120.0f;
    camera_.dolly(std::pow(0.9f, steps));  // wheel up = zoom in (distance shrinks)
    update();
}

}  // namespace oe3dmol
