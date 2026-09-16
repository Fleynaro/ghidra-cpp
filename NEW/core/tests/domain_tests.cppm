module;

#include <gtest/gtest.h>

export module ghidra.core.tests.domain;

import ghidra.core;
import std;

namespace ghidra::core::tests {
namespace {

/// Verifies that address arithmetic preserves space identity and rejects overflow.
TEST(CoreDomainTest, AddressArithmeticIsCheckedAndSpaceAware) {
    const Address address{AddressSpaceId{"ram"}, 0x1000};
    const auto next = address.add(0x20);
    ASSERT_TRUE(next.has_value());
    EXPECT_EQ(next->space.name(), "ram");
    EXPECT_EQ(next->offset, 0x1020U);
    const Address maximum{AddressSpaceId{"ram"}, std::numeric_limits<std::uint64_t>::max()};
    const Address register_address{AddressSpaceId{"register"}, 0x1000};
    EXPECT_FALSE(maximum.add(1));
    EXPECT_NE(address, register_address);
}

/// Verifies that overlapping and adjacent ranges are normalized while other spaces remain distinct.
TEST(CoreDomainTest, AddressRangeSetNormalizesInclusiveRanges) {
    AddressRangeSet ranges;
    ranges.add(AddressRange{Address{AddressSpaceId{"ram"}, 10}, Address{AddressSpaceId{"ram"}, 20}});
    ranges.add(AddressRange{Address{AddressSpaceId{"ram"}, 21}, Address{AddressSpaceId{"ram"}, 30}});
    ranges.add(AddressRange{Address{AddressSpaceId{"register"}, 0}, Address{AddressSpaceId{"register"}, 4}});
    ASSERT_EQ(ranges.ranges().size(), 2U);
    EXPECT_TRUE(ranges.contains(Address{AddressSpaceId{"ram"}, 25}));
    EXPECT_FALSE(ranges.contains(Address{AddressSpaceId{"ram"}, 31}));
    EXPECT_TRUE(ranges.contains(Address{AddressSpaceId{"register"}, 4}));
}

/// Verifies that event payload fields escape separators and round-trip deterministically.
TEST(CoreDomainTest, EventFieldsRoundTripEscapedValues) {
    const auto encoded = events::encode_fields({{"name", "a;b=c"}, {"space", "ram"}});
    const auto decoded = events::decode_fields(encoded);
    ASSERT_EQ(decoded.size(), 2U);
    EXPECT_EQ(decoded.at("name"), "a;b=c");
    EXPECT_EQ(decoded.at("space"), "ram");
    EXPECT_EQ(events::checksum("abc"), events::checksum("abc"));
    EXPECT_NE(events::checksum("abc"), events::checksum("abd"));
}

} // namespace
} // namespace ghidra::core::tests
