#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <fstream>
#include <algorithm>
#include <stdexcept>

class Task {
private:
    std::string description;
    bool completed;
    int id;

public:
    Task(int taskId, const std::string& desc) 
        : id(taskId), description(desc), completed(false) {}
    
    // Rule of Zero - use default operations since we use std::string
    Task(const Task&) = default;
    Task(Task&&) = default;
    Task& operator=(const Task&) = default;
    Task& operator=(Task&&) = default;
    ~Task() = default;

    void toggleComplete() { completed = !completed; }
    bool isCompleted() const { return completed; }
    int getId() const { return id; }
    const std::string& getDescription() const { return description; }
    
    void setDescription(const std::string& desc) { description = desc; }
};

//File Handler for persistent storage
class FileHandler {
private:
    std::string filename;
    
public:
    explicit FileHandler(const std::string& fname) : filename(fname) {}
    
    // Delete copy operations to prevent multiple file ownership
    FileHandler(const FileHandler&) = delete;
    FileHandler& operator=(const FileHandler&) = delete;
    
    std::vector<std::unique_ptr<Task>> loadTasks() {
        std::vector<std::unique_ptr<Task>> tasks;
        std::ifstream file(filename);
        
        if (!file.is_open()) {
            return tasks; // Return empty vector if file doesn't exist
        }
        
        int id, completed;
        std::string desc;
        
        while (file >> id >> completed) {
            file.ignore(); // Skip the space after completed flag
            std::getline(file, desc);
            auto task = std::make_unique<Task>(id, desc);
            if (completed) {
                task->toggleComplete();
            }
            tasks.push_back(std::move(task));
        }
        
        return tasks;
    }
    
    void saveTasks(const std::vector<std::unique_ptr<Task>>& tasks) {
        std::ofstream file(filename);
        
        if (!file.is_open()) {
            throw std::runtime_error("Unable to open file for writing");
        }
        
        for (const auto& task : tasks) {
            file << task->getId() << " " 
                 << task->isCompleted() << " " 
                 << task->getDescription() << "\n";
        }
    }
};

// Checklist Manager
class ChecklistManager {
private:
    std::vector<std::unique_ptr<Task>> tasks;
    std::unique_ptr<FileHandler> fileHandler;
    int nextId;
    
    int getNextId() {
        int maxId = 0;
        for (const auto& task : tasks) {
            maxId = std::max(maxId, task->getId());
        }
        return maxId + 1;
    }

public:
    explicit ChecklistManager(const std::string& filename) 
        : fileHandler(std::make_unique<FileHandler>(filename)), nextId(1) {
        tasks = fileHandler->loadTasks();
        nextId = getNextId();
    }
    
    // Rule of Five - prevent copying but allow moving
    ChecklistManager(const ChecklistManager&) = delete;
    ChecklistManager& operator=(const ChecklistManager&) = delete;
    ChecklistManager(ChecklistManager&&) = default;
    ChecklistManager& operator=(ChecklistManager&&) = default;
    
    ~ChecklistManager() {
        try {
            fileHandler->saveTasks(tasks);
        } catch (const std::exception& e) {
            std::cerr << "Error saving tasks: " << e.what() << std::endl;
        }
    }
    
    void addTask(const std::string& description) {
        tasks.push_back(std::make_unique<Task>(nextId++, description));
        std::cout << "Task added successfully!\n";
    }
    
    void removeTask(int id) {
        auto it = std::remove_if(tasks.begin(), tasks.end(),
            [id](const std::unique_ptr<Task>& task) {
                return task->getId() == id;
            });
        
        if (it != tasks.end()) {
            tasks.erase(it, tasks.end());
            std::cout << "Task removed successfully!\n";
        } else {
            std::cout << "Task not found!\n";
        }
    }
    
    void toggleTask(int id) {
        auto it = std::find_if(tasks.begin(), tasks.end(),
            [id](const std::unique_ptr<Task>& task) {
                return task->getId() == id;
            });
        
        if (it != tasks.end()) {
            (*it)->toggleComplete();
            std::cout << "Task status toggled!\n";
        } else {
            std::cout << "Task not found!\n";
        }
    }
    
    void listTasks() const {
        if (tasks.empty()) {
            std::cout << "\nNo tasks in the checklist.\n";
            return;
        }
        
        std::cout << "\n=== CHECKLIST ===\n";
        for (const auto& task : tasks) {
            std::cout << "[" << task->getId() << "] "
                     << (task->isCompleted() ? "[X] " : "[ ] ")
                     << task->getDescription() << "\n";
        }
        std::cout << "=================\n";
    }
};

// menu Handler
class Menu {
private:
    std::unique_ptr<ChecklistManager> manager;
    
    void displayMenu() const {
        std::cout << "\n--- Checklist Manager ---\n";
        std::cout << "1. Add Task\n";
        std::cout << "2. Remove Task\n";
        std::cout << "3. Toggle Task\n";
        std::cout << "4. List Tasks\n";
        std::cout << "5. Exit\n";
        std::cout << "Choice: ";
    }
    
public:
    explicit Menu(const std::string& filename) 
        : manager(std::make_unique<ChecklistManager>(filename)) {}
    
    void run() {
        int choice;
        std::string input;
        
        while (true) {
            displayMenu();
            std::cin >> choice;
            std::cin.ignore();
            
            switch (choice) {
                case 1: {
                    std::cout << "Enter task description: ";
                    std::getline(std::cin, input);
                    manager->addTask(input);
                    break;
                }
                case 2: {
                    int id;
                    std::cout << "Enter task ID to remove: ";
                    std::cin >> id;
                    manager->removeTask(id);
                    break;
                }
                case 3: {
                    int id;
                    std::cout << "Enter task ID to toggle: ";
                    std::cin >> id;
                    manager->toggleTask(id);
                    break;
                }
                case 4:
                    manager->listTasks();
                    break;
                case 5:
                    std::cout << "Saving and exiting...\n";
                    return;
                default:
                    std::cout << "Invalid choice!\n";
            }
        }
    }
};

int main() {
    try {
        Menu menu("checklist.txt");
        menu.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}