// Task Scheduler with delays, thread safe.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <functional>
#include <vector>

class TaskScheduler {
public:
    TaskScheduler() : stop(false) {}

    ~TaskScheduler() {
        stopScheduler();
    }

    void startScheduler() {
        schedulerThread = std::thread(&TaskScheduler::runScheduler, this);
    }

    void stopScheduler() {
        {
            std::unique_lock<std::mutex> lock(mutexScheduler);
            stop = true;
        }
        conditionScheduler.notify_all();

        if (schedulerThread.joinable()) {
            schedulerThread.join();
        }
    }

    void addTask(std::function<void()> task, std::chrono::milliseconds interval) {
        std::unique_lock<std::mutex> lock(mutexTasks);
        tasks.emplace_back(task, interval);
        conditionTasks.notify_one();
    }

private:
    void runScheduler() {
        while (true) {
            std::unique_lock<std::mutex> lock(mutexScheduler);
            conditionScheduler.wait(lock, [this]() { return stop || !tasks.empty(); });

            if (stop) {
                break;
            }

            auto currentTime = std::chrono::steady_clock::now();

            {
                std::unique_lock<std::mutex> lockTasks(mutexTasks);

                for (auto& task : tasks) {
                    if (currentTime - task.lastExecution >= task.interval) {
                        task.lastExecution = currentTime;
                        std::thread(task.task).detach();
                    }
                }

                tasks.erase(std::remove_if(tasks.begin(), tasks.end(),
                    [](const TaskInfo& task) {
                        return task.interval == std::chrono::milliseconds(0);
                    }),
                    tasks.end());
            }

            conditionTasks.notify_one();
        }
    }

private:
    struct TaskInfo {
        std::function<void()> task;
        std::chrono::milliseconds interval;
        std::chrono::steady_clock::time_point lastExecution;

        TaskInfo(std::function<void()> t, std::chrono::milliseconds i)
            : task(std::move(t)), interval(i), lastExecution(std::chrono::steady_clock::now()) {}
    };

    std::thread schedulerThread;
    std::mutex mutexScheduler;
    std::condition_variable conditionScheduler;
    std::mutex mutexTasks;
    std::condition_variable conditionTasks;
    std::vector<TaskInfo> tasks;
    bool stop;
};

int main() {
    TaskScheduler scheduler;

    // Start the scheduler
    scheduler.startScheduler();

    // Add periodic tasks
    scheduler.addTask([]() { std::cout << "Task 1 executed" << std::endl; }, std::chrono::seconds(1));
    scheduler.addTask([]() { std::cout << "Task 2 executed" << std::endl; }, std::chrono::seconds(2));

    // Keep the program running for some time
    std::this_thread::sleep_for(std::chrono::seconds(10));

    // Stop the scheduler
    scheduler.stopScheduler();

    return 0;
}


// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
