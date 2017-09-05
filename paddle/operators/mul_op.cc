/* Copyright (c) 2016 PaddlePaddle Authors. All Rights Reserve.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License. */

#include "paddle/operators/mul_op.h"

namespace paddle {
namespace operators {

using framework::Tensor;

class MulOp : public framework::OperatorWithKernel {
 public:
  using framework::OperatorWithKernel::OperatorWithKernel;

 protected:
  void InferShape(const framework::InferShapeContext &ctx) const override {
    auto x_dim = ctx.Input<Tensor>("X")->dims();
    auto y_dim = ctx.Input<Tensor>("Y")->dims();
    int x_num_row_dims = GetAttr<int>("x_num_row_dims");
    int y_num_row_dims = GetAttr<int>("y_num_row_dims");

    PADDLE_ENFORCE(x_dim.size() > x_num_row_dims,
                   "The rank of input tensor X(%s) should be larger than "
                   "`mul_op`'s `x_num_row_dims`.",
                   ctx.op().Input("X"));
    PADDLE_ENFORCE(y_dim.size() > y_num_row_dims,
                   "The rank of input tensor Y(%s) should be larger than "
                   "`mul_op`'s `y_num_row_dims`.",
                   ctx.op().Input("Y"));
    PADDLE_ENFORCE_EQ(
        product(x_dim, x_dim.size() - x_num_row_dims, x_dim.size()),
        product(y_dim, 0, y_dim.size() - y_num_row_dims),
        "First matrix's width must be equal with second matrix's height.");
    ctx.Output<Tensor>("Out")->Resize(
        {static_cast<int>(product(x_dim, 0, x_dim.size() - x_num_row_dims)),
         static_cast<int>(
             product(y_dim, y_dim.size() - y_num_row_dims, y_dim.size()))});
  }
};

class MulOpMaker : public framework::OpProtoAndCheckerMaker {
 public:
  MulOpMaker(framework::OpProto *proto, framework::OpAttrChecker *op_checker)
      : OpProtoAndCheckerMaker(proto, op_checker) {
    AddInput("X", "The first input of mul op");
    AddInput("Y", "The second input of mul op");
    AddOutput("Out", "The output of mul op");
    AddAttr<int>(
        "x_num_row_dims",
        "mul_op can take tensors with more than two dimensions as input `X`, "
        "in that case, tensors will be flattened to a matrix. The matrix's "
        "second dimension(row length) will be the product of tensor's last "
        "`num_row_dims` dimensions, and the matrix's first dimension(column "
        "length) will be the product of tensor's first `rank - num_row_dims` "
        "dimensions.")
        .SetDefault(1)
        .EqualLargerThan(1);
    AddAttr<int>(
        "y_num_row_dims",
        "mul_op can take tensors with more than two dimensions as input `Y`, "
        "in that case, tensors will be flattened to a matrix. Just like input "
        "`X`.")
        .SetDefault(1)
        .EqualLargerThan(1);
    AddComment(R"DOC(
Two Element Mul Operator.

The equation is: Out = X * Y
)DOC");
  }
};

class MulOpGrad : public framework::OperatorWithKernel {
 public:
  using framework::OperatorWithKernel::OperatorWithKernel;

 protected:
  void InferShape(const framework::InferShapeContext &ctx) const override {
    PADDLE_ENFORCE_NOT_NULL(ctx.InputVar("X"), "Input(X) should not be null");
    PADDLE_ENFORCE_NOT_NULL(ctx.InputVar("Y"), "Input(Y) should not be null");
    PADDLE_ENFORCE_NOT_NULL(ctx.InputVar(framework::GradVarName("Out")),
                            "Input(Out@GRAD) should not be null");
    auto x_dims = ctx.Input<Tensor>("X")->dims();
    auto y_dims = ctx.Input<Tensor>("Y")->dims();
    auto out_dims = ctx.Input<Tensor>(framework::GradVarName("Out"))->dims();
    auto *x_grad = ctx.Output<Tensor>(framework::GradVarName("X"));
    auto *y_grad = ctx.Output<Tensor>(framework::GradVarName("Y"));
    PADDLE_ENFORCE(
        product(x_dims, 0, x_dims.size() - GetAttr<int>("x_num_row_dims")) ==
            out_dims[0],
        "The first dimension of Out@GRAD must equal to the first dimension of "
        "the first operand.");
    PADDLE_ENFORCE(
        product(y_dims, y_dims.size() - GetAttr<int>("y_num_row_dims"),
                y_dims.size()) == out_dims[1],
        "The second dimension of Out@GRAD must equal to the second "
        "dimension of the second operand.");

    x_grad->Resize(x_dims);
    y_grad->Resize(y_dims);
  }
};

}  // namespace operators
}  // namespace paddle

namespace ops = paddle::operators;
REGISTER_OP(mul, ops::MulOp, ops::MulOpMaker, mul_grad, ops::MulOpGrad);
REGISTER_OP_CPU_KERNEL(mul, ops::MulKernel<paddle::platform::CPUPlace, float>);
REGISTER_OP_CPU_KERNEL(mul_grad,
                       ops::MulGradKernel<paddle::platform::CPUPlace, float>);
