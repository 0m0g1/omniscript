// #ifndef CANVASOBJECT_H
// #define CANVASOBJECT_H

// #include <GLFW/glfw3.h>
// #include "omniscript/omniscript_pch.h"
// #include <omniscript/runtime/object.h>
// #include <omniscript/threadsafequeue.h>
// #include <omniscript/runtime/graphics/Window.h>
// #include <omniscript/runtime/graphics/Context2D.h>
// #include <omniscript/runtime/graphics/Context3D.h>


// class CanvasObject : public Object {
// private:
//     ThreadSafeQueue taskQueue;
//     Context2D context2d;
//     Context3D context3d;
//     std::unordered_map<std::string, std::shared_ptr<Window>> windows;
//     std::mutex windowMutex;
//     std::thread canvasThread;
//     std::atomic<bool> running{false};
//     int fps = 16;

//     void start();
//     void runLoop();  // Declaration of the loop that runs in a separate thread
//     void stop();

// public:
//     CanvasObject();  // Constructor
//     ~CanvasObject(); // Destructor

//      bool isRunning() const {
//         return running.load();  // Load the value of the atomic variable and return it
//     }

//     // std::string toString(int indentLevel = 0) const override {
//     //     return "[Canvas]";
//     // }
// };

// #endif