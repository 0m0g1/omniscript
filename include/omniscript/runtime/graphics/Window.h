// #ifndef Window_H
// #define Window_H

// // #include <memory>
// // #include <string>
// #include <GLFW/glfw3.h>
// #include <omniscript/omniscript_pch.h>
// #include <omniscript/runtime/object.h>
// #include <omniscript/runtime/graphics/Context2D.h>
// #include <omniscript/runtime/graphics/Context3D.h>

// struct WindowProps {
//     std::string ID;
//     std::string Title;
//     unsigned int Width;
//     unsigned int Height;
//     bool VSync = false;

//     WindowProps(
//                 const std::string& id = "window_?",
//                 const std::string& title = "Omniscript Window",
//                 unsigned int width = 1280,
//                 unsigned int height = 720,
//                 bool vSync = false)
//         : ID(id), Title(title), Width(width), Height(height), VSync(vSync) {}
// };

// // Basic window class using GLFW
// class Window : public Object {
// public:
//     Window(const WindowProps& props = WindowProps());
//     ~Window();

//     inline unsigned int GetWidth() const { return windowData.Width; }
//     inline unsigned int GetHeight() const { return windowData.Height; }
//     SymbolTable::ValueType getContext(std::string& type);

//     inline GLFWwindow* GetNativeWindow() const { return m_Window; }
//     static std::shared_ptr<Window> createWindow(const WindowProps& props = WindowProps());

//     void onUpdate();
//     void Shutdown();

//     void SetVSync(bool enable = true);
//     bool isVSync();


// private:
//     void Init(const WindowProps& props);

//     GLFWwindow* m_Window;

//     WindowProps windowData;

//     std::shared_ptr<Context2D> context2d;
//     std::shared_ptr<Context3D> context3d;
// };

// #endif