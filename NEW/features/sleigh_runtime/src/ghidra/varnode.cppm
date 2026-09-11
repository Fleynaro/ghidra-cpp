/* ###
 * IP: GHIDRA
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
module sleigh_runtime.ghidra;

namespace ghidra {

/// Decodes a VarnodeData from an `<addr>`, `<register>`, or `<varnode>` element.
void VarnodeData::decode(Decoder& decoder) {
    const uint4 element = decoder.openElement();
    decodeFromAttributes(decoder);
    decoder.closeElement(element);
}

/// Reads a VarnodeData from attributes that may be mixed with other metadata.
void VarnodeData::decodeFromAttributes(Decoder& decoder) {
    space = nullptr;
    size = 0;
    for (;;) {
        const uint4 attribute = decoder.getNextAttributeId();
        if (attribute == 0)
            break;
        if (attribute == ATTRIB_SPACE) {
            space = decoder.readSpace();
            decoder.rewindAttributes();
            offset = space->decodeAttributes(decoder, size);
            break;
        }
        if (attribute == ATTRIB_NAME) {
            const Translate* translator = decoder.getAddrSpaceManager()->getDefaultCodeSpace()->getTrans();
            const VarnodeData& register_location(translator->getRegister(decoder.readString()));
            *this = register_location;
            break;
        }
    }
}

/// Tests whether another raw storage range is contained by this one.
bool VarnodeData::contains(const VarnodeData& other) const {
    if (space != other.space || other.offset < offset)
        return false;
    return (offset + (size - 1)) >= (other.offset + (other.size - 1));
}

/// Tests whether another raw storage range immediately precedes this one.
bool VarnodeData::isContiguous(const VarnodeData& other) const {
    if (space != other.space)
        return false;
    if (space->isBigEndian())
        return space->wrapOffset(offset + size) == other.offset;
    return space->wrapOffset(other.offset + other.size) == offset;
}

} // namespace ghidra
