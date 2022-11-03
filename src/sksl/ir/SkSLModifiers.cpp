/*
 * Copyright 2021 Google LLC.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "include/private/SkSLModifiers.h"

#include "include/core/SkTypes.h"
#include "include/sksl/SkSLErrorReporter.h"
#include "include/sksl/SkSLPosition.h"
#include "src/core/SkMathPriv.h"
#include "src/sksl/SkSLContext.h"

namespace SkSL {

bool Modifiers::checkPermitted(const Context& context,
                               Position pos,
                               int permittedModifierFlags,
                               int permittedLayoutFlags) const {
    static constexpr struct { Modifiers::Flag flag; const char* name; } kModifierFlags[] = {
        { Modifiers::kConst_Flag,          "const" },
        { Modifiers::kIn_Flag,             "in" },
        { Modifiers::kOut_Flag,            "out" },
        { Modifiers::kUniform_Flag,        "uniform" },
        { Modifiers::kFlat_Flag,           "flat" },
        { Modifiers::kNoPerspective_Flag,  "noperspective" },
        { Modifiers::kPure_Flag,           "$pure" },
        { Modifiers::kInline_Flag,         "inline" },
        { Modifiers::kNoInline_Flag,       "noinline" },
        { Modifiers::kHighp_Flag,          "highp" },
        { Modifiers::kMediump_Flag,        "mediump" },
        { Modifiers::kLowp_Flag,           "lowp" },
        { Modifiers::kExport_Flag,         "$export" },
        { Modifiers::kES3_Flag,            "$es3" },
        { Modifiers::kThreadgroup_Flag,    "threadgroup" },
        { Modifiers::kReadOnly_Flag,       "readonly" },
        { Modifiers::kWriteOnly_Flag,      "writeonly" },
        { Modifiers::kBuffer_Flag,         "buffer" },
    };

    bool success = true;
    int modifierFlags = fFlags;
    for (const auto& f : kModifierFlags) {
        if (modifierFlags & f.flag) {
            if (!(permittedModifierFlags & f.flag)) {
                context.fErrors->error(pos, "'" + std::string(f.name) + "' is not permitted here");
                success = false;
            }
            modifierFlags &= ~f.flag;
        }
    }
    SkASSERT(modifierFlags == 0);

    constexpr int allBackendFlags = Layout::kSPIRV_Flag | Layout::kMetal_Flag | Layout::kGL_Flag;
    int backendFlags = fLayout.fFlags & allBackendFlags;
    if (SkPopCount(backendFlags) > 1) {
        context.fErrors->error(pos, "only one backend qualifier can be used");
        success = false;
    }

    static constexpr struct { Layout::Flag flag; const char* name; } kLayoutFlags[] = {
        { Layout::kOriginUpperLeft_Flag,          "origin_upper_left"},
        { Layout::kPushConstant_Flag,             "push_constant"},
        { Layout::kBlendSupportAllEquations_Flag, "blend_support_all_equations"},
        { Layout::kColor_Flag,                    "color"},
        { Layout::kLocation_Flag,                 "location"},
        { Layout::kOffset_Flag,                   "offset"},
        { Layout::kBinding_Flag,                  "binding"},
        { Layout::kTexture_Flag,                  "texture"},
        { Layout::kSampler_Flag,                  "sampler"},
        { Layout::kIndex_Flag,                    "index"},
        { Layout::kSet_Flag,                      "set"},
        { Layout::kBuiltin_Flag,                  "builtin"},
        { Layout::kInputAttachmentIndex_Flag,     "input_attachment_index"},
        { Layout::kSPIRV_Flag,                    "spirv"},
        { Layout::kMetal_Flag,                    "metal"},
        { Layout::kGL_Flag,                       "gl"},
    };

    int layoutFlags = fLayout.fFlags;
    for (const auto& lf : kLayoutFlags) {
        if (layoutFlags & lf.flag) {
            if (!(permittedLayoutFlags & lf.flag)) {
                context.fErrors->error(pos, "layout qualifier '" + std::string(lf.name) +
                                            "' is not permitted here");
                success = false;
            }
            layoutFlags &= ~lf.flag;
        }
    }
    SkASSERT(layoutFlags == 0);
    return success;
}

} // namespace SkSL
