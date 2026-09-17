(function () {
  "use strict";

  // Same-origin by default (nginx proxies /api to the backend). Override with
  // window.TASKBOARD_API_BASE when serving the frontend from elsewhere.
  const API_BASE = window.TASKBOARD_API_BASE || "/api";
  const STATUSES = ["todo", "in_progress", "done"];

  const form = document.getElementById("task-form");
  const titleInput = document.getElementById("title");
  const descInput = document.getElementById("description");
  const errorBox = document.getElementById("error");
  const healthDot = document.getElementById("health");
  const template = document.getElementById("task-template");

  async function api(path, options = {}) {
    const res = await fetch(API_BASE + path, {
      headers: { "Content-Type": "application/json" },
      ...options,
    });
    if (res.status === 204) return null;
    const body = await res.json().catch(() => ({}));
    if (!res.ok) throw new Error(body.error || `HTTP ${res.status}`);
    return body;
  }

  function showError(message) {
    errorBox.textContent = message;
    errorBox.hidden = !message;
  }

  function render(tasks) {
    for (const status of STATUSES) {
      document.getElementById(`col-${status}`).replaceChildren();
    }
    for (const task of tasks) {
      const node = template.content.firstElementChild.cloneNode(true);
      node.dataset.id = task.id;
      node.querySelector(".task-title").textContent = task.title;
      node.querySelector(".task-desc").textContent = task.description;
      node.querySelector(".task-time").textContent = new Date(task.created_at).toLocaleString();

      const index = STATUSES.indexOf(task.status);
      const prev = node.querySelector(".btn-prev");
      const next = node.querySelector(".btn-next");
      prev.disabled = index === 0;
      next.disabled = index === STATUSES.length - 1;
      prev.addEventListener("click", () => move(task.id, STATUSES[index - 1]));
      next.addEventListener("click", () => move(task.id, STATUSES[index + 1]));
      node.querySelector(".btn-delete").addEventListener("click", () => remove(task.id));

      document.getElementById(`col-${task.status}`).appendChild(node);
    }
  }

  function renderStats(stats) {
    for (const key of ["total", ...STATUSES]) {
      document.getElementById(`stat-${key}`).textContent = stats[key] ?? 0;
    }
  }

  async function refresh() {
    try {
      const [tasks, stats] = await Promise.all([api("/tasks"), api("/stats")]);
      render(tasks);
      renderStats(stats);
      showError("");
    } catch (err) {
      showError(err.message);
    }
  }

  async function move(id, status) {
    try {
      await api(`/tasks/${id}`, { method: "PUT", body: JSON.stringify({ status }) });
      await refresh();
    } catch (err) {
      showError(err.message);
    }
  }

  async function remove(id) {
    try {
      await api(`/tasks/${id}`, { method: "DELETE" });
      await refresh();
    } catch (err) {
      showError(err.message);
    }
  }

  async function checkHealth() {
    try {
      await api("/health");
      healthDot.className = "health up";
      healthDot.title = "Backend: online";
    } catch {
      healthDot.className = "health down";
      healthDot.title = "Backend: offline";
    }
  }

  form.addEventListener("submit", async (event) => {
    event.preventDefault();
    const title = titleInput.value.trim();
    if (!title) return;
    try {
      await api("/tasks", {
        method: "POST",
        body: JSON.stringify({ title, description: descInput.value.trim() }),
      });
      form.reset();
      titleInput.focus();
      await refresh();
    } catch (err) {
      showError(err.message);
    }
  });

  checkHealth();
  refresh();
  setInterval(checkHealth, 15000);
})();
