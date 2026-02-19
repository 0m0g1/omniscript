function brainfuckInterpreter(code, input) {
    var memory = [].fill(0, 50); // Initialize memory
 

    var pointer = 0; // Memory pointer
    var output = "";
    var inputPointer = 0;

    var i = 0; // Code pointer

    while (i < code.length) {
        var command = code[i];

        if (command == '>') {
            pointer++;
        } else if (command == '<') {
            pointer--;
        } else if (command == '+') {
            memory[pointer] = (memory[pointer] + 1) % 256; // Wrap around
        } else if (command == '-') {
            memory[pointer] = (memory[pointer] - 1 + 256) % 256; // Wrap around
        } else if (command == '.') {
            output += "".fromCharCode(memory[pointer]);
        } else if (command == ',') {
            memory[pointer] = inputPointer < input.length
                ? input.charCodeAt(inputPointer++)
                : 0;
        } else if (command == '[') {
            if (memory[pointer] == 0) {
                // Jump forward to matching ']'
                var openBrackets = 1;
                while (openBrackets > 0) {
                    i++;
                    if (code[i] == '[') openBrackets++;
                    else if (code[i] == ']') openBrackets--;
                }
            }
        } else if (command == ']') {
            if (memory[pointer] != 0) {
                // Jump back to matching '['
                var closeBrackets = 1;
                while (closeBrackets > 0) {
                    i--;
                    if (code[i] == '[') closeBrackets--;
                    else if (code[i] == ']') closeBrackets++;
                }
            }
        }

        i++; // Move to the next command
    }

    return output;
}

// Example usage
// var brainfuckCode = "++++++++[>++++++++<-]>++++.+.";
var brainfuckCode = ">+++++++++[<++++++++>-]<.>+++++++[<++++>-]<+.+++++++..+++.[-]>++++++++[<++++>-] <.>+++++++++++[<++++++++>-]<-.--------.+++.------.--------.[-]>++++++++[<++++>- ]<+.[-]++++++++++.";
var result = brainfuckInterpreter(brainfuckCode, "");
console.log(result); // Should print "HI"