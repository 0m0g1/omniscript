// // OmniScript String Behavior Examples
// // Demonstrating different string types and their newline handling

extern "C" fn printf(...fmt: char*) => int;

// let text: char* = "hello world";
printf("%s\n", "this is a really really long text which becomes unreadable because it has a lot of words. 
                Using a normal string it will be formated into one line.");

// let array = ['h', 'i', '\0'];
// // printf("%s\n", &array);

// printf("%c\n", 'a');
// printf("%s\n", 'hello world');
// printf("%s\n", "hello 
//     world");
// printf("%s\n", r"hello world");
// printf("%s\n", `hello world`);

// extern "C" fn printf(...fmt: char*) => int;
// extern "C" fn sprintf(buffer: char*, ...fmt: char*) => int;

// // =============================================================================
// // 1. REGULAR STRINGS ("...") - Remove newlines & whitespace
// // =============================================================================

// // Long message without line breaks in output
// let longMessage = "This is a very long message that would be hard to read 
//                    if it were all on one line, but we don't want actual 
//                    line breaks in the final string.";
// printf("Regular string result: %s\n\n", longMessage);

// // SQL query building - clean single line
// let sqlQuery = "SELECT users.name, users.email, profiles.bio 
//                 FROM users 
//                 JOIN profiles ON users.id = profiles.user_id 
//                 WHERE users.active = true 
//                 ORDER BY users.created_at DESC";
// printf("SQL Query: %s\n\n", sqlQuery);

// // HTML generation without extra whitespace
// let htmlSnippet = "<div class='container'>
//                    <h1>Welcome</h1>
//                    <p>This is a paragraph</p>
//                    </div>";
// printf("HTML: %s\n\n", htmlSnippet);

// // =============================================================================
// // 2. TEMPLATE STRINGS (`...`) - Keep newlines, skip leading whitespace
// // =============================================================================

// // Multi-line string with preserved formatting and clean indentation
// let emailTemplate = `Dear Customer,

//     Thank you for your order! Here are your items:
//     - Widget A
//     - Widget B

//     Best regards,
//     The Team`;
// printf("Email template:\n%s\n\n", emailTemplate);

// // Code template with proper indentation
// let classTemplate = `class MyClass {
//     constructor() {
//         // Initialize
//     }
    
//     greet() {
//         return "Hello World";
//     }
// }`;
// printf("Generated code:\n%s\n\n", classTemplate);

// // Configuration file with clean formatting
// let configTemplate = `server:
//     host: localhost
//     port: 8080
    
// database:
//     url: postgresql://localhost:5432/mydb
//     pool_size: 10`;
// printf("Config:\n%s\n\n", configTemplate);

// // =============================================================================
// // 3. RAW STRINGS (r"...") - Preserve everything exactly
// // =============================================================================

// // Regular expressions without double escaping
// let phoneRegex = r"^\d{3}-\d{2}-\d{4}$";
// let pathRegex = r"C:\Users\[^\\]+\\Documents";
// printf("Phone regex: %s\n\n", phoneRegex);
// printf("Path regex: %s\n\n", pathRegex);

// // File paths without escape issues
// let windowsPath = r"C:\Program Files\MyApp\config.ini";
// let networkPath = r"\\server\share\folder\file.txt";
// printf("Windows path: %s\n\n", windowsPath);
// printf("Network path: %s\n\n", networkPath);

// // ASCII art with exact formatting
// let asciiArt = r`
//     /\_/\  
//    ( o.o ) 
//     > ^ <
// `;
// printf("ASCII Art:%s\n\n", asciiArt);

// // JSON template with exact formatting
// let jsonTemplate = r`{
//     "name": "${name}",
//     "value": ${value},
//     "timestamp": "${timestamp}"
// }`;
// printf("JSON Template:%s\n\n", jsonTemplate);

// // Raw template without expressions - just preserves formatting
// let rawTemplate = `Config File:
//     Server: localhost
//     Port: 8080
    
//     # This is a comment with \special \characters
//     Path: C:\Program Files\App\\`;
// printf("Raw template:\n%s\n\n", rawTemplate);

// // =============================================================================
// // 4. CHARACTER LITERALS ('x') - Single character validation
// // =============================================================================

// // State machine tokens
// let stateA = 'A';
// let stateB = 'B';
// let delimiter = ',';
// let quote = '"';

// printf("State A: %c\n\n", stateA);
// printf("State B: %c\n\n", stateB);
// printf("Delimiter: %c\n\n", delimiter);
// printf("Quote char: %c\n\n", quote);

// // Control characters
// let tab = '\t';
// let newline = '\n';
// let escape = '\x1B';  // ESC character
// let unicode = '\u2603';  // Snowman ☃

// printf("Tab char: [%c]\n\n", tab);
// printf("Newline char: [%c]\n\n", newline);
// // printf("Escape char: [%c]\n", escape);
// printf("Unicode char: [%c]\n\n", unicode);

// // =============================================================================
// // 5. TEMPLATE STRINGS FOR FORMATTING - Multi-line content with clean indentation
// // =============================================================================

// // Simple message formatting
// let welcomeMessage = `Welcome to our application!
    
//     Please read the documentation
//     and follow the setup instructions.`;
// printf("Welcome message:\n%s\n\n", welcomeMessage);

// // Report template with static content
// let reportTemplate = `Sales Report for January
    
//     Total Revenue: $15,432.50
//     Units Sold: 1,247
    
//     Top Product:
//     Widget Pro: 342 units`;
// printf("Sales Report:\n%s\n\n", reportTemplate);

// // =============================================================================
// // 6. MIXED USAGE EXAMPLES
// // =============================================================================

// // HTML template with static content
// let htmlTemplate = `<!DOCTYPE html>
// <html>
// <head>
//     <title>Page Title</title>
// </head>
// <body>
//     <h1>Welcome</h1>
//     <p>This is static content</p>
// </body>
// </html>`;
// printf("HTML Template:\n%s\n", htmlTemplate);

// // Configuration with mixed string types
// let configFile = `# Configuration File
// database_url = postgresql://user:pass@localhost:5432/db
// log_pattern = ^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}
// welcome_message = Welcome to our application!
//                   Please read the documentation.`;
// printf("Config File:\n%s\n", configFile);

// // =============================================================================
// // 7. PRACTICAL EXAMPLES
// // =============================================================================

// // File path handling
// fn processFile(filename: char*) => void {
//     let basePath = r"C:\Users\Documents\";
//     let logMessage = `Processing file from directory
//                       C:\\Users\Documents\
//                       Please wait...`;
    
//     printf("Base path: %s\n\n", basePath);
//     printf("Log: %s\n\n", logMessage);
// }

// processFile("data.txt");

// // Command template
// let commandTemplate = `converter --input=input.dat --output=result.txt --verbose`;
// printf("Command: %s\n\n", commandTemplate);

// // Multi-language string handling
// let greeting_en = "Hello World\n";
// let greeting_jp = "こんにちは世界\n";
// let greeting_emoji = "👋 🌍\n";

// printf("English: %s\n", greeting_en);
// printf("Japanese: %s\n", greeting_jp);
// printf("Emoji: %s\n", greeting_emoji);

// // // Raw string for regex with Unicode
// let unicodeRegex = r"[\u0080-\uFFFF]+";
// printf("Unicode regex: %s\n\n", unicodeRegex);