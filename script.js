document.getElementById('addTaskBtn').addEventListener('click', async () => {
    const taskInput = document.getElementById('taskInput');
    const description = taskInput.value.trim();
    if (description) {
        await fetch('/tasks/add', {
            method: 'POST',
            headers: { 'Content-Type': 'text/plain' },
            body: description
        });
        taskInput.value = '';
        loadTasks();
    }
});

async function loadTasks() {
    const response = await fetch('/tasks');
    const tasks = await response.json();
    const taskList = document.getElementById('taskList');
    taskList.innerHTML = '';
    tasks.forEach(task => {
        const li = document.createElement('li');
        li.innerHTML = `
            <span>${task.description}</span>
            <button onclick="deleteTask(${task.id})">Удалить</button>
        `;
        taskList.appendChild(li);
    });
}

async function deleteTask(id) {
    await fetch(`/tasks/delete/${id}`, { method: 'DELETE' });
    loadTasks();
}

loadTasks();