/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef GrTextureAdjuster_DEFINED
#define GrTextureAdjuster_DEFINED

#include "src/core/SkTLazy.h"
#include "src/gpu/GrTextureProducer.h"
#include "src/gpu/GrTextureProxy.h"

class GrRecordingContext;

/**
 * GrTextureProducer subclass that can be used when the user already has a texture that represents
 * image contents.
 */
class GrTextureAdjuster final : public GrTextureProducer {
public:
    GrTextureAdjuster(GrRecordingContext*, GrSurfaceProxyView, const GrColorInfo&,
                      uint32_t uniqueID, bool useDecal = false);

    std::unique_ptr<GrFragmentProcessor> createFragmentProcessor(
            const SkMatrix& textureMatrix,
            const SkRect& constraintRect,
            FilterConstraint,
            bool coordsLimitedToConstraintRect,
            const GrSamplerState::Filter* filterOrNullForBicubic) override;

private:
    GrSurfaceProxyView onRefTextureProxyViewForParams(GrSamplerState, bool willBeMipped) override;

    GrSurfaceProxyView makeMippedCopy();

    GrSurfaceProxyView fOriginal;
    uint32_t fUniqueID;

    typedef GrTextureProducer INHERITED;
};

#endif
