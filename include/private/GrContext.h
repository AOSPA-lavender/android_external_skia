/*
 * Copyright 2010 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef GrContext_DEFINED
#define GrContext_DEFINED

#include "include/core/SkMatrix.h"
#include "include/core/SkPathEffect.h"
#include "include/core/SkTypes.h"
#include "include/gpu/GrBackendSurface.h"
#include "include/gpu/GrContextOptions.h"
#include "include/gpu/GrRecordingContext.h"

// We shouldn't need this but currently Android is relying on this being include transitively.
#include "include/core/SkUnPreMultiply.h"

class GrAtlasManager;
class GrBackendSemaphore;
class GrCaps;
class GrClientMappedBufferManager;
class GrContextPriv;
class GrContextThreadSafeProxy;
struct GrD3DBackendContext;
class GrFragmentProcessor;
struct GrGLInterface;
class GrGpu;
struct GrMockOptions;
class GrPath;
class GrRenderTargetContext;
class GrResourceCache;
class GrResourceProvider;
class GrSmallPathAtlasMgr;
class GrStrikeCache;
class GrSurfaceProxy;
class GrSwizzle;
class GrTextureProxy;
struct GrVkBackendContext;

class SkImage;
class SkString;
class SkSurfaceCharacterization;
class SkSurfaceProps;
class SkTaskGroup;
class SkTraceMemoryDump;

/**
 * This deprecated class is being merged into GrDirectContext and removed.
 * Do not add new subclasses, new API, or attempt to instantiate one.
 * If new API requires direct GPU access, add it to GrDirectContext.
 * Otherwise, add it to GrRecordingContext.
 */
class SK_API GrContext : public GrRecordingContext {
public:
    ~GrContext() override;

    ///////////////////////////////////////////////////////////////////////////
    // Misc.

    /**
     * Inserts a list of GPU semaphores that the current GPU-backed API must wait on before
     * executing any more commands on the GPU. If this call returns false, then the GPU back-end
     * will not wait on any passed in semaphores, and the client will still own the semaphores,
     * regardless of the value of deleteSemaphoresAfterWait.
     *
     * If deleteSemaphoresAfterWait is false then Skia will not delete the semaphores. In this case
     * it is the client's responsibility to not destroy or attempt to reuse the semaphores until it
     * knows that Skia has finished waiting on them. This can be done by using finishedProcs on
     * flush calls.
     */
    bool wait(int numSemaphores, const GrBackendSemaphore* waitSemaphores,
              bool deleteSemaphoresAfterWait = true);

    /**
     * Call to ensure all drawing to the context has been flushed and submitted to the underlying 3D
     * API. This is equivalent to calling GrContext::flush with a default GrFlushInfo followed by
     * GrContext::submit(syncCpu).
     */
    void flushAndSubmit(bool syncCpu = false) {
        this->flush(GrFlushInfo());
        this->submit(syncCpu);
    }

    /**
     * Call to ensure all drawing to the context has been flushed to underlying 3D API specific
     * objects. A call to GrContext::submit is always required to ensure work is actually sent to
     * the gpu. Some specific API details:
     *     GL: Commands are actually sent to the driver, but glFlush is never called. Thus some
     *         sync objects from the flush will not be valid until a submission occurs.
     *
     *     Vulkan/Metal/D3D/Dawn: Commands are recorded to the backend APIs corresponding command
     *         buffer or encoder objects. However, these objects are not sent to the gpu until a
     *         submission occurs.
     *
     * If the return is GrSemaphoresSubmitted::kYes, only initialized GrBackendSemaphores will be
     * submitted to the gpu during the next submit call (it is possible Skia failed to create a
     * subset of the semaphores). The client should not wait on these semaphores until after submit
     * has been called, and must keep them alive until then. If this call returns
     * GrSemaphoresSubmitted::kNo, the GPU backend will not submit any semaphores to be signaled on
     * the GPU. Thus the client should not have the GPU wait on any of the semaphores passed in with
     * the GrFlushInfo. Regardless of whether semaphores were submitted to the GPU or not, the
     * client is still responsible for deleting any initialized semaphores.
     * Regardleess of semaphore submission the context will still be flushed. It should be
     * emphasized that a return value of GrSemaphoresSubmitted::kNo does not mean the flush did not
     * happen. It simply means there were no semaphores submitted to the GPU. A caller should only
     * take this as a failure if they passed in semaphores to be submitted.
     */
    GrSemaphoresSubmitted flush(const GrFlushInfo& info);

    void flush() { this->flush({}); }

    /**
     * Submit outstanding work to the gpu from all previously un-submitted flushes. The return
     * value of the submit will indicate whether or not the submission to the GPU was successful.
     *
     * If the call returns true, all previously passed in semaphores in flush calls will have been
     * submitted to the GPU and they can safely be waited on. The caller should wait on those
     * semaphores or perform some other global synchronization before deleting the semaphores.
     *
     * If it returns false, then those same semaphores will not have been submitted and we will not
     * try to submit them again. The caller is free to delete the semaphores at any time.
     *
     * If the syncCpu flag is true this function will return once the gpu has finished with all
     * submitted work.
     */
    bool submit(bool syncCpu = false);

    /**
     * Checks whether any asynchronous work is complete and if so calls related callbacks.
     */
    void checkAsyncWorkCompletion();

    // Provides access to functions that aren't part of the public API.
    GrContextPriv priv();
    const GrContextPriv priv() const;  // NOLINT(readability-const-return-type)

    /** Enumerates all cached GPU resources and dumps their memory to traceMemoryDump. */
    // Chrome is using this!
    void dumpMemoryStatistics(SkTraceMemoryDump* traceMemoryDump) const;

    bool supportsDistanceFieldText() const;

    void storeVkPipelineCacheData();

    // Returns the gpu memory size of the the texture that backs the passed in SkImage. Returns 0 if
    // the SkImage is not texture backed. For external format textures this will also return 0 as we
    // cannot determine the correct size.
    static size_t ComputeImageSize(sk_sp<SkImage> image, GrMipmapped, bool useNextPow2 = false);

    /**
     * Retrieve the default GrBackendFormat for a given SkColorType and renderability.
     * It is guaranteed that this backend format will be the one used by the following
     * SkColorType and SkSurfaceCharacterization-based createBackendTexture methods.
     *
     * The caller should check that the returned format is valid.
     */
    GrBackendFormat defaultBackendFormat(SkColorType ct, GrRenderable renderable) const {
        return INHERITED::defaultBackendFormat(ct, renderable);
    }

   /**
    * The explicitly allocated backend texture API allows clients to use Skia to create backend
    * objects outside of Skia proper (i.e., Skia's caching system will not know about them.)
    *
    * It is the client's responsibility to delete all these objects (using deleteBackendTexture)
    * before deleting the GrContext used to create them. If the backend is Vulkan, the textures must
    * be deleted before abandoning the GrContext as well. Additionally, clients should only delete
    * these objects on the thread for which that GrContext is active.
    *
    * The client is responsible for ensuring synchronization between different uses
    * of the backend object (i.e., wrapping it in a surface, rendering to it, deleting the
    * surface, rewrapping it in a image and drawing the image will require explicit
    * sychronization on the client's part).
    */

    /**
     * If possible, create an uninitialized backend texture. The client should ensure that the
     * returned backend texture is valid.
     * For the Vulkan backend the layout of the created VkImage will be:
     *      VK_IMAGE_LAYOUT_UNDEFINED.
     */
    GrBackendTexture createBackendTexture(int width, int height,
                                          const GrBackendFormat&,
                                          GrMipmapped,
                                          GrRenderable,
                                          GrProtected = GrProtected::kNo);

    /**
     * If possible, create an uninitialized backend texture. The client should ensure that the
     * returned backend texture is valid.
     * If successful, the created backend texture will be compatible with the provided
     * SkColorType.
     * For the Vulkan backend the layout of the created VkImage will be:
     *      VK_IMAGE_LAYOUT_UNDEFINED.
     */
    GrBackendTexture createBackendTexture(int width, int height,
                                          SkColorType,
                                          GrMipmapped,
                                          GrRenderable,
                                          GrProtected = GrProtected::kNo);

    /**
     * If possible, create a backend texture initialized to a particular color. The client should
     * ensure that the returned backend texture is valid. The client can pass in a finishedProc
     * to be notified when the data has been uploaded by the gpu and the texture can be deleted. The
     * client is required to call GrContext::submit to send the upload work to the gpu. The
     * finishedProc will always get called even if we failed to create the GrBackendTexture.
     * For the Vulkan backend the layout of the created VkImage will be:
     *      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
     */
    GrBackendTexture createBackendTexture(int width, int height,
                                          const GrBackendFormat&,
                                          const SkColor4f& color,
                                          GrMipmapped,
                                          GrRenderable,
                                          GrProtected = GrProtected::kNo,
                                          GrGpuFinishedProc finishedProc = nullptr,
                                          GrGpuFinishedContext finishedContext = nullptr);

    /**
     * If possible, create a backend texture initialized to a particular color. The client should
     * ensure that the returned backend texture is valid. The client can pass in a finishedProc
     * to be notified when the data has been uploaded by the gpu and the texture can be deleted. The
     * client is required to call GrContext::submit to send the upload work to the gpu. The
     * finishedProc will always get called even if we failed to create the GrBackendTexture.
     * If successful, the created backend texture will be compatible with the provided
     * SkColorType.
     * For the Vulkan backend the layout of the created VkImage will be:
     *      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
     */
    GrBackendTexture createBackendTexture(int width, int height,
                                          SkColorType,
                                          const SkColor4f& color,
                                          GrMipmapped,
                                          GrRenderable,
                                          GrProtected = GrProtected::kNo,
                                          GrGpuFinishedProc finishedProc = nullptr,
                                          GrGpuFinishedContext finishedContext = nullptr);

    /**
     * If possible, create a backend texture initialized with the provided pixmap data. The client
     * should ensure that the returned backend texture is valid. The client can pass in a
     * finishedProc to be notified when the data has been uploaded by the gpu and the texture can be
     * deleted. The client is required to call GrContext::submit to send the upload work to the gpu.
     * The finishedProc will always get called even if we failed to create the GrBackendTexture.
     * If successful, the created backend texture will be compatible with the provided
     * pixmap(s). Compatible, in this case, means that the backend format will be the result
     * of calling defaultBackendFormat on the base pixmap's colortype. The src data can be deleted
     * when this call returns.
     * If numLevels is 1 a non-mipMapped texture will result. If a mipMapped texture is desired
     * the data for all the mipmap levels must be provided. In the mipmapped case all the
     * colortypes of the provided pixmaps must be the same. Additionally, all the miplevels
     * must be sized correctly (please see SkMipmap::ComputeLevelSize and ComputeLevelCount).
     * Note: the pixmap's alphatypes and colorspaces are ignored.
     * For the Vulkan backend the layout of the created VkImage will be:
     *      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
     */
    GrBackendTexture createBackendTexture(const SkPixmap srcData[], int numLevels,
                                          GrRenderable, GrProtected,
                                          GrGpuFinishedProc finishedProc = nullptr,
                                          GrGpuFinishedContext finishedContext = nullptr);

    // Helper version of above for a single level.
    GrBackendTexture createBackendTexture(const SkPixmap& srcData,
                                          GrRenderable renderable,
                                          GrProtected isProtected,
                                          GrGpuFinishedProc finishedProc = nullptr,
                                          GrGpuFinishedContext finishedContext = nullptr) {
        return this->createBackendTexture(&srcData, 1, renderable, isProtected, finishedProc,
                                          finishedContext);
    }

    /**
     * If possible, updates a backend texture to be filled to a particular color. The client should
     * check the return value to see if the update was successful. The client can pass in a
     * finishedProc to be notified when the data has been uploaded by the gpu and the texture can be
     * deleted. The client is required to call GrContext::submit to send the upload work to the gpu.
     * The finishedProc will always get called even if we failed to update the GrBackendTexture.
     * For the Vulkan backend after a successful update the layout of the created VkImage will be:
     *      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
     */
    bool updateBackendTexture(const GrBackendTexture&,
                              const SkColor4f& color,
                              GrGpuFinishedProc finishedProc,
                              GrGpuFinishedContext finishedContext);

    /**
     * If possible, updates a backend texture to be filled to a particular color. The data in
     * GrBackendTexture and passed in color is interpreted with respect to the passed in
     * SkColorType. The client should check the return value to see if the update was successful.
     * The client can pass in a finishedProc to be notified when the data has been uploaded by the
     * gpu and the texture can be deleted. The client is required to call GrContext::submit to send
     * the upload work to the gpu. The finishedProc will always get called even if we failed to
     * update the GrBackendTexture.
     * For the Vulkan backend after a successful update the layout of the created VkImage will be:
     *      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
     */
    bool updateBackendTexture(const GrBackendTexture&,
                              SkColorType skColorType,
                              const SkColor4f& color,
                              GrGpuFinishedProc finishedProc,
                              GrGpuFinishedContext finishedContext);

    /**
     * If possible, updates a backend texture filled with the provided pixmap data. The client
     * should check the return value to see if the update was successful. The client can pass in a
     * finishedProc to be notified when the data has been uploaded by the gpu and the texture can be
     * deleted. The client is required to call GrContext::submit to send the upload work to the gpu.
     * The finishedProc will always get called even if we failed to create the GrBackendTexture.
     * The backend texture must be compatible with the provided pixmap(s). Compatible, in this case,
     * means that the backend format is compatible with the base pixmap's colortype. The src data
     * can be deleted when this call returns.
     * If the backend texture is mip mapped, the data for all the mipmap levels must be provided.
     * In the mipmapped case all the colortypes of the provided pixmaps must be the same.
     * Additionally, all the miplevels must be sized correctly (please see
     * SkMipmap::ComputeLevelSize and ComputeLevelCount).
     * Note: the pixmap's alphatypes and colorspaces are ignored.
     * For the Vulkan backend after a successful update the layout of the created VkImage will be:
     *      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
     */
    bool updateBackendTexture(const GrBackendTexture&,
                              const SkPixmap srcData[],
                              int numLevels,
                              GrGpuFinishedProc finishedProc,
                              GrGpuFinishedContext finishedContext);

    /**
     * Retrieve the GrBackendFormat for a given SkImage::CompressionType. This is
     * guaranteed to match the backend format used by the following
     * createCompressedsBackendTexture methods that take a CompressionType.
     * The caller should check that the returned format is valid.
     */
    GrBackendFormat compressedBackendFormat(SkImage::CompressionType compression) const {
        return INHERITED::compressedBackendFormat(compression);
    }

    /**
     *If possible, create a compressed backend texture initialized to a particular color. The
     * client should ensure that the returned backend texture is valid. The client can pass in a
     * finishedProc to be notified when the data has been uploaded by the gpu and the texture can be
     * deleted. The client is required to call GrContext::submit to send the upload work to the gpu.
     * The finishedProc will always get called even if we failed to create the GrBackendTexture.
     * For the Vulkan backend the layout of the created VkImage will be:
     *      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
     */
    GrBackendTexture createCompressedBackendTexture(int width, int height,
                                                    const GrBackendFormat&,
                                                    const SkColor4f& color,
                                                    GrMipmapped,
                                                    GrProtected = GrProtected::kNo,
                                                    GrGpuFinishedProc finishedProc = nullptr,
                                                    GrGpuFinishedContext finishedContext = nullptr);

    GrBackendTexture createCompressedBackendTexture(int width, int height,
                                                    SkImage::CompressionType,
                                                    const SkColor4f& color,
                                                    GrMipmapped,
                                                    GrProtected = GrProtected::kNo,
                                                    GrGpuFinishedProc finishedProc = nullptr,
                                                    GrGpuFinishedContext finishedContext = nullptr);

    /**
     * If possible, create a backend texture initialized with the provided raw data. The client
     * should ensure that the returned backend texture is valid. The client can pass in a
     * finishedProc to be notified when the data has been uploaded by the gpu and the texture can be
     * deleted. The client is required to call GrContext::submit to send the upload work to the gpu.
     * The finishedProc will always get called even if we failed to create the GrBackendTexture
     * If numLevels is 1 a non-mipMapped texture will result. If a mipMapped texture is desired
     * the data for all the mipmap levels must be provided. Additionally, all the miplevels
     * must be sized correctly (please see SkMipmap::ComputeLevelSize and ComputeLevelCount).
     * For the Vulkan backend the layout of the created VkImage will be:
     *      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
     */
    GrBackendTexture createCompressedBackendTexture(int width, int height,
                                                    const GrBackendFormat&,
                                                    const void* data, size_t dataSize,
                                                    GrMipmapped,
                                                    GrProtected = GrProtected::kNo,
                                                    GrGpuFinishedProc finishedProc = nullptr,
                                                    GrGpuFinishedContext finishedContext = nullptr);

    GrBackendTexture createCompressedBackendTexture(int width, int height,
                                                    SkImage::CompressionType,
                                                    const void* data, size_t dataSize,
                                                    GrMipmapped,
                                                    GrProtected = GrProtected::kNo,
                                                    GrGpuFinishedProc finishedProc = nullptr,
                                                    GrGpuFinishedContext finishedContext = nullptr);

    /**
     * If possible, updates a backend texture filled with the provided color. If the texture is
     * mipmapped, all levels of the mip chain will be updated to have the supplied color. The client
     * should check the return value to see if the update was successful. The client can pass in a
     * finishedProc to be notified when the data has been uploaded by the gpu and the texture can be
     * deleted. The client is required to call GrContext::submit to send the upload work to the gpu.
     * The finishedProc will always get called even if we failed to create the GrBackendTexture.
     * For the Vulkan backend after a successful update the layout of the created VkImage will be:
     *      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
     */
    bool updateCompressedBackendTexture(const GrBackendTexture&,
                                        const SkColor4f& color,
                                        GrGpuFinishedProc finishedProc,
                                        GrGpuFinishedContext finishedContext);

    /**
     * If possible, updates a backend texture filled with the provided raw data. The client
     * should check the return value to see if the update was successful. The client can pass in a
     * finishedProc to be notified when the data has been uploaded by the gpu and the texture can be
     * deleted. The client is required to call GrContext::submit to send the upload work to the gpu.
     * The finishedProc will always get called even if we failed to create the GrBackendTexture.
     * If a mipMapped texture is passed in, the data for all the mipmap levels must be provided.
     * Additionally, all the miplevels must be sized correctly (please see
     * SkMipMap::ComputeLevelSize and ComputeLevelCount).
     * For the Vulkan backend after a successful update the layout of the created VkImage will be:
     *      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
     */
    bool updateCompressedBackendTexture(const GrBackendTexture&,
                                        const void* data,
                                        size_t dataSize,
                                        GrGpuFinishedProc finishedProc,
                                        GrGpuFinishedContext finishedContext);

    /**
     * Updates the state of the GrBackendTexture/RenderTarget to have the passed in
     * GrBackendSurfaceMutableState. All objects that wrap the backend surface (i.e. SkSurfaces and
     * SkImages) will also be aware of this state change. This call does not submit the state change
     * to the gpu, but requires the client to call GrContext::submit to send it to the GPU. The work
     * for this call is ordered linearly with all other calls that require GrContext::submit to be
     * called (e.g updateBackendTexture and flush). If finishedProc is not null then it will be
     * called with finishedContext after the state transition is known to have occurred on the GPU.
     *
     * See GrBackendSurfaceMutableState to see what state can be set via this call.
     *
     * If the backend API is Vulkan, the caller can set the GrBackendSurfaceMutableState's
     * VkImageLayout to VK_IMAGE_LAYOUT_UNDEFINED or queueFamilyIndex to VK_QUEUE_FAMILY_IGNORED to
     * tell Skia to not change those respective states.
     *
     * If previousState is not null and this returns true, then Skia will have filled in
     * previousState to have the values of the state before this call.
     */
    bool setBackendTextureState(const GrBackendTexture&,
                                const GrBackendSurfaceMutableState&,
                                GrBackendSurfaceMutableState* previousState = nullptr,
                                GrGpuFinishedProc finishedProc = nullptr,
                                GrGpuFinishedContext finishedContext = nullptr);
    bool setBackendRenderTargetState(const GrBackendRenderTarget&,
                                     const GrBackendSurfaceMutableState&,
                                     GrBackendSurfaceMutableState* previousState = nullptr,
                                     GrGpuFinishedProc finishedProc = nullptr,
                                     GrGpuFinishedContext finishedContext = nullptr);

    void deleteBackendTexture(GrBackendTexture);

    // This interface allows clients to pre-compile shaders and populate the runtime program cache.
    // The key and data blobs should be the ones passed to the PersistentCache, in SkSL format.
    //
    // Steps to use this API:
    //
    // 1) Create a GrContext as normal, but set fPersistentCache on GrContextOptions to something
    //    that will save the cached shader blobs. Set fShaderCacheStrategy to kSkSL. This will
    //    ensure that the blobs are SkSL, and are suitable for pre-compilation.
    // 2) Run your application, and save all of the key/data pairs that are fed to the cache.
    //
    // 3) Switch over to shipping your application. Include the key/data pairs from above.
    // 4) At startup (or any convenient time), call precompileShader for each key/data pair.
    //    This will compile the SkSL to create a GL program, and populate the runtime cache.
    //
    // This is only guaranteed to work if the context/device used in step #2 are created in the
    // same way as the one used in step #4, and the same GrContextOptions are specified.
    // Using cached shader blobs on a different device or driver are undefined.
    bool precompileShader(const SkData& key, const SkData& data);

#ifdef SK_ENABLE_DUMP_GPU
    /** Returns a string with detailed information about the context & GPU, in JSON format. */
    SkString dump() const;
#endif

protected:
    GrContext(sk_sp<GrContextThreadSafeProxy>);

    virtual GrAtlasManager* onGetAtlasManager() = 0;
    virtual GrSmallPathAtlasMgr* onGetSmallPathAtlasMgr() = 0;

private:
    friend class GrDirectContext; // for access to fGpu

    // fTaskGroup must appear before anything that uses it (e.g. fGpu), so that it is destroyed
    // after all of its users. Clients of fTaskGroup will generally want to ensure that they call
    // wait() on it as they are being destroyed, to avoid the possibility of pending tasks being
    // invoked after objects they depend upon have already been destroyed.
    std::unique_ptr<SkTaskGroup>            fTaskGroup;
    std::unique_ptr<GrStrikeCache>          fStrikeCache;
    sk_sp<GrGpu>                            fGpu;
    std::unique_ptr<GrResourceCache>        fResourceCache;
    std::unique_ptr<GrResourceProvider>     fResourceProvider;

    bool                                    fDidTestPMConversions;
    // true if the PM/UPM conversion succeeded; false otherwise
    bool                                    fPMUPMConversionsRoundTrip;

    GrContextOptions::PersistentCache*      fPersistentCache;
    GrContextOptions::ShaderErrorHandler*   fShaderErrorHandler;

    std::unique_ptr<GrClientMappedBufferManager> fMappedBufferManager;

    // TODO: have the GrClipStackClip use renderTargetContexts and rm this friending
    friend class GrContextPriv;

    using INHERITED = GrRecordingContext;
};

#endif
