// Call methods
const now = Time.now();
console.log("Current timestamp: " + now.toString());

const formatted = Time.format(now, "%Y-%m-%d %H:%M:%S");
console.log("Formatted time: " + formatted.toString());

console.log("Sleeping for 2 seconds...");
Time.sleep(2000);
console.log("Woke up!");
