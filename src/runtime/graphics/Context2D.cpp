// #include <GLFW/glfw3.h>
// #include <omniscript/utils.h>
// #include <omniscript/debuggingtools/console.h>
// #include <omniscript/runtime/graphics/Context2D.h>

// // Context2D::Context2D(ThreadSafeQueue* queue) : RenderingContext(queue) {
// //     addMethod("drawRect", [this](const ArgumentDefinition& args) {
// //         return drawRect(args);
// //     });
// //     addMethod("fillRect", [this](const ArgumentDefinition& args) {
// //         return fillRect(args);
// //     });
// //     addMethod("clearRect", [this](const ArgumentDefinition& args) {
// //         return fillRect(args);
// //     });
// // }

// Context2D::Context2D() : RenderingContext() {
//     addMethod("drawRect", [this](const ArgumentDefinition& args) {
//         return drawRect(args);
//     });
//     addMethod("fillRect", [this](const ArgumentDefinition& args) {
//         return fillRect(args);
//     });
//     addMethod("clearRect", [this](const ArgumentDefinition& args) {
//         return fillRect(args);
//     });
// }

// SymbolTable::ValueType Context2D::drawRect(const ArgumentDefinition& args) {
//     if (args.size() < 4) {
//         console.error("drawRect requires 4 arguments: x, y, width, height");
//         return SymbolTable::ValueType{};
//     }

//     // Extract the arguments (x, y, width, height)
//     float x = toFloat(args[0]);
//     float y = toFloat(args[1]);
//     float width = toFloat(args[2]);
//     float height = toFloat(args[3]);

//     // Optional: Check for an optional color argument (RGB values)
//     float rectColor[3] = {0.8f, 0.8f, 0.8f};  // Default to gray
//     if (args.size() >= 7) {  // Expect 7 args for color (x, y, width, height, r, g, b)
//         rectColor[0] = toFloat(args[4]);
//         rectColor[1] = toFloat(args[5]);
//         rectColor[2] = toFloat(args[6]);
//     }

//     // taskQueue->push([this, x, y, width, height, rectColor](){
//         // Begin OpenGL rendering for the rectangle
//         glBegin(GL_QUADS);
//         glColor3f(rectColor[0], rectColor[1], rectColor[2]);
//         glVertex2f(x, y);                 // Bottom-left corner
//         glVertex2f(x + width, y);         // Bottom-right corner
//         glVertex2f(x + width, y + height); // Top-right corner
//         glVertex2f(x, y + height);        // Top-left corner
//         glEnd();
//     // });

//     return SymbolTable::ValueType{};
// }

// SymbolTable::ValueType Context2D::fillRect(const ArgumentDefinition& args) {
//     if (args.size() < 4) {
//         console.error("fillRect requires 4 arguments: x, y, width, height");
//         return SymbolTable::ValueType{};
//     }

//     float x = toFloat(args[0]);
//     float y = toFloat(args[1]);
//     float width = toFloat(args[2]);
//     float height = toFloat(args[3]);

//     // taskQueue->push([x, y, width, height]() {
//         glBegin(GL_QUADS);
//         glColor3f(1.0f, 1.0f, 1.0f); // Default white
//         glVertex2f(x, y);
//         glVertex2f(x + width, y);
//         glVertex2f(x + width, y + height);
//         glVertex2f(x, y + height);
//         glEnd();
//     // });

//     return SymbolTable::ValueType{};
// }

// SymbolTable::ValueType Context2D::clearRect(const ArgumentDefinition& args) {
//     if (args.size() < 4) {
//         console.error("clearRect requires 4 arguments: x, y, width, height");
//         return SymbolTable::ValueType{};
//     }

//     float x = toFloat(args[0]);
//     float y = toFloat(args[1]);
//     float width = toFloat(args[2]);
//     float height = toFloat(args[3]);
    
//     // taskQueue->push([this, x, y, width, height]() {
//         // Set the scissor region
//         glEnable(GL_SCISSOR_TEST); // Enable scissor testing
//         glScissor(static_cast<int>(x), static_cast<int>(y), static_cast<int>(width), static_cast<int>(height));

//         // Clear the area inside the scissor region
//         glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

//         // Disable scissor testing to allow normal drawing again
//         glDisable(GL_SCISSOR_TEST);
//     // });

//     return SymbolTable::ValueType{};
// }

