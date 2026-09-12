module;

#include <gtest/gtest.h>

export module native_marshal_tests;

import ghidra.decompiler;
import std;

// Port provenance: Ghidra/Features/Decompiler/src/decompile/unittests/testmarshal.cc.
// This module preserves the portable marshal contracts from test_signed_attributes,
// test_unsigned_attributes, test_mixed_attributes, test_attributes, test_hierarchy,
// test_unexpected_eof, test_noremaining, test_openmismatch, test_closemismatch, and
// test_bufferpad without copying that file's test harness or translator scaffolding.

namespace {

/// Supplies the one processor address space needed by the original address-space
/// attribute test while leaving all marshal behavior under test in the exported API.
class MarshalAddressSpaceManager final : public ghidra::AddrSpaceManager {
public:
    /// Constructs a manager with the original test space at marshal index three.
    MarshalAddressSpaceManager() {
        insertSpace(new ghidra::AddrSpace(this, nullptr, ghidra::IPTR_PROCESSOR, "ram", false, 8, 1, 3,
                                          ghidra::AddrSpace::hasphysical, 1, 1));
    }
};

/// Initializes the exported attribute and element registries required by XML name lookup.
void initialize_marshal_identifiers() {
    static const bool initialized = [] {
        ghidra::AttributeId::initialize();
        ghidra::ElementId::initialize();
        return true;
    }();
    (void)initialized;
}

/// Returns the stable minimal address-space manager shared by marshal tests.
MarshalAddressSpaceManager& marshal_address_spaces() {
    static MarshalAddressSpaceManager manager;
    return manager;
}

/// Returns the signed values that exercise each packed integer-width boundary from
/// test_signed_attributes in the original testmarshal.cc.
const auto& signed_attribute_cases() {
    using Case = std::pair<const ghidra::AttributeId*, ghidra::intb>;
    static const std::array<Case, 9> cases{
        Case{&ghidra::ATTRIB_ALIGN, 3},
        Case{&ghidra::ATTRIB_BIGENDIAN, -0x100},
        Case{&ghidra::ATTRIB_CONSTRUCTOR, 0x1fffff},
        Case{&ghidra::ATTRIB_DESTRUCTOR, -0xabcdefa},
        Case{&ghidra::ATTRIB_EXTRAPOP, 0x300000000LL},
        Case{&ghidra::ATTRIB_FORMAT, -0x30101010101LL},
        Case{&ghidra::ATTRIB_ID, 0x123456789011LL},
        Case{&ghidra::ATTRIB_INDEX, -0xf0f0f0f0f0f0f0LL},
        Case{&ghidra::ATTRIB_METATYPE, 0x7fffffffffffffffLL},
    };
    return cases;
}

/// Returns the unsigned values that exercise each packed integer-width boundary,
/// including the 64-bit value from test_unsigned_attributes in testmarshal.cc.
const auto& unsigned_attribute_cases() {
    using Case = std::pair<const ghidra::AttributeId*, ghidra::uintb>;
    static const std::array<Case, 10> cases{
        Case{&ghidra::ATTRIB_ALIGN, 3},
        Case{&ghidra::ATTRIB_BIGENDIAN, 0x100},
        Case{&ghidra::ATTRIB_CONSTRUCTOR, 0x1fffff},
        Case{&ghidra::ATTRIB_DESTRUCTOR, 0xabcdefa},
        Case{&ghidra::ATTRIB_EXTRAPOP, 0x300000000ULL},
        Case{&ghidra::ATTRIB_FORMAT, 0x30101010101ULL},
        Case{&ghidra::ATTRIB_ID, 0x123456789011ULL},
        Case{&ghidra::ATTRIB_INDEX, 0xf0f0f0f0f0f0f0ULL},
        Case{&ghidra::ATTRIB_METATYPE, 0x7fffffffffffffffULL},
        Case{&ghidra::ATTRIB_MODEL, 0x8000000000000000ULL},
    };
    return cases;
}

/// Encodes and decodes the signed boundary values through a real Encoder and Decoder.
/// The traversal checks both attribute ordering and the value returned by readSignedInteger.
void round_trip_signed_attributes(std::ostringstream& output, ghidra::Encoder& encoder, ghidra::Decoder& decoder) {
    encoder.openElement(ghidra::ELEM_ADDR);
    for (const auto& entry : signed_attribute_cases())
        encoder.writeSignedInteger(*entry.first, entry.second);
    encoder.closeElement(ghidra::ELEM_ADDR);

    std::istringstream input(output.str());
    decoder.ingestStream(input);
    const ghidra::uint4 element_id = decoder.openElement(ghidra::ELEM_ADDR);
    for (const auto& entry : signed_attribute_cases()) {
        ASSERT_EQ(decoder.getNextAttributeId(), entry.first->getId());
        EXPECT_EQ(decoder.readSignedInteger(), entry.second);
    }
    EXPECT_EQ(decoder.getNextAttributeId(), 0U);
    decoder.closeElement(element_id);
}

/// Encodes and decodes the unsigned boundary values through a real Encoder and Decoder,
/// including the high bit of the exported 64-bit unsigned representation.
void round_trip_unsigned_attributes(std::ostringstream& output, ghidra::Encoder& encoder, ghidra::Decoder& decoder) {
    encoder.openElement(ghidra::ELEM_ADDR);
    for (const auto& entry : unsigned_attribute_cases())
        encoder.writeUnsignedInteger(*entry.first, entry.second);
    encoder.closeElement(ghidra::ELEM_ADDR);

    std::istringstream input(output.str());
    decoder.ingestStream(input);
    const ghidra::uint4 element_id = decoder.openElement(ghidra::ELEM_ADDR);
    for (const auto& entry : unsigned_attribute_cases()) {
        ASSERT_EQ(decoder.readUnsignedInteger(*entry.first), entry.second);
    }
    decoder.closeElement(element_id);
}

/// Verifies the signed-or-string compatibility reader used by test_mixed_attributes
/// in testmarshal.cc for both packed and XML encodings.
void round_trip_mixed_attributes(std::ostringstream& output, ghidra::Encoder& encoder, ghidra::Decoder& decoder) {
    encoder.openElement(ghidra::ELEM_ADDR);
    encoder.writeSignedInteger(ghidra::ATTRIB_ALIGN, 456);
    encoder.writeString(ghidra::ATTRIB_EXTRAPOP, "unknown");
    encoder.closeElement(ghidra::ELEM_ADDR);

    std::istringstream input(output.str());
    decoder.ingestStream(input);
    const ghidra::uint4 element_id = decoder.openElement(ghidra::ELEM_ADDR);
    ghidra::intb align = -1;
    ghidra::intb extrapop = -1;
    ASSERT_EQ(decoder.getNextAttributeId(), ghidra::ATTRIB_ALIGN.getId());
    align = decoder.readSignedIntegerExpectString("00blah", 700);
    ASSERT_EQ(decoder.getNextAttributeId(), ghidra::ATTRIB_EXTRAPOP.getId());
    extrapop = decoder.readSignedIntegerExpectString("unknown", 800);
    EXPECT_EQ(decoder.getNextAttributeId(), 0U);
    decoder.closeElement(element_id);
    EXPECT_EQ(align, 456);
    EXPECT_EQ(extrapop, 800);
}

/// Verifies booleans, a processor address space, empty and ordinary strings, XML
/// escaping, UTF-8 bytes, and a long string from test_attributes in testmarshal.cc.
void round_trip_attributes(std::ostringstream& output, ghidra::Encoder& encoder, ghidra::Decoder& decoder) {
    MarshalAddressSpaceManager& spaces = marshal_address_spaces();
    ghidra::AddrSpace* ram = spaces.getSpace(3);
    ASSERT_NE(ram, nullptr);

    const std::string special_text = "<<\xE2\x82\xAC>>&\"bl a  h'\\bleh\n\t";
    const std::string long_text = "one to three four five six seven eight nine ten eleven twelve thirteen "
                                  "fourteen fifteen sixteen seventeen eighteen nineteen twenty twenty one "
                                  "blahblahblahblahblahblahblahblahblahblahblahblahblahblahblahblahblahblah";

    encoder.openElement(ghidra::ELEM_DATA);
    encoder.writeBool(ghidra::ATTRIB_ALIGN, true);
    encoder.writeBool(ghidra::ATTRIB_BIGENDIAN, false);
    encoder.writeSpace(ghidra::ATTRIB_SPACE, ram);
    encoder.writeString(ghidra::ATTRIB_VAL, "");
    encoder.writeString(ghidra::ATTRIB_VALUE, "hello");
    encoder.writeString(ghidra::ATTRIB_CONSTRUCTOR, special_text);
    encoder.writeString(ghidra::ATTRIB_DESTRUCTOR, long_text);
    encoder.closeElement(ghidra::ELEM_DATA);

    std::istringstream input(output.str());
    decoder.ingestStream(input);
    const ghidra::uint4 element_id = decoder.openElement(ghidra::ELEM_DATA);
    EXPECT_TRUE(decoder.readBool(ghidra::ATTRIB_ALIGN));
    EXPECT_FALSE(decoder.readBool(ghidra::ATTRIB_BIGENDIAN));
    EXPECT_EQ(decoder.readSpace(ghidra::ATTRIB_SPACE), ram);
    EXPECT_EQ(decoder.readString(ghidra::ATTRIB_VAL), "");
    EXPECT_EQ(decoder.readString(ghidra::ATTRIB_VALUE), "hello");
    EXPECT_EQ(decoder.readString(ghidra::ATTRIB_CONSTRUCTOR), special_text);
    EXPECT_EQ(decoder.readString(ghidra::ATTRIB_DESTRUCTOR), long_text);
    decoder.closeElement(element_id);
}

/// Verifies depth-first traversal, peeking, generic opening, explicit opening, and
/// sibling skipping for the hierarchy built by test_hierarchy in testmarshal.cc.
void round_trip_hierarchy(std::ostringstream& output, ghidra::Encoder& encoder, ghidra::Decoder& decoder) {
    encoder.openElement(ghidra::ELEM_DATA);
    encoder.writeBool(ghidra::ATTRIB_CONTENT, true);
    encoder.openElement(ghidra::ELEM_INPUT);
    encoder.openElement(ghidra::ELEM_OUTPUT);
    encoder.writeSignedInteger(ghidra::ATTRIB_ID, 0x1000);
    encoder.openElement(ghidra::ELEM_DATA);
    encoder.openElement(ghidra::ELEM_DATA);
    encoder.openElement(ghidra::ELEM_OFF);
    encoder.closeElement(ghidra::ELEM_OFF);
    encoder.openElement(ghidra::ELEM_OFF);
    encoder.writeString(ghidra::ATTRIB_ID, "blahblah");
    encoder.closeElement(ghidra::ELEM_OFF);
    encoder.openElement(ghidra::ELEM_OFF);
    encoder.closeElement(ghidra::ELEM_OFF);
    encoder.closeElement(ghidra::ELEM_DATA);
    encoder.closeElement(ghidra::ELEM_DATA);
    encoder.openElement(ghidra::ELEM_SYMBOL);
    encoder.writeUnsignedInteger(ghidra::ATTRIB_ID, 17);
    encoder.openElement(ghidra::ELEM_TARGET);
    encoder.closeElement(ghidra::ELEM_TARGET);
    encoder.closeElement(ghidra::ELEM_SYMBOL);
    encoder.closeElement(ghidra::ELEM_OUTPUT);
    encoder.closeElement(ghidra::ELEM_INPUT);
    for (int index = 0; index < 6; ++index) {
        encoder.openElement(ghidra::ELEM_INPUT);
        encoder.closeElement(ghidra::ELEM_INPUT);
    }
    encoder.closeElement(ghidra::ELEM_DATA);

    std::istringstream input(output.str());
    decoder.ingestStream(input);
    const ghidra::uint4 root_id = decoder.openElement(ghidra::ELEM_DATA);
    EXPECT_TRUE(decoder.readBool(ghidra::ATTRIB_CONTENT));
    const ghidra::uint4 input_id = decoder.openElement(ghidra::ELEM_INPUT);
    const ghidra::uint4 output_id = decoder.openElement(ghidra::ELEM_OUTPUT);
    EXPECT_EQ(decoder.readSignedInteger(ghidra::ATTRIB_ID), 0x1000);
    EXPECT_EQ(decoder.peekElement(), ghidra::ELEM_DATA.getId());
    const ghidra::uint4 outer_data_id = decoder.openElement();
    EXPECT_EQ(outer_data_id, ghidra::ELEM_DATA.getId());
    const ghidra::uint4 inner_data_id = decoder.openElement(ghidra::ELEM_DATA);
    for (int index = 0; index < 3; ++index) {
        const ghidra::uint4 off_id = decoder.openElement(ghidra::ELEM_OFF);
        decoder.closeElement(off_id);
    }
    decoder.closeElement(inner_data_id);
    decoder.closeElement(outer_data_id);
    decoder.closeElementSkipping(output_id);
    decoder.closeElement(input_id);

    for (int index = 0; index < 2; ++index) {
        const ghidra::uint4 sibling_id = decoder.openElement(ghidra::ELEM_INPUT);
        decoder.closeElement(sibling_id);
    }
    decoder.closeElementSkipping(root_id);
}

/// Verifies that both exported decoders reject the deliberately unclosed stream
/// produced by test_unexpected_eof in testmarshal.cc.
void expect_unexpected_end(std::ostringstream& output, ghidra::Encoder& encoder, ghidra::Decoder& decoder) {
    encoder.openElement(ghidra::ELEM_DATA);
    encoder.openElement(ghidra::ELEM_INPUT);
    encoder.writeString(ghidra::ATTRIB_NAME, "hello");
    encoder.closeElement(ghidra::ELEM_INPUT);

    EXPECT_THROW(
        {
            std::istringstream input(output.str());
            decoder.ingestStream(input);
            const ghidra::uint4 root_id = decoder.openElement(ghidra::ELEM_DATA);
            const ghidra::uint4 input_id = decoder.openElement(ghidra::ELEM_INPUT);
            decoder.closeElement(input_id);
            decoder.closeElement(root_id);
        },
        ghidra::DecoderError);
}

/// Verifies the no-remaining-child error contract from test_noremaining in testmarshal.cc.
void expect_no_remaining_child(std::ostringstream& output, ghidra::Encoder& encoder, ghidra::Decoder& decoder) {
    encoder.openElement(ghidra::ELEM_INPUT);
    encoder.openElement(ghidra::ELEM_OFF);
    encoder.closeElement(ghidra::ELEM_OFF);
    encoder.closeElement(ghidra::ELEM_INPUT);

    std::istringstream input(output.str());
    decoder.ingestStream(input);
    const ghidra::uint4 input_id = decoder.openElement(ghidra::ELEM_INPUT);
    const ghidra::uint4 off_id = decoder.openElement(ghidra::ELEM_OFF);
    decoder.closeElement(off_id);
    EXPECT_THROW(decoder.openElement(ghidra::ELEM_OFF), ghidra::DecoderError);
    decoder.closeElement(input_id);
}

/// Verifies that an explicitly requested child tag mismatch is rejected by both
/// exported decoder implementations, matching test_openmismatch in testmarshal.cc.
void expect_open_mismatch(std::ostringstream& output, ghidra::Encoder& encoder, ghidra::Decoder& decoder) {
    encoder.openElement(ghidra::ELEM_INPUT);
    encoder.openElement(ghidra::ELEM_OFF);
    encoder.closeElement(ghidra::ELEM_OFF);
    encoder.closeElement(ghidra::ELEM_INPUT);

    std::istringstream input(output.str());
    decoder.ingestStream(input);
    decoder.openElement(ghidra::ELEM_INPUT);
    EXPECT_THROW(decoder.openElement(ghidra::ELEM_OUTPUT), ghidra::DecoderError);
}

/// Verifies the packed decoder's encoded-close-id mismatch contract from
/// test_closemismatch in testmarshal.cc; the XML close-id check is debug-only in the
/// exported implementation and is therefore intentionally not asserted here.
void expect_packed_close_mismatch(std::ostringstream& output, ghidra::PackedEncode& encoder,
                                  ghidra::PackedDecode& decoder) {
    encoder.openElement(ghidra::ELEM_INPUT);
    encoder.openElement(ghidra::ELEM_OFF);
    encoder.closeElement(ghidra::ELEM_OFF);
    encoder.closeElement(ghidra::ELEM_INPUT);

    std::istringstream input(output.str());
    decoder.ingestStream(input);
    const ghidra::uint4 input_id = decoder.openElement(ghidra::ELEM_INPUT);
    EXPECT_THROW(decoder.closeElement(input_id), ghidra::DecoderError);
}

/// Verifies the exact one-buffer packed encoding boundary and all 511 alternating
/// boolean attributes from test_bufferpad in testmarshal.cc.
void expect_packed_buffer_boundary() {
    ASSERT_EQ(ghidra::PackedDecode::BUFFER_SIZE, 1024);

    std::ostringstream output;
    ghidra::PackedEncode encoder(output);
    encoder.openElement(ghidra::ELEM_INPUT);
    for (int index = 0; index < 511; ++index)
        encoder.writeBool(ghidra::ATTRIB_CONTENT, (index & 1) == 0);
    encoder.closeElement(ghidra::ELEM_INPUT);
    ASSERT_EQ(output.str().size(), 1024U);

    std::istringstream input(output.str());
    ghidra::PackedDecode decoder(&marshal_address_spaces());
    decoder.ingestStream(input);
    const ghidra::uint4 element_id = decoder.openElement(ghidra::ELEM_INPUT);
    for (int index = 0; index < 511; ++index) {
        ASSERT_EQ(decoder.getNextAttributeId(), ghidra::ATTRIB_CONTENT.getId());
        EXPECT_EQ(decoder.readBool(), (index & 1) == 0);
    }
    EXPECT_EQ(decoder.peekElement(), 0U);
    decoder.closeElement(element_id);
}

/// Verifies signed packed integer widths through the real ghidra.decompiler implementation.
TEST(NativeMarshal, SignedPacked) {
    initialize_marshal_identifiers();
    std::ostringstream output;
    ghidra::PackedEncode encoder(output);
    ghidra::PackedDecode decoder(&marshal_address_spaces());
    round_trip_signed_attributes(output, encoder, decoder);
}

/// Verifies signed XML integer values through the real ghidra.decompiler implementation.
TEST(NativeMarshal, SignedXml) {
    initialize_marshal_identifiers();
    std::ostringstream output;
    ghidra::XmlEncode encoder(output);
    ghidra::XmlDecode decoder(&marshal_address_spaces());
    round_trip_signed_attributes(output, encoder, decoder);
}

/// Verifies unsigned packed integer widths, including the 64-bit high-bit case.
TEST(NativeMarshal, UnsignedPacked) {
    initialize_marshal_identifiers();
    std::ostringstream output;
    ghidra::PackedEncode encoder(output);
    ghidra::PackedDecode decoder(&marshal_address_spaces());
    round_trip_unsigned_attributes(output, encoder, decoder);
}

/// Verifies unsigned XML integer values and their hexadecimal XML representation.
TEST(NativeMarshal, UnsignedXml) {
    initialize_marshal_identifiers();
    std::ostringstream output;
    ghidra::XmlEncode encoder(output);
    ghidra::XmlDecode decoder(&marshal_address_spaces());
    round_trip_unsigned_attributes(output, encoder, decoder);
}

/// Verifies mixed integer and expected-string decoding in packed format.
TEST(NativeMarshal, MixedPacked) {
    initialize_marshal_identifiers();
    std::ostringstream output;
    ghidra::PackedEncode encoder(output);
    ghidra::PackedDecode decoder(&marshal_address_spaces());
    round_trip_mixed_attributes(output, encoder, decoder);
}

/// Verifies mixed integer and expected-string decoding in XML format.
TEST(NativeMarshal, MixedXml) {
    initialize_marshal_identifiers();
    std::ostringstream output;
    ghidra::XmlEncode encoder(output);
    ghidra::XmlDecode decoder(&marshal_address_spaces());
    round_trip_mixed_attributes(output, encoder, decoder);
}

/// Verifies packed booleans, spaces, strings, escaping, and long values.
TEST(NativeMarshal, AttributesPacked) {
    initialize_marshal_identifiers();
    std::ostringstream output;
    ghidra::PackedEncode encoder(output);
    ghidra::PackedDecode decoder(&marshal_address_spaces());
    round_trip_attributes(output, encoder, decoder);
}

/// Verifies XML booleans, spaces, strings, escaping, and long values.
TEST(NativeMarshal, AttributesXml) {
    initialize_marshal_identifiers();
    std::ostringstream output;
    ghidra::XmlEncode encoder(output);
    ghidra::XmlDecode decoder(&marshal_address_spaces());
    round_trip_attributes(output, encoder, decoder);
}

/// Verifies packed hierarchy traversal and child skipping.
TEST(NativeMarshal, HierarchyPacked) {
    initialize_marshal_identifiers();
    std::ostringstream output;
    ghidra::PackedEncode encoder(output);
    ghidra::PackedDecode decoder(&marshal_address_spaces());
    round_trip_hierarchy(output, encoder, decoder);
}

/// Verifies XML hierarchy traversal and child skipping.
TEST(NativeMarshal, HierarchyXml) {
    initialize_marshal_identifiers();
    std::ostringstream output;
    ghidra::XmlEncode encoder(output);
    ghidra::XmlDecode decoder(&marshal_address_spaces());
    round_trip_hierarchy(output, encoder, decoder);
}

/// Verifies packed rejection of an unclosed element stream.
TEST(NativeMarshal, UnexpectedEndPacked) {
    initialize_marshal_identifiers();
    std::ostringstream output;
    ghidra::PackedEncode encoder(output);
    ghidra::PackedDecode decoder(&marshal_address_spaces());
    expect_unexpected_end(output, encoder, decoder);
}

/// Verifies XML rejection of an unclosed element stream.
TEST(NativeMarshal, UnexpectedEndXml) {
    initialize_marshal_identifiers();
    std::ostringstream output;
    ghidra::XmlEncode encoder(output);
    ghidra::XmlDecode decoder(&marshal_address_spaces());
    expect_unexpected_end(output, encoder, decoder);
}

/// Verifies packed rejection when no child remains under the current element.
TEST(NativeMarshal, NoRemainingPacked) {
    initialize_marshal_identifiers();
    std::ostringstream output;
    ghidra::PackedEncode encoder(output);
    ghidra::PackedDecode decoder(&marshal_address_spaces());
    expect_no_remaining_child(output, encoder, decoder);
}

/// Verifies XML rejection when no child remains under the current element.
TEST(NativeMarshal, NoRemainingXml) {
    initialize_marshal_identifiers();
    std::ostringstream output;
    ghidra::XmlEncode encoder(output);
    ghidra::XmlDecode decoder(&marshal_address_spaces());
    expect_no_remaining_child(output, encoder, decoder);
}

/// Verifies packed rejection of an explicitly mismatched child element.
TEST(NativeMarshal, OpenMismatchPacked) {
    initialize_marshal_identifiers();
    std::ostringstream output;
    ghidra::PackedEncode encoder(output);
    ghidra::PackedDecode decoder(&marshal_address_spaces());
    expect_open_mismatch(output, encoder, decoder);
}

/// Verifies XML rejection of an explicitly mismatched child element.
TEST(NativeMarshal, OpenMismatchXml) {
    initialize_marshal_identifiers();
    std::ostringstream output;
    ghidra::XmlEncode encoder(output);
    ghidra::XmlDecode decoder(&marshal_address_spaces());
    expect_open_mismatch(output, encoder, decoder);
}

/// Verifies packed rejection of a mismatched encoded closing element.
TEST(NativeMarshal, CloseMismatchPacked) {
    initialize_marshal_identifiers();
    std::ostringstream output;
    ghidra::PackedEncode encoder(output);
    ghidra::PackedDecode decoder(&marshal_address_spaces());
    expect_packed_close_mismatch(output, encoder, decoder);
}

/// Verifies the packed decoder's exact one-buffer padding boundary.
TEST(NativeMarshal, BufferPad) {
    initialize_marshal_identifiers();
    expect_packed_buffer_boundary();
}

} // namespace
