/*
 * Copyright 2023 Google LLC
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "tests/Test.h"

#include "include/core/SkBitmap.h"
#include "include/gpu/graphite/Context.h"
#include "include/gpu/graphite/Recorder.h"
#include "src/gpu/GpuTypesPriv.h"
#include "src/gpu/graphite/ProxyCache.h"
#include "src/gpu/graphite/RecorderPriv.h"
#include "src/gpu/graphite/Texture.h"
#include "src/gpu/graphite/TextureProxy.h"
#include "tools/Resources.h"

#include <thread>

namespace skgpu::graphite {

// This test exercises the basic MessageBus behavior of the ProxyCache by manually inserting an
// SkBitmap into the proxy cache and then changing its contents. This simple test should create
// an IDChangeListener that will remove the entry in the cache when the bitmap is changed and
// the resulting message processed.
DEF_GRAPHITE_TEST_FOR_ALL_CONTEXTS(ProxyCacheTest1, r, context) {
    std::unique_ptr<Recorder> recorder = context->makeRecorder();
    ProxyCache* proxyCache = recorder->priv().proxyCache();

    SkBitmap bitmap;
    bool success = GetResourceAsBitmap("images/mandrill_128.png", &bitmap);
    REPORTER_ASSERT(r, success);
    if (!success) {
        return;
    }

    REPORTER_ASSERT(r, proxyCache->numCached() == 0);

    sk_sp<TextureProxy> proxy = proxyCache->findOrCreateCachedProxy(recorder.get(), bitmap,
                                                                    Mipmapped::kNo);

    REPORTER_ASSERT(r, proxyCache->numCached() == 1);

    bitmap.eraseColor(SK_ColorBLACK);

    proxyCache->forceProcessInvalidKeyMsgs();

    REPORTER_ASSERT(r, proxyCache->numCached() == 0);
}

// This test checks that, if the same bitmap is added to two separate ProxyCaches, when it is
// changed, both of the ProxyCaches will receive the message.
DEF_GRAPHITE_TEST_FOR_ALL_CONTEXTS(ProxyCacheTest2, r, context) {
    std::unique_ptr<Recorder> recorder1 = context->makeRecorder();
    ProxyCache* proxyCache1 = recorder1->priv().proxyCache();
    std::unique_ptr<Recorder> recorder2 = context->makeRecorder();
    ProxyCache* proxyCache2 = recorder2->priv().proxyCache();

    SkBitmap bitmap;
    bool success = GetResourceAsBitmap("images/mandrill_128.png", &bitmap);
    REPORTER_ASSERT(r, success);
    if (!success) {
        return;
    }

    REPORTER_ASSERT(r, proxyCache1->numCached() == 0);
    REPORTER_ASSERT(r, proxyCache2->numCached() == 0);

    sk_sp<TextureProxy> proxy1 = proxyCache1->findOrCreateCachedProxy(recorder1.get(), bitmap,
                                                                      Mipmapped::kNo);
    sk_sp<TextureProxy> proxy2 = proxyCache2->findOrCreateCachedProxy(recorder2.get(), bitmap,
                                                                      Mipmapped::kNo);

    REPORTER_ASSERT(r, proxyCache1->numCached() == 1);
    REPORTER_ASSERT(r, proxyCache2->numCached() == 1);

    bitmap.eraseColor(SK_ColorBLACK);

    proxyCache1->forceProcessInvalidKeyMsgs();
    proxyCache2->forceProcessInvalidKeyMsgs();

    REPORTER_ASSERT(r, proxyCache1->numCached() == 0);
    REPORTER_ASSERT(r, proxyCache2->numCached() == 0);
}

// This test exercises mipmap selectivity of the ProxyCache. Mipmapped and non-mipmapped version
// of the same bitmap are keyed differently. Requesting a non-mipmapped version will
// return a mipmapped version, if it exists.
DEF_GRAPHITE_TEST_FOR_ALL_CONTEXTS(ProxyCacheTest3, r, context) {
    std::unique_ptr<Recorder> recorder = context->makeRecorder();
    ProxyCache* proxyCache = recorder->priv().proxyCache();

    SkBitmap bitmap;
    bool success = GetResourceAsBitmap("images/mandrill_128.png", &bitmap);
    REPORTER_ASSERT(r, success);
    if (!success) {
        return;
    }

    REPORTER_ASSERT(r, proxyCache->numCached() == 0);

    sk_sp<TextureProxy> nonMipmapped = proxyCache->findOrCreateCachedProxy(recorder.get(), bitmap,
                                                                           Mipmapped::kNo);
    REPORTER_ASSERT(r, nonMipmapped->mipmapped() == Mipmapped::kNo);
    REPORTER_ASSERT(r, proxyCache->numCached() == 1);

    sk_sp<TextureProxy> test = proxyCache->findOrCreateCachedProxy(recorder.get(), bitmap,
                                                                   Mipmapped::kNo);
    REPORTER_ASSERT(r, nonMipmapped == test);
    REPORTER_ASSERT(r, proxyCache->numCached() == 1);

    sk_sp<TextureProxy> mipmapped = proxyCache->findOrCreateCachedProxy(recorder.get(), bitmap,
                                                                        Mipmapped::kYes);
    REPORTER_ASSERT(r, mipmapped->mipmapped() == Mipmapped::kYes);
    REPORTER_ASSERT(r, mipmapped != nonMipmapped);

    REPORTER_ASSERT(r, proxyCache->numCached() == 2);

    test = proxyCache->findOrCreateCachedProxy(recorder.get(), bitmap, Mipmapped::kNo);
    REPORTER_ASSERT(r, mipmapped == test);

    REPORTER_ASSERT(r, proxyCache->numCached() == 2);

    test = proxyCache->findOrCreateCachedProxy(recorder.get(), bitmap, Mipmapped::kYes);
    REPORTER_ASSERT(r, mipmapped == test);

    REPORTER_ASSERT(r, proxyCache->numCached() == 2);
}

// This test exercises the ProxyCache's freeUniquelyHeld method.
DEF_GRAPHITE_TEST_FOR_ALL_CONTEXTS(ProxyCacheTest4, r, context) {
    std::unique_ptr<Recorder> recorder = context->makeRecorder();
    ProxyCache* proxyCache = recorder->priv().proxyCache();

    SkBitmap bitmap;
    bool success = GetResourceAsBitmap("images/mandrill_128.png", &bitmap);
    REPORTER_ASSERT(r, success);
    if (!success) {
        return;
    }

    REPORTER_ASSERT(r, proxyCache->numCached() == 0);

    sk_sp<TextureProxy> nonMipmapped = proxyCache->findOrCreateCachedProxy(recorder.get(), bitmap,
                                                                           Mipmapped::kNo);
    REPORTER_ASSERT(r, proxyCache->numCached() == 1);

    sk_sp<TextureProxy> mipmapped = proxyCache->findOrCreateCachedProxy(recorder.get(), bitmap,
                                                                        Mipmapped::kYes);
    REPORTER_ASSERT(r, proxyCache->numCached() == 2);

    {
        // Ensure the Textures get created and any lingering creation refs are cleaned up
        auto recording = recorder->snap();
        context->insertRecording({ recording.get() });
        context->submit(SyncToCpu::kYes);
    }

    proxyCache->forceFreeUniquelyHeld();

    REPORTER_ASSERT(r, proxyCache->numCached() == 2);

    nonMipmapped.reset();
    proxyCache->forceFreeUniquelyHeld();

    REPORTER_ASSERT(r, proxyCache->numCached() == 1);

    mipmapped.reset();
    proxyCache->forceFreeUniquelyHeld();

    REPORTER_ASSERT(r, proxyCache->numCached() == 0);
}

// This test exercises the ProxyCache's purgeProxiesNotUsedSince method.
DEF_GRAPHITE_TEST_FOR_ALL_CONTEXTS(ProxyCacheTest5, r, context) {
    std::unique_ptr<Recorder> recorder = context->makeRecorder();
    ProxyCache* proxyCache = recorder->priv().proxyCache();

    SkBitmap bitmap;
    bool success = GetResourceAsBitmap("images/mandrill_128.png", &bitmap);
    REPORTER_ASSERT(r, success);
    if (!success) {
        return;
    }

    REPORTER_ASSERT(r, proxyCache->numCached() == 0);

    sk_sp<TextureProxy> proxy1 = proxyCache->findOrCreateCachedProxy(recorder.get(), bitmap,
                                                                     Mipmapped::kNo);
    REPORTER_ASSERT(r, proxyCache->numCached() == 1);

    {
        // Ensure proxy1's Texture is created (and timestamped) at this time
        auto recording = recorder->snap();
        context->insertRecording({ recording.get() });
        context->submit(SyncToCpu::kYes);
    }

    REPORTER_ASSERT(r, !proxy1->texture()->testingShouldDeleteASAP());

    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    auto time0 = skgpu::StdSteadyClock::now();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    sk_sp<TextureProxy> proxy2 = proxyCache->findOrCreateCachedProxy(recorder.get(), bitmap,
                                                                     Mipmapped::kYes);
    REPORTER_ASSERT(r, proxyCache->numCached() == 2);

    {
        // Ensure proxy2's Texture is created (and timestamped) at this time
        auto recording = recorder->snap();
        context->insertRecording({ recording.get() });
        context->submit(SyncToCpu::kYes);
    }

    REPORTER_ASSERT(r, !proxy2->texture()->testingShouldDeleteASAP());

    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    auto time1 = skgpu::StdSteadyClock::now();

    proxyCache->forcePurgeProxiesNotUsedSince(time0);
    REPORTER_ASSERT(r, proxyCache->numCached() == 1);
    REPORTER_ASSERT(r, proxy1->texture()->testingShouldDeleteASAP());
    REPORTER_ASSERT(r, !proxy2->texture()->testingShouldDeleteASAP());

    sk_sp<TextureProxy> test = proxyCache->find(bitmap, Mipmapped::kNo);
    REPORTER_ASSERT(r, !test);   // proxy1 should've been purged

    proxyCache->forcePurgeProxiesNotUsedSince(time1);
    REPORTER_ASSERT(r, proxyCache->numCached() == 0);
    REPORTER_ASSERT(r, proxy1->texture()->testingShouldDeleteASAP());
    REPORTER_ASSERT(r, proxy2->texture()->testingShouldDeleteASAP());
}

// This test simply verifies that the ProxyCache is correctly updating the Resource's
// last access time stamp.
DEF_GRAPHITE_TEST_FOR_ALL_CONTEXTS(ProxyCacheTest6, r, context) {
    std::unique_ptr<Recorder> recorder = context->makeRecorder();
    ProxyCache* proxyCache = recorder->priv().proxyCache();

    SkBitmap bitmap1, bitmap2;
    bool success1 = GetResourceAsBitmap("images/mandrill_128.png", &bitmap1);
    bool success2 = GetResourceAsBitmap("images/mandrill_256.png", &bitmap2);
    REPORTER_ASSERT(r, success1 && success2);
    if (!success1 || !success2) {
        return;
    }

    REPORTER_ASSERT(r, proxyCache->numCached() == 0);

    sk_sp<TextureProxy> proxy1 = proxyCache->findOrCreateCachedProxy(recorder.get(), bitmap1,
                                                                     Mipmapped::kNo);
    REPORTER_ASSERT(r, proxyCache->numCached() == 1);

    {
        // Ensure proxy1's Texture is created (and timestamped) at this time
        auto recording = recorder->snap();
        context->insertRecording({ recording.get() });
        context->submit(SyncToCpu::kYes);
    }

    REPORTER_ASSERT(r, !proxy1->texture()->testingShouldDeleteASAP());

    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    auto time0 = skgpu::StdSteadyClock::now();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    sk_sp<TextureProxy> proxy2 = proxyCache->findOrCreateCachedProxy(recorder.get(), bitmap2,
                                                                     Mipmapped::kNo);
    REPORTER_ASSERT(r, proxyCache->numCached() == 2);

    {
        // Ensure proxy2's Texture is created (and timestamped) at this time
        auto recording = recorder->snap();
        context->insertRecording({ recording.get() });
        context->submit(SyncToCpu::kYes);
    }

    REPORTER_ASSERT(r, !proxy2->texture()->testingShouldDeleteASAP());

    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    auto time1 = skgpu::StdSteadyClock::now();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    // update proxy1's timestamp
    sk_sp<TextureProxy> test = proxyCache->findOrCreateCachedProxy(recorder.get(), bitmap1,
                                                                   Mipmapped::kNo);
    REPORTER_ASSERT(r, test == proxy1);

    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    auto time2 = skgpu::StdSteadyClock::now();

    proxyCache->forcePurgeProxiesNotUsedSince(time0);
    REPORTER_ASSERT(r, proxyCache->numCached() == 2);
    REPORTER_ASSERT(r, !proxy1->texture()->testingShouldDeleteASAP());
    REPORTER_ASSERT(r, !proxy2->texture()->testingShouldDeleteASAP());

    proxyCache->forcePurgeProxiesNotUsedSince(time1);
    REPORTER_ASSERT(r, proxyCache->numCached() == 1);
    REPORTER_ASSERT(r, !proxy1->texture()->testingShouldDeleteASAP());
    REPORTER_ASSERT(r, proxy2->texture()->testingShouldDeleteASAP());

    test = proxyCache->find(bitmap2, Mipmapped::kNo);
    REPORTER_ASSERT(r, !test);   // proxy2 should've been purged

    proxyCache->forcePurgeProxiesNotUsedSince(time2);
    REPORTER_ASSERT(r, proxyCache->numCached() == 0);
    REPORTER_ASSERT(r, proxy1->texture()->testingShouldDeleteASAP());
    REPORTER_ASSERT(r, proxy2->texture()->testingShouldDeleteASAP());
}

// Verify that the ProxyCache's purgeProxiesNotUsedSince method can clear out multiple proxies.
DEF_GRAPHITE_TEST_FOR_ALL_CONTEXTS(ProxyCacheTest7, r, context) {
    std::unique_ptr<Recorder> recorder = context->makeRecorder();
    ProxyCache* proxyCache = recorder->priv().proxyCache();

    SkBitmap bitmap;
    bool success = GetResourceAsBitmap("images/mandrill_128.png", &bitmap);
    REPORTER_ASSERT(r, success);
    if (!success) {
        return;
    }

    REPORTER_ASSERT(r, proxyCache->numCached() == 0);

    sk_sp<TextureProxy> proxy1 = proxyCache->findOrCreateCachedProxy(recorder.get(), bitmap,
                                                                     Mipmapped::kNo);
    REPORTER_ASSERT(r, proxyCache->numCached() == 1);

    {
        // Ensure proxy1's Texture is created (and timestamped) at this time
        auto recording = recorder->snap();
        context->insertRecording({ recording.get() });
        context->submit(SyncToCpu::kYes);
    }

    sk_sp<TextureProxy> proxy2 = proxyCache->findOrCreateCachedProxy(recorder.get(), bitmap,
                                                                     Mipmapped::kYes);
    REPORTER_ASSERT(r, proxyCache->numCached() == 2);

    {
        // Ensure proxy2's Texture is created (and timestamped) at this time
        auto recording = recorder->snap();
        context->insertRecording({ recording.get() });
        context->submit(SyncToCpu::kYes);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    auto time0 = skgpu::StdSteadyClock::now();

    REPORTER_ASSERT(r, !proxy1->texture()->testingShouldDeleteASAP());
    REPORTER_ASSERT(r, !proxy2->texture()->testingShouldDeleteASAP());

    proxyCache->forcePurgeProxiesNotUsedSince(time0);
    REPORTER_ASSERT(r, proxyCache->numCached() == 0);
    REPORTER_ASSERT(r, proxy1->texture()->testingShouldDeleteASAP());
    REPORTER_ASSERT(r, proxy2->texture()->testingShouldDeleteASAP());
}

// Verify that the ProxyCache's freeUniquelyHeld behavior is working in the ResourceCache.
DEF_GRAPHITE_TEST_FOR_ALL_CONTEXTS(ProxyCacheTest8, r, context) {
    std::unique_ptr<Recorder> recorder = context->makeRecorder();
    ResourceCache* resourceCache = recorder->priv().resourceCache();
    ProxyCache* proxyCache = recorder->priv().proxyCache();

    resourceCache->setMaxBudget(0);

    SkBitmap bitmap;
    bool success = GetResourceAsBitmap("images/mandrill_128.png", &bitmap);
    REPORTER_ASSERT(r, success);
    if (!success) {
        return;
    }

    REPORTER_ASSERT(r, proxyCache->numCached() == 0);

    sk_sp<TextureProxy> proxy1 = proxyCache->findOrCreateCachedProxy(recorder.get(), bitmap,
                                                                     Mipmapped::kNo);
    REPORTER_ASSERT(r, proxyCache->numCached() == 1);

    {
        // Ensure proxy1's Texture is created (and timestamped) at this time
        auto recording = recorder->snap();
        context->insertRecording({ recording.get() });
        context->submit(SyncToCpu::kYes);
    }

    sk_sp<TextureProxy> proxy2 = proxyCache->findOrCreateCachedProxy(recorder.get(), bitmap,
                                                                     Mipmapped::kYes);
    REPORTER_ASSERT(r, proxyCache->numCached() == 2);

    {
        // Ensure proxy2's Texture is created (and timestamped) at this time
        auto recording = recorder->snap();
        context->insertRecording({ recording.get() });
        context->submit(SyncToCpu::kYes);
    }

    resourceCache->forcePurgeAsNeeded();

    REPORTER_ASSERT(r, proxyCache->numCached() == 2);

    proxy1.reset();
    proxyCache->forceProcessInvalidKeyMsgs();

    sk_sp<TextureProxy> test = proxyCache->find(bitmap, Mipmapped::kNo);
    REPORTER_ASSERT(r, test);
    test.reset();

    resourceCache->forcePurgeAsNeeded();

    REPORTER_ASSERT(r, proxyCache->numCached() == 1);
    test = proxyCache->find(bitmap, Mipmapped::kNo);
    REPORTER_ASSERT(r, !test);   // proxy1 should've been purged

    proxy2.reset();
    proxyCache->forceProcessInvalidKeyMsgs();

    test = proxyCache->find(bitmap, Mipmapped::kYes);
    REPORTER_ASSERT(r, test);
    test.reset();

    resourceCache->forcePurgeAsNeeded();

    REPORTER_ASSERT(r, proxyCache->numCached() == 0);
}

}  // namespace skgpu::graphite
