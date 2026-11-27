async function load() {
  const r = await fetch("/api/drugs");
  const drugs = await r.json();
  const ul = document.getElementById("list");
  ul.innerHTML = "";
  for (const d of drugs) {
    const li = document.createElement("li");
    li.textContent = `${d.name}: ${d.stock}`;   // textContent = safe, no escaping needed
    ul.appendChild(li);
  }
}
load();