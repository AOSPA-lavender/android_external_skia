/*
 * Copyright 2018 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/**************************************************************************************************
 *** This file was autogenerated from GrClampedGradientEffect.fp; do not modify.
 **************************************************************************************************/
#ifndef GrClampedGradientEffect_DEFINED
#define GrClampedGradientEffect_DEFINED

#include "include/core/SkM44.h"
#include "include/core/SkTypes.h"

#include "src/gpu/GrFragmentProcessor.h"

class GrClampedGradientEffect : public GrFragmentProcessor {
public:
    static std::unique_ptr<GrFragmentProcessor> Make(
            std::unique_ptr<GrFragmentProcessor> colorizer,
            std::unique_ptr<GrFragmentProcessor> gradLayout,
            SkPMColor4f leftBorderColor,
            SkPMColor4f rightBorderColor,
            bool makePremul,
            bool colorsAreOpaque) {
        return std::unique_ptr<GrFragmentProcessor>(new GrClampedGradientEffect(
                std::move(colorizer), std::move(gradLayout), leftBorderColor, rightBorderColor,
                makePremul, colorsAreOpaque));
    }
    GrClampedGradientEffect(const GrClampedGradientEffect& src);
    std::unique_ptr<GrFragmentProcessor> clone() const override;
    const char* name() const override { return "ClampedGradientEffect"; }
    SkPMColor4f leftBorderColor;
    SkPMColor4f rightBorderColor;
    bool makePremul;
    bool colorsAreOpaque;

private:
    GrClampedGradientEffect(std::unique_ptr<GrFragmentProcessor> colorizer,
                            std::unique_ptr<GrFragmentProcessor> gradLayout,
                            SkPMColor4f leftBorderColor,
                            SkPMColor4f rightBorderColor,
                            bool makePremul,
                            bool colorsAreOpaque)
            : INHERITED(kGrClampedGradientEffect_ClassID,
                        (OptimizationFlags)kCompatibleWithCoverageAsAlpha_OptimizationFlag |
                                (colorsAreOpaque && gradLayout->preservesOpaqueInput()
                                         ? kPreservesOpaqueInput_OptimizationFlag
                                         : kNone_OptimizationFlags))
            , leftBorderColor(leftBorderColor)
            , rightBorderColor(rightBorderColor)
            , makePremul(makePremul)
            , colorsAreOpaque(colorsAreOpaque) {
        SkASSERT(colorizer);
        this->registerChild(std::move(colorizer), SkSL::SampleUsage::Explicit());
        SkASSERT(gradLayout);
        this->registerChild(std::move(gradLayout), SkSL::SampleUsage::PassThrough());
    }
    GrGLSLFragmentProcessor* onCreateGLSLInstance() const override;
    void onGetGLSLProcessorKey(const GrShaderCaps&, GrProcessorKeyBuilder*) const override;
    bool onIsEqual(const GrFragmentProcessor&) const override;
#if GR_TEST_UTILS
    SkString onDumpInfo() const override;
#endif
    GR_DECLARE_FRAGMENT_PROCESSOR_TEST
    using INHERITED = GrFragmentProcessor;
};
#endif
