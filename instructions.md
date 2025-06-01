OmniScript Build Instructions
=============================

Prerequisites:
--------------
- No external downloads required.
- All necessary tools (`make.exe` and `premake5.exe`) are included in the `scripts` folder and can be used and installed directly from there.

Build Steps:
------------

1. **Generate Makefiles (run when the Lua project script is changed or a new C++ file is added):**
   Run:
   ./scripts/premake/premake5.exe gmake2

2. **Clean Build (run only if any C++ file is moved to a different location):**
   Run:
   make.exe clean

3. **Compile in Debug Mode:**
   Run:
   make.exe config=debug

4. **Compile in Release Mode:**
   Run:
   make.exe config=release

Running the Engine:
-------------------

After compiling, you can run an example script to test both modes:

- **Debug Mode:**
.\bin\Debug-windows-x86_64\Osengine.exe .\examples\types.os --execute --debug

- **Release Mode:**
.\bin\Release-windows-x86_64\Osengine.exe .\examples\types.os --execute


> ⚠️ **Note:** The output folder depends on your system and architecture.
> The format used is: `bin/{config}-{system}-{arch}`  
> (e.g., `Debug-windows-x86_64` on a 64-bit Windows system)

Post-Build Notes:
-----------------

- On Windows, the `premake5.lua` script automatically copies required LLVM `.dll` files to the build output folder.
- Ensure the LLVM headers and libraries are correctly placed inside `dependencies/llvm/include` and `dependencies/llvm/lib` respectively.