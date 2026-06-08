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

/// \brief Headless RHI device with a single offscreen color+depth target.
class RhiContext {
public:
    /// \brief Create a headless RHI device and offscreen target of given size.
    /// \param size Pixel dimensions of the offscreen framebuffer.
    /// \throws RenderError if the device or target cannot be created.
    explicit RhiContext(const QSize& size);
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
