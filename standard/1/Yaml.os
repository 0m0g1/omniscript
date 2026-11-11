// standard/1/Yaml.os  -- STUB (not implemented yet)
// Tier 7: YAML parser/emitter. Plan: parse into the Json.JsonValue DOM so both share an API.
// All functions return false / do nothing until implemented.
module Json = import "standard/1/Json.os";
function parse(text: char*) => Json.JsonValue* { return nullptr; }                 // TODO: indentation blocks, flow [] {}, scalars, anchors
function stringify(v: Json.JsonValue*, out: void*) => bool { return false; }       // TODO
