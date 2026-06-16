#include "oe3dmol/rhi_context.h"

#include "oe3dmol/render_errors.h"

#include <rhi/qrhi.h>

namespace oe3dmol {

namespace {

std::unique_ptr<QRhi> create_rhi() {
#if defined(Q_OS_MACOS)
    QRhiMetalInitParams params;
    std::unique_ptr<QRhi> rhi(QRhi::create(QRhi::Metal, &params));
#elif defined(Q_OS_WIN)
    QRhiD3D11InitParams params;
    std::unique_ptr<QRhi> rhi(QRhi::create(QRhi::D3D11, &params));
#else
    QRhiVulkanInitParams params;
    std::unique_ptr<QRhi> rhi(QRhi::create(QRhi::Vulkan, &params));
#endif
    if (!rhi) {
        QRhiNullInitParams null_params;
        rhi.reset(QRhi::create(QRhi::Null, &null_params));
    }
    return rhi;
}

}  // namespace

RhiContext::RhiContext(const QSize& size, BackendPolicy policy) : size_(size) {
    rhi_ = create_rhi();
    if (!rhi_) {
        throw RenderError("RhiContext: failed to create any QRhi backend");
    }
    if (policy == BackendPolicy::RequireHardware && is_null_backend()) {
        throw RenderError(
            "RhiContext: no hardware GPU backend available (got Null). Offscreen "
            "rendering requires a real device; pass BackendPolicy::AllowNull only "
            "for construction/non-pixel tests.");
    }

    color_.reset(rhi_->newTexture(QRhiTexture::RGBA8, size_, 1,
                                  QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
    render_require(color_ && color_->create(),
                   "RhiContext: failed to create color texture on backend " +
                       backend_name().toStdString());

    depth_.reset(rhi_->newRenderBuffer(QRhiRenderBuffer::DepthStencil, size_, 1));
    render_require(depth_ && depth_->create(),
                   "RhiContext: failed to create depth buffer on backend " +
                       backend_name().toStdString());

    QRhiTextureRenderTargetDescription desc;
    QRhiColorAttachment color_att(color_.get());
    desc.setColorAttachments({color_att});
    desc.setDepthStencilBuffer(depth_.get());

    rt_.reset(rhi_->newTextureRenderTarget(desc));
    render_require(rt_ != nullptr, "RhiContext: failed to create texture render target");
    rp_.reset(rt_->newCompatibleRenderPassDescriptor());
    render_require(rp_ != nullptr, "RhiContext: failed to create render pass descriptor");
    rt_->setRenderPassDescriptor(rp_.get());
    render_require(rt_->create(), "RhiContext: failed to create render target on backend " +
                                      backend_name().toStdString());
}

RhiContext::~RhiContext() = default;

QString RhiContext::backend_name() const {
    return rhi_ ? QString::fromLatin1(rhi_->backendName()) : QStringLiteral("none");
}

bool RhiContext::is_null_backend() const {
    return rhi_ && rhi_->backend() == QRhi::Null;
}

QImage RhiContext::render_frame(
    const QColor& clear_color,
    const std::function<void(QRhiCommandBuffer*)>& draw_callback) {
    QRhiCommandBuffer* cb = nullptr;
    if (rhi_->beginOffscreenFrame(&cb) != QRhi::FrameOpSuccess) {
        throw RenderError("RhiContext: beginOffscreenFrame failed on backend " +
                          backend_name().toStdString());
    }

    const QColor clear = clear_color.toRgb();
    const QRhiDepthStencilClearValue ds_clear(1.0f, 0);

    cb->beginPass(rt_.get(), clear, ds_clear);
    if (draw_callback) {
        draw_callback(cb);
    }

    QRhiReadbackResult readback;
    bool readback_done = false;
    readback.completed = [&readback_done]() { readback_done = true; };
    QRhiResourceUpdateBatch* batch = rhi_->nextResourceUpdateBatch();
    QRhiReadbackDescription rb(color_.get());
    batch->readBackTexture(rb, &readback);

    cb->endPass(batch);
    const QRhi::FrameOpResult end_result = rhi_->endOffscreenFrame();

    if (!readback_done || readback.data.isEmpty()) {
        const bool frame_ok = end_result == QRhi::FrameOpSuccess;
        throw RenderError(
            std::string("RhiContext: texture readback produced no data on backend ") +
            backend_name().toStdString() +
            (frame_ok ? "" : " (endOffscreenFrame reported a non-success result, "
                             "e.g. device loss)"));
    }

    QImage img(reinterpret_cast<const uchar*>(readback.data.constData()),
               readback.pixelSize.width(), readback.pixelSize.height(),
               QImage::Format_RGBA8888);
    QImage owned = img.copy();
    if (rhi_->isYUpInFramebuffer()) {
        owned = owned.flipped(Qt::Vertical);
    }
    return owned;
}

}  // namespace oe3dmol
