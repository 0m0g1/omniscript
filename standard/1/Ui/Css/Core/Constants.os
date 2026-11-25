// standard/1/Ui/Css/Core/Constants.os
// Result codes, style-note codes and selector pseudo bits shared by every Css module.

// Result of applyProp / status of a StyleNote
let AP_APPLIED: int = 0;  let AP_UNSUPPORTED: int = 1;  let AP_INVALID: int = 2;
let AP_SKIP: int = 0 - 1;        // declaration belongs to the other pass
let AP_NOMATCH: int = 0 - 2;     // property group does not know this name (internal to Properties/*)
let SN_APPLIED: int = 0;  let SN_UNSUPPORTED: int = 1;  let SN_INVALID: int = 2;  let SN_OVERRIDDEN: int = 3;
// StyleNote.source
let SRC_TAG: int = 0;  let SRC_AUTHOR: int = 1;  let SRC_INLINE: int = 2;

// Selector pseudo bits added by Css (the original PSE_* bits live in Constants.os of Ui and stay below 65536).
let PSE_ONLY: int = 65536;       // :only-child
let PSE_EMPTY: int = 131072;     // :empty
let PSE_FIRST_T: int = 262144;   // :first-of-type
let PSE_LAST_T: int = 524288;    // :last-of-type
let PSE_LINK: int = 1048576;     // :link :any-link
let PSE_EXT_MASK: int = 2031616; // all of the above
