module Console {
//   public enum LogLevel { LOG, INFO, WARN, ERROR, DEBUG };

//   public var debugEnabled: bool = false;

//   public function setDebug(on: bool) => void { debugEnabled = on; }

//   public function log(msg: string) => void { write(msg, LOG); }
//   public function info(msg: string) => void { write(msg, INFO); }
//   public function warn(msg: string) => void { write(msg, WARN); }
//   public function error(msg: string) => void { write(msg, ERROR); }
//   public function debug(msg: string) => void {
//     if (debugEnabled) write(msg, DEBUG);
//   }

//   public function write(msg: string,
//                         level: LogLevel = LOG,
//                         newline: bool = true) => void {
//     // choose ANSI color based on level…
//     Backend.write(coloredMsg);
//     if (newline) Backend.write("\n");
//   }

//   public function assert(cond: bool, msg: string = "") => void {
//     if (!cond) error("Assertion failed: " + msg);
//   }

//   public function clear() => void { Backend.clearScreen(); }

//   public function count(label: string = "default") => void { /* … */ }
//   public function countReset(label: string = "default") => void { /* … */ }

//   public function group(label: string = "") => void { /* … */ }
//   public function groupCollapsed(label: string = "") => void { /* … */ }
//   public function groupEnd() => void { /* … */ }

//   public function time(label: string = "default") => void { /* … */ }
//   public function timeLog(label: string = "default") => void { /* … */ }
//   public function timeEnd(label: string = "default") => void { /* … */ }

//   public function trace(msg: string = "") => void { /* … */ }

//   public function dir(obj: any) => void { /* … */ }
//   public function dirxml(obj: any) => void { /* … */ }

//   public function table(data: any[], columns?: string[]) => void { /* … */ }

//   public function input(prompt: string = "") => string {
//     Backend.write(prompt);
//     return Backend.readLine();
//   }

//   public function beep() => void { Backend.write("\u0007"); }

//   private module Backend {
//     public function write(text: string) => void = "__console_write";
//     public function clearScreen() => void = "__console_clear";
//     public function readLine() => string = "__console_readline";
//   }
}
