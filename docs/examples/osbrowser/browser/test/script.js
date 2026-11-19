let clicks = 0;
console.log("script started");

function onBoxClick() {
  clicks = clicks + 1;
  document.getElementById("status").textContent = "Clicks: " + clicks;
  let box = document.getElementById("box");
  if (clicks > 2) {
    box.style.backgroundColor = "#ffe9a8";
    box.textContent = "You clicked " + clicks + " times!";
  }
}

document.getElementById("box").addEventListener("click", onBoxClick);
document.getElementById("title").textContent = "Hello from JS";
