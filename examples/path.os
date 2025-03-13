console.log("Join Paths:", Path.join(["/home", "user", "docs"]));
console.log("Directory name:", Path.dirname("/home/user/docs/file.txt"));
console.log("Base name:", Path.basename("/home/user/docs/file.txt"));
console.log("Base name without extension:", Path.basename("/home/user/docs/file.txt", ".txt"));
console.log("Extension name:", Path.extname("/home/user/docs/file.txt"));
console.log("Resolved Path:", Path.resolve(["./folder", "../file.txt"]));
console.log("Normalized Path:", Path.normalize("/home//user/../docs/./file.txt"));
console.log("Is absolute Path:", Path.isAbsolute("/home/user/docs"));
