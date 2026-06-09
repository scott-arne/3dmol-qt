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
#include "oe3dmol/style.h"
#include "oe3dmol/style_spec.h"

namespace OEChem {
class OEMolBase;
}

namespace oe3dmol {

class SceneRenderer;

/// \brief Live molecular viewer widget: left-drag orbit, wheel zoom, right/shift
/// -drag pan. Reuses the SceneRenderer; models/styles mirror Viewer.
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

protected:
    void initialize(QRhiCommandBuffer* cb) override;
    void render(QRhiCommandBuffer* cb) override;
    void releaseResources() override;

    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void wheelEvent(QWheelEvent* e) override;

private:
    struct Model {
        const OEChem::OEMolBase* mol = nullptr;
        std::vector<StyleSpec> styles;
    };

    void rebuild_geometry();  // models -> AtomData/BondData -> camera fit
    void refresh();           // rebuild geometry, re-upload if initialized, repaint

    std::vector<Model> models_;
    QColor background_ = QColor(255, 255, 255);

    std::unique_ptr<SceneRenderer> renderer_;
    Camera camera_;
    AtomData atoms_;
    BondData bonds_;
    bool camera_initialized_ = false;

    QPoint last_pos_;
};

}  // namespace oe3dmol
