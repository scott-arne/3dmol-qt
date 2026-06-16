#pragma once

/// \file molecule_viewer_widget.h
/// \brief Interactive QRhiWidget that renders OpenEye molecules with orbit/zoom/pan.

#include <memory>
#include <vector>

#include <QColor>
#include <QMatrix4x4>
#include <QPoint>
#include <QRhiWidget>
#include <QString>

#include "oe3dmol/atom_data.h"
#include "oe3dmol/bond_data.h"
#include "oe3dmol/camera.h"
#include "oe3dmol/scene_builder.h"
#include "oe3dmol/style.h"
#include "oe3dmol/style_spec.h"

namespace OEChem {
class OEMolBase;
}

namespace oe3dmol {

class SceneRenderer;

/// \brief Live molecular viewer widget: left-drag orbit, wheel zoom, right/shift
/// -drag pan, middle-drag move the camera along its view axis (clip inspection).
/// Reuses the SceneRenderer; models/styles mirror Viewer.
///
/// \note Molecules are held by non-owning pointer and are re-read on every
///       geometry rebuild (each set_style/add_model and each device-loss
///       reinitialize). The caller MUST keep every added OEMolBase alive for
///       the entire lifetime of the widget.
class MoleculeViewerWidget : public QRhiWidget {
    Q_OBJECT
public:
    explicit MoleculeViewerWidget(QWidget* parent = nullptr);
    ~MoleculeViewerWidget() override;

    /// \brief Add a molecule (non-owning). \returns its model index.
    int add_model(const OEChem::OEMolBase* mol);
    /// \brief Apply a single sphere style to a model.
    void set_style(int model_index, const SphereStyle& style);
    /// \brief Apply a list of styles to a model (sphere + stick = ball-and-stick).
    void set_style(int model_index, const std::vector<StyleSpec>& styles);
    /// \brief Background color (default white).
    void set_background_color(const QColor& color);

    /// \brief Fit and center the camera on the current scene, resetting the
    /// orientation and the near/far clip planes so a full orbit never clips.
    ///
    /// This is the routine "frame the molecule" action: it recomputes the scene
    /// bounds from the current models/styles and re-frames the camera (the same
    /// fit applied automatically when the first model is added). Bound to the
    /// 'R' key in the interactive viewer, and callable directly. No-op until a
    /// model with valid bounds has been added.
    void zoom_to();

    /// \brief Current camera view matrix (world -> view). Read-only camera-state
    /// accessor; does not trigger a repaint.
    QMatrix4x4 view_matrix() const { return camera_.view_matrix(); }

    /// \brief Current camera distance from its target (read-only accessor).
    float camera_distance() const { return camera_.distance(); }

    /// \brief The most recent widget-status message, or empty if the last
    /// operation was clean. Two sources write it: render() records a caught
    /// render-layer exception here (so it cannot escape Qt's virtual render
    /// callback), and rebuild_geometry() records when the current models produce
    /// no drawable geometry. It reflects the MOST RECENT operation: a subsequent
    /// successful render() clears it. So after a model with no drawable geometry,
    /// the message is set on set_style/add_model and persists only until the next
    /// successful (background-only) repaint clears it.
    QString last_error() const { return last_error_; }

protected:
    void initialize(QRhiCommandBuffer* cb) override;
    void render(QRhiCommandBuffer* cb) override;
    void releaseResources() override;

    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void wheelEvent(QWheelEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;

private:
    void rebuild_geometry();  // models -> AtomData/BondData -> camera fit
    void refresh();           // rebuild geometry, re-upload if initialized, repaint

    std::vector<SceneModel> models_;
    QColor background_ = QColor(255, 255, 255);

    std::unique_ptr<SceneRenderer> renderer_;
    Camera camera_;
    AtomData atoms_;
    BondData bonds_;
    BoundingBox last_bounds_;  // most recent scene bounds, for zoom_to()
    bool camera_initialized_ = false;
    QString last_error_;  // last render-layer error; empty after a good render

    QPoint last_pos_;
};

}  // namespace oe3dmol
