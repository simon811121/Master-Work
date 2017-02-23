// In This .h all the function 

#ifndef FUNCTION_H
#define FUNCTION_H

#include "systemmonitor.h"

using namespace android;

//******************************//
//  CPU							//
//******************************//
int get_cpu_freq(int cpu) {		//cpu => big: 4, 5
								//		 little: 0, 1, 2, 3 
	char buffer[100];
	FILE *cpu_f;
	
	sprintf(buffer,"%s/cpu%d/cpufreq/cpuinfo_cur_freq", cpu_fs, cpu);	
	cpu_f = fopen(buffer, "r");
	if (cpu_f) {
		fgets(buffer, 100, cpu_f);
		int f = atoi(buffer);
		fclose(cpu_f);
		return f / 1000;	// f / 1000 -> MHz
	} else {
		return 0;
	}
}

void set_cpu_freq(int cpu, int freq) {
	char buffer[100];
	FILE *cpu_f;

	sprintf(buffer,"%s/cpu%d/cpufreq/scaling_setspeed", cpu_fs, cpu);	
	cpu_f = fopen(buffer,"w");
	if (cpu_f) {
		fprintf(cpu_f, "%d", freq);
		fclose(cpu_f);
		return;
	} else {
		return;
	}
}

void get_cpu_time(int cpu, unsigned long long* busy, unsigned long long* total) {
	char buffer[100], buffer1[100];
	unsigned long long int data[10];
	FILE *cpu_u;

	sprintf(buffer,"/proc/stat");
	if (cpu == core_num) {
		sprintf(buffer1, "cpu");
	} else {
		sprintf(buffer1, "cpu%d", cpu);		
	}
	cpu_u = fopen(buffer, "r");
	if (cpu_u) {		
		while (fgets(buffer, 100, cpu_u)) {
			if (strstr(buffer, buffer1)) {
				sscanf(buffer, "%s %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu"
						, buffer1, &data[0], &data[1], &data[2], &data[3], &data[4]		//user nice system idle iowait 		
							 , &data[5], &data[6], &data[7], &data[8], &data[9]);		//irq softirq steal guest guest_nice 
				fclose(cpu_u);

				int i = 0, sum = 0;
				for (i = 0; i < 10 ; i++) sum = sum + data[i];	
				*busy = data[0] + data[1] + data[2];
				*total = sum;
			}
		}		
	} else {
		fclose(cpu_u);
		*busy = 0;
		*total = 0;		
	}
}

int get_cpu_util(int cpu, unsigned long long* usage_p, unsigned long long* time_p) {		//cpu => big: 4, 5 
																							//		 little: 0, 1, 2, 3
																							//		 6 for all cpu util sum
	unsigned long long usage, time;
	get_cpu_time(cpu, &usage, &time);
	int cpu_u;

	if (cpu == core_num) {
		cpu_u = (int)(((double)(usage - *usage_p) / (time - *time_p)) * 600.0); 
	} else {
		cpu_u = (int)(((double)(usage - *usage_p) / (time - *time_p)) * 100.0); 
	}
	*usage_p = usage;
	*time_p = time;	
	return cpu_u; 
}

int get_cpu_on(int cpu) {	//0 -> off, 1 -> on
	char buffer[100];
	FILE *cpu_f;

	sprintf(buffer,"%s/cpu%d/online", cpu_fs, cpu);	
	cpu_f = fopen(buffer, "r");
	if (cpu_f) {
		fgets(buffer, 100, cpu_f);
		int on = atoi(buffer);
		fclose(cpu_f);
		return on;
	} else {
		return 0;
	}
}

void set_cpu_on(int cpu, int on) {
	char buffer[100];
	FILE *cpu_f;

	sprintf(buffer,"%s/cpu%d/online", cpu_fs, cpu);	
	cpu_f = fopen(buffer,"w");
	if (cpu_f) {
		fprintf(cpu_f, "%d", on);
		fclose(cpu_f);
		return;
	} else {
		return;
	}
}

int find_max_util(int start_cpu, int cpu_utilization[]) { 	//return maximum utilization in a cluster (0~3: little, 4~5: big)
	int max_util = 0;										//start_cpu = 0 or 4
	int end_cpu = 0;

	if (start_cpu == 0) end_cpu = 3;
	else end_cpu = 5;
	for (int i = start_cpu; i <= end_cpu; i++) {
		if (cpu_utilization[i] > max_util) {
			max_util = cpu_utilization[i];
		}
	}
	return max_util;
}

void equal_computing_cycles(int now_cores, int now_frequency, int target_cores, double *target_frequency) {	
																					//return "same performance" cores and frequency
	if (now_cores == 1 && target_cores == 2) {
		*target_frequency = now_cores * now_frequency * 1.4 / target_cores;	
	}
	else if (now_cores == 1 && target_cores == 4) {
		*target_frequency = now_cores * now_frequency * 1.4 * 1.7 / target_cores;
	}
	else if (now_cores == 2 && target_cores == 1) {
		*target_frequency = now_cores * now_frequency / 1.4 / target_cores;
	}	
	else if (now_cores == 2 && target_cores == 4) {
		*target_frequency = now_cores * now_frequency * 1.7 / target_cores;
	}
	else if (now_cores == 4 && target_cores == 1) {
		*target_frequency = now_cores * now_frequency / 1.4 / 1.7 / target_cores;
	}
	else if (now_cores == 4 && target_cores == 2) {
		*target_frequency = now_cores * now_frequency / 1.7 / target_cores;		
	}
}

//******************************//
//  GPU							//
//******************************//
int get_gpu_freq() {
	char buffer[100];
	FILE *gpu_f;
	
	sprintf(buffer,"%s/devfreq/cur_freq", gpu_fs);	
	gpu_f = fopen(buffer,"r");
	if (gpu_f) {
		fgets(buffer, 100, gpu_f);
		int f = atoi(buffer);
		fclose(gpu_f);
		return f / 1000000;	// f / 1000000 -> MHZ
	} else {
		return 0;
	}
}

void set_gpu_freq(int freq) {
	char buffer[100];
	char buffer1[100];
	FILE *gpu_f;	
	FILE *gpu_f1;

	sprintf(buffer, "%s/devfreq/max_freq", gpu_fs);	
	sprintf(buffer1, "%s/devfreq/min_freq", gpu_fs);	
	gpu_f = fopen(buffer, "w");
	gpu_f1 = fopen(buffer1, "w");
	if (gpu_f) {
		if (gpu_f1) {
			fprintf(gpu_f, "%d", freq);
			fprintf(gpu_f1, "%d", freq);
			fclose(gpu_f);
			fclose(gpu_f1);
			return;
		}
	} else {
		return;
	}
}

int get_gpu_util() {
	char buffer[100];
	FILE *gpu_f;
	unsigned long long int g0 = 0, g1 = 0;	

	sprintf(buffer,"%s/gpubusy", gpu_fs);	
	gpu_f = fopen(buffer,"r");
	if (gpu_f) {
		fgets(buffer, 100, gpu_f);
		sscanf(buffer, "%llu %llu", &g0, &g1); 
		fclose(gpu_f);
		return (int)((double)(g0)/(double)(g1)*100.0);
	} else {
		return 0;
	}
}

//******************************//
//  Dispatcher					//
//******************************//
int get_app_pid(const char app_name[]) {		//return pid
	char buffer[100];
	char app[100];								//app name
	char a[50], b[50], c[50], d[50];			//other strings (not use now)
	FILE *app_f;
	pid_t pid, ppid;
	int z, y;									//memory information (not use now)

	pid = 0;

	sprintf(buffer, "ps | grep %s > /data/ps.txt", app_name);
	system(buffer);

	app_f = fopen("/data/ps.txt", "r");
	while (fscanf(app_f, "%s %d %d %d %d %s %s %s %s", a, &pid, &ppid, &z, &y, b ,c, d, app) != EOF) {
	}
	fclose(app_f);
	return pid;
}

int get_all_task_pid(pid_t pid, std::vector<int>& all_task) {		//return maximum task amount
	char buffer[100];
	FILE *task_f;
	int max_task = 0;
	int temp = 0;

	sprintf(buffer, "ls /proc/%d/task > /data/task.txt", pid);
	system(buffer);

	task_f = fopen("/data/task.txt", "r");
	while (fscanf(task_f, "%d", &temp) != EOF) {
		all_task.push_back(temp);
		max_task++;
	}
	fclose(task_f);
	return max_task;
}

int get_main_task_pid(pid_t pid, std::vector<int>& main_task) {		//return main running task amount
	char buffer[100];
	char a[50], b[50], c[50], d[50];			//other strings (not use now)
	char running[50];							//check if task is running
	FILE *app_f;
	pid_t temp_pid, temp_ppid;
	int z, y;
	int max_task = 0;

	sprintf(buffer, "ps -t %d > /data/ps.txt", pid);
	system(buffer);

	app_f = fopen("/data/ps.txt", "r");
	while (fscanf(app_f, "%s %d %d %d %d %s %s %s %s", a, &temp_pid, &temp_ppid, &z, &y, b ,c, running, d) != EOF) {
		if (strcmp(running, "R") == 0) {
			main_task.push_back(temp_pid);
			max_task++;
		}
	}
	fclose(app_f);		
	return max_task;
}

void set_all_pid_to_core(pid_t pid, int core[]) {				//all tasks
																//core: 0, 1, 2, 3, 4, 5 for now
																//core -> !!!not decided yet!!!
								//, char big, char little)		//big: 0, 1, 2, 3
																//little: 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, A, B, C, D, E, F
	char buffer[100];
	std::vector<int> task;	
	int max_task = 0;

	max_task = get_all_task_pid(pid, task);
	
	//setaffinity method
	cpu_set_t mask;
	CPU_ZERO(&mask);
	for (int i = 0; i < core_num; i++)	if (core[i] == 1) CPU_SET(i, &mask);

	for (std::vector<int>::iterator it = task.begin(); it != task.end(); ++it) {
		//setaffinity method
		sched_setaffinity(*it, sizeof(cpu_set_t), &mask);
		//taskset method
		//sprintf(buffer, "taskset -p %c%c %d > /data/buffer.txt", big, little, *it);
		//system(buffer);
	}
	return;
}

void set_main_pid_to_core(pid_t pid, int core[]) {				//main tasks(running tasks)
																//core: 0, 1, 2, 3, 4, 5 for now
																//core -> !!!not decided yet!!!
								 //, char big, char little)		//big: 0, 1, 2, 3
																//little: 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, A, B, C, D, E, F
	char buffer[100];
	std::vector<int> task;	
	int max_task = 0;

	max_task = get_main_task_pid(pid, task);
	
	//setaffinity method
	cpu_set_t mask;
	CPU_ZERO(&mask);
	for (int i = 0; i < core_num; i++)	if (core[i] == 1) CPU_SET(i, &mask);

	for (std::vector<int>::iterator it = task.begin(); it != task.end(); ++it) {
		//setaffinity method
		sched_setaffinity(*it, sizeof(cpu_set_t), &mask);
		//taskset method
		//sprintf(buffer, "taskset -p %c%c %d > /data/buffer.txt", big, little, *it);
		//system(buffer);
	}
	return;
}

int check_app_fg(pid_t pid) {		//check pid background/foreground
	SchedPolicy p;
	get_sched_policy(pid, &p);	

	if (strcmp("fg", get_sched_policy_name(p)) == 0) {
		return foreground;
	}
	else if (strcmp("bg",get_sched_policy_name(p)) == 0) {
		return background;
	}
	else {
		return -1;
	}
}

void get_fore_back_app_pid(int *fore_app, int *back_app) {		//Now: for dual-process -> foreground = process 1, background = process 2
																//Previous: return foreground and background pid
	pid_t temp;	
	int count = 0;	//to count which app in Fore_App_List[] is
					//use for dual thread purpose	EX: Chrome

	for (int j = 0; j < Fore_App_Nums; j++) {
		temp = get_app_pid(Fore_App_List[j]);
		if (temp != 0) {
			if (check_app_fg(temp) == foreground) {
				*fore_app = temp;
				count  = j;
				break;	
			}
		}
		else *fore_app = 0;
	}
	if (count == 7) { 									//"com.android.chrome:privileged_process" -> Chrome 1
		temp = get_app_pid(Multi_Thread_App_List[0]);	//"com.android.chrome:sandboxed_process" -> Chrome 2
		*back_app = temp;
	}
	else *back_app = 0;
	/*for (int j = 0; j < Back_App_Nums; j++) {		//Previous use
		temp = get_app_pid(Back_App_List[j]);
		if (temp != 0) {
			if (check_app_fg(temp) == background) {
				*back_app = temp;
				break;	
			} 
		}
		else *back_app = 0;		
	}*/
	return;
}

void get_thread_time(pid_t pid, unsigned long long* busy) {
	char file[64] = {0};
	char line_buff[1024] = {0};
	unsigned long long int utime, stime, custime, cstime;
	FILE *thread_f;

	if (pid == parent_pid_app_1_1)  sprintf(file, "/proc/%d/task/%d/stat", parent_pid_app_1_2, pid);
	else if (pid == parent_pid_app_2_1) sprintf(file, "/proc/%d/task/%d/stat", parent_pid_app_2_2, pid);
	else {
		if (now_pid == parent_pid_app_1_1)	sprintf(file, "/proc/%d/task/%d/stat", parent_pid_app_1_1, pid);
		else if (now_pid == parent_pid_app_2_1) sprintf(file, "/proc/%d/task/%d/stat", parent_pid_app_2_1, pid);
		else {
			printf("\n!!!Error in get_thread_time() in function.h\n");
			system("Pause");
		}
	}

	/*for (unsigned int i = 0; i < (sizeof(file)/sizeof(file[0])); i++) {	//print file result for debug use
		if (file[i] == '\0')	break;
		else printf("%c", file[i]);
	}*/

	thread_f = fopen(file, "r");
	if (thread_f) {		
		fgets(line_buff, sizeof(line_buff), thread_f);
		const char* q = get_items(line_buff, calculate_thread_utilization_offset);
		sscanf(q, "%llu %llu %llu %llu", &utime, &stime, &custime, &cstime);
		fclose(thread_f);
		*busy = utime + stime + custime + cstime;
	} else {
		fclose(thread_f);
		*busy = 0;		
	}
	return;
}

float get_thread_util(pid_t pid, unsigned long long* thread_busy_p, unsigned long long* cpu_busy_p, unsigned long long* cpu_total_p) {
														//get_thread_util return pid's utilization
														//thread_usage = thread usage time
														//cpu_busy_p = total cpu usage time, cpu_total_p = total cpu time(including idle time)
	unsigned long long cpu_busy, cpu_total;
	unsigned long long thread_busy;
	float thread_u;

	get_cpu_time(6, &cpu_busy, &cpu_total);
	get_thread_time(pid, &thread_busy);

	thread_u = 100.0 * (thread_busy - *thread_busy_p) / (cpu_total - *cpu_total_p);
	*cpu_busy_p = cpu_busy;
	*cpu_total_p = cpu_total;
	*thread_busy_p = thread_busy;

	return thread_u;
}

void thread_info_initial(pid_t pid, struct thread_information thread_matrix[], unsigned long long* cpu_busy_p, unsigned long long* cpu_total_p) {	
														//thread_info_initial: thread info matrix initialize
	std::vector<int> task;
	int max_task = 0, count = 0;
	static int check_process = 0;

	for (int i = 0; i < 100; i++) {		//thread matirx set to 0
		thread_matrix[i].pid = 0;
		thread_matrix[i].thread_busy = 0;
		thread_matrix[i].utilization = 0.0;
		thread_matrix[i].cpu_busy = 0;
		thread_matrix[i].cpu_total = 0;
	}

	max_task = get_all_task_pid(pid, task);
	check_process++;

	for (std::vector<int>::iterator it = task.begin(); it != task.end(); ++it) { 
		thread_matrix[count].pid = *it;
		thread_matrix[count].cpu_busy = *cpu_busy_p;
		thread_matrix[count].cpu_total = *cpu_total_p;
		count++;
	}

	if (check_process == 1) {
		parent_pid_app_1_1 = pid;
		parent_pid_app_1_2 = thread_matrix[1].pid;
		now_pid = parent_pid_app_1_1;
	}
	else if (check_process == 2) {
		parent_pid_app_2_1 = pid;
		parent_pid_app_2_2 = thread_matrix[1].pid;
		now_pid = parent_pid_app_2_1;
	}
	else {
		printf("parent_pid_app_x_x overflow in thread_info_initial() in function.h\n");
		system("Pause");
	}
	count = 0;

	for (std::vector<int>::iterator it = task.begin(); it != task.end(); ++it) { 
		get_thread_time(*it, &thread_matrix[count].thread_busy);
		count++;
	}
	return;
}

void thread_utilization_sort(struct thread_utilization_sort_information *thread_sort_matrix, struct thread_information *thread_matrix, int len) {
																	//thread_utilization_sort_information: thread utilization sorting function	
	for (int i = 0; i < len; i++) {
		thread_sort_matrix[i].pid = thread_matrix[i].pid;
		thread_sort_matrix[i].utilization = thread_matrix[i].utilization;
		//printf("%d, %4f\n", thread_sort_matrix[i].pid, thread_sort_matrix[i].utilization);
	}
	heap_sort(thread_sort_matrix, len);
}

//******************************//
//  Thermal						//
//******************************//
int get_temp(int num) {			//num=2,3,4,5 for cpu 2,3,4,5 (default for cpu 0,1)
	char buffer[100];			//num=7 for gpu, num=8 for battery(back)
	FILE *temp_f;		
	int zone = 0;	

	switch (num) {
		case 2:	//cpu2
			zone = 9;
			break;
		case 3:	//cpu3
			zone = 10;
			break;
		case 4:	//cpu4
			zone = 13;
			break;
		case 5:	//cpu5
			zone = 14;
			break;
		case 7:	//gpu
			zone = 12;
			break;
		case 8: //battery(back)
			zone = 1;
			break;
		default://cpu0,1
			zone = 8;
			break;
	}	
	sprintf(buffer, "%s/thermal_zone%d/temp", thermal_fs, zone);
	
	temp_f = fopen(buffer,"r");
	if (temp_f) {
		fgets(buffer, 100, temp_f);
		int t = atoi(buffer);
		fclose(temp_f);
		if (zone == 1) return t / 1000;
		else return t;
		return t;
	} else {
		return 0;
	}
}

int get_max_temp(int *core) {			//return maximum temperature and core number
	int max_temp = get_temp(0);			//Little Core 0, 1       
	*core = 0;
	
	if (get_temp(2) > max_temp) {		//Little Core 2
		max_temp = get_temp(2);
		*core = 2;
	}
	if (get_temp(3) > max_temp) {		//Little Core 3
		max_temp = get_temp(3);
		*core = 3;
	}
	if (get_temp(4) > max_temp) {		//Big Core 4
		max_temp = get_temp(4);
		*core = 4;
	}
	if (get_temp(5) > max_temp) {		//Big Core 5
		max_temp = get_temp(5);
		*core = 5;
	}
	if (get_temp(7) > max_temp) {		//GPU
		max_temp = get_temp(7);
		*core = 7;
	}
	return max_temp;
}

//******************************//
//  Power						//
//******************************//
double total_big_power(int big_core_nums, int big_freq, double voltage) {
	int big_d = 0;		//dynamic power
	int big_l = 0;		//leakage power
	int sum_big_power = 0;		//total power

	//Big Power
	if (big_core_nums == 1) {	
		big_l = 0.0034 * get_temp(4) - 0.005;
		big_d = 0.5 * aC_b * voltage * voltage * big_freq;
	}
	else if (big_core_nums == 2){ 
		big_l = 0.0034 * (get_temp(4) + get_temp(5)) - 0.01;
		big_d = aC_b * voltage * voltage * big_freq;
	}
	else {
		big_l = 0;
		big_d = 0;
	}
	sum_big_power = big_l + big_d;
	return sum_big_power;
}

double total_little_power(int little_core_nums, int little_freq, double voltage) {
	int little_d = 0;		//dynamic power
	int little_l = 0;		//leakage power
	int sum_little_power = 0;		//total power

	//Little Power
	if (little_core_nums == 1) {
		little_l = 0.0005 * get_temp(0) + 0.005;
		little_d = 0.25 * aC_l * voltage * voltage * little_freq;
	}
	else if (little_core_nums == 2) {
		little_l = 0.0005 * (get_temp(0) + get_temp(1)) + 0.01;
		little_d = 0.5 * aC_l * voltage * voltage * little_freq;
	}
	else if (little_core_nums == 3) {
		little_l = 0.0005 * (get_temp(0) + get_temp(1) + get_temp(2)) + 0.015;
		little_d = 0.75 * aC_l * voltage * voltage * little_freq;
	}
	else if (little_core_nums == 4) {
		little_l = 0.0005 * (get_temp(0) + get_temp(1) + get_temp(2) + get_temp(3)) + 0.02;
		little_d = aC_l * voltage * voltage * little_freq;
	}
	else {
		little_l = 0;
		little_d = 0;
	} 
	sum_little_power = little_l + little_d;	
	return sum_little_power;
}

//******************************//
//  Battery						//
//******************************//
int get_battery(int coef) {
	char s[100];
	FILE *bat_f;
	
	switch (coef){
		case battery_voltage:
			sprintf(s, "%s/voltage_now", battery_fs);
			break;
		case battery_current:
			sprintf(s, "%s/current_now", battery_fs);
			break;
		case battery_capacity:
			sprintf(s, "%s/capacity", battery_fs);
			break;
		case battery_temp:
			sprintf(s, "%s/temp", battery_fs);	
			break;
		default:
			sprintf(s, "%s/temp", battery_fs);		
	}	
	bat_f = fopen(s, "r");
	if (bat_f) {
		fgets(s, 100, bat_f);
		int b = atoi(s);
		fclose(bat_f);
		return b;
	} else {
		return 0;
	}
}

//******************************//
//  FPS							//
//******************************//
sp<IBinder> get_surfaceflinger() {
	sp<IServiceManager> sm(defaultServiceManager());
	sp<IBinder> binder = sm->getService(String16("SurfaceFlinger"));
	return (binder == 0)? 0 : binder;
}

int get_fps(sp<IBinder>& binder) {
	Parcel data, reply;
	binder->transact(BnSurfaceComposer::GET_FPS, data, &reply);
	return (int)reply.readInt32();
}

//******************************//
//  Brightness					//
//******************************//
int get_brightness() {
	char buffer[100];
	FILE *brightness_f;
	
	sprintf(buffer,"%s/brightness", brightness_fs);	
	brightness_f = fopen(buffer,"r");
	if (brightness_f) {
		fgets(buffer, 100, brightness_f);
		int b = atoi(buffer);
		fclose(brightness_f);
		return b;	
	} else {
		return 0;
	}
}

void set_brightness(int bright) {
	char buffer[100];
	FILE *brightness_f;
	
	sprintf(buffer, "%s/brightness", brightness_fs);	
	brightness_f = fopen(buffer, "w");
	if (brightness_f) {
		fprintf(brightness_f, "%d", bright);
		fclose(brightness_f);
		return;
	} else {
		return;
	}
}

//******************************//
//  Tool						//
//******************************//
double reciprocal(double input) {	//check input equal to 0? (1)if no, return input reciprocal (2)if yes, return 0 
	if (input){
		return 1.0/input;
	}
	return 0.0;
}

int AVG(int input[],int size) { 	//return average of input[]
	int sum = 0;
	for (int k = 0; k < size; k++){
		sum += input[k];
	}
	return sum / size;
}

/*void swap(int* a , int* b) {			//swap a and b
	*a ^= *b;
	*b ^= *a;
	*a ^= *b;
}*/

void sort(int* begging , int* end) {
	int *cur = begging + 1;
	while (cur <= end){
		int *test = cur;		
		while (test > begging){
			int *pre = test - 1;
			if (*test < *pre){
				std::swap(*test,*pre);		//revise by hao using <algorithm> library
				test = pre;
				continue;
			} else {
				break;
			}			
		}
		cur++;
	}
}

const char* get_items(char* buffer, int offset) {
	assert(buffer);
	char* p = buffer;
	int len = strlen(buffer);
	int count = 0;
	if (offset == 1 || offset < 1) return p;
	for (int i = 0; i < len; i++) {
		if (*p == ' ') {
			count++;
			if (count == offset - 1) {
				p++;
				break;
			}
		}
		p++;
	}
	return p;
}

void swap_float(float& a, float& b) {
	float tmp;
	tmp = a;
	a = b;
	b = a;
}

void max_heapify(struct thread_utilization_sort_information *thread_sort_matrix, int start, int end) {
	int dad = start;
	int son = dad * 2 + 1;
	while (son <= end) {
		if (son + 1 <= end && thread_sort_matrix[son].utilization < thread_sort_matrix[son + 1].utilization)	son++;
		if (thread_sort_matrix[dad].utilization > thread_sort_matrix[son].utilization) return;
		else {
			std::swap(thread_sort_matrix[dad].pid, thread_sort_matrix[son].pid);
			std::swap(thread_sort_matrix[dad].utilization, thread_sort_matrix[son].utilization);
			dad = son;
			son = dad * 2 + 1;
		}
	}
}

void heap_sort(struct thread_utilization_sort_information *thread_sort_matrix, int len) {
	for (int i = len / 2 - 1; i >= 0; i--) max_heapify(thread_sort_matrix, i, len - 1);
	for (int i = len - 1; i > 0; i--) {
		std::swap(thread_sort_matrix[0].pid, thread_sort_matrix[i].pid);
		std::swap(thread_sort_matrix[0].utilization, thread_sort_matrix[i].utilization);
		max_heapify(thread_sort_matrix, 0, i - 1);
	}
	/*for (int i = 0; i < len; i++) {
		printf("%d, %4f\n", thread_sort_matrix[i].pid, thread_sort_matrix[i].utilization);
	}*/
}

#endif
