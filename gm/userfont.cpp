/*
 * Copyright 2020 Google LLC
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "gm/gm.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkFont.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPath.h"
#include "include/core/SkSize.h"
#include "include/core/SkString.h"
#include "include/utils/SkCustomTypeface.h"
#include "tools/Resources.h"

static sk_sp<SkTypeface> make_tf() {
    SkCustomTypefaceBuilder builder;
    SkFont font;
    font.setSize(1.0f);
    font.setHinting(SkFontHinting::kNone);

    // Steal the first 128 chars from the default font
    for (SkGlyphID index = 0; index <= 127; ++index) {
        SkGlyphID glyph = font.unicharToGlyph(index);

        SkScalar width;
        font.getWidths(&glyph, 1, &width);
        SkPath path;
        font.getPath(glyph, &path);

        // we use the charcode to be our glyph index, since we have no cmap table
        builder.setGlyph(index, width, path);
    }

    return builder.detach();
}

#include "include/core/SkTextBlob.h"

class UserFontGM : public skiagm::GM {
    sk_sp<SkTypeface> fTF;
    sk_sp<SkTextBlob> fBlob;

    SkPath fPath;
public:
    UserFontGM() {}

    void onOnceBeforeDraw() override {
        fTF = make_tf();

        SkFont font(fTF);
        font.setSize(100);
        font.setEdging(SkFont::Edging::kAntiAlias);

        fBlob = SkTextBlob::MakeFromString("User Typeface", font);
    }

    bool runAsBench() const override { return true; }

    SkString onShortName() override { return SkString("user_typeface"); }

    SkISize onISize() override { return {512, 512}; }

    void onDraw(SkCanvas* canvas) override {
        SkScalar x = 20,
                 y = 250;

        SkPaint paint;
        paint.setStyle(SkPaint::kStroke_Style);
        canvas->drawRect(fBlob->bounds().makeOffset(x, y), paint);

        paint.setStyle(SkPaint::kFill_Style);
        paint.setColor(SK_ColorRED);
        canvas->drawTextBlob(fBlob, x, y, paint);
    }
};
DEF_GM(return new UserFontGM;)
