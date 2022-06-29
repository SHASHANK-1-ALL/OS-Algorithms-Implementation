#include <iostream>
#include <fstream>
#include <thread>
#include <time.h>
#include <atomic>
#include <random>
#include <unistd.h>
using namespace std;

int n, k;
double lambda1, lambda2;
ofstream log_file("CAS-Bounded-Logfile.txt"); // Log file
atomic<int> workingLock(0);                   // lock variable
time_t totalWaitingTime = 0;
time_t worstCaseWaitingTime = 0;
atomic<bool> *waiting; // atomic waiting array

void testingCAS_Bounded(int);
string getTimeinReqdFormat(time_t);

int main()
{
    ifstream inp_file;
    inp_file.open("inp-params.txt");

    if (inp_file.is_open())
    {
        inp_file >> n >> k >> lambda1 >> lambda2;

        // create n threads
        thread new_thread[n];

        waiting = new atomic<bool>[n];
        for (int i = 0; i < n; i++)
        {
            waiting[i] = false;
        }

        // Opening log file and writing logistics of CAS-Bounded
        log_file << "CAS-Bounded ME Output:" << endl;
        for (int i = 0; i < n; i++)
        {
            new_thread[i] = thread(testingCAS_Bounded, i);
        }

        // Waiting for all threads to join
        for (int i = 0; i < n; i++)
        {
            new_thread[i].join();
        }

        if (log_file.is_open())
        {
            log_file.close();
        }
        else
        {
            cout << "Output file CAS-Bounded-Logfile.txt failed to open" << endl;
        }

        // Opening stat file and writing stats of CAS-Bounded
        ofstream stat_file;
        stat_file.open("CAS-Bounded-Statfile.txt");
        if (stat_file.is_open())
        {
            stat_file << "Average time taken by a process to enter the CS = " << (double)totalWaitingTime / (n * k) << "sec" << endl;
            stat_file << "Worst Case waiting time = " << (double)worstCaseWaitingTime << "sec" << endl;
        }
        else
        {
            cout << "Output file CAS-Bounded-Statfile.txt failed to open" << endl;
        }
        delete[] waiting;
        inp_file.close();
    }
    else
    {
        cout << "Input file inp-params.txt failed to open" << endl;
    }
    return 0;
}

// Function to get time in reqd format of hrs:min:sec
string getTimeinReqdFormat(time_t realTime)
{
    struct tm *realTimeInfo;
    realTimeInfo = localtime(&realTime);
    string formattedTime;
    formattedTime.append(to_string(realTimeInfo->tm_hour)).append(":").append(to_string(realTimeInfo->tm_min)).append(":").append(to_string(realTimeInfo->tm_sec));
    return formattedTime;
}

void testingCAS_Bounded(int thread_id)
{
    exponential_distribution<double> t1(1 / lambda1), t2(1 / lambda2); // Exponential distributions
    default_random_engine random_number_generator(time(NULL));         // Random number generator

    for (int i = 0; i < k; i++)
    {
        bool key = false;
        waiting[thread_id] = true;

        time_t requestedEntryTime = time(NULL);
        string reqEnterTime = getTimeinReqdFormat(requestedEntryTime);

        // entry section
        while (waiting[thread_id] && !key)
        {
            int x = 0, y = 1;
            key = workingLock.compare_exchange_strong(x, y);
        }
        waiting[thread_id] = false;

        log_file << i + 1 << "th CS Requested at " << reqEnterTime << " by thread " << thread_id << "\n";

        // critical section
        time_t actualEntryTime = time(NULL);
        string actEnterTime = getTimeinReqdFormat(actualEntryTime);
        log_file << i + 1 << "th CS Entered at " << actEnterTime << " by thread " << thread_id << endl;

        // Updating totalWaitingTime and worstCaseWaitingTime
        totalWaitingTime += (actualEntryTime - requestedEntryTime);
        worstCaseWaitingTime = max(worstCaseWaitingTime, actualEntryTime - requestedEntryTime);

        // Simulation of critical-section
        usleep((t1)(random_number_generator)*1e6);

        // Exit Section
        time_t exitingTime = time(NULL);
        string exitTime = getTimeinReqdFormat(exitingTime);
        log_file << i + 1 << "th CS Exited at " << exitTime << " by thread " << thread_id << endl;

        // selecting which thread should go in CS
        int j = (thread_id + 1) % n;
        while ((j != thread_id) && !waiting[j])
            j = (j + 1) % n;

        if (j == thread_id)
            workingLock = 0;
        else
            waiting[j] = false;

        // Simulation of Reminder Section
        usleep((t2)(random_number_generator)*1e6);
    }
}
