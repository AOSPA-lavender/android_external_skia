/*
 * Copyright 2020 Google LLC
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef GrD3DSurfaceResource_DEFINED
#define GrD3DSurfaceResource_DEFINED

#include "include/core/SkTypes.h"
#include "include/gpu/GrBackendSurface.h"
#include "include/private/GrTypesPriv.h"
#include "src/gpu/GrManagedResource.h"
#include "src/gpu/d3d/GrD3D12.h"
#include "src/gpu/d3d/GrD3DResourceState.h"

class GrD3DGpu;

class GrD3DSurfaceResource : SkNoncopyable {
private:
    class Resource;

public:
    GrD3DSurfaceResource(const GrD3DTextureInfo& info, sk_sp<GrD3DResourceState> state,
                         GrBackendObjectOwnership ownership = GrBackendObjectOwnership::kOwned)
            : fInfo(info)
            , fState(std::move(state))
            , fStateExplicitlySet(true)
            , fIsBorrowed(GrBackendObjectOwnership::kBorrowed == ownership)
            , fResource(new Resource(fInfo.fTexture)) {
        if (fIsBorrowed) {
            fInfo.fTexture->AddRef();
        }
    }
    virtual ~GrD3DSurfaceResource();

    const Resource* resource() const {
        SkASSERT(fResource);
        return fResource.get();
    }
    DXGI_FORMAT dxgiFormat() const { return fInfo.fFormat; }
    GrBackendFormat getBackendFormat() const {
        return GrBackendFormat::MakeDxgi(this->dxgiFormat());
    }
    uint32_t mipLevels() const { return fInfo.fLevelCount; }
    bool isBorrowed() const { return fIsBorrowed; }

    sk_sp<GrD3DResourceState> grD3DResourceState() const { return fState; }

    D3D12_RESOURCE_STATES currentState() const {
        return fState->getResourceState();
    }

    void setResourceState(const GrD3DGpu* gpu, D3D12_RESOURCE_STATES newResourceState);

    // This simply updates our tracking of the resourceState and does not actually do any gpu work.
    // Externally, primarily used for implicit changes in resourceState due to certain GPU commands.
    void updateResourceState(D3D12_RESOURCE_STATES newState, bool explicitlySet) {
        SkASSERT(fResource);
        fState->setResourceState(newState);
        fStateExplicitlySet = explicitlySet;
    }

    static bool InitTextureInfo(GrD3DGpu* gpu, const D3D12_RESOURCE_DESC& desc, GrProtected,
                                GrD3DTextureInfo*);
    // Destroys the internal ID3D12Resource in the GrD3DTextureInfo
    static void DestroyTextureInfo(GrD3DTextureInfo*);

    void setResourceRelease(sk_sp<GrRefCntedCallback> releaseHelper);

protected:
    void releaseTexture(GrD3DGpu* gpu);
    bool hasResource() const { return SkToBool(fResource); }

    GrD3DTextureInfo fInfo;
    sk_sp<GrD3DResourceState> fState;
    bool fStateExplicitlySet;
    bool fIsBorrowed;

private:
    class Resource : public GrTextureResource {
    public:
        explicit Resource()
            : fResource(nullptr) {
        }

        Resource(ID3D12Resource* textureResource)
            : fResource(textureResource) {
            fResource->AddRef();
        }

        ~Resource() override {}

#ifdef SK_TRACE_MANAGED_RESOURCES
        void dumpInfo() const override {
            SkDebugf("GrD3DSurfaceResource: %d (%d refs)\n", fResource, this->getRefCnt());
        }
#endif

    private:
        void freeGPUData() const override;

        ID3D12Resource* fResource;

        typedef GrTextureResource INHERITED;
    };

    sk_sp<Resource> fResource;
};

#endif
