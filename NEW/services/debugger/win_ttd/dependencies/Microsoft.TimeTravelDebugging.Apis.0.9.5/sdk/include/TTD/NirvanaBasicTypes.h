// Copyright (c) Microsoft Corporation.
#pragma once

#include "IdnaBasicTypes.h"

#pragma message("The Nirvana namespace and this header file are deprecated and will be removed in a future release.")

// Nirvana is an old name used by some parts of TTD.
// The Nirvana namespace is here just for compatibility and will be removed in some future release.
namespace /*[[deprecated]]*/ Nirvana {

// Types
using TTD::FindValidDataLineMode;
using TTD::GuestAddress;
using TTD::GuestAddressRange;
using TTD::InjectMode;
using TTD::MessageSeverity;
using TTD::ProcessorArchitecture;
using TTD::ReplayCpuSupport;
using TTD::SelectRegisters;
using TTD::simd128_t;
using TTD::simd256_t;
using TTD::simd512_t;
using TTD::simd64_t;

// Variables
using TTD::c_nativeProcessorArchitecture;
using TTD::MaxInjectModeNameLength;

// Functions
using TTD::AddressInClosedRange;
using TTD::AddressInHalfOpenRange;
using TTD::GetInjectModeName;
using TTD::GetProcessorArchitectureName;
using TTD::GetReplayCpuSupportName;
using TTD::GuestAddressToPtr;
using TTD::PtrToGuestAddress;
using TTD::SanitizeAddressForUserMode;

} // namespace Nirvana
// namespace Nirvana
