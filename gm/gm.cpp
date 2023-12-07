/*
 * Copyright 2011 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "gm/gm.h"

#include "include/core/SkBitmap.h"
#include "include/core/SkBlendMode.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkFont.h"
#include "include/core/SkFontTypes.h"
#include "include/core/SkMatrix.h"
#include "include/core/SkPaint.h"
#include "include/core/SkRect.h"
#include "include/core/SkSamplingOptions.h"
#include "include/core/SkShader.h"  // IWYU pragma: keep
#include "include/core/SkTileMode.h"
#include "src/core/SkTraceEvent.h"
#include "tools/ToolUtils.h"
#include "tools/fonts/FontToolUtils.h"

#if defined(SK_GANESH)
#include "include/gpu/GrRecordingContext.h"
#endif

#include <atomic>
#include <cstdarg>
#include <cstdint>

using namespace skiagm;

static void draw_failure_message(SkCanvas* canvas, const char format[], ...) SK_PRINTF_LIKE(2, 3);

static void draw_failure_message(SkCanvas* canvas, const char format[], ...) {
    SkString failureMsg;

    va_list argp;
    va_start(argp, format);
    failureMsg.appendVAList(format, argp);
    va_end(argp);

    constexpr SkScalar kOffset = 5.0f;
    canvas->drawColor(SkColorSetRGB(200,0,0));
    SkFont font = ToolUtils::DefaultPortableFont();
    SkRect bounds;
    font.measureText(failureMsg.c_str(), failureMsg.size(), SkTextEncoding::kUTF8, &bounds);
    SkPaint textPaint(SkColors::kWhite);
    canvas->drawString(failureMsg, kOffset, bounds.height() + kOffset, font, textPaint);
}

static void draw_gpu_only_message(SkCanvas* canvas) {
    SkBitmap bmp;
    bmp.allocN32Pixels(128, 64);
    SkCanvas bmpCanvas(bmp);
    bmpCanvas.drawColor(SK_ColorWHITE);
    SkFont  font(ToolUtils::DefaultPortableTypeface(), 20);
    SkPaint paint(SkColors::kRed);
    bmpCanvas.drawString("GPU Only", 20, 40, font, paint);
    SkMatrix localM;
    localM.setRotate(35.f);
    localM.postTranslate(10.f, 0.f);
    paint.setShader(bmp.makeShader(SkTileMode::kMirror, SkTileMode::kMirror,
                                   SkSamplingOptions(SkFilterMode::kLinear,
                                                     SkMipmapMode::kNearest),
                                   localM));
    canvas->drawPaint(paint);
}

static void handle_gm_failure(SkCanvas* canvas, DrawResult result, const SkString& errorMsg) {
    if (DrawResult::kFail == result) {
        draw_failure_message(canvas, "DRAW FAILED: %s", errorMsg.c_str());
    } else if (SkString(GM::kErrorMsg_DrawSkippedGpuOnly) == errorMsg) {
        draw_gpu_only_message(canvas);
    } else {
        draw_failure_message(canvas, "DRAW SKIPPED: %s", errorMsg.c_str());
    }
}

GM::GM(SkColor bgColor) {
    fMode = kGM_Mode;
    fBGColor = bgColor;
}

GM::~GM() {}

DrawResult GM::gpuSetup(SkCanvas* canvas,
                        SkString* errorMsg,
                        GraphiteTestContext* graphiteTestContext) {
    TRACE_EVENT1("GM", TRACE_FUNC, "name", TRACE_STR_COPY(this->getName().c_str()));
    if (!fGpuSetup) {
        // When drawn in viewer, gpuSetup will be called multiple times with the same
        // GrContext or graphite::Context.
        fGpuSetup = true;
        fGpuSetupResult = this->onGpuSetup(canvas, errorMsg, graphiteTestContext);
    }
    if (fGpuSetupResult == DrawResult::kOk) {
        fGraphiteTestContext = graphiteTestContext;
    } else {
        handle_gm_failure(canvas, fGpuSetupResult, *errorMsg);
    }

    return fGpuSetupResult;
}

void GM::gpuTeardown() {
    this->onGpuTeardown();

    // After 'gpuTeardown' a GM can be reused with a different GrContext or graphite::Context. Reset
    // the flag so 'onGpuSetup' will be called.
    fGpuSetup = false;
    fGraphiteTestContext = nullptr;
}

DrawResult GM::draw(SkCanvas* canvas, SkString* errorMsg) {
    TRACE_EVENT1("GM", TRACE_FUNC, "name", TRACE_STR_COPY(this->getName().c_str()));
    this->drawBackground(canvas);
    return this->drawContent(canvas, errorMsg);
}

DrawResult GM::drawContent(SkCanvas* canvas, SkString* errorMsg) {
    TRACE_EVENT0("GM", TRACE_FUNC);
    this->onceBeforeDraw();
    SkAutoCanvasRestore acr(canvas, true);
    DrawResult drawResult = this->onDraw(canvas, errorMsg);
    if (DrawResult::kOk != drawResult) {
        handle_gm_failure(canvas, drawResult, *errorMsg);
    }
    return drawResult;
}

void GM::drawBackground(SkCanvas* canvas) {
    TRACE_EVENT0("GM", TRACE_FUNC);
    this->onceBeforeDraw();
    canvas->drawColor(fBGColor, SkBlendMode::kSrc);
}

DrawResult GM::onDraw(SkCanvas* canvas, SkString* errorMsg) {
    this->onDraw(canvas);
    return DrawResult::kOk;
}
void GM::onDraw(SkCanvas*) { SK_ABORT("Not implemented."); }

SkISize SimpleGM::getISize() { return fSize; }
SkString SimpleGM::getName() const { return fName; }
DrawResult SimpleGM::onDraw(SkCanvas* canvas, SkString* errorMsg) {
    return fDrawProc(canvas, errorMsg);
}

#if defined(SK_GANESH)
SkISize SimpleGpuGM::getISize() { return fSize; }
SkString SimpleGpuGM::getName() const { return fName; }
DrawResult SimpleGpuGM::onDraw(GrRecordingContext* rContext, SkCanvas* canvas, SkString* errorMsg) {
    return fDrawProc(rContext, canvas, errorMsg);
}
#endif

void GM::setBGColor(SkColor color) {
    fBGColor = color;
}

bool GM::animate(double nanos) { return this->onAnimate(nanos); }

bool GM::runAsBench() const { return false; }

void GM::onOnceBeforeDraw() {}

bool GM::onAnimate(double /*nanos*/) { return false; }

bool GM::onChar(SkUnichar uni) { return false; }

bool GM::onGetControls(SkMetaData*) { return false; }

void GM::onSetControls(const SkMetaData&) {}

/////////////////////////////////////////////////////////////////////////////////////////////

void GM::drawSizeBounds(SkCanvas* canvas, SkColor color) {
    canvas->drawRect(SkRect::Make(this->getISize()), SkPaint(SkColor4f::FromColor(color)));
}

// need to explicitly declare this, or we get some weird infinite loop llist
template GMRegistry* GMRegistry::gHead;

#if defined(SK_GANESH)
DrawResult GpuGM::onDraw(GrRecordingContext* rContext, SkCanvas* canvas, SkString* errorMsg) {
    this->onDraw(rContext, canvas);
    return DrawResult::kOk;
}
void GpuGM::onDraw(GrRecordingContext*, SkCanvas*) {
    SK_ABORT("Not implemented.");
}

DrawResult GpuGM::onDraw(SkCanvas* canvas, SkString* errorMsg) {

    auto rContext = canvas->recordingContext();
    if (!rContext) {
        *errorMsg = kErrorMsg_DrawSkippedGpuOnly;
        return DrawResult::kSkip;
    }
    if (rContext->abandoned()) {
        *errorMsg = "GrContext abandoned.";
        return DrawResult::kSkip;
    }
    return this->onDraw(rContext, canvas, errorMsg);
}
#endif

template <typename Fn>
static void mark(SkCanvas* canvas, SkScalar x, SkScalar y, Fn&& fn) {
    SkPaint alpha;
    alpha.setAlpha(0x50);
    canvas->saveLayer(nullptr, &alpha);
        canvas->translate(x,y);
        canvas->scale(2,2);
        fn();
    canvas->restore();
}

void MarkGMGood(SkCanvas* canvas, SkScalar x, SkScalar y) {
    mark(canvas, x,y, [&]{
        // A green circle.
        canvas->drawCircle(0, 0, 12, SkPaint(SkColor4f::FromColor(SkColorSetRGB(27, 158, 119))));

        // Cut out a check mark.
        SkPaint paint(SkColors::kTransparent);
        paint.setBlendMode(SkBlendMode::kSrc);
        paint.setStrokeWidth(2);
        paint.setStyle(SkPaint::kStroke_Style);
        canvas->drawLine(-6, 0,
                         -1, 5, paint);
        canvas->drawLine(-1, +5,
                         +7, -5, paint);
    });
}

void MarkGMBad(SkCanvas* canvas, SkScalar x, SkScalar y) {
    mark(canvas, x,y, [&] {
        // A red circle.
        canvas->drawCircle(0,0, 12, SkPaint(SkColor4f::FromColor(SkColorSetRGB(231, 41, 138))));

        // Cut out an 'X'.
        SkPaint paint(SkColors::kTransparent);
        paint.setBlendMode(SkBlendMode::kSrc);
        paint.setStrokeWidth(2);
        paint.setStyle(SkPaint::kStroke_Style);
        canvas->drawLine(-5,-5,
                         +5,+5, paint);
        canvas->drawLine(+5,-5,
                         -5,+5, paint);
    });
}

namespace skiagm {
void Register(skiagm::GM* gm) {
    // The skiagm::GMRegistry class is a subclass of sk_tools::Registry. Instances of
    // sk_tools::Registry form a linked list (there is one such list for each subclass), where each
    // instance holds a value and a pointer to the next sk_tools::Registry instance. The head of
    // this linked list is stored in a global variable. The sk_tools::Registry constructor
    // automatically pushes a new instance to the head of said linked list. Therefore, in order to
    // register a value in the GM registry, it suffices to just instantiate skiagm::GMRegistry with
    // the value we wish to register.
    new skiagm::GMRegistry([=]() { return std::unique_ptr<skiagm::GM>(gm); });
}
}  // namespace skiagm
