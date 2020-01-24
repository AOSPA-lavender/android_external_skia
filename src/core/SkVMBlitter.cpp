/*
 * Copyright 2019 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "include/private/SkImageInfoPriv.h"
#include "include/private/SkMacros.h"
#include "src/core/SkArenaAlloc.h"
#include "src/core/SkBlendModePriv.h"
#include "src/core/SkColorSpacePriv.h"
#include "src/core/SkColorSpaceXformSteps.h"
#include "src/core/SkCoreBlitters.h"
#include "src/core/SkLRUCache.h"
#include "src/core/SkVM.h"
#include "src/core/SkVMBlitter.h"
#include "src/shaders/SkColorFilterShader.h"

namespace {

    // Uniforms set by the Blitter itself,
    // rather than by the Shader, which follow this struct in the skvm::Uniforms buffer.
    struct BlitterUniforms {
        int right;  // First device x + blit run length n, used to get device x coordiate.
        int y;      // Device y coordiate.
    };
    static_assert(SkIsAlign4(sizeof(BlitterUniforms)), "");
    static constexpr int kBlitterUniformsCount = sizeof(BlitterUniforms) / 4;

    enum class Coverage { Full, UniformA8, MaskA8, MaskLCD16, Mask3D };

    struct Params {
        sk_sp<SkColorSpace> colorSpace;
        sk_sp<SkShader>     shader;
        SkColorType         colorType;
        SkAlphaType         alphaType;
        SkBlendMode         blendMode;
        Coverage            coverage;
        SkFilterQuality     quality;
        SkMatrix            ctm;

        Params withCoverage(Coverage c) const {
            Params p = *this;
            p.coverage = c;
            return p;
        }
    };

    SK_BEGIN_REQUIRE_DENSE;
    struct Key {
        uint64_t colorSpace;
        uint32_t shader;
        uint8_t  colorType,
                 alphaType,
                 blendMode,
                 coverage;
        // Params::quality and Params::ctm are only passed to shader->program(),
        // not used here by the blitter itself.  No need to include them in the key;
        // they'll be folded into the shader key if used.

        bool operator==(const Key& that) const {
            return this->colorSpace == that.colorSpace
                && this->shader     == that.shader
                && this->colorType  == that.colorType
                && this->alphaType  == that.alphaType
                && this->blendMode  == that.blendMode
                && this->coverage   == that.coverage;
        }

        Key withCoverage(Coverage c) const {
            Key k = *this;
            k.coverage = SkToU8(c);
            return k;
        }
    };
    SK_END_REQUIRE_DENSE;

    static SkString debug_name(const Key& key) {
        return SkStringPrintf("CT%d-AT%d-Cov%d-Blend%d-CS%llx-Shader%x",
                              key.colorType,
                              key.alphaType,
                              key.coverage,
                              key.blendMode,
                              key.colorSpace,
                              key.shader);
    }

    static SkLRUCache<Key, skvm::Program>* try_acquire_program_cache() {
    #if 0 || defined(SK_BUILD_FOR_IOS)
        // iOS doesn't support thread_local on versions less than 9.0. pthread
        // based fallbacks must be used there. We could also use an SkSpinlock
        // and tryAcquire()/release(), or...
        return nullptr;  // ... we could just not cache programs on those platforms.
    #else
        thread_local static auto* cache = new SkLRUCache<Key, skvm::Program>{8};
        return cache;
    #endif
    }

    static void release_program_cache() { }


    struct Builder : public skvm::Builder {

        // If Builder can't build this program, CacheKey() sets *ok to false.
        static Key CacheKey(const Params& params, skvm::Uniforms* uniforms, bool* ok) {
            SkASSERT(params.shader);
            uint32_t shaderHash = 0;
            {
                const SkShaderBase* shader = as_SB(params.shader);
                skvm::Builder p;
                skvm::F32 x = p.to_f32(p.sub(p.uniform32(uniforms->ptr,
                                                         offsetof(BlitterUniforms, right)),
                                             p.index())),
                          y = p.to_f32(p.uniform32(uniforms->ptr,
                                                   offsetof(BlitterUniforms, y)));
                x = p.add(x, p.splat(0.5f));
                y = p.add(y, p.splat(0.5f));
                skvm::F32 r,g,b,a;

                SkSTArenaAlloc<2*sizeof(void*)> tmp;
                if (shader->program(&p,
                                    params.ctm, /*localM=*/nullptr,
                                    params.quality, params.colorSpace.get(),
                                    uniforms,&tmp,
                                    x,y, &r,&g,&b,&a)) {
                    shaderHash = p.hash();
                } else {
                    *ok = false;
                }
            }

            switch (params.colorType) {
                default: *ok = false;        break;
                case kRGB_565_SkColorType:   break;
                case kRGBA_8888_SkColorType: break;
                case kBGRA_8888_SkColorType: break;
            }

            if (!skvm::BlendModeSupported(params.blendMode)) {
                *ok = false;
            }

            return {
                params.colorSpace ? params.colorSpace->hash() : 0,
                shaderHash,
                SkToU8(params.colorType),
                SkToU8(params.alphaType),
                SkToU8(params.blendMode),
                SkToU8(params.coverage),
            };
        }

        Builder(const Params& params, skvm::Uniforms* uniforms, SkArenaAlloc* alloc) {
            // First two arguments are always uniforms and the destination buffer.
            uniforms->ptr     = uniform();
            skvm::Arg dst_ptr = arg(SkColorTypeBytesPerPixel(params.colorType));
            // Other arguments depend on params.coverage:
            //    - Full:      (no more arguments)
            //    - Mask3D:    mul varying, add varying, 8-bit coverage varying
            //    - MaskA8:    8-bit coverage varying
            //    - MaskLCD16: 565 coverage varying
            //    - UniformA8: 8-bit coverage uniform

            skvm::Color src;
            SkASSERT(params.shader);
            skvm::F32 x = to_f32(sub(uniform32(uniforms->ptr,
                                               offsetof(BlitterUniforms, right)),
                                     index())),
                      y = to_f32(uniform32(uniforms->ptr,
                                           offsetof(BlitterUniforms, y)));
            x = add(x, splat(0.5f));
            y = add(y, splat(0.5f));
            SkAssertResult(as_SB(params.shader)->program(this,
                                                         params.ctm, /*localM=*/nullptr,
                                                         params.quality, params.colorSpace.get(),
                                                         uniforms, alloc,
                                                         x,y, &src.r, &src.g, &src.b, &src.a));
            if (params.coverage == Coverage::Mask3D) {
                skvm::F32 M = unorm(8, load8(varying<uint8_t>())),
                          A = unorm(8, load8(varying<uint8_t>()));

                src.r = min(mad(src.r, M, A), src.a);
                src.g = min(mad(src.g, M, A), src.a);
                src.b = min(mad(src.b, M, A), src.a);
            }

            // If we can determine this we can skip a fair bit of clamping!
            bool src_in_gamut = false;

            // Normalized premul formats can surprisingly represent some out-of-gamut
            // values (e.g. r=0xff, a=0xee fits in unorm8 but r = 1.07), but most code
            // working with normalized premul colors is not prepared to handle r,g,b > a.
            // So we clamp the shader to gamut here before blending and coverage.
            //
            // In addition, GL clamps all its color channels to limits of the format just
            // before the blend step (~here).  To match that auto-clamp, we clamp alpha to
            // [0,1] too, just in case someone gave us a crazy alpha.
            if (!src_in_gamut
                    && params.alphaType == kPremul_SkAlphaType
                    && SkColorTypeIsNormalized(params.colorType)) {
                src.a = clamp(src.a, splat(0.0f), splat(1.0f));
                src.r = clamp(src.r, splat(0.0f), src.a);
                src.g = clamp(src.g, splat(0.0f), src.a);
                src.b = clamp(src.b, splat(0.0f), src.a);
                src_in_gamut = true;
            }

            // There are several orderings here of when we load dst and coverage
            // and how coverage is applied, and to complicate things, LCD coverage
            // needs to know dst.a.  We're careful to assert it's loaded in time.
            skvm::Color dst;
            SkDEBUGCODE(bool dst_loaded = false;)

            // load_coverage() returns false when there's no need to apply coverage.
            auto load_coverage = [&](skvm::Color* cov) {
                switch (params.coverage) {
                    case Coverage::Full: return false;

                    case Coverage::UniformA8: cov->r = cov->g = cov->b = cov->a =
                                              unorm(8, uniform8(uniform(), 0));
                                              return true;

                    case Coverage::Mask3D:
                    case Coverage::MaskA8: cov->r = cov->g = cov->b = cov->a =
                                           unorm(8, load8(varying<uint8_t>()));
                                           return true;

                    case Coverage::MaskLCD16:
                        SkASSERT(dst_loaded);
                        *cov = unpack_565(load16(varying<uint16_t>()));
                        cov->a = select(lt(src.a, dst.a), min(cov->r, min(cov->g,cov->b))
                                                        , max(cov->r, max(cov->g,cov->b)));
                        return true;
                }
                // GCC insists...
                return false;
            };

            // The math for some blend modes lets us fold coverage into src before the blend,
            // obviating the need for the lerp afterwards. This early-coverage strategy tends
            // to be both faster and require fewer registers.
            bool lerp_coverage_post_blend = true;
            if (SkBlendMode_ShouldPreScaleCoverage(params.blendMode,
                                                   params.coverage == Coverage::MaskLCD16)) {
                skvm::Color cov;
                if (load_coverage(&cov)) {
                    src.r = mul(src.r, cov.r);
                    src.g = mul(src.g, cov.g);
                    src.b = mul(src.b, cov.b);
                    src.a = mul(src.a, cov.a);
                }
                lerp_coverage_post_blend = false;
            }

            // Load up the destination color.
            SkDEBUGCODE(dst_loaded = true;)
            switch (params.colorType) {
                default: SkUNREACHABLE;
                case kRGB_565_SkColorType:   dst = unpack_565 (load16(dst_ptr)); break;
                case kRGBA_8888_SkColorType: dst = unpack_8888(load32(dst_ptr)); break;
                case kBGRA_8888_SkColorType: dst = unpack_8888(load32(dst_ptr));
                                             std::swap(dst.r, dst.b);
                                             break;
            }

            // When a destination is tagged opaque, we may assume it both starts and stays fully
            // opaque, ignoring any math that disagrees.  So anything involving force_opaque is
            // optional, and sometimes helps cut a small amount of work in these programs.
            const bool force_opaque = true && params.alphaType == kOpaque_SkAlphaType;
            if (force_opaque) {
                dst.a = splat(1.0f);
            } else if (params.alphaType == kUnpremul_SkAlphaType) {
                premul(&dst.r, &dst.g, &dst.b, dst.a);
            }

            src = skvm::BlendModeProgram(this, params.blendMode, src, dst);

            // Lerp with coverage post-blend if needed.
            skvm::Color cov;
            if (lerp_coverage_post_blend && load_coverage(&cov)) {
                src.r = mad(sub(src.r, dst.r), cov.r, dst.r);
                src.g = mad(sub(src.g, dst.g), cov.g, dst.g);
                src.b = mad(sub(src.b, dst.b), cov.b, dst.b);
                src.a = mad(sub(src.a, dst.a), cov.a, dst.a);
            }

            // Clamp to fit destination color format if needed.
            if (src_in_gamut) {
                // An in-gamut src blended with an in-gamut dst should stay in gamut.
                // Being in-gamut implies all channels are in [0,1], so no need to clamp.
                assert_true(eq(src.a, clamp(src.a, splat(0.0f), splat(1.0f))));
                assert_true(eq(src.r, clamp(src.r, splat(0.0f), src.a)));
                assert_true(eq(src.g, clamp(src.g, splat(0.0f), src.a)));
                assert_true(eq(src.b, clamp(src.b, splat(0.0f), src.a)));
            } else if (SkColorTypeIsNormalized(params.colorType)) {
                src.r = clamp(src.r, splat(0.0f), splat(1.0f));
                src.g = clamp(src.g, splat(0.0f), splat(1.0f));
                src.b = clamp(src.b, splat(0.0f), splat(1.0f));
                src.a = clamp(src.a, splat(0.0f), splat(1.0f));
            }
            if (force_opaque) {
                src.a = splat(1.0f);
            } else if (params.alphaType == kUnpremul_SkAlphaType) {
                unpremul(&src.r, &src.g, &src.b, src.a);
            }

            // Store back to the destination.
            switch (params.colorType) {
                default: SkUNREACHABLE;

                case kRGB_565_SkColorType:
                    store16(dst_ptr, pack(pack(unorm(5,src.b),
                                               unorm(6,src.g), 5),
                                               unorm(5,src.r),11));
                    break;

                case kBGRA_8888_SkColorType: std::swap(src.r, src.b);  // fallthrough
                case kRGBA_8888_SkColorType:
                     store32(dst_ptr, pack(pack(unorm(8, src.r),
                                                unorm(8, src.g), 8),
                                           pack(unorm(8, src.b),
                                                unorm(8, src.a), 8), 16));
                     break;
            }
        }
    };

    struct NoopColorFilter : public SkColorFilter {
        bool onProgram(skvm::Builder*,
                       SkColorSpace*,
                       skvm::Uniforms*,
                       skvm::F32*, skvm::F32*, skvm::F32*, skvm::F32*) const override {
            return true;
        }

        bool onAppendStages(const SkStageRec&, bool) const override { return true; }

        // Only created here, should never be flattened / unflattened.
        Factory getFactory() const override { return nullptr; }
        const char* getTypeName() const override { return "NoopColorFilter"; }
    };

    static Params effective_params(const SkPixmap& device,
                                   const SkPaint& paint,
                                   const SkMatrix& ctm) {
        // Color filters have been handled for us by SkBlitter::Choose().
        SkASSERT(!paint.getColorFilter());

        // If there's no explicit shader, the paint color is the shader,
        // but if there is a shader, it's modulated by the paint alpha.
        sk_sp<SkShader> shader = paint.refShader();
        if (!shader) {
            shader = SkShaders::Color(paint.getColor4f(), nullptr);
        } else if (paint.getAlphaf() < 1.0f) {
            shader = sk_make_sp<SkColorFilterShader>(std::move(shader),
                                                     paint.getAlphaf(),
                                                     sk_make_sp<NoopColorFilter>());
        }

        // The most common blend mode is SrcOver, and it can be strength-reduced
        // _greatly_ to Src mode when the shader is opaque.
        SkBlendMode blendMode = paint.getBlendMode();
        if (blendMode == SkBlendMode::kSrcOver && shader->isOpaque()) {
            blendMode =  SkBlendMode::kSrc;
        }

        // In general all the information we use to make decisions here need to
        // be reflected in Params and Key to make program caching sound, and it
        // might appear that shader->isOpaque() is a property of the shader's
        // uniforms than its fundamental program structure and so unsafe to use.
        //
        // Opacity is such a powerful property that SkShaderBase::program()
        // forces opacity for any shader subclass that claims isOpaque(), so
        // the opaque bit is strongly guaranteed to be part of the program and
        // not just a property of the uniforms.  The shader program hash includes
        // this information, making it safe to use anywhere in the blitter codegen.

        return {
            device.refColorSpace(),
            std::move(shader),
            device.colorType(),
            device.alphaType(),
            blendMode,
            Coverage::Full,  // Placeholder... withCoverage() will change as needed.
            paint.getFilterQuality(),
            ctm,
        };
    }

    class Blitter final : public SkBlitter {
    public:
        Blitter(const SkPixmap& device, const SkPaint& paint, const SkMatrix& ctm, bool* ok)
            : fDevice(device)
            , fUniforms(kBlitterUniformsCount)
            , fParams(effective_params(device, paint, ctm))
            , fKey(Builder::CacheKey(fParams, &fUniforms, ok))
        {}

        ~Blitter() override {
            if (SkLRUCache<Key, skvm::Program>* cache = try_acquire_program_cache()) {
                auto cache_program = [&](skvm::Program&& program, Coverage coverage) {
                    if (!program.empty()) {
                        Key key = fKey.withCoverage(coverage);
                        if (skvm::Program* found = cache->find(key)) {
                            *found = std::move(program);
                        } else {
                            cache->insert(key, std::move(program));
                        }
                    }
                };
                cache_program(std::move(fBlitH),         Coverage::Full);
                cache_program(std::move(fBlitAntiH),     Coverage::UniformA8);
                cache_program(std::move(fBlitMaskA8),    Coverage::MaskA8);
                cache_program(std::move(fBlitMask3D),    Coverage::Mask3D);
                cache_program(std::move(fBlitMaskLCD16), Coverage::MaskLCD16);

                release_program_cache();
            }
        }

    private:
        SkPixmap       fDevice;
        skvm::Uniforms fUniforms;                // Most data is copied directly into fUniforms,
        SkArenaAlloc   fAlloc{2*sizeof(void*)};  // but a few effects need to ref large content.
        const Params   fParams;
        const Key      fKey;
        skvm::Program  fBlitH,
                       fBlitAntiH,
                       fBlitMaskA8,
                       fBlitMask3D,
                       fBlitMaskLCD16;

        skvm::Program buildProgram(Coverage coverage) {
            Key key = fKey.withCoverage(coverage);
            {
                skvm::Program p;
                if (SkLRUCache<Key, skvm::Program>* cache = try_acquire_program_cache()) {
                    if (skvm::Program* found = cache->find(key)) {
                        p = std::move(*found);
                    }
                    release_program_cache();
                }
                if (!p.empty()) {
                    return p;
                }
            }
            // We don't really _need_ to rebuild fUniforms here.
            // It's just more natural to have effects unconditionally emit them,
            // and more natural to rebuild fUniforms than to emit them into a dummy buffer.
            // fUniforms should reuse the exact same memory, so this is very cheap.
            SkDEBUGCODE(size_t prev = fUniforms.buf.size();)
            fUniforms.buf.resize(kBlitterUniformsCount);
            Builder builder{fParams.withCoverage(coverage), &fUniforms, &fAlloc};
            SkASSERT(fUniforms.buf.size() == prev);

            skvm::Program program = builder.done(debug_name(key).c_str());
            if (false) {
                static std::atomic<int> missed{0},
                                         total{0};
                if (!program.hasJIT()) {
                    SkDebugf("\ncouldn't JIT %s\n", debug_name(key).c_str());
                    builder.dump();
                    program.dump();
                    missed++;
                }
                if (0 == total++) {
                    atexit([]{ SkDebugf("SkVMBlitter compiled %d programs, %d without JIT.\n",
                                        total.load(), missed.load()); });
                }
            }
            return program;
        }

        void updateUniforms(int right, int y) {
            BlitterUniforms uniforms{right, y};
            memcpy(fUniforms.buf.data(), &uniforms, sizeof(BlitterUniforms));
        }

        void blitH(int x, int y, int w) override {
            if (fBlitH.empty()) {
                fBlitH = this->buildProgram(Coverage::Full);
            }
            this->updateUniforms(x+w, y);
            fBlitH.eval(w, fUniforms.buf.data(), fDevice.addr(x,y));
        }

        void blitAntiH(int x, int y, const SkAlpha cov[], const int16_t runs[]) override {
            if (fBlitAntiH.empty()) {
                fBlitAntiH = this->buildProgram(Coverage::UniformA8);
            }
            for (int16_t run = *runs; run > 0; run = *runs) {
                this->updateUniforms(x+run, y);
                fBlitAntiH.eval(run, fUniforms.buf.data(), fDevice.addr(x,y), cov);

                x    += run;
                runs += run;
                cov  += run;
            }
        }

        void blitMask(const SkMask& mask, const SkIRect& clip) override {
            if (mask.fFormat == SkMask::kBW_Format) {
                return SkBlitter::blitMask(mask, clip);
            }

            const skvm::Program* program = nullptr;
            switch (mask.fFormat) {
                default: SkUNREACHABLE;     // ARGB and SDF masks shouldn't make it here.

                case SkMask::k3D_Format:
                    if (fBlitMask3D.empty()) {
                        fBlitMask3D = this->buildProgram(Coverage::Mask3D);
                    }
                    program = &fBlitMask3D;
                    break;

                case SkMask::kA8_Format:
                    if (fBlitMaskA8.empty()) {
                        fBlitMaskA8 = this->buildProgram(Coverage::MaskA8);
                    }
                    program = &fBlitMaskA8;
                    break;

                case SkMask::kLCD16_Format:
                    if (fBlitMaskLCD16.empty()) {
                        fBlitMaskLCD16 = this->buildProgram(Coverage::MaskLCD16);
                    }
                    program = &fBlitMaskLCD16;
                    break;
            }

            SkASSERT(program);
            if (program) {
                for (int y = clip.top(); y < clip.bottom(); y++) {
                    int x = clip.left(),
                        w = clip.width();
                    void* dptr =        fDevice.writable_addr(x,y);
                    auto  mptr = (const uint8_t*)mask.getAddr(x,y);
                    this->updateUniforms(x+w,y);

                    if (program == &fBlitMask3D) {
                        size_t plane = mask.computeImageSize();
                        program->eval(w, fUniforms.buf.data(), dptr, mptr + 1*plane
                                                                   , mptr + 2*plane
                                                                   , mptr + 0*plane);
                    } else {
                        program->eval(w, fUniforms.buf.data(), dptr, mptr);
                    }
                }
            }
        }
    };

}  // namespace

bool skvm::BlendModeSupported(SkBlendMode mode) {
    return mode <= SkBlendMode::kScreen;
}

skvm::Color skvm::BlendModeProgram(skvm::Builder* p,
                                   SkBlendMode mode, skvm::Color src, skvm::Color dst) {
    auto mma = [&](skvm::F32 x, skvm::F32 y, skvm::F32 z, skvm::F32 w) {
        return p->mad(x,y, p->mul(z,w));
    };

    auto inv = [&](skvm::F32 x) {
        return p->sub(p->splat(1.0f), x);
    };

    switch (mode) {
        default: SkASSERT(false); /*but also, for safety, fallthrough*/

        case SkBlendMode::kClear: return {
            p->splat(0.0f),
            p->splat(0.0f),
            p->splat(0.0f),
            p->splat(0.0f),
        };

        case SkBlendMode::kSrc: return src;
        case SkBlendMode::kDst: return dst;

        case SkBlendMode::kDstOver: std::swap(src, dst); // fall-through
        case SkBlendMode::kSrcOver: return {
            p->mad(dst.r, inv(src.a), src.r),
            p->mad(dst.g, inv(src.a), src.g),
            p->mad(dst.b, inv(src.a), src.b),
            p->mad(dst.a, inv(src.a), src.a),
        };

        case SkBlendMode::kDstIn: std::swap(src, dst); // fall-through
        case SkBlendMode::kSrcIn: return {
            p->mul(src.r, dst.a),
            p->mul(src.g, dst.a),
            p->mul(src.b, dst.a),
            p->mul(src.a, dst.a),
        };

        case SkBlendMode::kDstOut: std::swap(src, dst); // fall-through
        case SkBlendMode::kSrcOut: return {
            p->mul(src.r, inv(dst.a)),
            p->mul(src.g, inv(dst.a)),
            p->mul(src.b, inv(dst.a)),
            p->mul(src.a, inv(dst.a)),
        };

        case SkBlendMode::kDstATop: std::swap(src, dst); // fall-through
        case SkBlendMode::kSrcATop: return {
            mma(src.r, dst.a,  dst.r, inv(src.a)),
            mma(src.g, dst.a,  dst.g, inv(src.a)),
            mma(src.b, dst.a,  dst.b, inv(src.a)),
            mma(src.a, dst.a,  dst.a, inv(src.a)),
        };

        case SkBlendMode::kXor: return {
            mma(src.r, inv(dst.a),  dst.r, inv(src.a)),
            mma(src.g, inv(dst.a),  dst.g, inv(src.a)),
            mma(src.b, inv(dst.a),  dst.b, inv(src.a)),
            mma(src.a, inv(dst.a),  dst.a, inv(src.a)),
        };

        case SkBlendMode::kPlus: return {
            p->min(p->add(src.r, dst.r), p->splat(1.0f)),
            p->min(p->add(src.g, dst.g), p->splat(1.0f)),
            p->min(p->add(src.b, dst.b), p->splat(1.0f)),
            p->min(p->add(src.a, dst.a), p->splat(1.0f)),
        };

        case SkBlendMode::kModulate: return {
            p->mul(src.r, dst.r),
            p->mul(src.g, dst.g),
            p->mul(src.b, dst.b),
            p->mul(src.a, dst.a),
        };

        case SkBlendMode::kScreen: return {
            p->sub(p->add(src.r, dst.r), p->mul(src.r, dst.r)),
            p->sub(p->add(src.g, dst.g), p->mul(src.g, dst.g)),
            p->sub(p->add(src.b, dst.b), p->mul(src.b, dst.b)),
            p->sub(p->add(src.a, dst.a), p->mul(src.a, dst.a)),
        };
    }
}

SkBlitter* SkCreateSkVMBlitter(const SkPixmap& device,
                               const SkPaint& paint,
                               const SkMatrix& ctm,
                               SkArenaAlloc* alloc) {
    bool ok = true;
    auto blitter = alloc->make<Blitter>(device, paint, ctm, &ok);
    return ok ? blitter : nullptr;
}