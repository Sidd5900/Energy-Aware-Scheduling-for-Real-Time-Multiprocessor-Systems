#include <bits/stdc++.h>

using namespace std;

struct Task{
    int tid;
    double exe_time;
    int period;
    double deadline;
    double reliability;
    double freq;
    int replicas;
    double energy;
    double cpu_time;
    double arrival_time;

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

    Schedule(double st=0, double ft=0, int tn=0){
        start_time = st;
        finish_time = ft;
        task_num = tn;
    }
};

struct arrival_time_comp{
bool operator()(const Task &a,const Task &b)
{
    return a.arrival_time > b.arrival_time;
}
};

struct deadline_comp{
bool operator()(const Task &a,const Task &b)
{
    double a_absolute_deadline = a.arrival_time + a.deadline;
    double b_absolute_deadline = b.arrival_time + b.deadline;
    if(a_absolute_deadline == b_absolute_deadline)
        return a.arrival_time > b.arrival_time;
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
int n,m,F;
vector<Task> task;
vector<double> freq_list;
int hyperperiod;
vector<vector<Schedule>> schedule;
int allocation[101][10];
vector<priority_queue<Task,vector<Task>,arrival_time_comp>> task_queue;
vector<priority_queue<Task,vector<Task>,deadline_comp>> ready_queue;
double global_time;

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

// sort on the basis of arrival time and in case of same arrival time, sort on basis of earliest deadline
bool comp(Task &A, Task &B)
{
    if(A.arrival_time == B.arrival_time)
    {
        return ((A.arrival_time + A.deadline) <= (B.arrival_time + B.deadline));
    }
    return (A.arrival_time < B.arrival_time);
}

// try to schedule all tasks up to task 'task_num' and its replica 'replica_num' using non-preemptive earliest deadline first algorithm
bool schedule_edf(int task_num, int replica_num)
{
    // ready_queue[k] will store all the tasks allocated to the k'th processor
    ready_queue.clear();
    ready_queue.resize(m);
    task_queue.clear();
    task_queue.resize(m);
    schedule.clear();
    schedule.resize(m);

    for(int i=0;i<=task_num;i++)
    {
        for(int j=0;j<=replica_num;j++)
        {
            int k=allocation[i][j];
            if(k != -1)
            {
                double cur_time=0;
                while(cur_time<hyperperiod)
                {
                    // tasks_queue.insert({cur_time, task[i]});
                    task[i].arrival_time = cur_time;
                    task_queue[k].push(task[i]);
                    cur_time += task[i].period;
                }
            }
        }
    }

    // check if all the allocated tasks in a particular processor can be allocated to it (without missing deadline) or not
    for(int k=0;k<m;k++)
    {
        double cur_time=0;
        while(!task_queue[k].empty())
        {
            cur_time = task_queue[k].top().arrival_time;
            while(!task_queue[k].empty() && task_queue[k].top().arrival_time <= cur_time)
            {
                Task temp = task_queue[k].top();
                task_queue[k].pop();
                ready_queue[k].push(temp);
            }

            double last_free_time=0;    // at what time was the current processor free or will be free

            while(!ready_queue[k].empty())
            {
                Task temp = ready_queue[k].top();
                ready_queue[k].pop();
                cur_time = max(cur_time, last_free_time);
                double finish_time = cur_time + temp.cpu_time;
                if(finish_time <= temp.arrival_time + temp.deadline)
                {
                    last_free_time = finish_time;
                    Schedule sch = Schedule(cur_time, finish_time, temp.tid);
                    schedule[k].push_back(sch);

                    while(!task_queue[k].empty() && task_queue[k].top().arrival_time <= last_free_time)
                    {
                        Task temp = task_queue[k].top();
                        task_queue[k].pop();
                        ready_queue[k].push(temp);
                    }
                }
                else
                {
                    return false;
                }
            }
        }
    }
    return true;
}

// Allocate j'th replica of i'th task on any of the processor between p and m-1
// i.e we can assign that job to either processor p or p+1 or p+2 .... or m-1.
// If schedule is possible (non-preemptive edf) using the allocation upto j'th replica of i'th task,
// then allocate the next replica or next task if it was the last replica.

bool Allocate(int i, int j, int p)
{
    if(i==n)    // all tasks have been allocated to the processors and are schedulable
    {
        return true;
    }
    for(int k=p;k<m;k++)
    {
        allocation[i][j]=k;
        bool schedulable = schedule_edf(i, j);
        if(schedulable)
        {
            if(j==task[i].replicas-1)   // last replica so go to next task and try to allocate its first replica
            {
                bool possible = Allocate(i+1, 0, 0);
                if(possible)
                {
                    return true;
                }
            }
            else    // now go to next replica of current task
            {
                bool possible = Allocate(i, j+1, k+1);
                if(possible)
                {
                    return true;
                }
            }
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
    // we can choose any processor from 0 to m-1 to allocate that job (0'th replica of the 0'th task)
    bool possible = Allocate(0,0,0);
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

        cout<<"\nSchedule\n";
        cout<<"Task, Start Time, Finish Time\n\n";
        for(int k=0;k<m;k++)
        {
            cout<<"Processor P"<<k<<endl;
            for(auto &sch: schedule[k])
            {
                cout<<sch.task_num<<"  "<<sch.start_time<<"  "<<sch.finish_time<<endl;
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
    }

    return 0;
}
