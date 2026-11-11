// standard/1/Toml.os  -- STUB (not implemented yet)
// Tier 7: TOML parser/emitter into the Json.JsonValue DOM (tables, arrays of tables, dates as strings).
// All functions return false / do nothing until implemented.
module Json = import "standard/1/Json.os";
function parse(text: char*) => Json.JsonValue* { return nullptr; }                 // TODO: [table], [[array]], dotted keys, inline tables
function stringify(v: Json.JsonValue*, out: void*) => bool { return false; }       // TODO
