#pragma once

/// \file molecule_viewer_widget.h
/// \brief Interactive QRhiWidget that renders OpenEye molecules with orbit/zoom/pan.

#include <memory>
#include <vector>

#include <QColor>
#include <QPoint>
#include <QRhiWidget>

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

    QPoint last_pos_;
};

}  // namespace oe3dmol
