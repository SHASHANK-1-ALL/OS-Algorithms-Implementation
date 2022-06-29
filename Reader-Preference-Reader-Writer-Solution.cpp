#include <iostream>
#include <fstream>
#include <thread>
#include <random>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <time.h>
#include <unistd.h>
#include <semaphore.h>
using namespace std;
using namespace chrono;

int nw, nr, kw, kr;                     // input variables
double mu_cs, mu_rem;                   // input variables
ofstream log_file("RW-log.txt");        // log file to print logistics
double totalWaitingTime_reader = 0;     // total wait time for reader
double worstCaseWaitingTime_reader = 0; // worst case wait time for reader
double totalWaitingTime_writer = 0;     // total wait time for writer
double worstCaseWaitingTime_writer = 0; // worst case wait time for writer
sem_t rw_mutex;                         // controls access (read/write) to the resource
sem_t mutex;                            // for syncing changes to shared variable read_count
int read_count = 0;                     // number of readers currently accessing resource

string getCurrentTimestamp(time_point<system_clock> &currentTime);
void rw_writer(int wtid);
void rw_reader(int rtid);

int main()
{
    // Taking input from input file
    ifstream inp_file("inp-params.txt");
    if (inp_file.is_open())
    {
        inp_file >> nw >> nr >> kw >> kr >> mu_cs >> mu_rem;
        mu_cs *= (double)pow(10, -3);
        mu_rem *= (double)pow(10, -3);

        // create nw writer threads and nr reader threads
        thread writer_thread[nw], reader_thread[nr];

        // Initializing all semaphores to 1
        sem_init(&rw_mutex, 0, 1);
        sem_init(&mutex, 0, 1);

        // Starting the process
        for (int i = 0; i < nw; i++)
        {
            writer_thread[i] = thread(rw_writer, i);
        }
        for (int i = 0; i < nr; i++)
        {
            reader_thread[i] = thread(rw_reader, i);
        }

        // Waiting for all threads to join
        for (int i = 0; i < nw; i++)
        {
            writer_thread[i].join();
        }
        for (int i = 0; i < nr; i++)
        {
            reader_thread[i].join();
        }

        // Destroying the semaphores created
        sem_destroy(&mutex);
        sem_destroy(&rw_mutex);

        // Closing the logfile if open
        if (log_file.is_open())
        {
            log_file.close();
        }
        else
        {
            cout << "Output file RW-log.txt failed to open" << endl;
        }

        // Opening stat file and writing stats
        ofstream stat_file("RW-Avg_time.txt");
        if (stat_file.is_open())
        {
            stat_file << "Average time taken by a writer thread to enter the CS = " << fixed << setprecision(2) << (double)(totalWaitingTime_writer / (nw * kw)) * 1000 << " millisec" << endl;
            stat_file << "Worst Case waiting time for a writer thread = " << fixed << setprecision(2) << (worstCaseWaitingTime_writer)*1000 << " millisec" << endl;
            stat_file << "Average time taken by a reader thread to enter the CS = " << fixed << setprecision(2) << (double)(totalWaitingTime_reader / (nr * kr)) * 1000 << " millisec" << endl;
            stat_file << "Worst Case waiting time for a reader thread = " << fixed << setprecision(2) << (worstCaseWaitingTime_reader)*1000 << " millisec" << endl;
            stat_file.close();
        }
        else
        {
            cout << "Output file RW-Avg_time.txt failed to open" << endl;
        }
        inp_file.close();
    }
    else
    {
        cerr << "inp-params.txt failed to open!!!" << endl;
    }
    return 0;
}

// Function to print time in hrs:mins:sec:millisec format
string getCurrentTimestamp(time_point<system_clock> &currentTime)
{
    string temp;
    char buffer[1000];

    auto transformed = currentTime.time_since_epoch().count() / 1000000;

    auto milli = transformed % 1000;

    time_t tt = system_clock::to_time_t(currentTime);
    auto timeinfo = localtime(&tt);

    strftime(buffer, 1000, "%H:%M:%S", timeinfo);
    temp = buffer;
    temp.append(":").append((to_string((int)milli)));

    return temp;
}

// Writer
void rw_writer(int wtid)
{
    exponential_distribution<double> RandCSTime(1 / mu_cs), RandRemTime(1 / mu_rem); // Exponential distributions
    default_random_engine random_number_generator(time(NULL));                       // Random number generator
    for (int i = 0; i < kw; i++)
    {
        time_point<system_clock> requestedEntryTime = system_clock::now();
        string reqEnterTime = getCurrentTimestamp(requestedEntryTime);

        // entry section
        sem_wait(&rw_mutex);

        log_file << i + 1 << "th CS Request by Writer thread " << wtid << " at " << reqEnterTime << endl;

        // critical section
        time_point<system_clock> actualEntryTime = system_clock::now();
        string actEnterTime = getCurrentTimestamp(actualEntryTime);
        log_file << i + 1 << "th CS Entry by Writer thread " << wtid << " at " << actEnterTime << endl;

        /* Writing is performed */

        // Updating totalWaitingTime and worstCaseWaitingTime
        duration<double> totwait_write = (actualEntryTime - requestedEntryTime);
        totalWaitingTime_writer += totwait_write.count();
        worstCaseWaitingTime_writer = max(worstCaseWaitingTime_writer, totwait_write.count());

        // Simulation of critical-section
        usleep((RandCSTime)(random_number_generator)*1e6);

        // Exit Section
        time_point<system_clock> exitingTime = system_clock::now();
        string exitTime = getCurrentTimestamp(exitingTime);

        log_file << i + 1 << "th CS Exit by Writer thread " << wtid << " at " << exitTime << endl;

        sem_post(&rw_mutex);

        // Simulation of Reminder Section
        usleep((RandRemTime)(random_number_generator)*1e6);
    }
}

// Reader
void rw_reader(int rtid)
{
    exponential_distribution<double> RandCSTime(1 / mu_cs), RandRemTime(1 / mu_rem); // Exponential distributions
    default_random_engine random_number_generator(time(NULL));
    ; // Random number generator
    for (int i = 0; i < kr; i++)
    {
        time_point<system_clock> requestedEntryTime = system_clock::now();
        string reqEnterTime = getCurrentTimestamp(requestedEntryTime);

        // entry section
        sem_wait(&mutex);
        read_count++;
        if (read_count == 1)
            sem_wait(&rw_mutex);
        log_file << i + 1 << "th CS Request by Reader thread " << rtid << " at " << reqEnterTime << endl;
        time_point<system_clock> actualEntryTime = system_clock::now();
        string actEnterTime = getCurrentTimestamp(actualEntryTime);
        log_file << i + 1 << "th CS Entry by Reader thread " << rtid << " at " << actEnterTime << endl;
        sem_post(&mutex);

        // critical section
        /* Reading is performed */

        // Updating totalWaitingTime and worstCaseWaitingTime
        duration<double> totwait_read = (actualEntryTime - requestedEntryTime);
        totalWaitingTime_reader += totwait_read.count();
        worstCaseWaitingTime_reader = max(worstCaseWaitingTime_reader, totwait_read.count());

        // Simulation of critical-section
        usleep((RandCSTime)(random_number_generator)*1e6);

        time_point<system_clock> exitingTime = system_clock::now();
        string exitTime = getCurrentTimestamp(exitingTime);

        // Exit Section
        sem_wait(&mutex);
        log_file << i + 1 << "th CS Exit by Reader thread " << rtid << " at " << exitTime << endl;
        read_count--;
        if (read_count == 0)
            sem_post(&rw_mutex);
        sem_post(&mutex);

        // Simulation of Reminder Section
        usleep((RandRemTime)(random_number_generator)*1e6);
    }
}