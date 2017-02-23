#include "systemmonitor.h"
#include "function.h"

using namespace android;

int cpu_utilization[core_num] = {0};	//each CPU uilization
static unsigned long long int total_cpu_busy_p, total_cpu_time_p;	//total CPU time for total cpu utilization
int cpu_total_utilization = 0;			//total CPU(Core0 ~ Core5) utilization

int main(void) {

	//Initial Variables
	char buffer[100];													//for command use
	static unsigned long long int usage_p[core_num], time_p[core_num];	//each CPU time for each CPU utilization
	int FPS = 0;														//FPS
	double voltage = 0.0;												//voltage
	pid_t foreground_app, background_app;								//foreground and background app pid (background app is used for second process if app has)
	
	//core 0 always on (i start at 1)
	for (int i = 1; i < core_num; i++) {
		if (core_assign[i] == 0) set_cpu_on(i, cpu_off);	//cpu_off -> close the cpu
		if (core_assign[i] == 1) set_cpu_on(i, cpu_on);		//cpu_on -> open the cpu
	}														
	//Initial Settings
	for (int c = 0; c < 6; c++) {					//Set Utilization Initial Time and Core Assign Matrix
		get_cpu_time(c, &usage_p[c], &time_p[c]);	//Initial time for utilization calculation
		core_assign[c] = 0;							//Clear core assign matrix
	}
	get_cpu_time(6, &total_cpu_busy_p, &total_cpu_time_p);

	//Initial State (Set frequency)
	set_cpu_freq(4, big_FL[12]);  			//max: 12
	set_cpu_freq(0, little_FL[8]);			//max: 8
	set_gpu_freq(gpu_FL[5]);				//max: 5

	system("su -c chmod 777 -R /sys/devices/system/cpu/");
	if (get_cpu_on(0)) { 
#if DEFAULT_GOVERNOR_SET == 0
		sprintf(buffer, "echo userspace > %s/cpu0/cpufreq/scaling_governor", cpu_fs);
#elif DEFAULT_GOVERNOR_SET == 1
		sprintf(buffer, "echo performance > %s/cpu0/cpufreq/scaling_governor", cpu_fs);
#elif DEFAULT_GOVERNOR_SET == 2
		sprintf(buffer, "echo powersave > %s/cpu0/cpufreq/scaling_governor", cpu_fs);
#elif DEFAULT_GOVERNOR_SET == 3
		sprintf(buffer, "echo interactive > %s/cpu0/cpufreq/scaling_governor", cpu_fs);
#endif
		system(buffer);
	}
	if (get_cpu_on(4)) {	
#if DEFAULT_GOVERNOR_SET == 0
		sprintf(buffer, "echo userspace > %s/cpu4/cpufreq/scaling_governor", cpu_fs); 
#elif DEFAULT_GOVERNOR_SET == 1
		sprintf(buffer, "echo performance > %s/cpu4/cpufreq/scaling_governor", cpu_fs);
#elif DEFAULT_GOVERNOR_SET == 2
		sprintf(buffer, "echo powersave > %s/cpu4/cpufreq/scaling_governor", cpu_fs);
#elif DEFAULT_GOVERNOR_SET == 3
		sprintf(buffer, "echo interactive > %s/cpu4/cpufreq/scaling_governor", cpu_fs);
#endif
		system(buffer);
	}

#if THERMAL_ENGINE == 0
	system("stop thermal-engine");
#elif THERMAL_ENGINE == 1
	system("start thermal-engine");
#endif

	//Initial Codes
	printf("System Monitor Start!!\n");
	printf("---Related Work Test---\n");	

	printf("Time\t"
		   "Litt_F\tBig_F\tG_F\t"
	       "Load_0\tLoad_1\tLoad_2\tLoad_3\t"
	       "Load_4\tLoad_5\t"
	       "Load_G\t"
	       "FPS\t"
	       "Temp_0\tTemp_1\tTemp_2\tTemp_3\t"
	       "Temp_4\tTemp_5\tTemp_G\tTemp_B\t"
           "V\tI\t"
		   "\n");				

	//System Parameter Setting
	binder = get_surfaceflinger();				//FPS		

	//Algorithm selection
#if algorithm_select != 0
	pthread_t alg;
	int ret_alg;
#endif
#if algorithm_select == 2
	ret_alg = pthread_create(&alg, NULL, related_work_kai, NULL);							//kai
#elif algorithm_select == 3
	ret_alg = pthread_create(&alg, NULL, related_work_kai_with_dispatcher, NULL);			//kai + power dispatcher
#elif algorithm_select == 4
	ret_alg = pthread_create(&alg, NULL, related_work_predictive, NULL);					//predictive
#elif algorithm_select == 5
	ret_alg = pthread_create(&alg, NULL, related_work_migration, NULL);						//migration
#elif algorithm_select == 6
	ret_alg = pthread_create(&alg, NULL, related_work_predictive_with_migration, NULL);		//predictive + migration
#endif
#if algorithm_select != 0
	if (ret_alg != 0) {
		printf ("Create algorithm thread error!\n");
		exit (1);
	}
#endif

	//get_fore_back_app_pid(&foreground_app, &background_app);
	//thread_info_initial(foreground_app, thread_info_1, &total_cpu_busy_p, &total_cpu_time_p);
	//if (background_app != 0) thread_info_initial(background_app, thread_info_2, &total_cpu_busy_p, &total_cpu_time_p);
	//Test Parameter Setting
	//std::vector<int> all_task;
	//int max_task = get_all_task_pid(foreground_app, all_task);
	//int abc = 0;
	//struct thread_utilization_sort_information sort_test[thread_matrix_len];

	for (int i = 0; i < testing_time + alogrithm_sampling_time; i++) {  
				
		//Frequency Setting (for experiments)
		set_cpu_freq(4, big_FL[4]);  			//max: 12
		set_cpu_freq(0, little_FL[5]);			//max: 8
		set_gpu_freq(gpu_FL[0]);				//max: 5

		//Dispather Setting (for experiments)
		/*if (i != 0) {
			abc = 0;
			for (int j = 0; j < max_task; j++) {
				now_pid = parent_pid_app_1_1;
				thread_info_1[abc].utilization = get_thread_util(thread_info_1[abc].pid, &thread_info_1[abc].thread_busy, &thread_info_1[abc].cpu_busy, &thread_info_1[abc].cpu_total);
				if (background_app != 0) {
					now_pid = parent_pid_app_2_1;
					thread_info_2[abc].utilization = get_thread_util(thread_info_2[abc].pid, &thread_info_2[abc].thread_busy, &thread_info_2[abc].cpu_busy, &thread_info_2[abc].cpu_total);
				} 
				abc++;	
			} 
			abc = 0;
			for (int j = 0; j < max_task; j++) {
				printf("1_Pid:%d, Utilization: %f\n", thread_info_1[abc].pid, thread_info_1[abc].utilization);
				if (background_app != 0)	printf("2_Pid:%d, Utilization: %f\n", thread_info_2[abc].pid, thread_info_2[abc].utilization);
				abc++;
			}
			printf("----------------------sort------------------------\n");
			thread_utilization_sort(sort_test, thread_info_1, thread_matrix_len);
			for (int j = thread_matrix_len - 1; j > thread_matrix_len - 6; j--) {
				printf("1_Pid:%d, Utilization:%3f\n", sort_test[j].pid, sort_test[j].utilization);
			}
			thread_utilization_sort(sort_test, thread_info_2, thread_matrix_len);
			for (int j = thread_matrix_len - 1; j > thread_matrix_len - 6; j--) {
				printf("2_Pid:%d, Utilization:%3f\n", sort_test[j].pid, sort_test[j].utilization);
			}
		}
		printf("------------------------------------------------\n");*/

		//All tasks
		//set_all_pid_to_core(foreground_app, core_assign);
		//Main tasks
		//set_main_pid_to_core(foreground_app, core_assign);

		//System Parameter Calculate
		FPS = get_fps(binder);
		voltage = ((double)get_battery(battery_voltage)/1000000);
		//Clear core assign matrix
		//for (int i = 0; i < core_num; i++) core_assign[i] = 0;
		//Update Utilization information
		cpu_total_utilization = get_cpu_util(6, &total_cpu_busy_p, &total_cpu_time_p);
		for (int u = 0; u < core_num; u++) cpu_utilization[u] = get_cpu_util(u, &usage_p[u], &time_p[u]);
		
 		if (i > alogrithm_sampling_time) {
			printf("%d\t"
				   "%d\t%d\t%d\t"
			       "%d\t%d\t%d\t%d\t"
			       "%d\t%d\t"
			       "%d\t"
			       "%d\t"
			       "%d\t%d\t%d\t%d\t"
			       "%d\t%d\t%d\t%d\t"
			       "%.5lf\t%.5lf\t"
			       "\n"
				 , i
			 	 , get_cpu_freq(0), get_cpu_freq(4), get_gpu_freq()
				 , cpu_utilization[0], cpu_utilization[1], cpu_utilization[2], cpu_utilization[3]
				 , cpu_utilization[4], cpu_utilization[5]
				 , get_gpu_util()
				 , FPS
				 , get_temp(0), get_temp(1), get_temp(2), get_temp(3)
				 , get_temp(4), get_temp(5), get_temp(7), get_temp(8)
				 , ((double)get_battery(battery_voltage)/1000000), ((double)get_battery(battery_current)/1000000));
		}
		usleep(1000000);
	}
#if algorithm_select != 0
	pthread_join(alg, NULL);
#endif
	printf("!!!Finish!!!\n");
	return 0;
}
#if algorithm_select == 1
void hao_work(int time, pid_t pid, int now_FPS) {	//only frequency controller now

	//Parameter Setting
	int pre_load = 0;	//predicted load
	static int zero_freq_big = 0, zero_freq_little = 0, zero_freq_gpu = 0;			//big, little gpu frequenct at zero second
	double cal_set_big_freq = 0, cal_set_little_freq = 0, cal_set_gpu_freq = 0;		//calculate set big, little, gpu frequency
	int set_big_freq_level = 0, set_little_freq_level = 0, set_gpu_freq_level = 0;	//set big, little, gpu frequency level
	static int zero_FPS = 0;	//FPS with highest big, little, gpu frequency at zero second 
	double avg_FPS = 0.0;		//average FPS during app working
	int high_FPS = 0;			//highest FPS during app working
	static std::vector<int> FPS_data;	//record FPS data for each time
	int FPS_sum = 0;					//FPS sum for average use
	static double coef_FPS_freq_big = 0.0, coef_FPS_freq_little = 0.0, coef_FPS_freq_gpu = 0.0; 
				  //coefficient (FPS and frequency) in big, little, gpu
	static int oriented_element = -1;	//enum {
										//    big_oriented,
										//    little_oriented,
										//    gpu_oriented
										//};
	static int load_big_record[5] = {0}, load_little_record[5] = {0}, load_gpu_record[5] = {0}; //record load for predict (5:interval)
	int predict_big_load = 0, predict_little_load = 0, predict_gpu_load = 0;				//predict load	
	int accept_big_load = 0, accept_little_load = 0, accept_gpu_load = 0;					//accept load for frequency adjustment	
	static int count = 0;
	static int temp_level[5] = {temp_constraint, 			//temperature level 0
								temp_constraint - 5, 		//			  level 1
								temp_constraint - 10, 		//			  level 2
								temp_constraint - 15, 		//			  level 3
								temp_constraint - 20};		//			  level 4
	int max_temp = 0, max_temp_core = 0;

	//Algorithm	(Initial: from 0 ~ 9s) 
	//			(Working: from 10s ~ end)
	if (time == 0) {
		zero_freq_big = get_cpu_freq(4);		//get big frequency at zero second
		zero_freq_little = get_cpu_freq(0);		//    little
		zero_freq_gpu = get_gpu_freq();			//    gpu

		zero_FPS = now_FPS;						//get FPS at zero second
		high_FPS = now_FPS;						//initial highest FPS
		FPS_data.push_back(now_FPS);			//record FPS

		set_all_pid_to_core(pid, 0);			//put app to big

		set_cpu_freq(4, big_FL[0]);				//set big frequency to minimum
		return;
	}
	else if (time == 1)	{
		coef_FPS_freq_big = (double)(zero_FPS - now_FPS)/(double)(zero_freq_big - get_cpu_freq(4)); //coef(FPS&frequency) big

		set_cpu_freq(4, big_FL[12]);					//set big frequency to maximum

		if (now_FPS > high_FPS) high_FPS = now_FPS;		//check high_FPS
		FPS_data.push_back(now_FPS);					//record FPS
		return;
	}
	else if (time == 2) {
		set_all_pid_to_core(pid, 0);					//put app to little

		if (now_FPS > high_FPS) high_FPS = now_FPS;		//check high_FPS
		FPS_data.push_back(now_FPS);					//record FPS

		set_cpu_freq(0, little_FL[0]);					//set little frequency to minimum
		return;	
	}
	else if (time == 3) {
		coef_FPS_freq_little = (double)(zero_FPS - now_FPS)/(double)(zero_freq_little - get_cpu_freq(0)); //coef(FPS&frequency) little

		set_cpu_freq(0, little_FL[8]);					//set little frequency to maximum

		if (now_FPS > high_FPS) high_FPS = now_FPS;		//check high_FPS
		FPS_data.push_back(now_FPS);					//record FPS
		return;
	}
	else if (time == 4) {
		set_all_pid_to_core(pid, 0);					//put app to big and little

		if (now_FPS > high_FPS) high_FPS = now_FPS;		//check high_FPS
		FPS_data.push_back(now_FPS);					//record FPS

		set_gpu_freq(gpu_FL[0]);						//set gpu frequency to minimum
		return;
	}
	else if (time == 5) {
		coef_FPS_freq_gpu = (double)(zero_FPS - now_FPS)/(double)(zero_freq_gpu - get_gpu_freq());	//coef(FPS&frequency) gpu

		set_gpu_freq(gpu_FL[5]);										//set gpu frequency to maximum

		if (now_FPS > high_FPS) high_FPS = now_FPS;						//check high_FPS
		FPS_data.push_back(now_FPS);									//record FPS

		load_big_record[0] = find_max_util(4, cpu_utilization);			//record big load 
		load_little_record[0] = find_max_util(0, cpu_utilization);		//       little
		load_gpu_record[0] = get_gpu_util();							//       gpu
		return;
	}
	else if (time <= 9) {
		if (time == 9) {	//find oriented element
			if (coef_FPS_freq_big > coef_FPS_freq_little) {
				if (coef_FPS_freq_big > coef_FPS_freq_gpu) oriented_element = big_oriented;
				else oriented_element = gpu_oriented; 
			}
			else {
				if (coef_FPS_freq_little > coef_FPS_freq_gpu) oriented_element = little_oriented;
				else oriented_element = gpu_oriented;
			}			
		}

		if (now_FPS > high_FPS) high_FPS = now_FPS;						//check high_FPS
		FPS_data.push_back(now_FPS);									//record FPS

		count++;
		load_big_record[count] = find_max_util(4, cpu_utilization);		//record big load 
		load_little_record[count] = find_max_util(0, cpu_utilization);	//		 little
		load_gpu_record[count] = get_gpu_util();						//		 gpu
		if (count == 4) count = 0;
		return;			
	}
	else {
		max_temp = get_max_temp(&max_temp_core);						//get maximum temp

		if (now_FPS > high_FPS) high_FPS = now_FPS;						//check high_FPS
		FPS_data.push_back(now_FPS);									//record FPS
		for (std::vector<int>::iterator it = FPS_data.begin(); it != FPS_data.end(); ++it) FPS_sum = FPS_sum + *it;	//summation FPS
		avg_FPS = (double)FPS_sum/(double)(time + 1);					//calculate average FPS

		predict_big_load = 0.3 * load_big_record[4] + 0.25 * load_big_record[3] 
						 + 0.2 * load_big_record[2] + 0.15 * load_big_record[1] + 0.1 * load_big_record[0];	//predict load
		predict_little_load = 0.3 * load_little_record[4] + 0.25 * load_little_record[3] 
							+ 0.2 * load_little_record[2] + 0.15 * load_little_record[1] + 0.1 * load_little_record[0];	//predict load
		predict_gpu_load = 0.3 * load_gpu_record[4] + 0.25 * load_gpu_record[3] 
						 + 0.2 * load_gpu_record[2] + 0.15 * load_gpu_record[1] + 0.1 * load_gpu_record[0];	//predict load
		for (int i = 0; i < 4; i++) {								//renew load record
			load_big_record[i] = load_big_record[i+1];				// big
			load_little_record[i] = load_little_record[i+1];		// little
			load_gpu_record[i] = load_gpu_record[i+1];				// gpu
		}
		load_big_record[4] = find_max_util(4, cpu_utilization);		//record big load 
		load_little_record[4] = find_max_util(0, cpu_utilization);	//		 little
		load_gpu_record[4] = get_gpu_util();						//		 gpu

		if (oriented_element == gpu_oriented) {			//gpu oriented case
			if (now_FPS < avg_FPS) {					//for better FPS purpose
				cal_set_gpu_freq = (double)get_gpu_freq() + ((avg_FPS - (double)now_FPS) / coef_FPS_freq_gpu);
			}
			else if (now_FPS < high_FPS) {
				if (max_temp < temp_level[0]) cal_set_gpu_freq = (double)get_gpu_freq() + 
																 ((high_FPS - (double)now_FPS) / coef_FPS_freq_gpu);
			}

			if (max_temp > temp_level[0]) {				//setting accept load
				accept_big_load = 70;
				accept_little_load = 60;
			}
			else if (max_temp > temp_level[1]) {
				accept_big_load = 60;
				accept_little_load = 50;
			}
			else if (max_temp > temp_level[2]) {
				accept_big_load = 50;
				accept_little_load = 40;
			}
			else if (max_temp > temp_level[3]) {
				accept_big_load = 40;
				accept_little_load = 30;
			}
			else if (max_temp > temp_level[4]) {
				accept_big_load = 30;
				accept_little_load = 20;
			}

			cal_set_big_freq = ((double)accept_big_load/(double)predict_big_load) * (double)get_cpu_freq(4); //calculate freq
			cal_set_little_freq = ((double)accept_little_load/(double)predict_little_load) * (double)get_cpu_freq(0);	
			for (int i = 0; i < gpu_freq_level; i++) {					//find gpu frequency
				if (cal_set_gpu_freq <= (gpu_FL[i] / 1000000)) set_gpu_freq_level = i;	
			}
			for (int i = 0; i < big_freq_level; i++) {					//find big frequency
				if (cal_set_big_freq <= (big_FL[i] / 1000)) set_big_freq_level = i;
			}
			for (int i = 0; i < little_freq_level; i++) {				//find little frequency
				if (cal_set_little_freq <= (little_FL[i] / 1000)) set_little_freq_level = i;
			}
			set_gpu_freq(gpu_FL[set_gpu_freq_level]);				//set gpu frequency
			set_cpu_freq(4, big_FL[set_big_freq_level]);			//set big frequency
			set_cpu_freq(0, little_FL[set_little_freq_level]);		//set little frequency
		}
		else if (oriented_element == big_oriented) {	//big oriented case
			if (now_FPS < avg_FPS) {					//for better FPS purpose
				cal_set_big_freq = (double)get_cpu_freq(4) + ((avg_FPS - (double)now_FPS) / coef_FPS_freq_big);
			}
			else if (now_FPS < high_FPS) {
				if (max_temp < temp_level[0]) cal_set_big_freq = (double)get_cpu_freq(4) + 
																 ((high_FPS - (double)now_FPS) / coef_FPS_freq_big);
			}

			if (max_temp > temp_level[0]) {				//setting accept load
				accept_little_load = 60;
				accept_gpu_load = 60;
			}
			else if (max_temp > temp_level[1]) {
				accept_little_load = 50;
				accept_gpu_load = 50;
			}
			else if (max_temp > temp_level[2]) {
				accept_little_load = 40;
				accept_gpu_load = 40;
			}
			else if (max_temp > temp_level[3]) {
				accept_little_load = 30;
				accept_gpu_load = 30;
			}
			else if (max_temp > temp_level[4]) {
				accept_little_load = 20;
				accept_gpu_load = 20;
			}

			cal_set_little_freq = ((double)accept_little_load/(double)predict_little_load) * (double)get_cpu_freq(0); //calculate freq
			cal_set_gpu_freq = ((double)accept_gpu_load/(double)predict_gpu_load) * (double)get_gpu_freq();
			for (int i = 0; i < gpu_freq_level; i++) {					//find gpu frequency
				if (cal_set_gpu_freq <= (gpu_FL[i] / 1000000)) set_gpu_freq_level = i;	
			}
			for (int i = 0; i < big_freq_level; i++) {					//find big frequency
				if (cal_set_big_freq <= (big_FL[i] / 1000)) set_big_freq_level = i;
			}
			for (int i = 0; i < little_freq_level; i++) {				//find little frequency
				if (cal_set_little_freq <= (little_FL[i] / 1000)) set_little_freq_level = i;
			}
			set_gpu_freq(gpu_FL[set_gpu_freq_level]);				//set gpu frequency
			set_cpu_freq(4, big_FL[set_big_freq_level]);			//set big frequency
			set_cpu_freq(0, little_FL[set_little_freq_level]);		//set little frequency
		}
		else {											//little oriented
			if (now_FPS < avg_FPS) {					//for better FPS purpose
				cal_set_little_freq = (double)get_cpu_freq(0) + ((avg_FPS - (double)now_FPS) / coef_FPS_freq_little);
			}
			else if (now_FPS < high_FPS) {
				if (max_temp < temp_level[0]) cal_set_little_freq = (double)get_cpu_freq(0) + 
																	((high_FPS - (double)now_FPS) / coef_FPS_freq_little);
			}

			if (max_temp > temp_level[0]) {				//setting accept load
				accept_big_load = 70;
				accept_gpu_load = 60;
			}
			else if (max_temp > temp_level[1]) {
				accept_big_load = 60;
				accept_gpu_load = 50;
			}
			else if (max_temp > temp_level[2]) {
				accept_big_load = 50;
				accept_gpu_load = 40;
			}
			else if (max_temp > temp_level[3]) {
				accept_big_load = 40;
				accept_gpu_load = 30;
			}
			else if (max_temp > temp_level[4]) {
				accept_big_load = 30;
				accept_gpu_load = 20;
			}

			cal_set_big_freq = ((double)accept_big_load/(double)predict_big_load) * (double)get_cpu_freq(4); //calculate freq
			cal_set_gpu_freq = ((double)accept_gpu_load/(double)predict_gpu_load) * (double)get_gpu_freq();
			for (int i = 0; i < gpu_freq_level; i++) {					//find gpu frequency
				if (cal_set_gpu_freq <= (gpu_FL[i] / 1000000)) set_gpu_freq_level = i;	
			}
			for (int i = 0; i < big_freq_level; i++) {					//find big frequency
				if (cal_set_big_freq <= (big_FL[i] / 1000)) set_big_freq_level = i;
			}
			for (int i = 0; i < little_freq_level; i++) {				//find little frequency
				if (cal_set_little_freq <= (little_FL[i] / 1000)) set_little_freq_level = i;
			}
			set_gpu_freq(gpu_FL[set_gpu_freq_level]);				//set gpu frequency
			set_cpu_freq(4, big_FL[set_big_freq_level]);			//set big frequency
			set_cpu_freq(0, little_FL[set_little_freq_level]);		//set little frequency
		}
	}
}
#elif algorithm_select == 2
void *related_work_kai(void*) {

	//Related Work Parameter Setting and Initial
	int CPU_Sensitive = 1;
	const int avg_interval = 5;		//average interval
	static int rec_interval = 3;
	const int avg_take = 3;
	
	static double Coef_Uc2Fc = 0, Coef_Q2Fc = 0, Coef_Q2Uc = 0;	//(cpu util/cpu f), (qual/cpu f), (qual/cpu util)
	static double Coef_Ug2Fg = 0, Coef_Q2Fg = 0, Coef_Q2Ug = 0;	//(gpu util/gpu f), (qual/gpu f), (qual/gpu util)
																					
	static int Q_target[avg_interval] = {0}, UC_target[avg_interval] = {0}, UG_target[avg_interval] = {0};	//for average
	static int FC_record[avg_interval] = {0}, FG_record[avg_interval] = {0}; 								//for delta
	static int UC_record[avg_interval] = {0}, UG_record[avg_interval] = {0};								//for delta
	static int Q_record[avg_interval] = {0};																//for delta

	int Uc, Ug;		//CPU and GPU utilization
	int FPS;		//FPS

	for (int tick = 0; tick < testing_time + alogrithm_sampling_time; tick++) {
		FPS = get_fps(binder);
		Uc = 0;
		for (int i = 0; i < core_num; i++) Uc = Uc + cpu_utilization[i]; 
		Ug = get_gpu_util();

		//Sampling
		//At tick == 0 do nothing, because for cpu utilization calculate first
		if (tick == 1) {
			printf("Sample_%d: Fc_b=%d, Fc_l=%d, Fg=%d, Uc=%d, Ug=%d\n", tick
															 		   , get_cpu_freq(4), get_cpu_freq(0), get_gpu_freq()
																	   , Uc, Ug);

			Q_record[tick-1] = Q_target[tick-1] = FPS;		//FPS
			UC_record[tick-1] = UC_target[tick-1] = Uc;		//cpu util
			UG_record[tick-1] = UG_target[tick-1] = Ug;		//gpu util
			FC_record[tick-1] = little_FL[12];				//record max little freq (/1000 -> MHz)
			FG_record[tick-1] = gpu_FL[5];					//record max gpu freq (/1000000 -> MHz)

			set_cpu_freq(4, big_FL[0]);			//set min big freq
			set_cpu_freq(0, little_FL[0]);		//set min little freq
			set_gpu_freq(gpu_FL[5]);			//set max gpu freq
		}
		else if (tick == 2) {			
			set_gpu_freq(gpu_FL[5]);			//set max gpu freq
			printf("Sample_%d: Fc_b=%d, Fc_l=%d, Fg=%d, Uc=%d, Ug=%d\n", tick
															 		   , get_cpu_freq(4), get_cpu_freq(0), get_gpu_freq()
																	   , Uc, Ug);

			Coef_Q2Fc = (double)(little_FL[8] - little_FL[0]) * reciprocal(Q_target[0] - FPS);
			Coef_Uc2Fc = (double)(little_FL[8] - little_FL[0]) * reciprocal(UC_target[0] - Uc);
			Coef_Q2Ug = (double)(UG_target[0] - Ug) * reciprocal(Q_target[0] - FPS);

			Q_record[tick-1] = Q_target[tick-1] = FPS;			//FPS
			UC_record[tick-1] = UC_target[tick-1] = Uc;			//cpu util
			UG_record[tick-1] = UG_target[tick-1] = Ug;			//gpu util
			FC_record[tick-1] = little_FL[0];					//record min little freq (/1000 -> MHz)
			FG_record[tick-1] = gpu_FL[5];					//record max gpu freq (/1000000 -> MHz)

			set_cpu_freq(4, big_FL[12]);		//set max big freq
			set_cpu_freq(0, little_FL[8]);		//set max little freq
			set_gpu_freq(gpu_FL[0]);			//set min gpu freq
		}
		else if (tick == 3) {
			set_gpu_freq(gpu_FL[0]);			//set min gpu freq
			printf("Sample_%d: Fc_b=%d, Fc_l=%d, Fg=%d, Uc=%d, Ug=%d\n", tick
															 		   , get_cpu_freq(4), get_cpu_freq(0), get_gpu_freq()
																	   , Uc, Ug);

			Coef_Q2Fg = (double)(gpu_FL[5] - gpu_FL[0]) * reciprocal(Q_target[0] - FPS);
			Coef_Ug2Fg = (double)(gpu_FL[5] - gpu_FL[0]) * reciprocal(UG_target[0] - Ug);
			Coef_Q2Uc = (double)(UC_target[0] - Uc) * reciprocal(Q_target[0] - FPS);

			Q_record[tick-1] = Q_target[tick-1] = FPS;			//FPS
			UC_record[tick-1] = UC_target[tick-1] = Uc;			//cpu util
			UG_record[tick-1] = UG_target[tick-1] = Ug;			//gpu util
			FC_record[tick-1] = little_FL[12];				//record max little freq (/1000 -> MHz)
			FG_record[tick-1] = gpu_FL[0];					//record max gpu freq (/1000000 -> MHz)

			set_cpu_freq(4, big_FL[12]);		//set max big freq
			set_cpu_freq(0, little_FL[8]);		//set max little freq
			set_gpu_freq(gpu_FL[5]);			//set max gpu freq
		}
		else if (tick == 4) {
			printf("Sample_%d: Fc_b=%d, Fc_l=%d, Fg=%d\n", tick, get_cpu_freq(4), get_cpu_freq(0), get_gpu_freq());
			printf("Q: %d, Uc: %d, Ug: %d\n", AVG(Q_target, avg_take), AVG(UC_target, avg_take), AVG(UG_target, avg_take));			
			printf("Coef_Q2Fc: %f\tCoef_Uc2Fc: %f\tCoef_Q2Ug : %f\n", Coef_Q2Fc, Coef_Uc2Fc, Coef_Q2Ug);			
			printf("Coef_Q2Fg: %f\tCoef_Ug2Fg: %f\tCoef_Q2Uc : %f\n", Coef_Q2Fg, Coef_Ug2Fg, Coef_Q2Uc);
		}
		else {
			//calculate pointers
			rec_interval %= avg_interval;
			int pre0 = rec_interval; 										//current
			int pre1 = (rec_interval + avg_interval - 1) % avg_interval; 	//t-1
			int pre2 = (rec_interval + avg_interval - 2) % avg_interval; 	//t-2		
			//record current status
			scale_state state = SCALE_CHECK;
			static int s_period = 0;
			s_period += scale_period;
			if (s_period >= 1000000){
				Q_record[pre0] = FPS;
				s_period = 0;
			} 
			else {
				int temp = Q_record[avg_interval - 1];
				int mv = avg_interval - 1;
				for( ; mv > 0 ;mv--){
					Q_record[mv] = Q_record[mv - 1];
				}
				Q_record[mv] = temp;
				state = SCALE_U_Based;
			}
			
			UC_record[pre0] = Uc;
			UG_record[pre0] = Ug;

			for(int mv = 0; mv < avg_interval; mv++){
				Q_target[mv] = Q_record[mv];
				UC_target[mv] = UC_record[mv];
				UG_target[mv] = UG_record[mv];
			}			

			sort(Q_target, Q_target + avg_interval - 1);
			sort(UC_target, UC_target + avg_interval - 1);
			sort(UG_target, UG_target + avg_interval - 1);

			//calculate Q target
			int QT = 0;
			int L3 = AVG(Q_target + 2, avg_take);
			int M3 = AVG(Q_target + 1, avg_take);
			int S3 = AVG(Q_target, avg_take);
			if(S3 >= Q_target_set)
				QT = S3;
			else if(M3 >= Q_target_set)
				QT = M3;
			else
				QT = L3;

			//calculate cpu and gpu utilization target
			int UC_T = AVG(UC_target + 2, avg_take); 
			int UG_T = AVG(UG_target + 2, avg_take); 

			//scaling parameter
			static int FL_point = little_freq_level-1;
			static int pre_FL = FL_point;
			static int GFL_point = gpu_freq_level-1;
			static int pre_GFL = GFL_point;		
				
			int FC_Next = FC_record[pre1];
			int FG_Next = FG_record[pre1];
			int UC_Next = 0;
			int UG_Next = 0;
				
			double delta_Q = 0.0;
			double delta_FC = 0.0;
			double delta_Coef_Q2Fc = 0.0;
			double delta_Coef_Uc2Fc = 0.0;
				
			double delta_UG = 0.0; 
			double delta_Coef_Q2Ug = 0.0;
			
			double delta_FG = 0.0;
			double delta_Coef_Ug2Fg = 0.0;
			double delta_Coef_Q2Fg = 0.0;
			
			double delta_UC = 0.0;
			double delta_Coef_Q2Uc = 0.0;	
			
			//cpu or gpu sense
			if(AVG(UG_record, avg_interval) > 80)
				CPU_Sensitive = 0;	//GPU_Sense
			else
				CPU_Sensitive = 1;	//CPU_Sense

			//Q(t)-Q(t-1)
			delta_Q = Q_record[pre0] - Q_record[pre1];
			if (state == SCALE_CHECK) {
				if (Q_record[pre0] - QT < QLB || Q_record[pre0] - QT > QUB) {
					state = SCALE_Q_Based;
					if (Q_record[pre0] == AVG(Q_record, avg_interval)) {
						state = SCALE_U_Based;
					}
				}
				else {
					state = SCALE_U_Based;
				}
			}
			
			if (state == SCALE_Q_Based) {		//SCALE_Q_Based		//dropping or lifting more than 4

				if (CPU_Sensitive) {
					//Coef Learning
					delta_FC = FC_record[pre1] - FC_record[pre2];
					delta_Coef_Q2Fc = ((delta_FC * reciprocal(delta_Q)) - Coef_Q2Fc) * learn_rate; 
					Coef_Q2Fc += delta_Coef_Q2Fc;
					Coef_Q2Fc = abs(Coef_Q2Fc);				
						
					delta_UG = UG_record[pre0] - UG_record[pre1];
					delta_Coef_Q2Ug = ((delta_UG * reciprocal(delta_Q)) - Coef_Q2Ug) * learn_rate;
					Coef_Q2Ug += delta_Coef_Q2Ug;	
					
					delta_FG = FG_record[pre1] - FG_record[pre2];
					delta_Coef_Ug2Fg = ((delta_FG * reciprocal(delta_UG)) - Coef_Ug2Fg) * learn_rate;
					Coef_Ug2Fg += delta_Coef_Ug2Fg;
					Coef_Ug2Fg = -abs(Coef_Ug2Fg);
						
					delta_FG = FG_record[pre1] - FG_record[pre2];
					delta_Coef_Q2Fg = ((delta_FG * reciprocal(delta_Q))-Coef_Q2Fg)*learn_rate; 
					Coef_Q2Fg += delta_Coef_Q2Fg;
					Coef_Q2Fg = abs(Coef_Q2Fg);
						
					//System tuning				
					FC_Next = (int)(FC_record[pre1] + (QT - Q_record[pre0]) * Coef_Q2Fc);			
					UG_Next = UG_record[pre0] + (QT - Q_record[pre0]) * Coef_Q2Ug;
					FG_Next = (int)(FG_record[pre1] + (UG_T - UG_record[pre0]) * Coef_Ug2Fg);	
					
					//Check if Coef is converged
					if (FC_record[0] == AVG(FC_record, avg_interval)) {
						if (Q_record[pre0] - QT < QLB) {
							//Current Q is too low
							//FL+1
							FC_Next = little_FL[pre_FL + 1];						
						}
						else if (Q_record[pre0] - QT > QUB) {
							//Current Q is too high
							//FL-1
							FC_Next = little_FL[pre_FL - 1];						
						}
					}				
				}
				else {
					//Learn FG-Q
					delta_FG = FG_record[pre1] - FG_record[pre2];
					delta_Coef_Q2Fg = ((delta_FG * reciprocal(delta_Q)) - Coef_Q2Fg) * learn_rate; 
					Coef_Q2Fg += delta_Coef_Q2Fg;
					Coef_Q2Fg = abs(Coef_Q2Fg);
						
					//Learn UC-Q			
					delta_UC = UC_record[pre0] - UC_record[pre1];
					delta_Coef_Q2Uc = ((delta_UC * reciprocal(delta_Q)) - Coef_Q2Uc) * learn_rate;
					Coef_Q2Uc += delta_Coef_Q2Uc;		
						
					//Learn UC-FC Coef Coef_Ug2Fg
					delta_FC = FC_record[pre1] - FC_record[pre2];
					delta_Coef_Uc2Fc = ((delta_FC * reciprocal(delta_UC)) - Coef_Uc2Fc) * learn_rate;
					Coef_Uc2Fc += delta_Coef_Uc2Fc;
					Coef_Uc2Fc = -abs(Coef_Uc2Fc);
						
					delta_FC = FC_record[pre1] - FC_record[pre2];
					delta_Coef_Q2Fc = ((delta_FC * reciprocal(delta_Q)) - Coef_Q2Fc) * learn_rate; 				
					Coef_Q2Fc += delta_Coef_Q2Fc;
					Coef_Q2Fc = abs(Coef_Q2Fc);
						
					//System tuning
					FG_Next = (int)(FG_record[pre1] + (QT - Q_record[pre0]) * Coef_Q2Fg);	
					UC_Next = UC_record[pre0] + (QT - Q_record[pre0]) * Coef_Q2Uc;
					FC_Next = (int)(FC_record[pre1]+(UC_T - UC_record[pre0])*Coef_Uc2Fc);	
								
					//Check if Coef is converged
					if (FG_record[0] == AVG(FG_record, avg_interval)) {
						if (Q_record[pre0] - QT < QLB) {
							//Current Q is too low
							//FL+1
							FG_Next = gpu_FL[pre_GFL + 1];						
						}
						else if (Q_record[pre0] - QT > QUB) {
							//Current Q is too high
							//FL-1
							FG_Next = gpu_FL[pre_GFL - 1];						
						}
					}				
				}				
			}
			else {		//SCALE_U_Based		
				
				if (CPU_Sensitive) {//Learning Only
					//Coef Learning
					delta_FC = FC_record[pre1] - FC_record[pre2];
					delta_Coef_Q2Fc = ((delta_FC * reciprocal(delta_Q)) - Coef_Q2Fc) * learn_rate; 
					Coef_Q2Fc += delta_Coef_Q2Fc;
					Coef_Q2Fc = abs(Coef_Q2Fc);
						
					delta_UG = UG_record[pre0] - UG_record[pre1];
					delta_Coef_Q2Ug = ((delta_UG * reciprocal(delta_Q)) - Coef_Q2Ug) * learn_rate;
					Coef_Q2Ug += delta_Coef_Q2Ug;	
						
					delta_FG = FG_record[pre1] - FG_record[pre2];
					delta_Coef_Ug2Fg = ((delta_FG * reciprocal(delta_UG)) - Coef_Ug2Fg) * learn_rate;
					Coef_Ug2Fg += delta_Coef_Ug2Fg;
					Coef_Ug2Fg = -abs(Coef_Ug2Fg);		
						
					delta_FG = FG_record[pre1] - FG_record[pre2];
					delta_Coef_Q2Fg = ((delta_FG * reciprocal(delta_Q)) - Coef_Q2Fg) * learn_rate; 
					Coef_Q2Fg += delta_Coef_Q2Fg;
					Coef_Q2Fg = abs(Coef_Q2Fg);
						
					FG_Next = (int)(FG_record[pre1]+(UG_T - UG_record[pre0]) * Coef_Ug2Fg);
					int pd_Q = (FG_Next - FG_record[pre1]) * reciprocal(Coef_Q2Fg);
					if (pd_Q <= -4) {
						FG_Next = FG_record[pre1];
					}
					else if (-4 <= pd_Q && pd_Q <= 4) {
						if((FG_Next - FG_record[pre1]) > 0)
							FG_Next = FG_record[pre1];
					}
					
					//Check if Coef is converged
					if (FG_record[0] == AVG(FG_record, avg_interval)) {
						if (UG_T < UG_record[pre0]) {
							//Current UG is too high
							//FG_Next = GFL[pre_GFL+1];						
						}
						else if (UG_T > UG_record[pre0]) {
							//Current UG is too low
							FG_Next = gpu_FL[pre_GFL-1];						
						}
					}
				}
				else {
					//Learn FG-Q
					delta_FG = FG_record[pre1] - FG_record[pre2];
					delta_Coef_Q2Fg = ((delta_FG * reciprocal(delta_Q)) - Coef_Q2Fg) * learn_rate; 
					Coef_Q2Fg += delta_Coef_Q2Fg;
					Coef_Q2Fg = abs(Coef_Q2Fg);
						
					//Learn UC-Q			
					delta_UC = UC_record[pre0] - UC_record[pre1];
					delta_Coef_Q2Uc = ((delta_UC * reciprocal(delta_Q)) - Coef_Q2Uc) * learn_rate;
					Coef_Q2Uc += delta_Coef_Q2Uc;		
					
					//Learn UC-FC Coef Coef_Ug2Fg
					delta_FC = FC_record[pre1] - FC_record[pre2];
					delta_Coef_Uc2Fc = ((delta_FC * reciprocal(delta_UC)) - Coef_Uc2Fc) * learn_rate;
					Coef_Uc2Fc += delta_Coef_Uc2Fc;
					Coef_Uc2Fc = -abs(Coef_Uc2Fc);	
						
					delta_FC = FC_record[pre1] - FC_record[pre2];
					delta_Coef_Q2Fc = ((delta_FC * reciprocal(delta_Q)) - Coef_Q2Fc) * learn_rate; 
					Coef_Q2Fc += delta_Coef_Q2Fc;
					Coef_Q2Fc = abs(Coef_Q2Fc);
						
					FC_Next = (int)(FC_record[pre1]+(UC_T - UC_record[pre0]) * Coef_Uc2Fc);
					int pd_Q = (FC_Next - FC_record[pre1]) * reciprocal(Coef_Q2Fc);
					if (pd_Q <= -4) {
						FC_Next = FC_record[pre1];
					}
					else if (-4 <= pd_Q && pd_Q <= 4) {
						if((FC_Next - FC_record[pre1]) > 0)
							FC_Next = FC_record[pre1];
					}

					//Check if Coef is converged
					if (FC_record[0] == AVG(FC_record, avg_interval)) {
						if (UC_T < UC_record[pre0]) {
							//Current UC is too high
							//FC_Next = FL[pre_FL+1];						
						}
						else if (UC_T > UC_record[pre0]) {
							//Current UC is too low
							FC_Next = little_FL[pre_FL-1];						
						}
					}
				}
			}

			//find the fitness little and big frequency level
			FL_point = 0;		
			int FL_point_b = 0; 
			while (FL_point < little_freq_level && little_FL[FL_point] < FC_Next) FL_point++;
			while (FL_point_b < big_freq_level && big_FL[FL_point_b] < FC_Next) FL_point_b++;		
			//find the fitness gpu frequency level
			GFL_point = 0;		
			while (GFL_point < gpu_freq_level && gpu_FL[GFL_point] < FG_Next) GFL_point++;

			if (FL_point != pre_FL) {		
				if (FL_point > little_freq_level-1) {
					FL_point = little_freq_level-1;
				}
				else if (FL_point < 0) {
					FL_point = 0;
				}		
				pre_FL = FL_point;
				set_cpu_freq(0, little_FL[FL_point]);			
				if (FL_point_b > big_freq_level - 1) {
					FL_point_b = big_freq_level - 1;
				}
				else if (FL_point_b < 0) {
					FL_point_b = 0;
				}
				set_cpu_freq(4, big_FL[FL_point_b]);
			}
			if (GFL_point != pre_GFL) {			
				if(GFL_point > gpu_freq_level-1){
					GFL_point = gpu_freq_level-1;
				}
				else if (GFL_point < 0) {
					GFL_point = 0;
				}		
				pre_GFL = GFL_point;
				set_gpu_freq(gpu_FL[GFL_point]);			
			}	

			//Restore current
			FC_record[pre0] = little_FL[FL_point];
			FG_record[pre0] = gpu_FL[GFL_point];

			rec_interval++;
		}

		usleep(1000000);
	}
	return 0;	
}
#elif algorithm_select == 3
void *related_work_kai_with_dispatcher(void*) {

	//Related Work Parameter Setting and Initial
	int CPU_Sensitive = 1;
	const int avg_interval = 5;		//average interval
	static int rec_interval = 3;
	const int avg_take = 3;
	
	static double Coef_Uc2Fc = 0, Coef_Q2Fc = 0, Coef_Q2Uc = 0;	//(cpu util/cpu f), (qual/cpu f), (qual/cpu util)
	static double Coef_Ug2Fg = 0, Coef_Q2Fg = 0, Coef_Q2Ug = 0;	//(gpu util/gpu f), (qual/gpu f), (qual/gpu util)
																					
	static int Q_target[avg_interval] = {0}, UC_target[avg_interval] = {0}, UG_target[avg_interval] = {0};	//for average
	static int FC_record[avg_interval] = {0}, FG_record[avg_interval] = {0}; 								//for delta
	static int UC_record[avg_interval] = {0}, UG_record[avg_interval] = {0};								//for delta
	static int Q_record[avg_interval] = {0};																//for delta

	int Uc, Ug;
	int FPS;

	//Dispatcher by Hao Parameter Setting
	pid_t foreground_app;		//foreground app pid
	pid_t background_app;		//background app pid
	int hottest_core = 0;
	
	for (int tick = 0; tick < testing_time + alogrithm_sampling_time; tick++) {
	
		FPS = get_fps(binder);
		Uc = 0;
		for (int i = 0; i < core_num; i++) Uc = Uc + cpu_utilization[i]; 
		Ug = get_gpu_util();

		//Sampling
		//At tick == 0 do nothing, because for cpu utilization calculate first
		//Kai Algorithm Start
		//Kai Algorithm Sampling (1 ~ 4 s) (frequency)
		if (tick == 0) {
			set_cpu_freq(4, big_FL[12]);		//set max big freq
			set_cpu_freq(0, little_FL[8]);		//set max little freq
			set_gpu_freq(gpu_FL[5]);			//set max gpu freq
			get_fore_back_app_pid(&foreground_app, &background_app);	//get foreground and background app pid
			for (int j = 0; j < core_num; j++) core_assign[j] = 1;
			set_all_pid_to_core(foreground_app, core_assign);			//set foreground app to big and little
			if (background_app != 0) set_all_pid_to_core(background_app, core_assign);
		}
		else if (tick == 1) {
			/*get_fore_back_app_pid(&foreground_app, &background_app);	//get foreground and background app pid
			for (int j = 0; j < core_num; j++) core_assign[j] = 1;
			set_all_pid_to_core(foreground_app, core_assign);			//set foreground app to big and little
			if (background_app != 0) set_all_pid_to_core(background_app, core_assign);*/
			set_gpu_freq(gpu_FL[5]);			//set max gpu freq
			printf("Sample_%d: Fc_b=%d, Fc_l=%d, Fg=%d, Uc=%d, Ug=%d, FPS=%d\n", tick
															 		   , get_cpu_freq(4), get_cpu_freq(0), get_gpu_freq()
																	   , Uc, Ug
																	   , FPS);

			Q_record[tick-1] = Q_target[tick-1] = FPS;		//FPS
			UC_record[tick-1] = UC_target[tick-1] = Uc;		//cpu util
			UG_record[tick-1] = UG_target[tick-1] = Ug;		//gpu util
			FC_record[tick-1] = little_FL[12];				//record max little freq (/1000 -> MHz)
			FG_record[tick-1] = gpu_FL[5];					//record max gpu freq (/1000000 -> MHz)

			set_cpu_freq(4, big_FL[0]);			//set min big freq
			set_cpu_freq(0, little_FL[0]);		//set min little freq
			set_gpu_freq(gpu_FL[5]);			//set max gpu freq
		}
		else if (tick == 2) {			
			set_gpu_freq(gpu_FL[5]);			//set max gpu freq
			printf("Sample_%d: Fc_b=%d, Fc_l=%d, Fg=%d, Uc=%d, Ug=%d, FPS=%d\n", tick
															 		   , get_cpu_freq(4), get_cpu_freq(0), get_gpu_freq()
																	   , Uc, Ug
																	   , FPS);									

			Coef_Q2Fc = (double)(little_FL[8] - little_FL[0]) * reciprocal(Q_target[0] - FPS);
			Coef_Uc2Fc = (double)(little_FL[8] - little_FL[0]) * reciprocal(UC_target[0] - Uc);
			Coef_Q2Ug = (double)(UG_target[0] - Ug) * reciprocal(Q_target[0] - FPS);

			Q_record[tick-1] = Q_target[tick-1] = FPS;			//FPS
			UC_record[tick-1] = UC_target[tick-1] = Uc;			//cpu util
			UG_record[tick-1] = UG_target[tick-1] = Ug;			//gpu util
			FC_record[tick-1] = little_FL[0];					//record min little freq (/1000 -> MHz)
			FG_record[tick-1] = gpu_FL[5];					//record max gpu freq (/1000000 -> MHz)

			set_cpu_freq(4, big_FL[12]);		//set max big freq
			set_cpu_freq(0, little_FL[8]);		//set max little freq
			set_gpu_freq(gpu_FL[0]);			//set min gpu freq
		}
		else if (tick == 3) {
			set_gpu_freq(gpu_FL[0]);			//set min gpu freq
			printf("Sample_%d: Fc_b=%d, Fc_l=%d, Fg=%d, Uc=%d, Ug=%d, FPS=%d\n", tick
															 		   , get_cpu_freq(4), get_cpu_freq(0), get_gpu_freq()
																	   , Uc, Ug
																	   , FPS);

			Coef_Q2Fg = (double)(gpu_FL[5] - gpu_FL[0]) * reciprocal(Q_target[0] - FPS);
			Coef_Ug2Fg = (double)(gpu_FL[5] - gpu_FL[0]) * reciprocal(UG_target[0] - Ug);
			Coef_Q2Uc = (double)(UC_target[0] - Uc) * reciprocal(Q_target[0] - FPS);

			Q_record[tick-1] = Q_target[tick-1] = FPS;			//FPS
			UC_record[tick-1] = UC_target[tick-1] = Uc;			//cpu util
			UG_record[tick-1] = UG_target[tick-1] = Ug;			//gpu util
			FC_record[tick-1] = little_FL[12];				//record max little freq (/1000 -> MHz)
			FG_record[tick-1] = gpu_FL[0];					//record max gpu freq (/1000000 -> MHz)

			set_cpu_freq(4, big_FL[12]);		//set max big freq
			set_cpu_freq(0, little_FL[8]);		//set max little freq
			set_gpu_freq(gpu_FL[5]);			//set max gpu freq
		}
		else if (tick == 4) {
			printf("Sample_%d: Fc_b=%d, Fc_l=%d, Fg=%d\n", tick, get_cpu_freq(4), get_cpu_freq(0), get_gpu_freq());
			printf("AVG_Q: %d, AVG_Uc: %d, AVG_Ug: %d\n", AVG(Q_target, avg_take), AVG(UC_target, avg_take), AVG(UG_target, avg_take));			
			printf("Coef_Q2Fc: %f\tCoef_Uc2Fc: %f\tCoef_Q2Ug : %f\n", Coef_Q2Fc, Coef_Uc2Fc, Coef_Q2Ug);			
			printf("Coef_Q2Fg: %f\tCoef_Ug2Fg: %f\tCoef_Q2Uc : %f\n", Coef_Q2Fg, Coef_Ug2Fg, Coef_Q2Uc);
			printf("---------------Frequency Sampling Finish---------------\n");
			//Dispatcher by Hao Sampling
			set_cpu_freq(4, big_FL[4]);
			set_cpu_freq(0, little_FL[5]);
			set_gpu_freq(gpu_FL[0]);
			
			get_fore_back_app_pid(&foreground_app, &background_app);
			core_assign[4] = 1;
			core_assign[5] = 1;
			set_all_pid_to_core(foreground_app, core_assign);
			if (background_app != 0) set_all_pid_to_core(background_app, core_assign);
		}
		else if (tick == 5 || tick == 6 || tick == 7) {
			set_gpu_freq(gpu_FL[0]);

			/*get_fore_back_app_pid(&foreground_app, &background_app);
			core_assign[4] = 1;
			core_assign[5] = 1;
			set_main_pid_to_core(foreground_app, core_assign);
			if (background_app != 0) set_main_pid_to_core(background_app, core_assign);*/

			printf("Sample_%d: Fc_b=%d, Fc_l=%d, Fg=%d\n", tick
													  	 , get_cpu_freq(4), get_cpu_freq(0), get_gpu_freq());
			get_max_temp(&hottest_core);
			FPS = get_fps(binder);
			printf("All in Big - FPS: %d - Hot Core:%d - Temp: %d\n", FPS, hottest_core, get_temp(hottest_core));
		}
		else if (tick == 8) { 
			get_fore_back_app_pid(&foreground_app, &background_app);
			core_assign[0] = 1;
			core_assign[1] = 1;
			core_assign[2] = 1;
			core_assign[3] = 1;
			set_all_pid_to_core(foreground_app, core_assign);
			if (background_app != 0) set_all_pid_to_core(background_app, core_assign);
		}
		else if (tick == 9 || tick == 10 || tick == 11) { //(tick == 8 || tick == 9 || tick == 10) {
			set_gpu_freq(gpu_FL[0]);

			/*get_fore_back_app_pid(&foreground_app, &background_app);
			core_assign[0] = 1;
			core_assign[1] = 1;
			core_assign[2] = 1;
			core_assign[3] = 1;
			set_main_pid_to_core(foreground_app, core_assign);
			if (background_app != 0) set_main_pid_to_core(background_app, core_assign);*/

			printf("Sample_%d: Fc_b=%d, Fc_l=%d, Fg=%d\n", tick
														 , get_cpu_freq(4), get_cpu_freq(0), get_gpu_freq());
			get_max_temp(&hottest_core);
			FPS = get_fps(binder);
			printf("All in Little - FPS: %d - Hot Core:%d - Temp: %d\n", FPS, hottest_core, get_temp(hottest_core));
		}
		else if (tick == 12) { 
			get_fore_back_app_pid(&foreground_app, &background_app);
			//core_assign[4] = 1;
			core_assign[1] = 1;
			core_assign[3] = 1;
			set_all_pid_to_core(foreground_app, core_assign);
			if (background_app != 0) set_all_pid_to_core(background_app, core_assign);
		}
		else if (tick == 13 || tick == 14 || tick == 15) {
			set_gpu_freq(gpu_FL[0]);

			/*get_fore_back_app_pid(&foreground_app, &background_app);
			core_assign[4] = 1;
			set_main_pid_to_core(foreground_app, core_assign);
			if (background_app != 0) set_main_pid_to_core(background_app, core_assign);*/

			printf("Sample_%d: Fc_b=%d, Fc_l=%d, Fg=%d\n", tick
														 , get_cpu_freq(4), get_cpu_freq(0), get_gpu_freq());
			get_max_temp(&hottest_core);
			FPS = get_fps(binder);
			//printf("All in Core 4 - FPS: %d - Hot Core:%d - Temp: %d\n", FPS, hottest_core, get_temp(hottest_core));
			printf("All in Core 1,3 - FPS: %d - Hot Core:%d - Temp: %d\n", FPS, hottest_core, get_temp(hottest_core));
		}
		else if (tick == 16) { 
			get_fore_back_app_pid(&foreground_app, &background_app);
			core_assign[1] = 1;
			set_all_pid_to_core(foreground_app, core_assign);
			if (background_app != 0) set_all_pid_to_core(background_app, core_assign);
		}
		else if (tick == 17 || tick == 18 || tick == 19) {
			set_gpu_freq(gpu_FL[0]);

			/*get_fore_back_app_pid(&foreground_app, &background_app);
			core_assign[1] = 1;
			set_main_pid_to_core(foreground_app, core_assign);
			if (background_app != 0) set_main_pid_to_core(background_app, core_assign);*/

			printf("Sample_%d: Fc_b=%d, Fc_l=%d, Fg=%d\n", tick
														 , get_cpu_freq(4), get_cpu_freq(0), get_gpu_freq());
			get_max_temp(&hottest_core);
			FPS = get_fps(binder);
			printf("All in Core 1 - FPS: %d - Hot Core:%d - Temp: %d\n", FPS, hottest_core, get_temp(hottest_core));
		}
		else {
			//calculate pointers
			rec_interval %= avg_interval;
			int pre0 = rec_interval; 										//current
			int pre1 = (rec_interval + avg_interval - 1) % avg_interval; 	//t-1
			int pre2 = (rec_interval + avg_interval - 2) % avg_interval; 	//t-2		
			//record current status
			scale_state state = SCALE_CHECK;
			static int s_period = 0;
			s_period += scale_period;
			if (s_period >= 1000000){
				Q_record[pre0] = FPS;
				s_period = 0;
			} 
			else {
				int temp = Q_record[avg_interval - 1];
				int mv = avg_interval - 1;
				for( ; mv > 0 ;mv--){
					Q_record[mv] = Q_record[mv - 1];
				}
				Q_record[mv] = temp;
				state = SCALE_U_Based;
			}
			
			UC_record[pre0] = Uc;
			UG_record[pre0] = Ug;

			for(int mv = 0; mv < avg_interval; mv++){
				Q_target[mv] = Q_record[mv];
				UC_target[mv] = UC_record[mv];
				UG_target[mv] = UG_record[mv];
			}			

			sort(Q_target, Q_target + avg_interval - 1);
			sort(UC_target, UC_target + avg_interval - 1);
			sort(UG_target, UG_target + avg_interval - 1);

			//calculate Q target
			int QT = 0;
			int L3 = AVG(Q_target + 2, avg_take);
			int M3 = AVG(Q_target + 1, avg_take);
			int S3 = AVG(Q_target, avg_take);
			if(S3 >= Q_target_set)
				QT = S3;
			else if(M3 >= Q_target_set)
				QT = M3;
			else
				QT = L3;

			//calculate cpu and gpu utilization target
			int UC_T = AVG(UC_target + 2, avg_take); 
			int UG_T = AVG(UG_target + 2, avg_take); 

			//scaling parameter
			static int FL_point = little_freq_level-1;
			static int pre_FL = FL_point;
			static int GFL_point = gpu_freq_level-1;
			static int pre_GFL = GFL_point;		
				
			int FC_Next = FC_record[pre1];
			int FG_Next = FG_record[pre1];
			int UC_Next = 0;
			int UG_Next = 0;
				
			double delta_Q = 0.0;
			double delta_FC = 0.0;
			double delta_Coef_Q2Fc = 0.0;
			double delta_Coef_Uc2Fc = 0.0;
				
			double delta_UG = 0.0; 
			double delta_Coef_Q2Ug = 0.0;
			
			double delta_FG = 0.0;
			double delta_Coef_Ug2Fg = 0.0;
			double delta_Coef_Q2Fg = 0.0;
			
			double delta_UC = 0.0;
			double delta_Coef_Q2Uc = 0.0;	
			
			//cpu or gpu sense
			if(AVG(UG_record, avg_interval) > 80)
				CPU_Sensitive = 0;	//GPU_Sense
			else
				CPU_Sensitive = 1;	//CPU_Sense

			//Q(t)-Q(t-1)
			delta_Q = Q_record[pre0] - Q_record[pre1];
			if (state == SCALE_CHECK) {
				if (Q_record[pre0] - QT < QLB || Q_record[pre0] - QT > QUB) {
					state = SCALE_Q_Based;
					if (Q_record[pre0] == AVG(Q_record, avg_interval)) {
						state = SCALE_U_Based;
					}
				}
				else {
					state = SCALE_U_Based;
				}
			}
			
			if (state == SCALE_Q_Based) {		//SCALE_Q_Based		//dropping or lifting more than 4

				if (CPU_Sensitive) {
					//Coef Learning
					delta_FC = FC_record[pre1] - FC_record[pre2];
					delta_Coef_Q2Fc = ((delta_FC * reciprocal(delta_Q)) - Coef_Q2Fc) * learn_rate; 
					Coef_Q2Fc += delta_Coef_Q2Fc;
					Coef_Q2Fc = abs(Coef_Q2Fc);				
						
					delta_UG = UG_record[pre0] - UG_record[pre1];
					delta_Coef_Q2Ug = ((delta_UG * reciprocal(delta_Q)) - Coef_Q2Ug) * learn_rate;
					Coef_Q2Ug += delta_Coef_Q2Ug;	
					
					delta_FG = FG_record[pre1] - FG_record[pre2];
					delta_Coef_Ug2Fg = ((delta_FG * reciprocal(delta_UG)) - Coef_Ug2Fg) * learn_rate;
					Coef_Ug2Fg += delta_Coef_Ug2Fg;
					Coef_Ug2Fg = -abs(Coef_Ug2Fg);
						
					delta_FG = FG_record[pre1] - FG_record[pre2];
					delta_Coef_Q2Fg = ((delta_FG * reciprocal(delta_Q))-Coef_Q2Fg)*learn_rate; 
					Coef_Q2Fg += delta_Coef_Q2Fg;
					Coef_Q2Fg = abs(Coef_Q2Fg);
						
					//System tuning				
					FC_Next = (int)(FC_record[pre1] + (QT - Q_record[pre0]) * Coef_Q2Fc);			
					UG_Next = UG_record[pre0] + (QT - Q_record[pre0]) * Coef_Q2Ug;
					FG_Next = (int)(FG_record[pre1] + (UG_T - UG_record[pre0]) * Coef_Ug2Fg);	
					
					//Check if Coef is converged
					if (FC_record[0] == AVG(FC_record, avg_interval)) {
						if (Q_record[pre0] - QT < QLB) {
							//Current Q is too low
							//FL+1
							FC_Next = little_FL[pre_FL + 1];						
						}
						else if (Q_record[pre0] - QT > QUB) {
							//Current Q is too high
							//FL-1
							FC_Next = little_FL[pre_FL - 1];						
						}
					}				
				}
				else {
					//Learn FG-Q
					delta_FG = FG_record[pre1] - FG_record[pre2];
					delta_Coef_Q2Fg = ((delta_FG * reciprocal(delta_Q)) - Coef_Q2Fg) * learn_rate; 
					Coef_Q2Fg += delta_Coef_Q2Fg;
					Coef_Q2Fg = abs(Coef_Q2Fg);
						
					//Learn UC-Q			
					delta_UC = UC_record[pre0] - UC_record[pre1];
					delta_Coef_Q2Uc = ((delta_UC * reciprocal(delta_Q)) - Coef_Q2Uc) * learn_rate;
					Coef_Q2Uc += delta_Coef_Q2Uc;		
						
					//Learn UC-FC Coef Coef_Ug2Fg
					delta_FC = FC_record[pre1] - FC_record[pre2];
					delta_Coef_Uc2Fc = ((delta_FC * reciprocal(delta_UC)) - Coef_Uc2Fc) * learn_rate;
					Coef_Uc2Fc += delta_Coef_Uc2Fc;
					Coef_Uc2Fc = -abs(Coef_Uc2Fc);
						
					delta_FC = FC_record[pre1] - FC_record[pre2];
					delta_Coef_Q2Fc = ((delta_FC * reciprocal(delta_Q)) - Coef_Q2Fc) * learn_rate; 				
					Coef_Q2Fc += delta_Coef_Q2Fc;
					Coef_Q2Fc = abs(Coef_Q2Fc);
						
					//System tuning
					FG_Next = (int)(FG_record[pre1] + (QT - Q_record[pre0]) * Coef_Q2Fg);	
					UC_Next = UC_record[pre0] + (QT - Q_record[pre0]) * Coef_Q2Uc;
					FC_Next = (int)(FC_record[pre1]+(UC_T - UC_record[pre0])*Coef_Uc2Fc);	
								
					//Check if Coef is converged
					if (FG_record[0] == AVG(FG_record, avg_interval)) {
						if (Q_record[pre0] - QT < QLB) {
							//Current Q is too low
							//FL+1
							FG_Next = gpu_FL[pre_GFL + 1];						
						}
						else if (Q_record[pre0] - QT > QUB) {
							//Current Q is too high
							//FL-1
							FG_Next = gpu_FL[pre_GFL - 1];						
						}
					}				
				}				
			}
			else {		//SCALE_U_Based		
				
				if (CPU_Sensitive) {//Learning Only
					//Coef Learning
					delta_FC = FC_record[pre1] - FC_record[pre2];
					delta_Coef_Q2Fc = ((delta_FC * reciprocal(delta_Q)) - Coef_Q2Fc) * learn_rate; 
					Coef_Q2Fc += delta_Coef_Q2Fc;
					Coef_Q2Fc = abs(Coef_Q2Fc);
						
					delta_UG = UG_record[pre0] - UG_record[pre1];
					delta_Coef_Q2Ug = ((delta_UG * reciprocal(delta_Q)) - Coef_Q2Ug) * learn_rate;
					Coef_Q2Ug += delta_Coef_Q2Ug;	
						
					delta_FG = FG_record[pre1] - FG_record[pre2];
					delta_Coef_Ug2Fg = ((delta_FG * reciprocal(delta_UG)) - Coef_Ug2Fg) * learn_rate;
					Coef_Ug2Fg += delta_Coef_Ug2Fg;
					Coef_Ug2Fg = -abs(Coef_Ug2Fg);		
						
					delta_FG = FG_record[pre1] - FG_record[pre2];
					delta_Coef_Q2Fg = ((delta_FG * reciprocal(delta_Q)) - Coef_Q2Fg) * learn_rate; 
					Coef_Q2Fg += delta_Coef_Q2Fg;
					Coef_Q2Fg = abs(Coef_Q2Fg);
						
					FG_Next = (int)(FG_record[pre1]+(UG_T - UG_record[pre0]) * Coef_Ug2Fg);
					int pd_Q = (FG_Next - FG_record[pre1]) * reciprocal(Coef_Q2Fg);
					if (pd_Q <= -4) {
						FG_Next = FG_record[pre1];
					}
					else if (-4 <= pd_Q && pd_Q <= 4) {
						if((FG_Next - FG_record[pre1]) > 0)
							FG_Next = FG_record[pre1];
					}
					
					//Check if Coef is converged
					if (FG_record[0] == AVG(FG_record, avg_interval)) {
						if (UG_T < UG_record[pre0]) {
							//Current UG is too high
							//FG_Next = GFL[pre_GFL+1];						
						}
						else if (UG_T > UG_record[pre0]) {
							//Current UG is too low
							FG_Next = gpu_FL[pre_GFL-1];						
						}
					}
				}
				else {
					//Learn FG-Q
					delta_FG = FG_record[pre1] - FG_record[pre2];
					delta_Coef_Q2Fg = ((delta_FG * reciprocal(delta_Q)) - Coef_Q2Fg) * learn_rate; 
					Coef_Q2Fg += delta_Coef_Q2Fg;
					Coef_Q2Fg = abs(Coef_Q2Fg);
						
					//Learn UC-Q			
					delta_UC = UC_record[pre0] - UC_record[pre1];
					delta_Coef_Q2Uc = ((delta_UC * reciprocal(delta_Q)) - Coef_Q2Uc) * learn_rate;
					Coef_Q2Uc += delta_Coef_Q2Uc;		
					
					//Learn UC-FC Coef Coef_Ug2Fg
					delta_FC = FC_record[pre1] - FC_record[pre2];
					delta_Coef_Uc2Fc = ((delta_FC * reciprocal(delta_UC)) - Coef_Uc2Fc) * learn_rate;
					Coef_Uc2Fc += delta_Coef_Uc2Fc;
					Coef_Uc2Fc = -abs(Coef_Uc2Fc);	
						
					delta_FC = FC_record[pre1] - FC_record[pre2];
					delta_Coef_Q2Fc = ((delta_FC * reciprocal(delta_Q)) - Coef_Q2Fc) * learn_rate; 
					Coef_Q2Fc += delta_Coef_Q2Fc;
					Coef_Q2Fc = abs(Coef_Q2Fc);
						
					FC_Next = (int)(FC_record[pre1]+(UC_T - UC_record[pre0]) * Coef_Uc2Fc);
					int pd_Q = (FC_Next - FC_record[pre1]) * reciprocal(Coef_Q2Fc);
					if (pd_Q <= -4) {
						FC_Next = FC_record[pre1];
					}
					else if (-4 <= pd_Q && pd_Q <= 4) {
						if((FC_Next - FC_record[pre1]) > 0)
							FC_Next = FC_record[pre1];
					}

					//Check if Coef is converged
					if (FC_record[0] == AVG(FC_record, avg_interval)) {
						if (UC_T < UC_record[pre0]) {
							//Current UC is too high
							//FC_Next = FL[pre_FL+1];						
						}
						else if (UC_T > UC_record[pre0]) {
							//Current UC is too low
							FC_Next = little_FL[pre_FL-1];						
						}
					}
				}
			}

			//find the fitness little and big frequency level
			FL_point = 0;		
			int FL_point_b = 0; 
			while (FL_point < little_freq_level && little_FL[FL_point] < FC_Next) FL_point++;
			while (FL_point_b < big_freq_level && big_FL[FL_point_b] < FC_Next) FL_point_b++;		
			//find the fitness gpu frequency level
			GFL_point = 0;		
			while (GFL_point < gpu_freq_level && gpu_FL[GFL_point] < FG_Next) GFL_point++;

			if (FL_point > little_freq_level-1) {
					FL_point = little_freq_level-1;
			}
			else if (FL_point < 0) {
				FL_point = 0;
			}
			if (FL_point_b > big_freq_level - 1) {
				FL_point_b = big_freq_level - 1;
			}
			else if (FL_point_b < 0) {
				FL_point_b = 0;
			}
			//Kai Algorithm End

			//Dispatcher by Hao Algorithm Start
			double b_one_core_power = 0.0, b_two_core_power = 0.0;
			double l_one_core_power = 0.0, l_two_core_power = 0.0, l_four_core_power = 0.0;
			double b_target_one_frequency = 0.0, b_target_two_frequency = 0.0;
			double l_target_one_frequency = 0.0, l_target_two_frequency = 0.0, l_target_four_frequency = 0.0;
			int b_set_frequency = 0, l_set_frequency = 0;
			int b_core_nums = 0, l_core_nums = 0;
			double voltage = ((double)get_battery(battery_voltage)/1000000);
			for (int i = 0; i < 6; i++) {
				if (get_cpu_on(i) == 1) {
					if (i <= 3) l_core_nums++;
					else b_core_nums++;
				}
			}
			//big core determine
			if (b_core_nums == 1) {
				//one core power
				b_target_one_frequency = big_FL[FL_point_b];
				b_one_core_power = total_big_power(1, big_FL[FL_point_b], voltage); 		
				//two core power
				equal_computing_cycles(b_core_nums, big_FL[FL_point_b], 2, &b_target_two_frequency);
				for (int i = 0; i < big_freq_level; i++) {
					if (b_target_two_frequency > big_FL[big_freq_level - 1]) {
						b_target_two_frequency = big_FL[big_freq_level - 1];
						break;
					}
					if (b_target_two_frequency < big_FL[i]) {
						b_target_two_frequency = big_FL[i];
						break;
					}
				}
				b_two_core_power = total_big_power(2, b_target_two_frequency, voltage);
			}
			else {
				//two core power
				b_target_two_frequency = big_FL[FL_point_b];
				b_two_core_power = total_big_power(2, big_FL[FL_point_b], voltage); 		
				//one core power
				equal_computing_cycles(b_core_nums, big_FL[FL_point_b], 1, &b_target_one_frequency);
				for (int i = 0; i < big_freq_level; i++) {
					if (b_target_one_frequency > big_FL[big_freq_level - 1]) {
						b_target_one_frequency = big_FL[big_freq_level - 1];
						break;
					}
					if (b_target_one_frequency < big_FL[i]) {
						b_target_one_frequency = big_FL[i];
						break;
					}
				}
				b_one_core_power = total_big_power(1, b_target_one_frequency, voltage);
			}
			if (b_one_core_power < b_two_core_power) {
				b_set_frequency = b_target_one_frequency;
				set_cpu_on(4, cpu_on);
				set_cpu_on(5, cpu_off);
			}
			else {
				b_set_frequency = b_target_two_frequency;
				set_cpu_on(4, cpu_on);
				set_cpu_on(5, cpu_on);
			}
			set_cpu_freq(4, b_set_frequency);
			//little core determine
			if (l_core_nums == 1) {
				//one core power
				l_target_one_frequency = little_FL[FL_point];
				l_one_core_power = total_little_power(1, little_FL[FL_point], voltage); 		
				//two core power
				equal_computing_cycles(l_core_nums, little_FL[FL_point], 2, &l_target_two_frequency);
				for (int i = 0; i < little_freq_level; i++) {
					if (l_target_two_frequency > little_FL[little_freq_level - 1]) {
						l_target_two_frequency = little_FL[little_freq_level - 1];
						break;
					}
					if (l_target_two_frequency < little_FL[i]) {
						l_target_two_frequency = little_FL[i];
						break;
					}
				}
				l_two_core_power = total_little_power(2, l_target_two_frequency, voltage);
				//four core power
				equal_computing_cycles(l_core_nums, little_FL[FL_point], 4, &l_target_four_frequency);
				for (int i = 0; i < little_freq_level; i++) {
					if (l_target_four_frequency > little_FL[little_freq_level - 1]) {
						l_target_four_frequency = little_FL[little_freq_level - 1];
						break;
					}
					if (l_target_four_frequency < little_FL[i]) {
						l_target_four_frequency = little_FL[i];
						break;
					}
				}
				l_four_core_power = total_little_power(4, l_target_four_frequency, voltage);
			}
			else if (l_core_nums == 2) {
				//two core power
				l_target_two_frequency = little_FL[FL_point];
				l_two_core_power = total_little_power(2, little_FL[FL_point], voltage); 		
				//one core power
				equal_computing_cycles(l_core_nums, little_FL[FL_point], 1, &l_target_one_frequency);
				for (int i = 0; i < little_freq_level; i++) {
					if (l_target_one_frequency > little_FL[little_freq_level - 1]) {
						l_target_one_frequency = little_FL[little_freq_level - 1];
						break;
					}
					if (l_target_one_frequency < little_FL[i]) {
						l_target_one_frequency = little_FL[i];
						break;
					}
				}
				l_one_core_power = total_little_power(1, l_target_one_frequency, voltage);
				//four core power
				equal_computing_cycles(l_core_nums, little_FL[FL_point], 4, &l_target_four_frequency);
				for (int i = 0; i < little_freq_level; i++) {
					if (l_target_four_frequency > little_FL[little_freq_level - 1]) {
						l_target_four_frequency = little_FL[little_freq_level - 1];
						break;
					}
					if (l_target_four_frequency < little_FL[i]) {
						l_target_four_frequency = little_FL[i];
						break;
					}
				}
				l_four_core_power = total_little_power(4, l_target_four_frequency, voltage);
			}
			else if (l_core_nums == 4) {
				//four core power
				l_target_four_frequency = little_FL[FL_point];
				l_four_core_power = total_little_power(4, little_FL[FL_point], voltage); 		
				//one core power
				equal_computing_cycles(l_core_nums, little_FL[FL_point], 1, &l_target_one_frequency);
				for (int i = 0; i < little_freq_level; i++) {
					if (l_target_one_frequency > little_FL[little_freq_level - 1]) {
						l_target_one_frequency = little_FL[little_freq_level - 1];
						break;
					}
					if (l_target_one_frequency < little_FL[i]) {
						l_target_one_frequency = little_FL[i];
						break;
					}
				}
				l_one_core_power = total_little_power(1, l_target_one_frequency, voltage);
				//two core power
				equal_computing_cycles(l_core_nums, little_FL[FL_point], 2, &l_target_two_frequency);
				for (int i = 0; i < little_freq_level; i++) {
					if (l_target_two_frequency > little_FL[little_freq_level - 1]) {
						l_target_two_frequency = little_FL[little_freq_level - 1];
						break;
					}
					if (l_target_two_frequency < little_FL[i]) {
						l_target_two_frequency = little_FL[i];
						break;
					}
				}
				l_two_core_power = total_little_power(2, l_target_four_frequency, voltage);
			}
			if (l_one_core_power < l_two_core_power) {
				if (l_one_core_power < l_four_core_power) {
					l_set_frequency = l_target_one_frequency;
					set_cpu_on(0, cpu_on);
					set_cpu_on(1, cpu_off);
					set_cpu_on(2, cpu_off);
					set_cpu_on(3, cpu_off);
				}
				else {
					l_set_frequency = l_target_four_frequency;
					set_cpu_on(0, cpu_on);
					set_cpu_on(1, cpu_on);
					set_cpu_on(2, cpu_on);
					set_cpu_on(3, cpu_on);			
				}
			}
			else {
				if (l_two_core_power < l_four_core_power) {
					l_set_frequency = l_target_two_frequency;
					set_cpu_on(0, cpu_on);
					set_cpu_on(1, cpu_on);
					set_cpu_on(2, cpu_off);
					set_cpu_on(3, cpu_off);
				}
				else {
					l_set_frequency = l_target_four_frequency;
					set_cpu_on(0, cpu_on);
					set_cpu_on(1, cpu_on);
					set_cpu_on(2, cpu_on);
					set_cpu_on(3, cpu_on);
				}
			}
			set_cpu_freq(0, l_set_frequency);
			//Dispatcher by Hao Algorithm End

			/*if (FL_point != pre_FL) {		
				if (FL_point > little_freq_level-1) {
					FL_point = little_freq_level-1;
				}
				else if (FL_point < 0) {
					FL_point = 0;
				}		
				pre_FL = FL_point;
				set_cpu_freq(0, little_FL[FL_point]);			
				if (FL_point_b > big_freq_level - 1) {
					FL_point_b = big_freq_level - 1;
				}
				else if (FL_point_b < 0) {
					FL_point_b = 0;
				}
				set_cpu_freq(4, big_FL[FL_point_b]);
			}*/
			if (GFL_point != pre_GFL) {			
				if(GFL_point > gpu_freq_level-1){
					GFL_point = gpu_freq_level-1;
				}
				else if (GFL_point < 0) {
					GFL_point = 0;
				}		
				pre_GFL = GFL_point;
				set_gpu_freq(gpu_FL[GFL_point]);			
			}	

			//Restore current
			FC_record[pre0] = little_FL[FL_point];
			FG_record[pre0] = gpu_FL[GFL_point];

			rec_interval++;
		}
		for (int i = 0; i < core_num; i++) core_assign[i] = 0;
		
		usleep(1000000);
	}
	return 0;
}
#elif algorithm_select == 4
void *related_work_predictive(void*) {

	//Related Work Parameter Setting and Initial
	double pre_big_temp = 0.0, pre_little_temp = 0.0;	//predicted big, little temperature
	int set_big_freq = 0, set_little_freq = 0;			//set big, little frequency
	int gpu_now_freq = 0;								//now gpu frequency 
	double sum_power = 0.0;								//total power(big, little, gpu)
	double sum_big_power = 0.0, sum_little_power = 0.0;	//total big, little power
	double sum_gpu_power = 0.0;							//total gpu power
	double big_l[2] = {0.0}, little_l[4] = {0.0}, sum_big_l = 0.0, sum_little_l = 0.0, gpu_l = 0.0;		//leakage power
	double big_d[2] = {0.0}, little_d[4] = {0.0}, sum_big_d = 0.0, sum_little_d = 0.0, gpu_d = 0.0;		//dynamic power
	double voltage = 0;

	for (int tick = 1; tick < testing_time; tick++) {	//Algorithm Start
		voltage = ((double)get_battery(battery_voltage)/1000000);
		//Power Calculating
		//Big Power
		if (get_cpu_on(4)) {
			big_l[0] = 0.0034 * get_temp(4) - 0.005;
			if (cpu_utilization[4] > 0)
				big_d[0] = 0.5 * aC_b * voltage * voltage * get_cpu_freq(4);		//0.5 for 1/2 * aC
		}
		if (get_cpu_on(5)) {
			big_l[1] = 0.0034 * get_temp(5) - 0.005;
			if (cpu_utilization[5] > 0) 
				big_d[1] = 0.5 * aC_b * voltage * voltage * get_cpu_freq(5);
		}
		for (int b = 0; b < 2; b++) {
			sum_big_l = sum_big_l + big_l[b];
			sum_big_d = sum_big_d + big_d[b];
		}
		sum_big_power = sum_big_l + sum_big_d;
		//Little Power
		if (get_cpu_on(0)) {
			little_l[0] = 0.0005 * get_temp(0) + 0.005;
			if (cpu_utilization[0] > 0) 
				little_d[0] = 0.25 * aC_l * voltage * voltage * get_cpu_freq(0);	//0.25 for 1/4 * aC
		}
		if (get_cpu_on(1)) {
			little_l[1] = 0.0005 * get_temp(1) + 0.005;
			if (cpu_utilization[1] > 0)
				little_d[1] = 0.25 * aC_l * voltage * voltage * get_cpu_freq(1);
		}
		if (get_cpu_on(2)) {
			little_l[2] = 0.0005 * get_temp(2) + 0.005;
			if (cpu_utilization[2] > 0)
				little_d[2] = 0.25 * aC_l * voltage * voltage * get_cpu_freq(2);
		}
		if (get_cpu_on(3)) {
			little_l[3] = 0.0005 * get_temp(3) + 0.005;
			if (cpu_utilization[3] > 0)
				little_d[3] = 0.25 * aC_l * voltage * voltage * get_cpu_freq(3);
		}
		for (int l = 0; l < 4; l++) {
			sum_little_l = sum_little_l + little_l[l];
			sum_little_d = sum_little_d + little_d[l];
		}
		sum_little_power = sum_little_l + sum_little_d;	
		//GPU Power
		gpu_l = 0.0025 * get_temp(7) + 0.02;
		if (get_gpu_util() > 0) {
			gpu_now_freq = get_gpu_freq();
			if (gpu_now_freq == 180) gpu_d = gpu_dynamic[0];
			else if (gpu_now_freq == 300) gpu_d = gpu_dynamic[1];
			else if (gpu_now_freq == 367) gpu_d = gpu_dynamic[2];
			else if (gpu_now_freq == 450) gpu_d = gpu_dynamic[3];
			else if (gpu_now_freq == 490) gpu_d = gpu_dynamic[4];
			else if (gpu_now_freq == 600) gpu_d = gpu_dynamic[5];
			else gpu_d = 0.0;
			sum_gpu_power = gpu_l + gpu_d;
		}
		else {
			sum_gpu_power = gpu_l + gpu_d;
		}
		sum_power = sum_little_l + sum_little_d + sum_big_l + sum_big_d + gpu_l + gpu_d;

		//Frequency and On/Off State Deciding
		//Big Frequency
		if (get_cpu_on(4) && get_cpu_on(5)) {
			sum_big_l = big_l[0] + big_l[1];
			for (int a = big_freq_level - 1; a >= 0; a--) {
				sum_big_d = aC_b * voltage * voltage * (big_FL[a] / 1000);
				sum_big_power = sum_big_l + sum_big_d;
				pre_big_temp = 0.718 * get_temp(4) + 27.9268 * sum_big_power;
				set_big_freq = a;
				if ((double)temp_constraint - 10.0 > pre_big_temp) break;
				else {
					if (set_big_freq == 0) set_cpu_on(5, cpu_off);
					continue;
				}
			}
			set_cpu_freq(4, big_FL[set_big_freq]);
			set_gpu_freq(gpu_FL[5]);
			gpu_f_decide = 5;
		}
		else if (get_cpu_on(4)) {
			sum_big_l = big_l[0];
			for (int a = big_freq_level - 1; a >= 0; a--) {
				sum_big_d = 0.5 * aC_b * voltage * voltage * (big_FL[a] / 1000);
				sum_big_power = sum_big_l + sum_big_d;
				pre_big_temp = 0.718 * get_temp(4) + 40.7121 * sum_big_power;
				set_big_freq = a;
				if ((double)temp_constraint - 10.0 > pre_big_temp) break;
				else {
					if (set_big_freq == 0) set_cpu_on(4, cpu_off);
					continue;
				}
			}
			set_cpu_freq(4, big_FL[set_big_freq]);
			set_gpu_freq(gpu_FL[5]);
			gpu_f_decide = 5;
		}
		//Little Frequency
		if (get_cpu_on(0) && get_cpu_on(1) && get_cpu_on(2) && get_cpu_on(3)) {
			sum_little_l = little_l[0] + little_l[1] + little_l[2] + little_l[3];
			for (int a = little_freq_level - 1; a >= 0; a--) {
				sum_little_d = aC_l * voltage * voltage * (little_FL[a] / 1000);
				sum_little_power = sum_little_l + sum_little_d;
				pre_little_temp = 0.9511 * get_temp(0) + 2.1042 * sum_little_power;
				set_little_freq = a;
				if ((double)temp_constraint - 10.0 > pre_little_temp) break;
				else {
					if (set_little_freq == 0) set_cpu_on(3, cpu_off);
					continue;
				}
			}
			set_cpu_freq(0, little_FL[set_little_freq]);
			set_gpu_freq(gpu_FL[5]);
			gpu_f_decide = 5;
		} 
		else if (get_cpu_on(0) && get_cpu_on(1) && get_cpu_on(2)) {
			sum_little_l = little_l[0] + little_l[1] + little_l[2];
			for (int a = little_freq_level - 1; a >= 0; a--) {
				sum_little_d = 0.75 * aC_l * voltage * voltage * (little_FL[a] / 1000);		//0.75 for 3/4 aC_l
				sum_little_power = sum_little_l + sum_little_d;
				pre_little_temp = 0.9511 * get_temp(0) + 2.1042 * sum_little_power;
				set_little_freq = a;
				if ((double)temp_constraint - 10.0 > pre_little_temp) break;
				else {
					if (set_little_freq == 0) set_cpu_on(2, cpu_off);
					continue;
				}
			}
			set_cpu_freq(0, little_FL[set_little_freq]);
			set_gpu_freq(gpu_FL[5]);
			gpu_f_decide = 5;
		}
		else if (get_cpu_on(0) && get_cpu_on(1)) {
			sum_little_l = little_l[0] + little_l[1];
			for (int a = little_freq_level - 1; a >= 0; a--) {
				sum_little_d = 0.5 * aC_l * voltage * voltage * (little_FL[a] / 1000);		//0.5 for 2/4 aC_l
				sum_little_power = sum_little_l + sum_little_d;
				pre_little_temp = 0.9511 * get_temp(0) + 2.1042 * sum_little_power;
				set_little_freq = a;
				if ((double)temp_constraint - 10.0 > pre_little_temp) break;
				else {
					if (set_little_freq == 0) set_cpu_on(1, cpu_off);
					continue;
				}
			}
			set_cpu_freq(0, little_FL[set_little_freq]);
			set_gpu_freq(gpu_FL[5]);
			gpu_f_decide = 5;
		}
		else if (get_cpu_on(0)) {
			sum_little_l = little_l[0];
			for (int a = little_freq_level - 1; a >= 0; a--) {
				sum_little_d = 0.25 * aC_l * voltage * voltage * (little_FL[a] / 1000);		//0.25 for 1/4 aC_l
				sum_little_power = sum_little_l + sum_little_d;
				pre_little_temp = 0.9511 * get_temp(0) + 2.1042 * sum_little_power;
				set_little_freq = a;
				if ((double)temp_constraint - 10.0 > pre_little_temp) break;
				else {
					if (set_little_freq == 0) {									//GPU decrease frequency
						gpu_f_decide--;
						if (gpu_f_decide < 0) gpu_f_decide = 0;
						set_gpu_freq(gpu_FL[gpu_f_decide]);
					}
					continue;
				}
			}
			set_cpu_freq(0, little_FL[set_little_freq]);
			if (set_little_freq != 0) set_gpu_freq(gpu_FL[5]);
		}
		usleep(1000000);
	}
	return 0;
}
#elif algorithm_select == 5
void *related_work_migration(void*) {

	//Related Work Parameter Setting and Initial
	int max_temp_core = 0, max_temp = 0;
	static int flag = 0;					//flag for migration one time
	pid_t foreground_app;					//foreground app pid
	pid_t background_app;					//background app pid (not use yet)

	for (int tick = 1; tick < testing_time; tick++) {	//Algorithm Start	
		//System Setting
		for (int i = 0; i < core_num; i++) set_cpu_on(i, cpu_on);
		get_fore_back_app_pid(&foreground_app, &background_app);

		//Check Temperature and Migration
		max_temp = get_max_temp(&max_temp_core);
		if (max_temp_core == 4 || max_temp_core == 5) {
			if (max_temp >= temp_constraint) {
				if (flag == 1) {
					set_cpu_freq(4, big_FL[12]);  			//max: 12
					set_cpu_freq(0, little_FL[8]);			//max: 8
					for (int i = 0; i < 4; i++) core_assign[i] = 1;			//in Little cluster
					set_all_pid_to_core(foreground_app, core_assign);		//all or main
					if (background_app != 0) set_all_pid_to_core(foreground_app, core_assign);
					flag = 0;
				}
			}
			else {
				if (flag == 0) {
					set_cpu_freq(4, big_FL[12]);  			//max: 12
					set_cpu_freq(0, little_FL[8]);			//max: 8
					for (int i = 4; i < 6; i++) core_assign[i] = 1;			//in Big cluster
					set_all_pid_to_core(foreground_app, core_assign);		//all or main
					if (background_app != 0) set_all_pid_to_core(foreground_app, core_assign);
					flag = 1;
				}
			}
		}
		else {
			if (flag == 0) {
				set_cpu_freq(4, big_FL[12]);  			//max: 12
				set_cpu_freq(0, little_FL[8]);			//max: 8
				for (int i = 4; i < 6; i++) core_assign[i] = 1;			//in Big cluster
				set_all_pid_to_core(foreground_app, core_assign);		//all or main
				if (background_app != 0) set_all_pid_to_core(foreground_app, core_assign);
				flag = 1;		
			}
		}
		//Clear core assign matrix
		for (int i = 0; i < core_num; i++) core_assign[i] = 0;
		usleep(1000000);
	}
	return 0;
}
#elif algorithm_select == 6
void *related_work_predictive_with_migration(void*) {

	//Related Work(Predictive) Parameter Setting and Initial
	double pre_big_temp = 0.0, pre_little_temp = 0.0;	//predicted big, little temperature
	int set_big_freq = 0, set_little_freq = 0;			//set big, little frequency
	int gpu_now_freq = 0;								//now gpu frequency 
	double sum_power = 0.0;								//total power(big, little, gpu)
	double sum_big_power = 0.0, sum_little_power = 0.0;	//total big, little power
	double sum_gpu_power = 0.0;							//total gpu power
	double big_l[2] = {0.0}, little_l[4] = {0.0}, sum_big_l = 0.0, sum_little_l = 0.0, gpu_l = 0.0;		//leakage power
	double big_d[2] = {0.0}, little_d[4] = {0.0}, sum_big_d = 0.0, sum_little_d = 0.0, gpu_d = 0.0;		//dynamic power
	double voltage = 0;

	//Related Work (Migration) Parameter Setting and Initial
	int max_temp_core = 0, max_temp = 0;
	static int flag = 0;					//flag for migration one time
	pid_t foreground_app;					//foreground app pid
	pid_t background_app;					//background app pid (not use yet)

	for (int tick = 1; tick < testing_time; tick++) {
		//Predictive Algorithm Start	
		voltage = ((double)get_battery(battery_voltage)/1000000);
		//Power Calculating
		//Big Power
		if (get_cpu_on(4)) {
			big_l[0] = 0.0034 * get_temp(4) - 0.005;
			if (cpu_utilization[4] > 0)
				big_d[0] = 0.5 * aC_b * voltage * voltage * get_cpu_freq(4);		//0.5 for 1/2 * aC
		}
		if (get_cpu_on(5)) {
			big_l[1] = 0.0034 * get_temp(5) - 0.005;
			if (cpu_utilization[5] > 0) 
				big_d[1] = 0.5 * aC_b * voltage * voltage * get_cpu_freq(5);
		}
		for (int b = 0; b < 2; b++) {
			sum_big_l = sum_big_l + big_l[b];
			sum_big_d = sum_big_d + big_d[b];
		}
		sum_big_power = sum_big_l + sum_big_d;
		//Little Power
		if (get_cpu_on(0)) {
			little_l[0] = 0.0005 * get_temp(0) + 0.005;
			if (cpu_utilization[0] > 0) 
				little_d[0] = 0.25 * aC_l * voltage * voltage * get_cpu_freq(0);	//0.25 for 1/4 * aC
		}
		if (get_cpu_on(1)) {
			little_l[1] = 0.0005 * get_temp(1) + 0.005;
			if (cpu_utilization[1] > 0)
				little_d[1] = 0.25 * aC_l * voltage * voltage * get_cpu_freq(1);
		}
		if (get_cpu_on(2)) {
			little_l[2] = 0.0005 * get_temp(2) + 0.005;
			if (cpu_utilization[2] > 0)
				little_d[2] = 0.25 * aC_l * voltage * voltage * get_cpu_freq(2);
		}
		if (get_cpu_on(3)) {
			little_l[3] = 0.0005 * get_temp(3) + 0.005;
			if (cpu_utilization[3] > 0)
				little_d[3] = 0.25 * aC_l * voltage * voltage * get_cpu_freq(3);
		}
		for (int l = 0; l < 4; l++) {
			sum_little_l = sum_little_l + little_l[l];
			sum_little_d = sum_little_d + little_d[l];
		}
		sum_little_power = sum_little_l + sum_little_d;	
		//GPU Power
		gpu_l = 0.0025 * get_temp(7) + 0.02;
		if (get_gpu_util() > 0) {
			gpu_now_freq = get_gpu_freq();
			if (gpu_now_freq == 180) gpu_d = gpu_dynamic[0];
			else if (gpu_now_freq == 300) gpu_d = gpu_dynamic[1];
			else if (gpu_now_freq == 367) gpu_d = gpu_dynamic[2];
			else if (gpu_now_freq == 450) gpu_d = gpu_dynamic[3];
			else if (gpu_now_freq == 490) gpu_d = gpu_dynamic[4];
			else if (gpu_now_freq == 600) gpu_d = gpu_dynamic[5];
			else gpu_d = 0.0;
			sum_gpu_power = gpu_l + gpu_d;
		}
		else {
			sum_gpu_power = gpu_l + gpu_d;
		}
		sum_power = sum_little_l + sum_little_d + sum_big_l + sum_big_d + gpu_l + gpu_d;

		//Frequency and On/Off State Deciding
		//Big Frequency
		if (get_cpu_on(4) && get_cpu_on(5)) {
			sum_big_l = big_l[0] + big_l[1];
			for (int a = big_freq_level - 1; a >= 0; a--) {
				sum_big_d = aC_b * voltage * voltage * (big_FL[a] / 1000);
				sum_big_power = sum_big_l + sum_big_d;
				pre_big_temp = 0.718 * get_temp(4) + 27.9268 * sum_big_power;
				set_big_freq = a;
				if ((double)temp_constraint - 10.0 > pre_big_temp) break;
				else {
					if (set_big_freq == 0) set_cpu_on(5, cpu_off);
					continue;
				}
			}
			set_cpu_freq(4, big_FL[set_big_freq]);
			set_gpu_freq(gpu_FL[5]);
			gpu_f_decide = 5;
		}
		else if (get_cpu_on(4)) {
			sum_big_l = big_l[0];
			for (int a = big_freq_level - 1; a >= 0; a--) {
				sum_big_d = 0.5 * aC_b * voltage * voltage * (big_FL[a] / 1000);
				sum_big_power = sum_big_l + sum_big_d;
				pre_big_temp = 0.718 * get_temp(4) + 40.7121 * sum_big_power;
				set_big_freq = a;
				if ((double)temp_constraint - 10.0 > pre_big_temp) break;
				else {
					if (set_big_freq == 0) set_cpu_on(4, cpu_off);
					continue;
				}
			}
			set_cpu_freq(4, big_FL[set_big_freq]);
			set_gpu_freq(gpu_FL[5]);
			gpu_f_decide = 5;
		}
		//Little Frequency
		if (get_cpu_on(0) && get_cpu_on(1) && get_cpu_on(2) && get_cpu_on(3)) {
			sum_little_l = little_l[0] + little_l[1] + little_l[2] + little_l[3];
			for (int a = little_freq_level - 1; a >= 0; a--) {
				sum_little_d = aC_l * voltage * voltage * (little_FL[a] / 1000);
				sum_little_power = sum_little_l + sum_little_d;
				pre_little_temp = 0.9511 * get_temp(0) + 2.1042 * sum_little_power;
				set_little_freq = a;
				if ((double)temp_constraint - 10.0 > pre_little_temp) break;
				else {
					if (set_little_freq == 0) set_cpu_on(3, cpu_off);
					continue;
				}
			}
			set_cpu_freq(0, little_FL[set_little_freq]);
			set_gpu_freq(gpu_FL[5]);
			gpu_f_decide = 5;
		} 
		else if (get_cpu_on(0) && get_cpu_on(1) && get_cpu_on(2)) {
			sum_little_l = little_l[0] + little_l[1] + little_l[2];
			for (int a = little_freq_level - 1; a >= 0; a--) {
				sum_little_d = 0.75 * aC_l * voltage * voltage * (little_FL[a] / 1000);		//0.75 for 3/4 aC_l
				sum_little_power = sum_little_l + sum_little_d;
				pre_little_temp = 0.9511 * get_temp(0) + 2.1042 * sum_little_power;
				set_little_freq = a;
				if ((double)temp_constraint - 10.0 > pre_little_temp) break;
				else {
					if (set_little_freq == 0) set_cpu_on(2, cpu_off);
					continue;
				}
			}
			set_cpu_freq(0, little_FL[set_little_freq]);
			set_gpu_freq(gpu_FL[5]);
			gpu_f_decide = 5;
		}
		else if (get_cpu_on(0) && get_cpu_on(1)) {
			sum_little_l = little_l[0] + little_l[1];
			for (int a = little_freq_level - 1; a >= 0; a--) {
				sum_little_d = 0.5 * aC_l * voltage * voltage * (little_FL[a] / 1000);		//0.5 for 2/4 aC_l
				sum_little_power = sum_little_l + sum_little_d;
				pre_little_temp = 0.9511 * get_temp(0) + 2.1042 * sum_little_power;
				set_little_freq = a;
				if ((double)temp_constraint - 10.0 > pre_little_temp) break;
				else {
					if (set_little_freq == 0) set_cpu_on(1, cpu_off);
					continue;
				}
			}
			set_cpu_freq(0, little_FL[set_little_freq]);
			set_gpu_freq(gpu_FL[5]);
			gpu_f_decide = 5;
		}
		else if (get_cpu_on(0)) {
			sum_little_l = little_l[0];
			for (int a = little_freq_level - 1; a >= 0; a--) {
				sum_little_d = 0.25 * aC_l * voltage * voltage * (little_FL[a] / 1000);		//0.25 for 1/4 aC_l
				sum_little_power = sum_little_l + sum_little_d;
				pre_little_temp = 0.9511 * get_temp(0) + 2.1042 * sum_little_power;
				set_little_freq = a;
				if ((double)temp_constraint - 10.0 > pre_little_temp) break;
				else {
					if (set_little_freq == 0) {									//GPU decrease frequency
						gpu_f_decide--;
						if (gpu_f_decide < 0) gpu_f_decide = 0;
						set_gpu_freq(gpu_FL[gpu_f_decide]);
					}
					continue;
				}
			}
			set_cpu_freq(0, little_FL[set_little_freq]);
			if (set_little_freq != 0) set_gpu_freq(gpu_FL[5]);
		}
		//Predictive Algorithm End

		//Migration Algorithm Start
		//System Setting
		for (int i = 0; i < core_num; i++) set_cpu_on(i, cpu_on);
		get_fore_back_app_pid(&foreground_app, &background_app);

		//Check Temperature and Migration
		max_temp = get_max_temp(&max_temp_core);
		if (max_temp_core == 4 || max_temp_core == 5) {
			if (max_temp >= temp_constraint) {
				if (flag == 1) {
					for (int i = 0; i < 4; i++) core_assign[i] = 1;			//in Little cluster
					set_all_pid_to_core(foreground_app, core_assign);		//all or main
					if (background_app != 0) set_all_pid_to_core(foreground_app, core_assign);
					flag = 0;
				}
			}
			else {
				if (flag == 0) {
					for (int i = 4; i < 6; i++) core_assign[i] = 1;			//in Big cluster
					set_all_pid_to_core(foreground_app, core_assign);		//all or main
					if (background_app != 0) set_all_pid_to_core(foreground_app, core_assign);
					flag = 1;
				}
			}
		}
		else {
			if (flag == 0) {
				for (int i = 4; i < 6; i++) core_assign[i] = 1;			//in Big cluster
				set_all_pid_to_core(foreground_app, core_assign);		//all or main
				if (background_app != 0) set_all_pid_to_core(foreground_app, core_assign);
				flag = 1;		
			}
		}
		//Migration Algorithm End
		//Clear core assign matrix
		for (int i = 0; i < core_num; i++) core_assign[i] = 0;
		usleep(1000000);
	}
	return 0;
}
#endif