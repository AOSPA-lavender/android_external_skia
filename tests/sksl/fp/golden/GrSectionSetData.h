

/**************************************************************************************************
 *** This file was autogenerated from GrSectionSetData.fp; do not modify.
 **************************************************************************************************/
#ifndef GrSectionSetData_DEFINED
#define GrSectionSetData_DEFINED

#include "include/core/SkM44.h"
#include "include/core/SkTypes.h"


#include "src/gpu/GrFragmentProcessor.h"

class GrSectionSetData : public GrFragmentProcessor {
public:
    static std::unique_ptr<GrFragmentProcessor> Make(float provided) {
        return std::unique_ptr<GrFragmentProcessor>(new GrSectionSetData(provided));
    }
    GrSectionSetData(const GrSectionSetData& src);
    std::unique_ptr<GrFragmentProcessor> clone() const override;
    const char* name() const override { return "SectionSetData"; }
    float provided;
private:
    GrSectionSetData(float provided)
    : INHERITED(kGrSectionSetData_ClassID, kNone_OptimizationFlags)
    , provided(provided) {
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
