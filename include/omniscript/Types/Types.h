#pragma once

#include <vector>
#include <string>
#include <memory>
#include <omniscript/Target_config.h>
#include <omniscript/Types/TypeKind.h>
#include <omniscript/Types/BaseType.h>
#include <omniscript/Types/DerivedTypes.h>

namespace Omniscript {

std::shared_ptr<Type> resolveType(const std::vector<std::string>& dataTypes);
std::shared_ptr<Type> resolveFunctionType(const std::vector<std::string>& dataTypes, size_t& index);

} // namespace Omniscript