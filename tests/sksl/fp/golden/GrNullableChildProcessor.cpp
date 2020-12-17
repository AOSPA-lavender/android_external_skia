

/**************************************************************************************************
 *** This file was autogenerated from GrNullableChildProcessor.fp; do not modify.
 **************************************************************************************************/
#include "GrNullableChildProcessor.h"

#include "src/core/SkUtils.h"
#include "src/gpu/GrTexture.h"
#include "src/gpu/glsl/GrGLSLFragmentProcessor.h"
#include "src/gpu/glsl/GrGLSLFragmentShaderBuilder.h"
#include "src/gpu/glsl/GrGLSLProgramBuilder.h"
#include "src/sksl/SkSLCPP.h"
#include "src/sksl/SkSLUtil.h"
class GrGLSLNullableChildProcessor : public GrGLSLFragmentProcessor {
public:
    GrGLSLNullableChildProcessor() {}
    void emitCode(EmitArgs& args) override {
        GrGLSLFPFragmentBuilder* fragBuilder = args.fFragBuilder;
        const GrNullableChildProcessor& _outer = args.fFp.cast<GrNullableChildProcessor>();
        (void) _outer;
        fragBuilder->codeAppendf(
R"SkSL(if (%s) {)SkSL"
, _outer.childProcessor(0) ? "true" : "false");
        SkString _sample0 = this->invokeChild(0, args);
        fragBuilder->codeAppendf(
R"SkSL(
    return %s;
} else {
    return half4(0.5);
}
)SkSL"
, _sample0.c_str());
    }
private:
    void onSetData(const GrGLSLProgramDataManager& pdman, const GrFragmentProcessor& _proc) override {
    }
};
GrGLSLFragmentProcessor* GrNullableChildProcessor::onCreateGLSLInstance() const {
    return new GrGLSLNullableChildProcessor();
}
void GrNullableChildProcessor::onGetGLSLProcessorKey(const GrShaderCaps& caps, GrProcessorKeyBuilder* b) const {
}
bool GrNullableChildProcessor::onIsEqual(const GrFragmentProcessor& other) const {
    const GrNullableChildProcessor& that = other.cast<GrNullableChildProcessor>();
    (void) that;
    return true;
}
GrNullableChildProcessor::GrNullableChildProcessor(const GrNullableChildProcessor& src)
: INHERITED(kGrNullableChildProcessor_ClassID, src.optimizationFlags()) {
        this->cloneAndRegisterAllChildProcessors(src);
}
std::unique_ptr<GrFragmentProcessor> GrNullableChildProcessor::clone() const {
    return std::make_unique<GrNullableChildProcessor>(*this);
}
#if GR_TEST_UTILS
SkString GrNullableChildProcessor::onDumpInfo() const {
    return SkString();
}
#endif
