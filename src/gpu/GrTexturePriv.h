/*
 * Copyright 2014 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef GrTexturePriv_DEFINED
#define GrTexturePriv_DEFINED

#include "src/gpu/GrSamplerState.h"
#include "src/gpu/GrTexture.h"

/** Class that adds methods to GrTexture that are only intended for use internal to Skia.
    This class is purely a privileged window into GrTexture. It should never have additional data
    members or virtual methods.
    Non-static methods that are not trivial inlines should be spring-boarded (e.g. declared and
    implemented privately in GrTexture with a inline public method here). */
class GrTexturePriv {
public:
    void markMipmapsDirty() {
        fTexture->markMipmapsDirty();
    }

    void markMipmapsClean() {
        fTexture->markMipmapsClean();
    }

    GrMipmapStatus mipmapStatus() const { return fTexture->fMipmapStatus; }

    bool mipmapsAreDirty() const { return GrMipmapStatus::kValid != this->mipmapStatus(); }

    GrMipmapped mipmapped() const {
        if (GrMipmapStatus::kNotAllocated != this->mipmapStatus()) {
            return GrMipmapped::kYes;
        }
        return GrMipmapped::kNo;
    }

    int maxMipmapLevel() const {
        return fTexture->fMaxMipmapLevel;
    }

    GrTextureType textureType() const { return fTexture->fTextureType; }
    bool hasRestrictedSampling() const {
        return GrTextureTypeHasRestrictedSampling(this->textureType());
    }

    static void ComputeScratchKey(const GrCaps& caps,
                                  const GrBackendFormat& format,
                                  SkISize dimensions,
                                  GrRenderable,
                                  int sampleCnt,
                                  GrMipmapped,
                                  GrProtected,
                                  GrScratchKey* key);

private:
    GrTexturePriv(GrTexture* texture) : fTexture(texture) { }
    GrTexturePriv(const GrTexturePriv& that) : fTexture(that.fTexture) { }
    GrTexturePriv& operator=(const GrTexturePriv&); // unimpl

    // No taking addresses of this type.
    const GrTexturePriv* operator&() const;
    GrTexturePriv* operator&();

    GrTexture* fTexture;

    friend class GrTexture; // to construct/copy this type.
};

inline GrTexturePriv GrTexture::texturePriv() { return GrTexturePriv(this); }

inline const GrTexturePriv GrTexture::texturePriv () const {
    return GrTexturePriv(const_cast<GrTexture*>(this));
}

#endif
