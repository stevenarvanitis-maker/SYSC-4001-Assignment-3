/**
 * @file interrupts.cpp
 * @author Sasisekhar Govind
 * @brief template main.cpp file for Assignment 3 Part 1 of SYSC4001
 * 
 */

#include<interrupts_101209704_101303797.hpp>
static const unsigned int TIME_SLICE = 100;

void Rnd_Robin(std::vector<PCB> &ready_queue) {
    std::sort( 
                ready_queue.begin(),
                ready_queue.end(),
                []( const PCB &first, const PCB &second ){
                    return (first.arrival_time > second.arrival_time); 
                } 
            );
}

std::tuple<std::string > run_simulation(std::vector<PCB> list_processes) {

    std::vector<PCB> ready_queue;   //The ready queue of processes
    std::vector<PCB> wait_queue;    //The wait queue of processes
    std::vector<PCB> job_list;     

    unsigned int current_time = 0;
    unsigned int slice_counter = 0;

    PCB running;
    idle_CPU(running); //Initialize an empty running process

    std::string execution_status;
    execution_status = print_exec_header();
    
    while (job_list.empty() || !all_process_terminated(job_list))
    {
       
    //arrrived processes should be moved to the ready queue to wait for its turn to use the cpu
        for (auto &p : list_processes)
        {
            if (p.arrival_time == current_time)
            {
                if(assign_memory(p)){
                p.state = READY;
                ready_queue.push_back(p);
                job_list.push_back(p);
                execution_status += print_exec_status(current_time, p.PID, NEW, READY);
                }
            }
        }

    //This handles the wait queue,processes are kept here when waiting for their i/o to finish to continue using the cpu
        for (size_t i = 0; i < wait_queue.size(); )
        {
            PCB &p = wait_queue[i];
            if (p.io_duration > 0) {
                p.io_duration--;
                sync_queue(job_list, p);
                i++;
                continue;
            }

            // when i/o is finished process goes back to ready state
            p.state = READY;
            ready_queue.push_back(p);
            execution_status += print_exec_status(current_time,p.PID,WAITING,READY);
            sync_queue(job_list, p);

            wait_queue.erase(wait_queue.begin() + i);
        }

        
    //If the CPU is idle we are meant to pick next process already in ready state 
        if (running.state == NOT_ASSIGNED && !ready_queue.empty())
        {
            running = ready_queue.front();
            ready_queue.erase(ready_queue.begin());
            running.state = RUNNING;
            running.start_time = current_time;
            slice_counter = 0;  
            sync_queue(job_list, running);
            execution_status += print_exec_status(current_time, running.PID,READY,RUNNING);
        }

        
    //CPU execution for 1ms
        if (running.state == RUNNING)
        {
            running.remaining_time--;
            sync_queue(job_list, running);
            current_time++;

            // when doing i/o request
            if (running.io_freq > 0 &&(running.processing_time - running.remaining_time) % running.io_freq == 0 &&
                running.remaining_time > 0)
            {
                running.state = WAITING;
                wait_queue.push_back(running);
                execution_status += print_exec_status(current_time,running.PID,RUNNING,WAITING);
                idle_CPU(running);
            }
            //  after the cpu's work has been finsihed
            else if (running.remaining_time == 0)
            {
                running.state = TERMINATED;
                free_memory(running);
                sync_queue(job_list, running);
                execution_status += print_exec_status(current_time, running.PID, RUNNING,TERMINATED);
                idle_CPU(running);
               
            }
            // we do a time slice of 100ms, when a process has used up its time it getts preempted and placed back to ready queue to allow another process run
            else if (slice_counter == TIME_SLICE) {
                running.state = READY;
                ready_queue.push_back(running);
                sync_queue(job_list, running);
                execution_status += print_exec_status(current_time,running.PID,RUNNING,READY);
                idle_CPU(running);
            }
        }
    else {
        current_time++;
    }
 }
    execution_status += print_exec_footer();
    return std::make_tuple(execution_status);
}


int main(int argc, char** argv) {

    //Get the input file from the user
    if(argc != 2) {
        std::cout <<"ERROR: expected input file.\n";
        return -1;
    }
    //Ensures that the file actually opens
    std::ifstream input(argv[1]);
    if (!input.is_open()) {
        std::cout << "Cannot open input file.\n";
        return -1;
    }
    //Parse the entire input file and populate a vector of PCBs.
    //To do so, the add_process() helper function is used (see include file).
    std::vector<PCB> list_process;
    std::string line;
    while (std::getline(input, line))
    {
        auto tokens = split_delim(line, ", ");
        list_process.push_back(add_process(tokens));
    }
    input.close();

    //With the list of processes, run the simulation
    auto [exec] = run_simulation(list_process);

    write_output(exec, "execution.txt");

    return 0;
}