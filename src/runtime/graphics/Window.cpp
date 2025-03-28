// #include <omniscript/Core.h>
// #include <omniscript/debuggingtools/console.h>
// #include <omniscript/runtime/graphics/Window.h>
// #include <omniscript/runtime/graphics/Context2D.h>
// #include <omniscript/runtime/graphics/Context3D.h>

// Window::Window(const WindowProps& props) : windowData(props) {
//     Init(props);
//     addMethod("getContext", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
//         if (args.size() != 1) {
//             console.error("Get context only takes one argument");
//             return nullptr;  // Error case
//         }

//         std::string type;
//         try {
//             type = std::get<std::string>(args[0]);  // Extract string from argument
//         } catch (const std::bad_variant_access&) {
//             console.error("Argument must be a string");
//             return nullptr;  // Error case
//         }

//         if (type != "2d" && type != "3d") {
//             console.error("Invalid context type " + type);
//         }

//         return getContext(type);
//     });
// };

// Window::~Window() {
//     Shutdown();
// }

// std::shared_ptr<Window> Window::createWindow(const WindowProps& props) {
//     return std::make_shared<Window>(props);
// }

// void Window::Shutdown() {
//     glfwDestroyWindow(m_Window);
// }

// SymbolTable::ValueType Window::getContext(std::string& type) {
//     if (type == "2d") {
//         return context2d;
//     } else if (type == "3d") {
//         return context3d;
//     }

//     console.error("Invalid context type " + type);

//     return SymbolTable::ValueType{};
// }

// void Window::Init(const WindowProps& props) {
//     if (!Omniscript::isGlfwInitialized()) {
//         if (!glfwInit()) {
//             console.error("Failed to initialize GLFW");
//         }
//         Omniscript::glfwInitialized = true;
//     }

//     m_Window = glfwCreateWindow(props.Width, props.Height, props.Title.c_str(), nullptr, nullptr);

//     if (!m_Window) {
//         console.error("Window Creation Failed");
//     }
//     glfwSetWindowUserPointer(m_Window, &windowData);
// }

// void Window::SetVSync(bool enabled) {
//     if (enabled) {
//         glfwSwapInterval(1);
//     } else {
//         glfwSwapInterval(0);
//     }

//     windowData.VSync = enabled;
// }

// bool Window::isVSync() {
//     return windowData.VSync;
// }

// void Window::onUpdate() {
//     glfwSwapBuffers(m_Window);
// }