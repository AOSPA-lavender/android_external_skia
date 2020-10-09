/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SKSL_FUNCTIONCALL
#define SKSL_FUNCTIONCALL

#include "src/sksl/ir/SkSLExpression.h"
#include "src/sksl/ir/SkSLFunctionDeclaration.h"

namespace SkSL {

/**
 * A function invocation.
 */
class FunctionCall : public Expression {
public:
    static constexpr Kind kExpressionKind = Kind::kFunctionCall;

    FunctionCall(int offset, const Type* type, const FunctionDeclaration* function,
                 std::vector<std::unique_ptr<Expression>> arguments)
    : INHERITED(offset, FunctionCallData{type, function}) {
        fExpressionChildren = std::move(arguments);
        ++this->function().callCount();
    }

    ~FunctionCall() override {
        --this->function().callCount();
    }

    const Type& type() const override {
        return *this->functionCallData().fType;
    }

    const FunctionDeclaration& function() const {
        return *this->functionCallData().fFunction;
    }

    std::vector<std::unique_ptr<Expression>>& arguments() {
        return fExpressionChildren;
    }

    const std::vector<std::unique_ptr<Expression>>& arguments() const {
        return fExpressionChildren;
    }

    bool hasProperty(Property property) const override {
        if (property == Property::kSideEffects && (this->function().modifiers().fFlags &
                                                   Modifiers::kHasSideEffects_Flag)) {
            return true;
        }
        for (const auto& arg : this->arguments()) {
            if (arg->hasProperty(property)) {
                return true;
            }
        }
        return false;
    }

    std::unique_ptr<Expression> clone() const override {
        std::vector<std::unique_ptr<Expression>> cloned;
        for (const auto& arg : this->arguments()) {
            cloned.push_back(arg->clone());
        }
        return std::unique_ptr<Expression>(new FunctionCall(fOffset, &this->type(),
                                                            &this->function(), std::move(cloned)));
    }

    String description() const override {
        String result = String(this->function().name()) + "(";
        String separator;
        for (size_t i = 0; i < this->arguments().size(); i++) {
            result += separator;
            result += this->arguments()[i]->description();
            separator = ", ";
        }
        result += ")";
        return result;
    }

private:
    using INHERITED = Expression;
};

}  // namespace SkSL

#endif
