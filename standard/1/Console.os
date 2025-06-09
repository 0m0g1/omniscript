module Console {
    extern "C" fn printf(...fmt: char*) => int;
    // extern "C" fn Beep(freq: int, duration: int) => int;
    // extern "C" fn time(ptr: void*) => int;
    // extern "C" fn localtime(ptr: void*) => void*;
    // extern "C" fn strftime(buf: void*, size: int, fmt: char*, tm: void*) => int;

//   enum LogLevel { LOG, INFO, WARN, ERROR, DEBUG };

//   var debugEnabled: bool = false;

//   fn setDebug(on: bool) => void {
//     debugEnabled = on;
//   }

//   fn getTimestamp() => char* {
//     var t: int = 0;
//     time(&t);
//     var tm = localtime(&t);
//     var buf: [64]char;
//     strftime(&buf[0], 64, "%Y-%m-%d %H:%M:%S", tm);
//     return &buf[0];
//   }

//   fn getColor(level: LogLevel) => char* {
//     if (level == INFO) return "\x1b[32m"; // Green
//     if (level == WARN) return "\x1b[33m"; // Yellow
//     if (level == ERROR) return "\x1b[31m"; // Red
//     if (level == DEBUG) return "\x1b[36m"; // Cyan
//     return "\x1b[0m"; // Reset
//   }

//   fn getLevelLabel(level: LogLevel) => char* {
//     if (level == INFO) return "INFO";
//     if (level == WARN) return "WARN";
//     if (level == ERROR) return "ERROR";
//     if (level == DEBUG) return "DEBUG";
//     return "LOG";
//   }

//   fn write(msg: char*, level: LogLevel = LOG, newline: bool = true) => void {
//     var timestamp = getTimestamp();
//     var color = getColor(level);
//     var label = getLevelLabel(level);

//     printf("%s[%s] [%s] %s\x1b[0m", color, timestamp, label, msg);
//     if (newline) {
//       printf("\n");
//     }
//   }

    // public fn log(msg: char*) => void {
    //     write(msg, LOG);
    // }

//   fn info(msg: char*) => void {
//     write(msg, INFO);
//   }

//   fn warn(msg: char*) => void {
//     write(msg, WARN);
//   }

//   fn error(msg: char*) => void {
//     write(msg, ERROR);
//   }

//   fn debug(msg: char*) => void {
//     if (debugEnabled) {
//       write(msg, DEBUG);
//     }
//   }

//   fn assert(cond: bool, msg: char* = "") => void {
//     if (!cond) {
//       error("Assertion failed:");
//       if (msg[0] != 0) {
//         error(msg);
//       }
//     }
//   }

  // fn beep() => void {
  //   printf("\a");
  //   Beep(800, 200); // windows
  // }

  // fn beep(freq: int, duration: int) => void {
  //   Beep(freq, duration); // windows
  // }

  // fn clear() => void {
  //   printf("\x1b[2J\x1b[H"); // ANSI: clear screen and move cursor to top-left
  // }
}
