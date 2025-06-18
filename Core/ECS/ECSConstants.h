#pragma once

#include <cstddef>
#include <cstdint>

namespace BGE {

// Component type identifier
using ComponentTypeID = uint32_t;

// Maximum number of component types
constexpr size_t MAX_COMPONENTS = 512;

// Invalid component type
constexpr ComponentTypeID INVALID_COMPONENT_TYPE = UINT32_MAX;

} // namespace BGE