/*
 * Copyright 2019 Google LLC
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/**************************************************************************************************
 *** This file was autogenerated from GrRGBToHSLFilterEffect.fp; do not modify.
 **************************************************************************************************/
#include "GrRGBToHSLFilterEffect.h"

#include "src/gpu/GrTexture.h"
#include "src/gpu/glsl/GrGLSLFragmentProcessor.h"
#include "src/gpu/glsl/GrGLSLFragmentShaderBuilder.h"
#include "src/gpu/glsl/GrGLSLProgramBuilder.h"
#include "src/sksl/SkSLCPP.h"
#include "src/sksl/SkSLUtil.h"
class GrGLSLRGBToHSLFilterEffect : public GrGLSLFragmentProcessor {
public:
    GrGLSLRGBToHSLFilterEffect() {}
    void emitCode(EmitArgs& args) override {
        GrGLSLFPFragmentBuilder* fragBuilder = args.fFragBuilder;
        const GrRGBToHSLFilterEffect& _outer = args.fFp.cast<GrRGBToHSLFilterEffect>();
        (void)_outer;
        SkString _input1173 = SkStringPrintf("%s", args.fInputColor);
        SkString _sample1173;
        if (_outer.inputFP_index >= 0) {
            _sample1173 = this->invokeChild(_outer.inputFP_index, _input1173.c_str(), args);
        } else {
            _sample1173 = _input1173;
        }
        fragBuilder->codeAppendf(
                "half4 c = %s;\nhalf4 p = c.y < c.z ? half4(c.zy, -1.0, 0.66666666666666663) : "
                "half4(c.yz, 0.0, -0.33333333333333331);\nhalf4 q = c.x < p.x ? half4(p.x, c.x, "
                "p.yw) : half4(c.x, p.x, p.yz);\n\nhalf pmV = q.x;\nhalf pmC = pmV - min(q.y, "
                "q.z);\nhalf pmL = pmV - pmC * 0.5;\nhalf H = abs(q.w + (q.y - q.z) / (pmC * 6.0 + "
                "9.9999997473787516e-05));\nhalf S = pmC / ((c.w + 9.9999997473787516e-05) - "
                "abs(pmL * 2.0 - c.w));\nhalf L = pmL / (c.w + 9.9999997473787516e-05);\n%s = "
                "half4(H, S, L, c.w);\n",
                _sample1173.c_str(), args.fOutputColor);
    }

private:
    void onSetData(const GrGLSLProgramDataManager& pdman,
                   const GrFragmentProcessor& _proc) override {}
};
GrGLSLFragmentProcessor* GrRGBToHSLFilterEffect::onCreateGLSLInstance() const {
    return new GrGLSLRGBToHSLFilterEffect();
}
void GrRGBToHSLFilterEffect::onGetGLSLProcessorKey(const GrShaderCaps& caps,
                                                   GrProcessorKeyBuilder* b) const {}
bool GrRGBToHSLFilterEffect::onIsEqual(const GrFragmentProcessor& other) const {
    const GrRGBToHSLFilterEffect& that = other.cast<GrRGBToHSLFilterEffect>();
    (void)that;
    return true;
}
GrRGBToHSLFilterEffect::GrRGBToHSLFilterEffect(const GrRGBToHSLFilterEffect& src)
        : INHERITED(kGrRGBToHSLFilterEffect_ClassID, src.optimizationFlags()) {
    if (src.inputFP_index >= 0) {
        auto inputFP_clone = src.childProcessor(src.inputFP_index).clone();
        if (src.childProcessor(src.inputFP_index).isSampledWithExplicitCoords()) {
            inputFP_clone->setSampledWithExplicitCoords();
        }
        inputFP_index = this->registerChildProcessor(std::move(inputFP_clone));
    }
}
std::unique_ptr<GrFragmentProcessor> GrRGBToHSLFilterEffect::clone() const {
    return std::unique_ptr<GrFragmentProcessor>(new GrRGBToHSLFilterEffect(*this));
}
