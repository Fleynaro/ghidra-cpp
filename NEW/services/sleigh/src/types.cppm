/* ###
 * IP: GHIDRA
 * NOTE: Decompiler specific flags, refers to sparc,linux,windows,i386,apple,alpha,powerpc
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
/* typedefs for getting specific word sizes */

export module sleigh_runtime:types;

import std;

export namespace ghidra {

// Use of uintm and intm is deprecated.  They must currently be set to be 32-bit.
typedef std::uint32_t uintm;
typedef std::int32_t intm;

typedef std::uint64_t uint8;
typedef std::int64_t int8;
typedef std::uint32_t uint4;
typedef std::int32_t int4;
typedef std::uint16_t uint2;
typedef std::int16_t int2;
typedef std::uint8_t uint1;
typedef std::int8_t int1;

/* uintp is intended to be an unsigned integer that is the same size as a pointer */
typedef std::uintptr_t uintp;

inline constexpr int HOST_ENDIAN = std::endian::native == std::endian::little ? 0 : 1;

/*
  Big integers: These are intended to be arbitrary precison integers. However
                for efficiency, these are currently implemented as fixed precision.
                So for coding purposes, these should be interpreted as fixed
                precision integers that store as big a number as you would ever need.
*/

typedef int8 intb;   /* This is a signed big integer */
typedef uint8 uintb; /* This is an unsigned big integer */

uintb uintbmasks[9] = {
    0, 0xff, 0xffff, 0xffffff, 0xffffffff, 0xffffffffffLL, 0xffffffffffffLL, 0xffffffffffffffLL, 0xffffffffffffffffLL};

/// \param size is the desired size in bytes
/// \return a value appropriate for masking off the first \e size bytes
inline uintb calc_mask(int4 size) {
    return uintbmasks[((uint4)size) < 8 ? size : 8];
}

} // End namespace ghidra
