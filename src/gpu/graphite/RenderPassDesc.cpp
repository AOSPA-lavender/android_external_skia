/*
 * Copyright 2024 Google LLC
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "src/gpu/graphite/RenderPassDesc.h"

#include "src/gpu/graphite/Caps.h"

namespace skgpu::graphite {

namespace {

const char* to_str(LoadOp op) {
    switch (op) {
        case LoadOp::kLoad:    return "kLoad";
        case LoadOp::kClear:   return "kClear";
        case LoadOp::kDiscard: return "kDiscard";
    }

    SkUNREACHABLE;
}

const char* to_str(StoreOp op) {
    switch (op) {
        case StoreOp::kStore:   return "kStore";
        case StoreOp::kDiscard: return "kDiscard";
    }

    SkUNREACHABLE;
}

} // anonymous namespace

RenderPassDesc RenderPassDesc::Make(const Caps* caps,
                                    const TextureInfo& targetInfo,
                                    LoadOp loadOp,
                                    StoreOp storeOp,
                                    SkEnumBitMask<DepthStencilFlags> depthStencilFlags,
                                    const std::array<float, 4>& clearColor,
                                    bool requiresMSAA,
                                    Swizzle writeSwizzle) {
    RenderPassDesc desc;
    desc.fWriteSwizzle = writeSwizzle;
    desc.fSampleCount = 1;
    // It doesn't make sense to have a storeOp for our main target not be store. Why are we doing
    // this DrawPass then
    SkASSERT(storeOp == StoreOp::kStore);
    if (requiresMSAA) {
        if (caps->msaaRenderToSingleSampledSupport()) {
            desc.fColorAttachment.fTextureInfo = targetInfo;
            desc.fColorAttachment.fLoadOp = loadOp;
            desc.fColorAttachment.fStoreOp = storeOp;
            desc.fSampleCount = caps->defaultMSAASamplesCount();
        } else {
            // TODO: If the resolve texture isn't readable, the MSAA color attachment will need to
            // be persistently associated with the framebuffer, in which case it's not discardable.
            auto msaaTextureInfo = caps->getDefaultMSAATextureInfo(targetInfo, Discardable::kYes);
            if (msaaTextureInfo.isValid()) {
                desc.fColorAttachment.fTextureInfo = msaaTextureInfo;
                if (loadOp != LoadOp::kClear) {
                    desc.fColorAttachment.fLoadOp = LoadOp::kDiscard;
                } else {
                    desc.fColorAttachment.fLoadOp = LoadOp::kClear;
                }
                desc.fColorAttachment.fStoreOp = StoreOp::kDiscard;

                desc.fColorResolveAttachment.fTextureInfo = targetInfo;
                if (loadOp != LoadOp::kLoad) {
                    desc.fColorResolveAttachment.fLoadOp = LoadOp::kDiscard;
                } else {
                    desc.fColorResolveAttachment.fLoadOp = LoadOp::kLoad;
                }
                desc.fColorResolveAttachment.fStoreOp = storeOp;

                desc.fSampleCount = msaaTextureInfo.numSamples();
            } else {
                // fall back to single sampled
                desc.fColorAttachment.fTextureInfo = targetInfo;
                desc.fColorAttachment.fLoadOp = loadOp;
                desc.fColorAttachment.fStoreOp = storeOp;
            }
        }
    } else {
        desc.fColorAttachment.fTextureInfo = targetInfo;
        desc.fColorAttachment.fLoadOp = loadOp;
        desc.fColorAttachment.fStoreOp = storeOp;
    }
    desc.fClearColor = clearColor;

    if (depthStencilFlags != DepthStencilFlags::kNone) {
        desc.fDepthStencilAttachment.fTextureInfo = caps->getDefaultDepthStencilTextureInfo(
                depthStencilFlags, desc.fSampleCount, targetInfo.isProtected());
        // Always clear the depth and stencil to 0 at the start of a DrawPass, but discard at the
        // end since their contents do not affect the next frame.
        desc.fDepthStencilAttachment.fLoadOp = LoadOp::kClear;
        desc.fClearDepth = 0.f;
        desc.fClearStencil = 0;
        desc.fDepthStencilAttachment.fStoreOp = StoreOp::kDiscard;
    }

    return desc;
}

SkString RenderPassDesc::toString() const {
    // This intentionally includes the fixed state that impacts pipeline compilation
    return SkStringPrintf("RP(color: %s, resolve: %s, ds: %s, samples: %d, swizzle: %s)",
                          fColorAttachment.toString().c_str(),
                          fColorResolveAttachment.toString().c_str(),
                          fDepthStencilAttachment.toString().c_str(),
                          fSampleCount,
                          fWriteSwizzle.asString().c_str());
}

SkString AttachmentDesc::toString() const {
    if (fTextureInfo.isValid()) {
        return SkStringPrintf("info: %s loadOp: %s storeOp: %s",
                              fTextureInfo.toString().c_str(),
                              to_str(fLoadOp),
                              to_str(fStoreOp));
    } else {
        return SkString("invalid attachment");
    }
}

} // namespace skgpu::graphite