// Copyright (c) 2020 PaddlePaddle Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "paddle/fluid/operators/index_select_op.h"

#include <memory>

#include "paddle/fluid/framework/infershape_utils.h"
#include "paddle/fluid/framework/op_registry.h"
#include "paddle/phi/infermeta/binary.h"

namespace paddle {
namespace operators {

class IndexSelectOp : public framework::OperatorWithKernel {
 public:
  using framework::OperatorWithKernel::OperatorWithKernel;

 protected:
  framework::OpKernelType GetExpectedKernelType(
      const framework::ExecutionContext& ctx) const override {
    auto data_type = OperatorWithKernel::IndicateVarDataType(ctx, "X");
    return framework::OpKernelType(data_type, ctx.device_context());
  }
};

class IndexSelectGradOp : public framework::OperatorWithKernel {
 public:
  using framework::OperatorWithKernel::OperatorWithKernel;

  void InferShape(framework::InferShapeContext* ctx) const override {
    PADDLE_ENFORCE_EQ(
        ctx->HasInput("Index"),
        true,
        platform::errors::InvalidArgument("Input(Index) should be not null."));
    PADDLE_ENFORCE_EQ(ctx->HasInput(framework::GradVarName("Out")),
                      true,
                      platform::errors::InvalidArgument(
                          "Input(Out@GRAD) should be not null."));
    PADDLE_ENFORCE_EQ(ctx->HasOutput(framework::GradVarName("X")),
                      true,
                      platform::errors::InvalidArgument(
                          "Output(X@GRAD) should be not null."));

    ctx->SetOutputDim(framework::GradVarName("X"), ctx->GetInputDim("X"));
  }

 protected:
  framework::OpKernelType GetExpectedKernelType(
      const framework::ExecutionContext& ctx) const override {
    return framework::OpKernelType(OperatorWithKernel::IndicateVarDataType(
                                       ctx, framework::GradVarName("Out")),
                                   ctx.device_context());
  }
};

class IndexSelectOpMaker : public framework::OpProtoAndCheckerMaker {
 public:
  void Make() override {
    AddInput("X", "(Tensor) the input tensor.");
    AddInput("Index", "the 1-D tensor containing the indices to index.");
    AddOutput("Out", "the output tensor.");
    AddAttr<int>("dim", "the dimension in which we index.").SetDefault(0);
    AddComment(R"DOC(
    Returns a new tensor which indexes the input tensor
    along dimension dim using the entries in index which
    is a Tensor.

    The returned tensor has the same number of dimensions
    as the original tensor (input). The dim-th dimension
    has the same size as the length of index; other dimensions
    have the same size as in the original tensor.
    )DOC");
  }
};

template <typename T>
class IndexSelectGradMaker : public framework::SingleGradOpMaker<T> {
 public:
  using framework::SingleGradOpMaker<T>::SingleGradOpMaker;

 protected:
  void Apply(GradOpPtr<T> op) const override {
    op->SetType("index_select_grad");

    op->SetInput("X", this->Input("X"));
    op->SetInput("Index", this->Input("Index"));
    op->SetInput(framework::GradVarName("Out"), this->OutputGrad("Out"));
    op->SetOutput(framework::GradVarName("X"), this->InputGrad("X"));
    op->SetAttrMap(this->Attrs());
  }
};

DECLARE_NO_NEED_BUFFER_VARS_INFERER(IndexSelectGradNoNeedBufferVarsInferer,
                                    "X");
}  // namespace operators
}  // namespace paddle

namespace ops = paddle::operators;
DECLARE_INFER_SHAPE_FUNCTOR(index_select,
                            IndexSelectInferShapeFunctor,
                            PD_INFER_META(phi::IndexSelectInferMeta));
REGISTER_OPERATOR(index_select,
                  ops::IndexSelectOp,
                  ops::IndexSelectOpMaker,
                  ops::IndexSelectGradMaker<paddle::framework::OpDesc>,
                  ops::IndexSelectGradMaker<paddle::imperative::OpBase>,
                  IndexSelectInferShapeFunctor);
REGISTER_OPERATOR(index_select_grad,
                  ops::IndexSelectGradOp,
                  ops::IndexSelectGradNoNeedBufferVarsInferer);
