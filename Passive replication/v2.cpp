#include <bits/stdc++.h>

using namespace std;

struct Task{
    int tid;
    int replica_num;
    double exe_time;
    int period;
    double deadline;
    double reliability;
    double freq;
    int replicas;
    double energy;
    double cpu_time;
    double arrival_time;
    double activation_time;

    Task(int id=0, double et=0,double p=0,double d=0,double r=0){
        tid = id;
        exe_time = et;
        period = p;
        deadline = d;
        reliability=r;
    }
};

struct Schedule
{
    double start_time;
    double finish_time;
    int task_num;
    int replica_num;

    Schedule(double st=0, double ft=0, int tn=0, int rn=0){
        start_time = st;
        finish_time = ft;
        task_num = tn;
        replica_num = rn;
    }
};

struct activation_time_comp{
bool operator()(const Task &a,const Task &b)
{
    return a.activation_time > b.activation_time;
}
};

struct deadline_comp{
bool operator()(const Task &a,const Task &b)
{
    double a_absolute_deadline = a.arrival_time + a.deadline;
    double b_absolute_deadline = b.arrival_time + b.deadline;
    if(a_absolute_deadline == b_absolute_deadline)
        return a.activation_time > b.activation_time;
    return a_absolute_deadline > b_absolute_deadline;
}
};

// n tasks : 0,1,2,...,n-1
// m processors : 0,1,2,...,m-1
// F : number of available frequency levels of the processors
// task : list of tasks given as input
// freq_list : list of the available normalized frequencies where size(freq_list)=F
// hyperperiod : lcm of all the task periods
// schedule[k] will store the schedule of the k'th processor
// allocation[i][j] will store the processor assigned to j'th replica of task i
// task_to_processors[i] will store the set of processors already allocated to different replicas of task[i]

int n,m,F;
vector<Task> task;
vector<double> freq_list;
int hyperperiod;
vector<vector<Schedule>> schedule;
int allocation[101][10];
priority_queue<Task,vector<Task>,activation_time_comp> global_queue;
vector<priority_queue<Task,vector<Task>,deadline_comp>> ready_queue;
double global_time;
vector<double> last_idle_time;
double min_energy;
vector<unordered_set<int>> task_to_processors;


// compute the hyperperiod
void find_hyperperiod()
{
    int lcm = task[0].period;
    int p;
    for(int i=1;i<n;i++)
    {
        p = task[i].period;
        lcm = (lcm*p) / __gcd(lcm, p);
    }
    hyperperiod = lcm;
}

// try to schedule all tasks up to task 'task_num' and its replica 'replica_num' using preemptive earliest deadline first algorithm
bool schedule_edf(int task_num, int replica_num)
{
    // ready_queue[k] will store all the tasks allocated to the k'th processor
    ready_queue.clear();
    ready_queue.resize(m);
    last_idle_time.clear();
    last_idle_time.resize(m, 0);
    schedule.clear();
    schedule.resize(m);

    // put the zeroth replica of each task (primary copy) into the global queue
    for(int i=0;i<=task_num;i++)
    {
        double cur_time = 0;
        while(cur_time < hyperperiod)
        {
            Task temp = task[i];
            temp.arrival_time = cur_time;
            temp.activation_time = cur_time;
            temp.replica_num = 0;
            global_queue.push(temp);
            cur_time+= temp.period;
        }
    }

    while(!global_queue.empty())
    {
        global_time = global_queue.top().activation_time;
        // pop all tasks from the global queue upto the current time and push into the ready queue of their respective allocated processor
        while(!global_queue.empty() && global_queue.top().activation_time <= global_time)
        {
            Task temp = global_queue.top();
            global_queue.pop();
            int tn = temp.tid;
            int rn = temp.replica_num;
            int alloc = allocation[tn][rn];
            ready_queue[alloc].push(temp);
        }

        // schedule a task from the ready queue of each processor if that processor is idle
        for(int k=0;k<m;k++)
        {
            // processor not free at current global time
            if(global_time < last_idle_time[k])
                continue;

            // schedule the task with the least deadline
            if(!ready_queue[k].empty())
            {
                Task temp = ready_queue[k].top();
                ready_queue[k].pop();
                int tn = temp.tid;
                int rn = temp.replica_num;
                double finish_time = global_time + temp.cpu_time;
                if(finish_time <= temp.arrival_time + temp.deadline)
                {
                    Schedule sch = Schedule(global_time, finish_time, tn, rn);
                    schedule[k].push_back(sch);
                    last_idle_time[k] = finish_time;

                    // push the next replica (backup copy) if any into the global queue
                    if(rn != temp.replicas-1 && rn+1 <= replica_num)
                    {
                        Task next_replica = temp;
                        next_replica.activation_time = finish_time;
                        next_replica.replica_num = rn + 1;
                        global_queue.push(next_replica);
                    }
                }
                else
                {
                    return false;
                }

            }

        }
    }

    // Now the global queue is empty
    // Schedule the remaining tasks present in the ready queue of each processor

    for(int k=0;k<m;k++)
    {
        while(!ready_queue[k].empty())
        {
            double cur_time = last_idle_time[k];
            Task temp = ready_queue[k].top();
            ready_queue[k].pop();
            int task_num = temp.tid;
            int replica_num = temp.replica_num;
            double finish_time = cur_time + temp.cpu_time;
            if(finish_time <= temp.arrival_time + temp.deadline)
            {
                Schedule sch = Schedule(cur_time, finish_time, task_num, replica_num);
                schedule[k].push_back(sch);
                last_idle_time[k] = finish_time;
            }
            else
            {
                return false;
            }
        }
    }

    return true;
}

// Allocate j'th replica of i'th task on any of the processor between 0 and m-1
// provided that all the replicas of a particular task execute on different processors
// If schedule is possible (non-preemptive edf) using the allocation upto j'th replica of i'th task,
// then allocate the next replica or next task if it was the last replica.

bool Allocate(int i, int j)
{
    if(i==n)    // all tasks have been allocated to the processors and are schedulable
    {
        return true;
    }

    for(int k=0;k<m;k++)
    {
        if(task_to_processors[i].find(k) == task_to_processors[i].end())
        {
            allocation[i][j] = k;
            task_to_processors[i].insert(k);
            bool schedulable = schedule_edf(i, j);
            if(schedulable)
            {
                if(j==task[i].replicas-1)   // last replica so go to next task and try to allocate its first replica
                {
                    bool possible = Allocate(i+1, 0);
                    if(possible)
                    {
                        return true;
                    }
                }
                else    // now go to next replica of current task
                {
                    bool possible = Allocate(i, j+1);
                    if(possible)
                    {
                        return true;
                    }
                }
            }
            task_to_processors[i].erase(k);
        }
    }
    return false;
}


int main()
{
    // number of processes, number of tasks
    cin>>n>>m;

    //number of available CPU frequencies
    cin>>F;
    double f;

    // Enter the normalized frequencies wrt fmax (fmax = 1)
    for(int i=0;i<F;i++)
    {
        cin>>f;
        freq_list.push_back(f);
    }

    task.resize(n);
    schedule.resize(m);
    double et, p, d, r;

    // Enter execution time, period, deadline, reliability of all the tasks one by one
    for(int i=0;i<n;i++)
    {
        cin>>et>>p>>d>>r;
        task[i] = Task(i, et, p, d, r);
        task[i].replicas = 2;
        task[i].freq = 1;
        task[i].energy = (task[i].freq)*(task[i].freq)*(task[i].exe_time);
        task[i].cpu_time = task[i].exe_time / task[i].freq;
    }

    find_hyperperiod();
    memset(allocation, -1, sizeof(allocation));

    // try to allocate all tasks starting from 0'th replica of the 0'th task where
    task_to_processors.resize(n);
    bool possible = Allocate(0,0);
    possible?cout<<"Possible\n":cout<<"Not possible\n";

    if(possible)
    {
        cout<<"\nAllocation[i][j] is the processor assigned to j'th instance of i'th task\n";
        for(int i=0;i<n;i++)
        {
            for(int j=0;j<task[i].replicas;j++)
            {
                cout<<allocation[i][j]<<"  ";
            }
            cout<<endl;
        }

        cout<<"\nSchedule:\n";
        cout<<"Task number, Replica number, Start Time, Finish Time\n\n";
        for(int k=0;k<m;k++)
        {
            cout<<"Processor P"<<k<<endl;
            for(auto &sch: schedule[k])
            {
                cout<<sch.task_num<<"  "<<sch.replica_num<<"  "<<sch.start_time<<"  "<<sch.finish_time<<endl;
            }
            cout<<endl;
        }

        cout<<"\nVisualization of Schedule\n";
        for(int k=0;k<m;k++)
        {
            cout<<"P"<<k<<"  ";
            int time=0;
            for(auto &sch: schedule[k])
            {
                while(time<sch.start_time)
                {
                    cout<<setw(2)<<-1<<"  ";
                    time++;
                }
                while(time>=sch.start_time && time<sch.finish_time)
                {
                    cout<<setw(2)<<sch.task_num<<"  ";
                    time++;
                }
            }
            while(time<hyperperiod)
            {
                cout<<-1<<"  ";
                time++;
            }
            cout<<endl;
        }

        cout<<"\nEnergy consumed: "<<endl;
        for(int i=0;i<n;i++)
        {
            min_energy += task[i].replicas * task[i].energy;
        }
        cout<<min_energy<<endl;
    }
    return 0;
}
