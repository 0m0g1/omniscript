// Open the file in write mode for modifications (write mode for append, prepend, and truncate operations)
const filewrite = FileAccess.open("example.txt", FileAccess.WRITE);

// Demonstrating append() method
filewrite.append("This is some appended text.\n");
const file = FileAccess.open("example.txt", FileAccess.READ);
console.log("After Appending:\n" + file.getAsString());

// Demonstrating prepend() method
filewrite.prepend("This is some prepended text.\n");
console.log("After Prepending:\n" + file.getAsString());

// Open a file in read mode

// Demonstrating getAsString() method
console.log("File Content:\n" + file.getAsString());

// Demonstrating getName() and getPath() methods
console.log("File Name: " + file.getName());
console.log("File Path: " + file.getPath());


// Demonstrating truncate() method (this should clear the file content)
// filewrite.truncate();
// console.log("After Truncating:\n" + file.getAsString());

// // Demonstrating getExtension() method
// console.log("File Extension: " + file.getExtension());

// // Set file permissions (if you want to test permissions)
// filewrite.setPermissions("read");  // Permissions are typically set during write operations
// console.log("File permissions set to read");

// Closing the files after all operations
file.close();
filewrite.close();

// Demonstrating remove() method
// file.remove();
// console.log("File Removed");

// // Try to reopen the file after removal (should not work)
// try {
//     const fileAfterRemove = FileAccess.open("example.txt", FileAccess.READ);
//     console.log("File after removal: " + fileAfterRemove.getAsString());
// } catch (e) {
//     console.log("Error: File not found after removal.");
// }
