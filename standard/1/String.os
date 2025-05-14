module String {
//   public class String {
//     private text: char*;

//     // Constructor
//     // public constructor(value: any) {
//     //   this.text = value.toString();
//     // }

//     public toString() => string {
//       return this.text;
//     }

//     public length() => int {
//       var count: int = 0;
//       for (cp in this.codePoints()) { count += 1; }
//       return count;
//     }

//     public charAt(pos: int) => String {
//       let cps = this.codePoints();
//       return new String(cps[pos] ? String.fromCodePoint(cps[pos]) : "");
//     }

//     public substring(start: int, end: int = this.length()) => String {
//       return new String(this.toString().slice(start, end));
//     }

//     public indexOf(substr: string, from: int = 0) => int {
//       return this.toString().indexOf(substr, from);
//     }

//     public includes(substr: string, pos: int = 0) => bool {
//       return this.toString().includes(substr, pos);
//     }

//     public toUpperCase() => String {
//       return new String(this.toString().toUpperCase());
//     }

//     public toLowerCase() => String {
//       return new String(this.toString().toLowerCase());
//     }

//     public split(delim: string, limit: int = -1) => String[] {
//       return this.toString().split(delim, limit);
//     }

//     // ...other instance methods...
//   }

//   // Static helpers
//   public static function fromCodePoint(...pts: int[]) => String {
//     return new String(String.rawFromCodePoints(pts));
//   }

//   public static function join(arr: any[], sep: string = "") => String {
//     return new String(arr.map(x => x.toString()).join(sep));
//   }

//   public static function format(template: string, args: any[]) => String {
//     let result = template;
//     for (i in 0 ..< args.length) {
//       result = result.replace("{" + i.toString() + "}", args[i].toString());
//     }
//     return new String(result);
//   }

//   public static function input(prompt: string = "") => String {
//     IO.Console.write(prompt, IO.Console.LogLevel.LOG, false);
//     return new String(IO.ConsoleBackend.readLine());
//   }

//   private module IO.ConsoleBackend {}  // platform I/O hooks
}
