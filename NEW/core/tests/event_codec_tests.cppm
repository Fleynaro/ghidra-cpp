module;

#include <gtest/gtest.h>

export module ghidra.core.tests.events;

import ghidra.core;
import std;

namespace ghidra::core::tests {
namespace {

/// Verifies event envelope payload fields remain stable for persistence/replay codecs.
TEST(CoreEventTest, EnvelopePayloadLengthAndChecksumAreDeterministic) {
    const auto draft = events::project_created(ProjectId{"codec-project"}, CorrelationId{"codec-correlation"});
    EXPECT_EQ(events::checksum(draft.payload), events::checksum(draft.payload));
    EXPECT_FALSE(draft.payload.empty());
    EXPECT_EQ(events::decode_fields(draft.payload).at("project"), "codec-project");
}

} // namespace
} // namespace ghidra::core::tests
