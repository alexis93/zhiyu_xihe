#include <stdio.h>

int main(){
	double data[7];
	sensor_init();
	while(1){
		get_ins_data(data);
		printf("%lf   %lf  %lf   %lf  %lf   %lf  %lf\n",data[0],data[1],data[2],data[3],data[4],data[5],data[6]);
	}
}
