module;

#include <gtest/gtest.h>

export module recode.core.tests.contracts;

import recode.core;
import std;

namespace recode::core::tests {
namespace {

/// Verifies cancellation and task values are independent of worker implementation details.
TEST(CoreContractTest, OperationCancellationIsSharedByTokenCopies) {
    contracts::OperationControl control;
    const auto first = control.cancellation();
    const auto second = control.cancellation();
    EXPECT_FALSE(first.stop_requested());
    control.request_cancel();
    EXPECT_TRUE(first.stop_requested());
    EXPECT_TRUE(second.stop_requested());
}

} // namespace
} // namespace recode::core::tests
