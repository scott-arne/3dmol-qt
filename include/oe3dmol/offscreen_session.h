#pragma once

/// \file offscreen_session.h
/// \brief Persistent offscreen render session: owns the QRhi device + pipelines
///        across many frames, for camera animation / batch poses / evolving scenes.

#include <memory>
#include <string>
#include <vector>

#include <QColor>
#include <QImage>
#include <QSize>

#include "oe3dmol/atom_data.h"
#include "oe3dmol/bond_data.h"
#include "oe3dmol/bounding_box.h"
#include "oe3dmol/camera.h"
#include "oe3dmol/scene_builder.h"
#include "oe3dmol/style.h"
#include "oe3dmol/style_spec.h"

namespace OEChem {
class OEMolBase;
}

namespace oe3dmol {

class RhiContext;
class SceneRenderer;

/// \brief Persistent offscreen render session. Unlike the one-shot Viewer, this
/// keeps the QRhi device, both pipelines, and GPU buffers alive across frames and
/// re-uploads geometry only when models/styles change (camera-only frames are
/// cheap redraws). The caller drives the camera and owns the per-frame loop.
///
/// \note Models are held by non-owning pointer (re-read at render time); the
///       caller MUST keep every added OEMolBase alive for the session's lifetime.
class OffscreenSession {
public:
    /// \brief Construct a session with a fixed offscreen framebuffer size.
    /// \throws RenderError if width or height is not positive.
    explicit OffscreenSession(int width, int height);
    ~OffscreenSession();

    OffscreenSession(const OffscreenSession&) = delete;
    OffscreenSession& operator=(const OffscreenSession&) = delete;

    /// \brief Add a molecule (non-owning). \returns its model index.
    /// \throws DataError if mol is null or has no atoms.
    int add_model(const OEChem::OEMolBase* mol);

    /// \brief Apply a single sphere style to a model.
    void set_style(int model_index, const SphereStyle& style);
    /// \brief Apply a list of styles to a model (sphere + stick = ball-and-stick).
    /// \throws RenderError if model_index is out of range.
    void set_style(int model_index, const std::vector<StyleSpec>& styles);

    /// \brief Set the background color (default white).
    void set_background_color(const QColor& color);

    /// \brief The mutable camera the caller drives per frame.
    Camera& camera() { return camera_; }
    /// \brief Read-only camera accessor.
    const Camera& camera() const { return camera_; }

    /// \brief Combined scene bounds of the current models/styles (invalid if no
    /// drawable geometry). For manual framing.
    BoundingBox scene_bounds() const { return scene_bounds_; }
    /// \brief Re-fit the camera to the current scene_bounds (no-op if invalid).
    void reframe();

    /// \brief Render one frame to a QImage using the persistent stack.
    /// \throws RenderError if there are no models, no drawable geometry, or device
    ///         loss recovery fails.
    QImage render_frame();
    /// \brief render_frame() + write a PNG. \throws RenderError on render/write failure.
    void save_frame(const std::string& path);

private:
    void rebuild_geometry();  // build_scene -> cache atoms/bonds/bounds, auto-frame, push if renderer exists
    void ensure_device();     // lazily build ctx_/renderer_ and push current geometry
    QImage render_once();     // one frame attempt (may throw DeviceLostError)

    QSize size_;
    QColor background_ = QColor(255, 255, 255);
    std::vector<SceneModel> models_;
    AtomData atoms_;
    BondData bonds_;
    BoundingBox scene_bounds_;
    Camera camera_;
    std::unique_ptr<RhiContext> ctx_;       // lazily built on first render_frame
    std::unique_ptr<SceneRenderer> renderer_;
    bool camera_initialized_ = false;       // auto-frame-once guard
};

}  // namespace oe3dmol
