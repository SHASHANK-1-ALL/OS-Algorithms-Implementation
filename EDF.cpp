#include <iostream>
#include <string>
#include <fstream>
#include <algorithm>
#include <vector>
#include <queue>
using namespace std;

// General process info
class process_type_info
{
public:
    int pid;
    int burst_time;
    int period;
    int no_of_its;
    process_type_info(int process_pid, int process_time, int process_period, int process_its)
    {
        pid = process_pid;
        burst_time = process_time;
        period = process_period;
        no_of_its = process_its;
    }
};

// Particular process info
class process_instance
{
public:
    int pid;
    int period;
    int entry_time;
    int deadline;
    int runtime_left;
    bool preempted;
    int duration;
    process_instance(int id, int p, int entry, int dead, int time)
    {
        pid = id;
        period = p;
        entry_time = entry;
        deadline = dead;
        runtime_left = time;
        duration = time;
        preempted = false;
    }
};

// operator== defined for process_instance
bool operator==(const process_instance &p1, process_instance &p2)
{
    if (p1.deadline == p2.deadline && p1.entry_time == p2.entry_time && p1.period == p2.period && p1.pid == p2.pid && p1.preempted == p2.preempted && p1.runtime_left == p2.runtime_left)
        return true;
    return false;
}

// Priority given to period in cpu priority queue
struct GivePriority
{
    bool operator()(process_instance &p1, process_instance &p2)
    {
        return p1.deadline > p2.deadline;
    }
};

// Priority given to entry time in process_not_entered priority queue
struct GivePriorityProcessNotEntered
{
    bool operator()(process_instance &p1, process_instance &p2)
    {
        return p1.entry_time > p2.entry_time;
    }
};

int main()
{
    // Opening input file
    ifstream inp_file;
    inp_file.open("input.txt");

    // Taking input if file is opened
    if (inp_file.is_open())
    {
        int n, total_times_run = 0;
        string temp;
        if (inp_file)
        {
            getline(inp_file, temp);
            n = stoi(temp);
        }
        process_type_info *point = (process_type_info *)malloc(n * sizeof(process_type_info));
        for (int i = 0; i < n && inp_file; i++)
        {
            getline(inp_file, temp);
            string init;
            int count_param = 0;
            int pid, t, p, k;
            for (int j = 0; j <= temp.length(); j++)
            {
                if (temp[j] != ' ' && j != temp.length())
                    init.push_back(temp[j]);
                else
                {
                    switch (count_param)
                    {
                    case 0:
                        pid = stoi(init);
                        break;
                    case 1:
                        t = stoi(init);
                        break;
                    case 2:
                        p = stoi(init);
                        break;
                    case 3:
                        k = stoi(init);
                        break;
                    }
                    init.clear();
                    count_param++;
                }
            }
            total_times_run += k;
            point[i] = process_type_info(pid, t, p, k);
        }
        inp_file.close();

        // Creating priority queues for process_executor(cpu) and process_sender(process_not_entered)
        priority_queue<process_instance, vector<process_instance>, GivePriority> cpu;
        priority_queue<process_instance, vector<process_instance>, GivePriorityProcessNotEntered> process_not_entered;
        // Vector which stores total waiting time for each process
        vector<pair<int, int>> total_wait_time;

        // Opening output file and printing logistics to log file
        ofstream log_file;
        log_file.open("EDF-Log.txt");
        if (log_file.is_open())
        {
            // Printing info about process which joined the system at time 0
            for (int i = 0; i < n; i++)
            {
                log_file << "Process P" << point[i].pid << ": processing time=" << point[i].burst_time << "; deadline:" << point[i].period << "; period:" << point[i].period << " joined the system at time 0" << endl;
                total_wait_time.push_back({0, point[i].no_of_its});
                int ent = 0, dead = point[i].period;
                // Pushing particular instance of process in process sender
                for (int j = 0; j < point[i].no_of_its; j++)
                {
                    process_not_entered.push(process_instance(point[i].pid, point[i].period, ent, dead, point[i].burst_time));
                    ent += point[i].period;
                    dead += point[i].period;
                }
            }

            // Initializing time,no. of processes entered,no. of processes success,no. of processes miss to 0
            int time = 0, finish_time = 0, no_of_process_enter = 0, no_of_success = 0, no_of_miss = 0;
            // Initializing start to true
            bool start = true;
            while (!cpu.empty() || total_times_run > 0)
            {
                // Checking completion of process
                if (!cpu.empty())
                {
                    if (cpu.top().runtime_left == 0)
                    {
                        log_file << "Process P" << cpu.top().pid << " finishes execution at time " << time << endl;
                        total_wait_time[cpu.top().pid - 1].first += time - cpu.top().entry_time - cpu.top().duration;
                        no_of_success++;
                        total_times_run--;
                        cpu.pop();
                        finish_time = time;
                        start = true;
                    }
                }
                // deadline miss check
                if (!cpu.empty())
                {
                    process_instance top_dead = cpu.top();
                    vector<process_instance> dead, temp_dead;
                    while (!cpu.empty())
                    {
                        dead.push_back(cpu.top());
                        temp_dead.push_back(cpu.top());
                        cpu.pop();
                    }
                    for (int i = 0; i < dead.size(); i++)
                    {
                        if (dead[i].deadline == time && dead[i].runtime_left > 0)
                        {
                            if (top_dead == dead[i])
                                start = true;
                            log_file << "Process P" << dead[i].pid << " missed the deadline at time " << time << ". Remaining time was " << dead[i].runtime_left << endl;
                            total_wait_time[dead[i].pid - 1].first += time - dead[i].entry_time - (dead[i].duration - dead[i].runtime_left);
                            no_of_miss++;
                            total_times_run--;
                            temp_dead[i].duration = -1;
                            finish_time = time;
                        }
                    }
                    for (int i = 0; i < temp_dead.size(); i++)
                    {
                        if (temp_dead[i].duration != -1)
                            cpu.push(temp_dead[i]);
                    }
                    temp_dead.clear();
                    dead.clear();
                }
                // new process arrival and preemption
                if (!process_not_entered.empty())
                {
                    while (process_not_entered.top().entry_time == time)
                    {
                        process_instance new_process = process_not_entered.top();
                        if (!cpu.empty())
                        {
                            if (!(cpu.top() == new_process) && cpu.top().deadline > new_process.deadline && time != 0 && !start)
                            {
                                log_file << "Process P" << cpu.top().pid << " is preempted by process P" << new_process.pid << " at time " << time << ". Remaining processing time: " << cpu.top().runtime_left << endl;
                                process_instance temp = cpu.top();
                                temp.preempted = true;
                                cpu.pop();
                                cpu.push(temp);
                                start = true;
                                finish_time = time;
                            }
                        }
                        cpu.push(new_process);
                        no_of_process_enter++;
                        process_not_entered.pop();
                        if (process_not_entered.empty())
                            break;
                    }
                }
                // Check if preemption happened, starting new process
                if (!cpu.empty() && start)
                {
                    if (cpu.top().preempted == false)
                    {
                        if (finish_time != time)
                            log_file << "CPU is idle until time " << time << endl;
                        log_file << "Process P" << cpu.top().pid << " starts execution at time " << time << endl;
                        start = false;
                    }
                    else
                    {
                        log_file << "Process P" << cpu.top().pid << " resumes execution at time " << time << endl;
                        start = false;
                    }
                }
                // run the process
                if (!cpu.empty())
                {
                    process_instance running = cpu.top();
                    running.runtime_left--;
                    cpu.pop();
                    cpu.push(running);
                }
                time++;
            }
            // Printing the statistics to stat file
            ofstream stat_file;
            stat_file.open("EDF-Stats.txt");
            if (stat_file.is_open())
            {
                stat_file << "Number of processes that came into the system: " << no_of_process_enter << endl;
                stat_file << "Number of processes that successfully completed: " << no_of_success << endl;
                stat_file << "Number of processes that missed their deadlines: " << no_of_miss << endl;
                int total_time = 0;
                int times_run = 0;
                for (int i = 0; i < total_wait_time.size(); i++)
                {
                    stat_file << "Average waiting time of process P" << i + 1 << ": " << (double)total_wait_time[i].first / (total_wait_time[i].second * 1.0) << endl;
                    total_time += total_wait_time[i].first;
                    times_run += total_wait_time[i].second;
                }
                stat_file << "Average waiting time " << (double)total_time / (times_run * 1.0) << endl;
                stat_file.close();
                log_file.close();
            }
            else
            {
                cout << "Stat file failed to open" << endl;
            }
        }
        else
        {
            cout << "Log file failed to open" << endl;
        }
        // Freeing memory
        free(point);
    }
    else
        cout << "Input file failed to open" << endl;
    return 0;
}