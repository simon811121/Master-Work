#ifndef SYSTEMMONITOR_H
#define SYSTEMMONITOR_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <cstdlib>
#include <vector>
#include <algorithm>

#include <string.h>
#include <utils/threads.h>
#include <pthread.h>
#include <sched.h>

#include <utils/Errors.h>
#include <utils/String16.h>

#include <binder/IServiceManager.h>
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
 
#include <binder/BinderService.h>
#include <SurfaceFlinger.h>
#include <cutils/sched_policy.h>
#include <sys/syscall.h>

using namespace android;

//******************************//
//  PATH						//
//******************************//
#define cpu_fs "/sys/devices/system/cpu"
#define gpu_fs "/sys/class/kgsl/kgsl-3d0"
#define thermal_fs "/sys/devices/virtual/thermal"
#define battery_fs "/sys/class/power_supply/battery"
#define brightness_fs "/sys/class/leds/lcd-backlight"

//******************************//
//  Experiment Setting			//
//******************************//
#define temp_constraint 80 		//temperature constrain

#define DEFAULT_GOVERNOR_SET 0	//0 for userspace, adjust frequency by user or algorithm
								//1 for performance
								//2 for powersave
								//3 for interactive

#define THERMAL_ENGINE 0 		//0 for closed thermal-engine
								//1 for open thermal-engine

#define testing_time 0		//testing time (Ex: if test 300 seconds -> 301)
								//The real algorithm working time (not include sampling time)
								//If testing "sampling interval", testing time can be set to 0

#define algorithm_select 0 		//default governor(performance, powersave, interactive): 0
								//sampling test and testing: 0
								//Hao: 1
								//Kai: 2
								//Kai + dispatcher: 3
								//predictive: 4
								//migration: 5
								//predictive + migration: 6

#define alogrithm_sampling_time 16 	//default governor(performance, powersave, interactive): 0
									//testing functions: 0
									//Hao: ?????
									//Kai: 4
									//Kai + dispatcher: ?????
									//predictive: 0
									//migration: 0
									//predictive + migration: 0	
/*!!!*/
//Core assign need to set up (in Dispatcher) for open/close core purpose
/*!!!*/

//******************************//
//  System Parameters			//
//******************************//
#define core_num 6
#define little_freq_level 9  
#define big_freq_level 13
#define gpu_freq_level 6 
#define brightness_level 5
#define calculate_thread_utilization_offset 14 	//for calculate thread utilization
#define thread_matrix_len 100 					//thread numbers

sp<IBinder> binder;

#define Fore_App_Nums 9
//Foreground App
const char* Fore_App_List[] = {"disneycrossyroad_goo",						//Disney Cross Road
						  	   "GloftA8HM",									//ASPHALT 8
						  	   "deerhunt16",								//Deer Hunt 2016
						  	   "GloftM5HM",									//Modern Warfare 5
						  	   "r3_row",									//Real Racing 3
						  	   "edgeoftomorrow",							//Edge of Tomorrow
						  	   "doNotStarvePocket",							//Don't Starve Together
						  	   "com.android.chrome:privileged_process"};	//Chrome 1
						  	   
//This list is for multithread app
const char* Multi_Thread_App_List[] = {"com.android.chrome:sandboxed_process"};	//Chrome 2(together with Chrome 1)
						  									
#define Back_App_Nums 3
//Background App
const char* Back_App_List[] = {"openmanager",				//Open Manager
							   "spotify",					//Spotify
							   "google.android.apps.maps"}; //Google Maps
/*Related Work Kai*/
int scale_period = 1000000;
enum scale_state{
	SCALE_UP,
	SCALE_DOWN,
	SCALE_BALANCE,
	SCALE_CHECK,
	SCALE_U_Based,
	SCALE_Q_Based
};
int Q_target_set = 31;
const int QLB = 0;
const int QUB = 4;
double learn_rate = 0.2;
/*End of Related Work Kai*/

/*Related Work Predictive Power Model*/
#define aC_b 0.000027728	//alpha * C (big core)(from related work)
#define aC_l 0.000058779	//alpha * C (little core)(from related work)
float gpu_dynamic[gpu_freq_level] = {0.05697, 0.10977, 0.17683, 0.29727, 0.37007, 0.61977};
int gpu_f_decide = 5;
/*End of Related Work Predictive Power Model*/

/*Hao Work*/
enum {
	big_oriented,
	little_oriented,
	gpu_oriented
};
/*End of Hao Work*/

//******************************//
//  Algorithm					//
//******************************//
void hao_work(int time, pid_t pid, int now_FPS);		//only frequency controller now
void *related_work_kai(void*);							//kai (sense from 1 ~ 4 secs, algorithm start at 5)
void *related_work_kai_with_dispatcher(void*);			//Frequency: kai, Dispatcher: power dispatcher 
void *related_work_predictive(void*);					//use model to predict temperature, and reduce it
void *related_work_migration(void*);					//use migration to reduce temperature
void *related_work_predictive_with_migration(void*); 	//Frequency: predictive, Dispatcher: migration

//******************************//
//  Frequency Level				//
//******************************//
int big_FL[big_freq_level] = {384000, 480000, 633600, 768000, 864000, 960000, 1248000, 1344000, 1440000, 1536000, 1632000, 1689600, 1824000};
int little_FL[little_freq_level] = {384000, 460800, 600000, 672000, 787200, 864000, 960000, 1248000, 1440000};
int gpu_FL[gpu_freq_level] = {180000000, 300000000, 367000000, 450000000, 490000000, 600000000};

//******************************//
//  CPU							//
//******************************//
int get_cpu_freq(int cpu);		//cpu => big: 4, 5
								//		 little: 0, 1, 2, 3 
void set_cpu_freq(int cpu, int freq);
void get_cpu_time(int cpu, unsigned long long* busy, unsigned long long* total);	//cpu => big: 4, 5 
																					//		 little: 0, 1, 2, 3
																					//		 6 for all cpu util sum
int get_cpu_util(int cpu);	//cpu => big: 4, 5 
							//		 little: 0, 1, 2, 3
							//		 6 for all cpu util sum
enum {
	cpu_off,
	cpu_on
};
int get_cpu_on(int cpu);		//0 -> off, 1 -> on
void set_cpu_on(int cpu, int on);
int find_max_util(int start_cpu, int cpu_utilization[]); 	//return maximum utilization in a cluster (0~3: little, 4~5: big)
															//start_cpu = 0 or 4 (0 for little cluster, 4 for big cluster)
void equal_computing_cycles(int now_cores, int now_frequency, int target_cores, double *target_frequency);	
					 										//return frequency with "same performance" different cores input

//******************************//
//  GPU							//
//******************************//
int get_gpu_freq();
void set_gpu_freq(int freq);
int get_gpu_util();

//******************************//
//  Dispatcher					//
//******************************//
int get_app_pid(const char app_name[]);							//return pid
int get_all_task_pid(pid_t pid, std::vector<int>& all_task);	//return maximum task amount
int get_main_task_pid(pid_t pid, std::vector<int>& main_task);	//return main running task amount
int core_assign[core_num]={1, 1, 1, 1, 1, 1};					//Core on/off Decide & Assign core matrix
																//core_assign[core_num] = {CPU0, CPU1, CPU2, CPU3, CPU4, CPU5}
																//if the value is 1 -> the core need assign
																//if the value is 0 -> the core not need assign
																//EX: core_assign[core_num] = {1, 1, 1, 0, 0, 0}
																//-> core 0~2 need, core 3~5 no need
void set_all_pid_to_core(pid_t pid, int core[]);				//all tasks
																//set all tasks related to pid(/proc/[pid]/task/)
																//depend on core_assign[core] matrix
								//, char big, char little)		//big: 0, 1, 2, 3
																//little: 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, A, B, C, D, E, F
void set_main_pid_to_core(pid_t pid, int core[]);				//main tasks(running tasks)
																//set main tasks related to pid(/proc/[pid]/task/)
																//depend on core_assign[core] matrix
								 //, char big, char little)		//big: 0, 1, 2, 3
																//little: 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, A, B, C, D, E, F
enum {
	foreground,
	background
};
int check_app_fg(pid_t pid);									//check pid background/foreground
void get_fore_back_app_pid(int *fore_app, int *back_app);		//Now: Use for Dual-Process -> foreground = process 1, background = process 2
																//Previous: return foreground and background pid 
struct thread_information {										//for thread info data structure
	pid_t pid;
	unsigned long long thread_busy;
	float utilization;
	unsigned long long cpu_busy;
	unsigned long long cpu_total;
};
struct thread_information thread_info_1[thread_matrix_len];		//thread info matrix 1
struct thread_information thread_info_2[thread_matrix_len];		//thread info matrix 2
int parent_pid_app_1_1 = 0, parent_pid_app_1_2 = 0;				//to note the pid get from get_fore_back_pid
int parent_pid_app_2_1 = 0, parent_pid_app_2_2 = 0;				//need to be used in get_thread_time to get the thread info
int now_pid = 0;												//this pid is for checking which process is setting(migrating or setting position) now
void get_thread_time(pid_t pid, unsigned long long* busy);		//get thread execution time
float get_thread_util(pid_t pid, unsigned long long* thread_busy_p, unsigned long long* cpu_busy_p, unsigned long long* cpu_total_p);
																//get_thread_util: return pid's utilization
																//thread_usage = thread usage time
																//cpu_busy_p = total cpu usage time, cpu_total_p = total cpu time(including idle time)
void thread_info_initial(pid_t pid, struct thread_information thread_matrix[], unsigned long long* cpu_busy_p, unsigned long long* cpu_total_p);
																//thread_info_initial: thread info matrix initialize
struct thread_utilization_sort_information {					//thread util sort info
	pid_t pid;
	float utilization;
};
void thread_utilization_sort(struct thread_utilization_sort_information *thread_sort_matrix, struct thread_information *thread_matrix, int len);
																//thread_utilization_sort_information: thread utilization sorting function
																
//******************************//
//  Thermal						//
//******************************//
int get_temp(int num);			//num=2,3,4,5 for cpu 2,3,4,5 (default for cpu 0,1)	//num=7 for gpu, num=8 for battery(back)
int get_max_temp(int *core);	//return maximum temperature and core number

//******************************//
//  Power						//
//******************************//
double total_big_power(int big_core_nums, int big_freq, double voltage);	//return total big power
				  															//big_core_nums: 0, 1, 2
double total_little_power(int little_core_nums, int little_freq, double voltage);	//return total little power
				  																	//little_core_nums: 0, 1, 2, 3, 4
				 
//******************************//
//  Battery						//
//******************************//
enum {
	battery_voltage,
	battery_current,
	battery_capacity,
	battery_temp
};
int get_battery(int coef);

//******************************//
//  FPS							//
//******************************//
sp<IBinder> get_surfaceflinger();
int get_fps(sp<IBinder>& binder);

//******************************//
//  Brightness					//
//******************************//
int brightness_L[brightness_level]={1, 64, 128, 192, 255};	// Dark --> bright (1%, 25%, 50%, 75%, 100%)
int get_brightness();
void set_brightness(int bright);

//******************************//
//  Tool						//
//******************************//
double reciprocal(double input);	//check input equal to 0? (1)if no, return input reciprocal (2)if yes, return 0 
int AVG(int input[],int size);		//return average of input[]
//void swap(int* a , int* b);			//swap a and b
void sort(int* begging , int* end);	//sorting from small to big value
const char* get_items(char* buffer, int offset);	//get the value from buffer head to the offset
void swap_float(float& a, float& b);
void max_heapify(struct thread_utilization_sort_information *thread_sort_matrix, int start, int end);	//for heap sorting
void heap_sort(struct thread_utilization_sort_information *thread_sort_matrix, int len);				//heap sorting

#endif
