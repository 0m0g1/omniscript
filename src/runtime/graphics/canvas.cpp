// #include <omniscript/utils.h>
// #include <omniscript/runtime/graphics/Window.h>
// #include <omniscript/runtime/graphics/canvas.h>
// #include <omniscript/debuggingtools/console.h>
// #include <omniscript/mainthreadrunner.h>
// #include <omniscript/threadsafequeue.h>
// #include <omniscript/omniscript_pch.h>
// #include <GLFW/glfw3.h>

// CanvasObject::CanvasObject() : taskQueue() {
//     if (!glfwInit()) {
//        console.error("Failed to initialize GLFW");
//     }

//     // Register methods using addMethod
//     addMethod("createWindow", [this](const ArgumentDefinition& args) {
//         int width = 800;  // Default width
//         int height = 600; // Default height
//         const char* title = "OmniScript Window";  // Default title

//         if (args.size() >= 1) {
//             width = std::get<int>(args[0]);  // Assuming args[0] is an integer for width
//         }
//         if (args.size() >= 2) {
//             height = std::get<int>(args[1]); // Assuming args[1] is an integer for height
//         }
//         if (args.size() >= 3) {
//             title = std::get<std::string>(args[2]).c_str(); // Assuming args[2] is a string for title
//         }

//         std::string windowId = "window_" + std::to_string(windows.size());
//         WindowProps winData(windowId, title, width, height);

//         // In canvas.cpp
//         std::shared_ptr<Window> windowObj = Window::createWindow(winData);

//         {
//             std::lock_guard<std::mutex> lock(windowMutex);
//             windows[windowId] = windowObj;
//             if (!isRunning()) {
//                 start();
//             }
//         }

//         return windowObj;
//     });

// }

// CanvasObject::~CanvasObject() {
//     glfwTerminate();
// }

// void CanvasObject::start() {
//     console.info("Starting the loop");
//     if (!isRunning()) {
//         canvasThread = std::thread(&CanvasObject::runLoop, this);
//     }
// }

// void CanvasObject::stop() {
//     console.info("Stopped running the loop");
//     {
//         std::lock_guard<std::mutex> lock(windowMutex);
//         running = false;
//         for (auto& [id, window] : windows) {
//             window->Shutdown();
//             console.debug("Destroyed window with ID:" + id);
//         }
//         windows.clear();
//     }
//     if (canvasThread.joinable()) {
//         canvasThread.join();
//     }
// }

// // All glfw and other rendering commands should run in the taskqueue.process
// void CanvasObject::runLoop() {
//     running = true;
//     // Sleep for one frame so everything can be initialized before the thread starts
//     // std::this_thread::sleep_for(std::chrono::milliseconds(fps));
//     while (running) {
//         {
//             std::lock_guard<std::mutex> lock(windowMutex);
            
//             for (auto it = windows.begin(); it != windows.end();) {
//                 std::shared_ptr<Window> window = it->second;

//                 window->onUpdate();
//             }

//             // Log if no windows remain
//             if (windows.empty()) {
//                 running = false;
//             }
//         }
        
//         // // TODO: Event objects
//         // ADD_MAIN_THREAD_EVENT([this]() {
//         //     std::this_thread::sleep_for(std::chrono::milliseconds(this->fps)); // Cap at ~60 FPS
//         // });
//         glfwPollEvents();

//         std::this_thread::sleep_for(std::chrono::milliseconds(fps)); // Cap at ~60 FPS
//     }
// }