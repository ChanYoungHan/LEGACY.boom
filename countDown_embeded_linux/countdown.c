#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include "fnddrv.h"
#include <linux/input.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include "time.h"

#define MODE_STATIC_DIS		0
#define MODE_TIME_DIS		1
#define MODE_COUNT_DIS		2

#define COUNTDOWN_NUM_10MILI 2000

//fnd
#define FND_DRIVER_NAME		"/dev/perifnd"

//buzzer
#define BUZZER_BASE_SYS_PATH	"/sys/bus/platform/devices/peribuzzer."
#define BUZZER_ENABLE_NAME		"enable"
#define BUZZER_FREQUENCY_NAME	"frequency"

// button
#define  INPUT_DEVICE_LIST	"/dev/input/event"
#define  SYS_PATH	"S: Sysfs="
#define  BUTTON_NAME	"ecube-button"
#define  LINE_FEED	0x0A
#define  MAX_BUFF	200

//led
#define LED_DRIVER_NAME		"/dev/periled"

//color led
#define MAX_SCALE_STEP		7
#define PWM_BASE_SYS_PATH       "/sys/class/pwm/pwmchip"
#define PWM_RED_BASE_SYS_PATH	"/sys/class/pwm/pwmchip0/pwm0/"
#define PWM_GREEN_BASE_SYS_PATH	"/sys/class/pwm/pwmchip1/pwm0/"
#define PWM_BLUE_BASE_SYS_PATH	"/sys/class/pwm/pwmchip2/pwm0/"
#define PWM_SHOW	"1"
#define PWM_HIDE	"0"
#define PWM_ENABLE_NAME		"enable"
#define PWM_FREQUENCY_NAME	"period"
#define PWM_DUTYCYCLE_NAME	"duty_cycle"
#define RED_INDEX	0
#define GREEN_INDEX	1
#define BLUE_INDEX	2
#define TRUE	1
#define FALSE	0
#define  PWM_FREQUENCY		100000
#define  MAX_INPUT_VALUE	100

char gBuzzerBaseSysDir[128];

//############## fnd function ###################
void doHelp(void)
{
	printf("option   c  : count from 30 to 0 .\n");
	printf("ex) fndtest 0		;display off \n");
}
#define ONE_SEG_DISPLAY_TIME_USEC	1000
// return 1 => success  , 0 => error
int fndDisp(int num , int dotflag)
{
	int fd;
	int temp,i;
	stFndWriteForm stWriteData;
	
	for (i = 0; i < MAX_FND_NUM ; i++ )
	{
		stWriteData.DataDot[i] = (dotflag & (0x1 << i)) ? 1 : 0;  
		stWriteData.DataValid[i] = 1;
	}
	// if 6 fnd
	temp = num % 1000000;
	stWriteData.DataNumeric[0]= temp /100000;

	temp = num % 100000;
	stWriteData.DataNumeric[1]= temp /10000;

	temp = num % 10000;
	stWriteData.DataNumeric[2] = temp /1000;

	temp = num %1000;
	stWriteData.DataNumeric[3] = temp /100;
	stWriteData.DataDot[3] = 1;

	temp = num %100;
	stWriteData.DataNumeric[4] = temp /10;

	stWriteData.DataNumeric[5] = num %10;

	fd = open(FND_DRIVER_NAME,O_RDWR);
	if ( fd < 0 )
	{
		perror("driver open error.\n");
		return 0;
	}	
	write(fd,&stWriteData,sizeof(stFndWriteForm));
	close(fd);
	return 1;
}
int fndOff()
{
	int fd,i;
	stFndWriteForm stWriteData;
	
	for (i = 0; i < MAX_FND_NUM ; i++ )
	{
		stWriteData.DataDot[i] =  0;  
		stWriteData.DataNumeric[i] = 0;
		stWriteData.DataValid[i] = 0;
	}
	fd = open(FND_DRIVER_NAME,O_RDWR);
	if ( fd < 0 )
	{
		perror("driver open error.\n");
		return 0;
	}	
	write(fd,&stWriteData,sizeof(stFndWriteForm));
	close(fd);
	return 1;
}


//############## buzzer function ################
int findBuzzerSysPath()
{
	char str[128];
	char result; 
	char *pdel; 
	char * s; 
	sprintf(str,"ls -d %s*", BUZZER_BASE_SYS_PATH); 
	FILE* p = popen(str, "r");
	if ( p == NULL)
	{
		printf("popen fail \n"); 
		return -1; 
	}
	// str ==> ex) /sys/bus/platform/devices/peribuzzer.30
	s = fgets(str,128,p);
	if ( s == NULL )
	{
		printf("fgets fail\n"); 
		return -1; 
	}
	pdel = strrchr(s,(int)'.');
	if ( pdel == NULL ) 
	{
		printf(" . is not found\n"); 
		return -1; 
	}
	int i=0;
	while(1)
	{
		if ( pdel[i] == 0x0A )
		{
			pdel[i] = 0; 
			break; 
		}
		i++;
	}
	sprintf(gBuzzerBaseSysDir,"%s%s/",BUZZER_BASE_SYS_PATH,pdel+1);
	printf("find %s\n",gBuzzerBaseSysDir);
	pclose(p);

	return 0; 
}
void buzzerEnable(int bEnable)
{
	char	strshellcmd[150];	
	if ( bEnable)
	{
		sprintf(strshellcmd,"echo '1' > %s%s\n",gBuzzerBaseSysDir,BUZZER_ENABLE_NAME);
//		printf(strshellcmd);
		system(strshellcmd);
	}
	else
	{
		sprintf(strshellcmd,"echo '0' > %s%s\n",gBuzzerBaseSysDir,BUZZER_ENABLE_NAME);
//		printf(strshellcmd);
		system(strshellcmd);
	}
}
void setFrequency(int frequency) 
{
	char	strshellcmd[150];	
	sprintf(strshellcmd,"echo '%d' > %s%s\n",frequency,gBuzzerBaseSysDir,BUZZER_FREQUENCY_NAME);
//	printf(strshellcmd);
	system(strshellcmd);	
}

//############## color led function ################
void pwmActivate(int bActivate, int pwmIndex)
{
	char strshellcmd[150];
	if ( bActivate)
	{
		sprintf(strshellcmd,"echo '0' > %s%d/export\n",PWM_BASE_SYS_PATH,pwmIndex);
		//printf(strshellcmd);
		system(strshellcmd);
	}
	else
	{
		sprintf(strshellcmd,"echo '0' > %s%d/unexport\n",PWM_BASE_SYS_PATH,pwmIndex);
		//printf(strshellcmd);
		system(strshellcmd);
	}
}
void pwmEnable(int bEnable , int pwmIndex)
{
	char	strshellcmd[150];	
	if ( bEnable)
	{
		sprintf(strshellcmd,"echo '1' > %s%d/pwm0/%s\n",PWM_BASE_SYS_PATH,pwmIndex,PWM_ENABLE_NAME);
		//printf(strshellcmd);
		system(strshellcmd);
	}
	else
	{
		sprintf(strshellcmd,"echo '0' > %s%d/pwm0/%s\n",PWM_BASE_SYS_PATH,pwmIndex,PWM_ENABLE_NAME);
		//printf(strshellcmd);
		system(strshellcmd);
	}
}
void writePWMPeriod(int frequency, int pwmIndex) 
{
	char	strshellcmd[150];	
	sprintf(strshellcmd,"echo '%d' > %s%d/pwm0/%s\n",frequency,PWM_BASE_SYS_PATH, pwmIndex,PWM_FREQUENCY_NAME);
	//printf(strshellcmd);
	system(strshellcmd);	
}
void writePWMDuty(int  DutyCycle , int pwmIndex)
{
	char	strshellcmd[150];	
	sprintf(strshellcmd,"echo '%d' > %s%d/pwm0/%s\n",DutyCycle,PWM_BASE_SYS_PATH,pwmIndex,PWM_DUTYCYCLE_NAME);
	//printf(strshellcmd);
	system(strshellcmd);	
}
void colerLedOn(int red, int green, int blue){
	int redduty,greenduty,blueduty;
	// inverse
	red = MAX_INPUT_VALUE - red;
	green = MAX_INPUT_VALUE - green;
	blue = MAX_INPUT_VALUE - blue;
	
	// percentage
	redduty = PWM_FREQUENCY * red / MAX_INPUT_VALUE;
	greenduty = PWM_FREQUENCY * green / MAX_INPUT_VALUE;
	blueduty = PWM_FREQUENCY * blue / MAX_INPUT_VALUE;
	pwmActivate(TRUE,RED_INDEX );
	writePWMPeriod(PWM_FREQUENCY, RED_INDEX);
	writePWMDuty(redduty, RED_INDEX);
	pwmEnable(TRUE, RED_INDEX);
	
	pwmActivate(TRUE,GREEN_INDEX );
	writePWMPeriod(PWM_FREQUENCY, GREEN_INDEX);
	writePWMDuty(greenduty, GREEN_INDEX);
	pwmEnable(TRUE, GREEN_INDEX);
	
	pwmActivate(TRUE,BLUE_INDEX );
	writePWMPeriod(PWM_FREQUENCY, BLUE_INDEX);
	writePWMDuty(blueduty, BLUE_INDEX);
	pwmEnable(TRUE, BLUE_INDEX);
}


//############## Count Down Function ################
int beep(int countdown, int step, int dur, int flag_beep){
	if (countdown % step == 0){
			buzzerEnable(0);
			return 0;
	} if (countdown % step == dur){
			buzzerEnable(1);
			return 0;
	}
}
void boom(int pid){
	kill(pid, SIGINT);
	colerLedOn(100,1,1);
	buzzerEnable(1);
	fndDisp(0, 0);
	sleep(1);
	colerLedOn(1,1,100);
	sleep(1);
	colerLedOn(100,1,1);
	sleep(1);
	colerLedOn(1,1,100);
	buzzerEnable(0);
}
void stop(int pid){
	kill(pid, SIGINT);
	colerLedOn(1,100,1);
	buzzerEnable(0);
}


int main(int argc , char **argv)
{
	ONE:
	printf("goto activated\n");
	//countdown
	int mode ;
	int countdown;
	int status_count = 0;
	int flag_but0 = 0, flag_but1 = 0, flag_but2 = 0, flag_but3 = 0;

	//button
	int    fp;
	char 	inputfileName[MAX_BUFF+1];
	int		readSize,inputIndex;
	struct input_event  stEvent;

	pid_t pid;
	int status = 0;

	//process
	int pd[2];	//pipe discreptor
	int key_boom;
	srand((unsigned) time(0));

	//coler led
	int red,green,blue;
	int redduty,greenduty,blueduty;

	//##### button driver open #####
	sprintf(inputfileName,"%s%d",INPUT_DEVICE_LIST,4);
	printf("read button event:%s\n",inputfileName);
	fp = open(inputfileName, O_RDONLY);
	
	if ( -1 == fp )
	{
		printf("%s file read error.\n",inputfileName);
		return 1;
	}

	//##### led driver #####
	unsigned int data;
	int fd;
	data = 0x00;// all off

	fd = open(LED_DRIVER_NAME,O_RDWR);
	if ( fd < 0 )
	{
		perror("driver (//dev//cnled) open error.\n");
		return 1;
	}
	
	key_boom = rand()%4;
	printf("key : %d", key_boom);

	//##### argument ######
	if (argc <  2)
	{
		doHelp();
		return 1;
	}

	else if ( argv[1][0] == 'c'  )
	{
		mode = MODE_COUNT_DIS;
	}
	else if (argv[1][0] == 'o' )
	{
		buzzerEnable(0);
		fndOff();
		return 0; 
	}
	else
	{
		doHelp();
		perror("option error \n");
		return 1;
	}

	if (findBuzzerSysPath() == -1)
	{
		printf("There is no buzzer sys directory\n");
		return -1; 
	}

	//##### pipe open #####
	if(pipe(pd) < 0) perror("pipe error\n");

	//##### activate #####
	if (mode == MODE_COUNT_DIS)
	{
		countdown = COUNTDOWN_NUM_10MILI;
		int flag_beep = 0;
		setFrequency(760);
		
		//parent

		if((pid = fork()) > 0){
			printf("parent start\n");
			while(1)
			{
				if(flag_but1 == 1 && flag_but2 == 1 && flag_but3 == 1) stop(pid);
				if(flag_but0 == 1 && flag_but2 == 1 && flag_but3 == 1) stop(pid);
				if(flag_but0 == 1 && flag_but1 == 1 && flag_but3 == 1) stop(pid);
				if(flag_but0 == 1 && flag_but1 == 1 && flag_but2 == 1) stop(pid);

				readSize = read(fp, &stEvent , sizeof(stEvent));
				if (readSize != sizeof(stEvent))
				{
					continue;
				}
				if ( !stEvent.value )
				{
					switch(stEvent.code)
					{
						case KEY_VOLUMEUP:
						printf("Volume up key, \n");
						colerLedOn(100,100,100);
						key_boom = rand()%4;
						printf("key : %d\n", key_boom);
						data = data & 0x00;
						write(fd,&data,4);
						close(pd[0]);
						int pipe_data = 1;
						write(pd[1], &pipe_data, 1);
						break;

						case KEY_HOME:
						printf("Home key, \n");
						data = data | 0x03;
						write(fd,&data,4);
						if(key_boom == 0) {
							boom(pid);
						}
						flag_but0 = 1;
						
						break;

						case KEY_VOLUMEDOWN:
							printf("Volume down key, \n");
							close(fp);
							close(fd);
							goto ONE;
							printf("goto is not, \n");
							break;


						case KEY_SEARCH:
						printf("Search key, \n");
						data = data | 0x30;
						write(fd,&data,4);
						if(key_boom == 2) boom(pid);
						flag_but2 = 1;
						break;

						case KEY_BACK:
						printf("Back key, \n");
						data = data | 0x0c;
						write(fd,&data,4);
						if(key_boom == 1) boom(pid);
						flag_but1 = 1;
						break;

						case KEY_MENU:
						printf("Menu key, \n");
						data = data | 0xc0;
						write(fd,&data,4);
						if(key_boom == 3) boom(pid);
						flag_but3 = 1;
						break;

						
					}
				}
				else{} // EV_SYN
			}
			close(fp);	
			close(fd);	
		}
		else if( pid == 0){
			printf("child start\n");
			while(1) {
				close(pd[1]);
				read(pd[0], &status_count, 1);
				while(status_count)
				{
					if (!fndDisp(countdown , 0))
						break;
					//read(pd[0], &status_count, 1);
					if (countdown < 200) flag_beep = beep(countdown, 10, 8, flag_beep);
					else if (countdown < 500) flag_beep = beep(countdown, 20, 10, flag_beep);
					else if (countdown < 900) flag_beep = beep(countdown, 50, 10, flag_beep);
					else if (countdown < 1400) flag_beep = beep(countdown, 100, 10, flag_beep);
					else flag_beep = beep(countdown, 200, 10, flag_beep);
					
					countdown --;
					usleep(10000);	
					if (countdown < 0 ){
						buzzerEnable(1);
						sleep(3);
						buzzerEnable(0);
						status_count = 0;
						exit(0);
						return 1;
					}
				}
			}
		}
		else
		{
			printf("fail to fork\n");
		}
		
	}
	return 0;
}