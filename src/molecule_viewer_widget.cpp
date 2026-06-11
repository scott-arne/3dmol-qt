#include "oe3dmol/molecule_viewer_widget.h"

#include <algorithm>
#include <cmath>

#include <QMouseEvent>
#include <QWheelEvent>

#include <rhi/qrhi.h>

#include "oe3dmol/arcball.h"
#include "oe3dmol/render_errors.h"
#include "oe3dmol/scene_builder.h"
#include "oe3dmol/scene_renderer.h"

namespace oe3dmol {

MoleculeViewerWidget::MoleculeViewerWidget(QWidget* parent) : QRhiWidget(parent) {}

MoleculeViewerWidget::~MoleculeViewerWidget() = default;

int MoleculeViewerWidget::add_model(const OEChem::OEMolBase* mol) {
    models_.push_back(SceneModel{mol, {SphereStyle{}}});
    const int index = static_cast<int>(models_.size()) - 1;
    refresh();
    return index;
}

void MoleculeViewerWidget::set_style(int model_index, const SphereStyle& style) {
    set_style(model_index, std::vector<StyleSpec>{style});
}

void MoleculeViewerWidget::set_style(int model_index, const std::vector<StyleSpec>& styles) {
    if (model_index < 0 || model_index >= static_cast<int>(models_.size())) {
        throw RenderError("MoleculeViewerWidget: set_style model index out of range");
    }
    models_[static_cast<std::size_t>(model_index)].styles = styles;
    refresh();
}

void MoleculeViewerWidget::refresh() {
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
    BoundingBox scene_bounds;
    build_scene(models_, atoms_, bonds_, scene_bounds);
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
    const float dx = static_cast<float>(pos.x() - last_pos_.x());
    const float dy = static_cast<float>(pos.y() - last_pos_.y());
    const bool pan = (e->buttons() & Qt::RightButton) ||
                     ((e->buttons() & Qt::LeftButton) && (e->modifiers() & Qt::ShiftModifier));
    if (pan) {
        // Pass raw pixel deltas; the camera converts them to world units with a
        // single per-pixel scale so the content tracks the cursor without skew.
        camera_.pan(dx, -dy, size());  // screen-down is -world-up
        update();
    } else if (e->buttons() & Qt::MiddleButton) {
        // Middle-drag moves the camera along its view axis without re-fitting
        // the clip planes, so dragging up pushes the near plane into the scene
        // (clip-plane inspection). Drag up = toward scene (negative dy).
        camera_.move_z(-dy, size());
        update();
    } else if (e->buttons() & Qt::LeftButton) {
        camera_.orbit(arcball_rotation(QPointF(last_pos_), QPointF(pos), size()));
        update();
    }
    last_pos_ = pos;
}

void MoleculeViewerWidget::wheelEvent(QWheelEvent* e) {
    const int delta_y = e->angleDelta().y();
    if (delta_y == 0) {
        return;  // horizontal-only / null wheel: nothing to dolly
    }
    const float steps = static_cast<float>(delta_y) / 120.0f;
    camera_.dolly(std::pow(0.9f, steps));  // wheel up = zoom in (distance shrinks)
    update();
}

}  // namespace oe3dmol
