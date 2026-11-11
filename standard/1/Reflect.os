// standard/1/Reflect.os  -- STUB (not implemented yet)
// Tier 7: runtime type info (type names, fields, sizeof, offsets). Needs compiler-emitted type metadata.
// All functions return false / do nothing until implemented.
struct TypeInfo { name: char* = nullptr; size: u64 = 0; fieldCount: u64 = 0; }
// TODO: typeOf<T>() => TypeInfo*, fieldName(i), fieldOffset(i), fieldType(i), getField/setField by name.
function typeOf<T>() => TypeInfo* { return nullptr; }  // TODO
