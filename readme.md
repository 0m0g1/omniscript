⚠️ **Early Development Notice**

This project is in **early development** and not ready for general use.  
APIs and behavior are subject to change without notice.

I'm building a new programming language originally called **OmniScript**, but since that name was already taken, I'm temporarily calling it **OS**.

Feedback, ideas, and discussion are very welcome!


[Temporary docs](https://github.com/0m0g1/omniscript/blob/main/docs/temp.md)


OS Build Instructions
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


Other Notes
===========
To enable syntax highlighting, you’ll need to install the OmniScript Language Server (LSP).

You can find it here: 
[OmniScript Language Server](https://github.com/0m0g1/omniscript-language-server)

After downloading, open VSCode and run:
Developer: Install Extension from Location
Then select the downloaded folder to complete the installation.