#pragma once

/// \file rhi_context.h
/// \brief Owns a headless QRhi device and an offscreen render target.

#include <functional>
#include <memory>

#include <QColor>
#include <QImage>
#include <QSize>
#include <QString>

QT_BEGIN_NAMESPACE
class QRhi;
class QRhiTexture;
class QRhiRenderBuffer;
class QRhiTextureRenderTarget;
class QRhiRenderPassDescriptor;
class QRhiCommandBuffer;
QT_END_NAMESPACE

namespace oe3dmol {

/// \brief Whether RhiContext may accept the QRhi::Null fallback backend.
/// RequireHardware (default) throws if only Null is available, so production
/// offscreen rendering fails loudly instead of producing an unrendered image.
/// AllowNull lets construction/non-pixel tests opt into Null explicitly.
enum class BackendPolicy { RequireHardware, AllowNull };

/// \brief Headless RHI device with a single offscreen color+depth target.
class RhiContext {
public:
    /// \brief Create a headless RHI device and offscreen target of given size.
    /// \param size Pixel dimensions of the offscreen framebuffer.
    /// \param policy Backend policy; RequireHardware (default) throws RenderError
    ///        if only the QRhi::Null fallback is available.
    /// \throws RenderError if the device or target cannot be created, or if
    ///         policy is RequireHardware and only Null is available.
    explicit RhiContext(const QSize& size, BackendPolicy policy = BackendPolicy::RequireHardware);
    ~RhiContext();

    RhiContext(const RhiContext&) = delete;
    RhiContext& operator=(const RhiContext&) = delete;

    /// \brief Access the underlying QRhi (for pipeline creation).
    QRhi* rhi() const { return rhi_.get(); }
    /// \brief The offscreen render target.
    QRhiTextureRenderTarget* render_target() const { return rt_.get(); }
    /// \brief Render pass descriptor compatible with the target.
    QRhiRenderPassDescriptor* render_pass_descriptor() const { return rp_.get(); }
    /// \brief Framebuffer size.
    QSize size() const { return size_; }
    /// \brief Name of the active RHI backend (for diagnostics).
    QString backend_name() const;
    /// \brief True if the active backend is the QRhi::Null fallback (no real GPU).
    bool is_null_backend() const;

    /// \brief Render one frame: clear to clear_color, invoke draw_callback, read back.
    /// \param clear_color Background color.
    /// \param draw_callback Receives the active command buffer to record draws.
    /// \returns The framebuffer contents as a QImage (RGBA8888).
    /// \throws RenderError on readback failure.
    QImage render_frame(const QColor& clear_color,
                        const std::function<void(QRhiCommandBuffer*)>& draw_callback);

private:
    QSize size_;
    std::unique_ptr<QRhi> rhi_;
    std::unique_ptr<QRhiTexture> color_;
    std::unique_ptr<QRhiRenderBuffer> depth_;
    std::unique_ptr<QRhiTextureRenderTarget> rt_;
    std::unique_ptr<QRhiRenderPassDescriptor> rp_;
};

}  // namespace oe3dmol
