# Omniscript: one source, export to Windows, Linux and Android

## 1. What "works everywhere by just exporting" means

The promise has one honest condition:

> Code written only against Omniscript's **portable standard library** builds for every target by changing only the target flag. Code that calls GLFW, desktop OpenGL or Win32 directly is platform code and stays on that platform.

`sphere.os` today calls GLFW and `glBegin`/`glOrtho` directly, so it is platform code. Making "just export" true means giving programs something portable to call instead. That is the main work. The compiler flag is the small part.

```
osengine game.os --make --target windows   ->  game.exe  (+ glfw DLLs if used)
osengine game.os --make --target linux     ->  game
osengine game.os --make --target android   ->  game.apk   (arm64; add --target-arch x86_64 for the emulator)
```

## 2. Architecture

```
 your program (game.os)             same source for every target
        |
 portable std modules               App, Gfx (Vulkan), Input, Audio, Image, Assets, Log, Time
        |
 platform backends                  chosen at import time by target OS
   desktop:  GLFW window, files on disk, mouse/keyboard
   android:  native_app_glue, ANativeWindow, AAssetManager, touch, logcat
        |
 native dependencies                stb, miniaudio, (glfw on desktop) built per target
        |
 compiler                           LLVM triple + linker/toolchain + packager per target
```

## 3. What a portable program looks like

Android owns the main loop and can pause, destroy the window and resume. A plain `main` with `while (!shouldClose)` cannot express that. So the portable `App` module owns the loop and the program supplies callbacks. On desktop `App.run` runs the loop itself. On Android the generated entry point runs it and calls the same callbacks.

```omniscript
module App = import "standard/1/App.os";
module G   = import "standard/1/Gfx.os";

struct Game { angle: float = 0.0; }

fn init(app: App.Context*, g: Game*) => void { /* create pipelines, load assets */ }
fn frame(app: App.Context*, g: Game*, dt: float) => void {
    g.angle = g.angle + dt * 20.0;
    app.gfx.clear(0.0, 0.0, 0.0, 1.0);
    /* draw */
}
fn resize(app: App.Context*, g: Game*, w: int, h: int) => void { }

fn main() => int { return App.run("Gradient Sphere", 800, 600, init, frame, resize); }
```

Nothing in it names a platform. `App.run` and everything below it come from the platform backend.

## 4. The pieces

### 4.1 Target flag and triples (compiler)
| Target | Triple | Output |
|---|---|---|
| windows | `x86_64-w64-windows-gnu` (MinGW linking today) | `.exe` |
| linux | `x86_64-pc-linux-gnu` | ELF |
| android | `aarch64-linux-android`, `armv7a-linux-androideabi`, `x86_64-linux-android`, `i686-linux-android` | `lib<name>.so` inside an `.apk` |

Status: `--target <os>` exists. The Android triples are fixed. Android shared libraries link with the NDK's clang wrapper (`<ndk>/toolchains/llvm/prebuilt/<host>/bin/<triple><api>-clang`).

### 4.2 Platform modules chosen by import path (small compiler feature)
Both branches of an `if (Target.isAndroid)` would still be type-checked, and calls to functions that don't exist on the other platform would fail ("Callable not found"). So platform code lives in separate modules, and the import path picks the folder.

```
module W = import "standard/1/platform/{os}/Window.os";
```

`{os}` expands to `windows`, `linux` or `android` before the file is loaded. This is a small change in the import resolver (`ModuleAndImportStatements.cpp`). The folders are:

```
standard/1/platform/desktop/   Window.os (GLFW), Input.os, Assets.os      used for windows + linux
standard/1/platform/android/   Window.os (ANativeWindow), Input.os, Assets.os
```
`{os}` can resolve to `desktop` for windows and linux by a lookup table, so there is no duplicate folder.

### 4.3 Logical library names for `extern`
`c/vulkan.os` hardcodes `C:/Windows/System32/vulkan-1.dll`. It needs a per-target name instead:

| Logical | Windows | Linux | Android |
|---|---|---|---|
| `vulkan` | `vulkan-1.dll` | `libvulkan.so.1` | `libvulkan.so` |
| `miniaudio` | `miniaudio.dll` | `libminiaudio.so` | `libminiaudio.so` (bundled in APK) |

Proposed syntax: `extern lib "vulkan" { fn ...; }`. The compiler maps the name through a table, as it already does for `getRequiredSystemLibraries`.

### 4.4 Native dependencies built per target
The stb image code, miniaudio and (on desktop) GLFW are C. Each needs a build per target:
- **Windows / Linux:** the host clang (or the target's sysroot) compiles a small shim, as `miniaudio_dll.c` does now.
- **Android:** the NDK clang compiles the same shims into `libminiaudio.so` and similar. They go into the APK under `lib/<abi>/`.

A dependency manifest, `deps.json`, lists source files, defines and exported symbols per target. `--make` builds what is missing and caches it.

### 4.5 Entry point and lifecycle
- **Desktop:** `main` calls the program's `main`.
- **Android:** a generated C shim exports `android_main(struct android_app*)`, reads `APP_CMD_*` events and calls into the Omniscript entry point. The shim is compiled with the NDK clang and linked into the `.so` along with `android_native_app_glue.c` from the NDK. It needs `-u ANativeActivity_onCreate` so the linker keeps the entry symbol.
- **Lifecycle the `App` module must handle:**
  - `APP_CMD_INIT_WINDOW`: create the Vulkan surface and swapchain.
  - `APP_CMD_TERM_WINDOW`: destroy the surface and swapchain; keep the device.
  - `APP_CMD_PAUSE` / `APP_CMD_RESUME`: stop and restart audio and animation.
  - Rotation or resize: recreate the swapchain (`Vulkan.os` already does this on resize).

### 4.6 Graphics
Vulkan is the one API present on all three targets (Windows, Linux, Android 7.0 and up), and `Vulkan.os` already exists. Changes needed:
- The instance enables `VK_KHR_surface` plus a platform extension: `VK_KHR_win32_surface`, `VK_KHR_xcb_surface` or `VK_KHR_xlib_surface`, or `VK_KHR_android_surface`. Today it asks GLFW for them.
- Surface creation moves into the platform backend. Desktop calls `glfwCreateWindowSurface`. Android calls `vkCreateAndroidSurfaceKHR` with the `ANativeWindow*`.
- Desktop OpenGL (`glBegin`, `glMatrixMode`) is not portable. Programs that want portability use the `Gfx` module instead.

### 4.7 Input, assets, audio, logging
- **Input:** one pointer model (mouse and touch both produce pointer events) plus optional keyboard. Android reports `AMotionEvent`s.
- **Assets:** `Assets.open("name")`. Desktop reads `assets/` on disk. Android reads from the APK through `AAssetManager`.
- **Audio:** miniaudio already supports Android (AAudio / OpenSL ES). Same `Audio.os`, different shared library.
- **Logging:** `Log.info(...)` prints to stdout on desktop and calls `__android_log_print` on Android (visible in `adb logcat`). Android has no console, so `printf` output is lost.

### 4.8 Packaging an APK (new compiler component)
Because the activity is a `NativeActivity`, no Java or Kotlin is needed (`android:hasCode="false"`). The packager does these steps:

1. Generate `AndroidManifest.xml` from the project file (section 5).
2. `aapt2 link --manifest AndroidManifest.xml -I <sdk>/platforms/android-<N>/android.jar -o base.apk`.
3. Add `lib/<abi>/lib<name>.so` and any dependency `.so` files, plus `assets/`, to the zip.
4. `zipalign -p 4`.
5. `apksigner sign` with a debug keystore. It is created once with `keytool` if missing.

Tools needed: the Android SDK build-tools (`aapt2`, `zipalign`, `apksigner`), a platform `android.jar`, the NDK, and a JDK for `keytool` and `apksigner`. The engine finds them through `ANDROID_HOME`, `ANDROID_NDK_HOME` and `JAVA_HOME`, and fails with a message that names what is missing.

Install and run: `adb install -r game.apk`, then `adb shell am start -n <package>/android.app.NativeActivity`. Optional flags: `--install` and `--run`.

## 5. Project file (keeps source files free of platform detail)

`omniscript.json` next to the main source:

```json
{
  "name": "Gradient Sphere",
  "id": "com.example.sphere",
  "version": "1.0.0",
  "main": "game.os",
  "assets": "assets/",
  "android": { "minApi": 24, "orientation": "landscape", "abis": ["arm64-v8a", "x86_64"] }
}
```

## 6. Milestones

| # | Milestone | Done when |
|---|---|---|
| M0 | Windows exe, target flags, Android triple, NDK `.so` link | **Done, pending your build and test.** `sphere.os` runs on Windows |
| M1 | Linux target | `--target linux` produces an ELF that runs under WSL (native link first, sysroot cross-link later) |
| M2 | Android hello world | A signed APK installs, starts, and prints "hello" to `adb logcat`. Proves the packager, shim and `.so` loading |
| M3 | Vulkan on Android | A clear-colour screen. Surface create and destroy across pause and resume |
| M4 | Portable `App`, `Gfx`, `Window` modules | One `game.os` draws the same rotating sphere on Windows, Linux and Android |
| M5 | Touch input, assets, audio | A small interactive demo with sound runs on all three targets |
| M6 | `--target all`, `deps.json`, caching, CI matrix | One command builds every target |
| later | iOS (needs macOS and Xcode), WebAssembly | Out of scope for now |

## 7. What I need from you, per milestone

| Milestone | Files |
|---|---|
| M1 | none for native Linux; for sysroot cross-linking, which sysroot you will use |
| M2 | where you installed the Android SDK and NDK, your phone's Android version and ABI (or "emulator") |
| M3 | `ModuleAndImportStatements.cpp` (done), your current `Vulkan.os` after the changes in 4.6 |
| M4 | the import resolver for `{os}`, and `Window.os` |
| M5 | `Audio.os`, `Image.os` (have both), the file-IO code in the compiler's standard library |

## 8. Limits and decisions

- **"Portable" is a contract, not magic.** A program stays portable only while it imports portable modules.
- **Windows triple.** `detectHostVendor` and `detectHostEnvironment` pick `msvc` or `gnu` from the compiler that built the engine, not from the target. Decide on `gnu` (MinGW) for all Windows output, or support both.
- **Android devices.** A phone needs arm64 (`arm64-v8a`). The Windows emulator needs x86_64. Build the one you test on.
- **Lifecycle.** An Android app can be killed at any time. Games need a `save`/`load` hook in the `App` callbacks.
- **Screen sizes and DPI** differ per phone. `App` reports size and density; layouts must use them.
- **Play Store.** It requires an `.aab`, not an `.apk`. APKs are fine for development and sideloading. An AAB packager is a later step.
