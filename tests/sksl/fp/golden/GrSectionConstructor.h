

/**************************************************************************************************
 *** This file was autogenerated from GrSectionConstructor.fp; do not modify.
 **************************************************************************************************/
#ifndef GrSectionConstructor_DEFINED
#define GrSectionConstructor_DEFINED

#include "include/core/SkM44.h"
#include "include/core/SkTypes.h"


#include "src/gpu/GrFragmentProcessor.h"

class GrSectionConstructor : public GrFragmentProcessor {
public:
    static std::unique_ptr<GrFragmentProcessor> Make() {
        return std::unique_ptr<GrFragmentProcessor>(new GrSectionConstructor());
    }
    GrSectionConstructor(const GrSectionConstructor& src);
    std::unique_ptr<GrFragmentProcessor> clone() const override;
    const char* name() const override { return "SectionConstructor"; }
private:
constructor section    GrGLSLFragmentProcessor* onCreateGLSLInstance() const override;
    void onGetGLSLProcessorKey(const GrShaderCaps&, GrProcessorKeyBuilder*) const override;
    bool onIsEqual(const GrFragmentProcessor&) const override;
#if GR_TEST_UTILS
    SkString onDumpInfo() const override;
#endif
    GR_DECLARE_FRAGMENT_PROCESSOR_TEST
    using INHERITED = GrFragmentProcessor;
};
#endif
