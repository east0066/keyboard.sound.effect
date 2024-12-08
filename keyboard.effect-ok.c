

/*
linux鼠标键盘监控整理, https://blog.csdn.net/native_lee/article/details/121560511
gcc keyboard.effect-ok.c keyboard.effect-read.cfg-ok.c -o keyboard.effect	-lX11 -lXtst -lpthread
程序运行可能需要, $ sudo apt-get install sox gnome-session-canberra
*/

#include <stdio.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/extensions/record.h>
#include <time.h>
#include <pthread.h>

#include <stdlib.h>	// system(cmd)

#include <sys/inotify.h>
#include <unistd.h>

#include <signal.h>	//signal用
#include <sys/wait.h>	//waitpid
#include <libgen.h>
#include <string.h>
#include <stdarg.h>	//m_printf


extern const int MAX_SOUNDS;
extern char *arrSoundFile[];
extern char *sConfigFilename;

extern void readcfg();


int print_message = 0;
void m_printf(const char *format, ...) {
	if (print_message) {
		va_list args;
		va_start(args, format);
		vprintf(format, args);
		va_end(args);
    }
}

char* strJoin(char *s1, char *s2)
{
	char *result = malloc(strlen(s1)+strlen(s2)+1);	//+1 for the zero-terminator
	if(result == NULL) {
		m_printf("Error! unable to allocate memory.\n");
		exit(1);
	}
	strcpy(result, s1);
	strcat(result, s2);
	return result;
}

//
void log_message(const char *message) {
	FILE *log_file = fopen("keyboard.effect.log", "a");	// 打开日志文件
	if (log_file == NULL) {
		perror("打开日志文件时出错");
		return;
	}
	
	time_t now = time(0); // 获取当前时间
	char *timestamp = ctime(&now); // 转换为可读格式
	timestamp[strcspn(timestamp, "\n")] = '\0';	//去掉timestamp的"\n"
	
	fprintf(log_file, "[%s] %s\n", timestamp, message); // 写入日志文件
	fflush(log_file); // 刷新文件缓冲区，确保写入磁盘
	fclose(log_file); // 关闭文件
}

const char *sPath = "/usr/bin/canberra-gtk-play";
const char *sProgram = "canberra-gtk-play";
const char *sPara = "-f";
char *sSoundFullFile = "";
char *sProgramPath = "";

int iPressCount =0;
int iScrollFrontCount =0;
int iScrollBackCount =0;
int iPlaySoundFileCount = 0;
int igetpid, igetpid_old;

//目录或文件存在
int iFileDirExist(const char *path) {
	if (access(path, F_OK) == 0) {
		//m_printf("%s, 目录或文件存在\n", path);
		return 1;
	} else {
		m_printf("%s, 目录或文件不存在\n", path);
		return 0;
	}
}

// 终端显示 当前时间 格式 hh:mm:ss
void printTime_hms() {
	time_t timep;
	time(&timep);
	struct tm *tm_local = localtime(&timep); // 转换为本地时间
	// 提取时分秒
	int hour = tm_local->tm_hour;
	int min = tm_local->tm_min;
	int sec = tm_local->tm_sec;
	m_printf("%02d:%02d:%02d\n", hour, min, sec); // 格式化输出时分秒
	//m_printf("当前时间为：%s", asctime( gmtime(&timep) ) );
}


/***********************************************
		playSoundFile
***********************************************/
void playSoundFile(int iKeyCode) {
	char *sSoundFile = "";
	// 1.esc, esc9
	if (iKeyCode == 9){
		//sSoundFile = "sound-material/cs1.6/m4a1_clipout.wav";
		sSoundFile = arrSoundFile[1];
	}
	// 2.function.alone, fun1, f1-f10, 67-76, f1195, f1296
	if ( (iKeyCode >= 67 && iKeyCode <= 76)
	 || (iKeyCode == 95 || iKeyCode == 96) ) {
		//sSoundFile = "sound-material/cs1.6/knife_hitwall1.wav";
		sSoundFile = arrSoundFile[2];
	}
	// 3.function.combination, fn+f1-f12, 121, 122, 123, 198, 232, 233, 133, 255, F9空, 201, 95, 96
	if ( (iKeyCode >= 121 && iKeyCode <= 123 ) 
	 || (iKeyCode == 198 || iKeyCode == 232 || iKeyCode == 233 || iKeyCode == 133 
	 || iKeyCode == 255 || iKeyCode == 201 || iKeyCode == 95 || iKeyCode == 96 )  
	 ) {
		//sSoundFile = "sound-material/cs1.6/knife_deploy1.wav";
		sSoundFile = arrSoundFile[3];
	}
	// 4.tilde, ~49
	if (iKeyCode == 49){
		//sSoundFile = "sound-material/cs1.6/m4a1_clipin.wav";
		sSoundFile = arrSoundFile[4];
	}
	// 5.number, 0-9 = 10-19
	if (iKeyCode>=10 && iKeyCode<=19){
		//sSoundFile = "sound-material/cs1.6/ric2.wav";
		sSoundFile = arrSoundFile[5];
	}
	// 6.alphabetic, a-z,大小写相同 = 24-33 + 38-46 + 52-58
	if ((iKeyCode>=24 && iKeyCode<=33)
	 || (iKeyCode>=38 && iKeyCode<=46)
	 || (iKeyCode>=52 && iKeyCode<=58)){
		//sSoundFile = "sound-material/cs1.6/m4a1-1.wav";
		sSoundFile = arrSoundFile[6];
	}
	// 7.punctuation, ,47 '48 ,59 .60 /61, [34, ]35, \51
	if ( (iKeyCode >= 47 && iKeyCode <= 48)
	 || (iKeyCode >= 59 && iKeyCode <= 61)
	 || (iKeyCode == 34 || iKeyCode ==35 || iKeyCode ==51 ) ) {
		//sSoundFile = "sound-material/cs1.6/m4a1_deploy.wav";
		sSoundFile = arrSoundFile[7];
	}
	// 8.backspace, 退格22
	if (iKeyCode == 22){
		//sSoundFile = "sound-material/cs1.6/m4a1_deploy.wav";
		sSoundFile = arrSoundFile[8];
	}
	// 9.enter, 回车36
	if (iKeyCode == 36){
		//sSoundFile = "sound-material/cs1.6/mp5-1.wav";
		sSoundFile = arrSoundFile[9];
	}
	// direction 上111 下116 左113 右114
	// 10.up, 上, 111
	if (iKeyCode == 111) {
		//sSoundFile = "sound-material/cs1.6/m4a1_clipin.wav";
		sSoundFile = arrSoundFile[10];
	}
	// 11.down, 下, 116
	if (iKeyCode == 116) {
		//sSoundFile = "sound-material/cs1.6/m4a1_clipout.wav";
		sSoundFile = arrSoundFile[11];
	}
	// 12.left, 左, 113
	if (iKeyCode == 113) {
		//sSoundFile = "sound-material/cs1.6/m4a1_silencer_off.wav";
		sSoundFile = arrSoundFile[12];
	}
	// 13.right, 右, 114
	if (iKeyCode == 114) {
		//sSoundFile = "sound-material/cs1.6/m4a1_silencer_on.wav";
		sSoundFile = arrSoundFile[13];
	}
	// 14.mouse.steel.scroll.front.up, 鼠标滚轮, 前滚, 4
	if (iKeyCode==4) {
		//sSoundFile = "sound-material/Borealis/camera-focus-03.ogg";
		sSoundFile = arrSoundFile[14];
	}
	// 15.mouse.steel.scroll.back, 鼠标滚轮, 后滚, 5
	if (iKeyCode==5) {
		//sSoundFile = "sound-material/Borealis/camera-focus-02.ogg";
		sSoundFile = arrSoundFile[15];
	}
	// 16.trash.clear, 清空回收站
	if (iKeyCode==221) {
		//sSoundFile = "sound-material/Borealis/s_recycle_splashdive4.ogg";
		sSoundFile = arrSoundFile[16];
	}	
	// 17.small.keyboard.0", 90
	if (iKeyCode == 90) {
		sSoundFile = arrSoundFile[17];
	}
	// 18.small.keyboard.1, 87
	if (iKeyCode == 87) {
		sSoundFile = arrSoundFile[18];
	}
	// 19.small.keyboard.2, 88
	if (iKeyCode == 88) {
		sSoundFile = arrSoundFile[19];
	}
	// 20.small.keyboard.3, 89
	if (iKeyCode == 89) {
		sSoundFile = arrSoundFile[20];
	}
	// 21.small.keyboard.4, 83
	if (iKeyCode == 83) {
		sSoundFile = arrSoundFile[21];
	}
	// 22.small.keyboard.5, 84
	if (iKeyCode == 84) {
		sSoundFile = arrSoundFile[22];
	}
	// 23.small.keyboard.6, 85
	if (iKeyCode == 85) {
		sSoundFile = arrSoundFile[23];
	}
	// 24.small.keyboard.7, 79
	if (iKeyCode == 79) {
		sSoundFile = arrSoundFile[24];
	}
	// 25.small.keyboard.8, 80
	if (iKeyCode == 80) {
		sSoundFile = arrSoundFile[25];
	}
	// 26.small.keyboard.9, 81
	if (iKeyCode == 81) {
		sSoundFile = arrSoundFile[26];
	}
	// 27.small.keyboard.enter, 小键盘enter	104
	if (iKeyCode == 104) {
		sSoundFile = arrSoundFile[27];
	}
	// 28.small.keyboard.dot, 91
	if (iKeyCode == 91) {
		sSoundFile = arrSoundFile[28];
	}
	// 29.small.keyboard.add, 86
	if (iKeyCode == 86) {
		sSoundFile = arrSoundFile[29];
	}
	// 30.small.keyboard.subtract, 82
	if (iKeyCode == 82) {
		sSoundFile = arrSoundFile[30];
	}
	// 31.small.keyboard.multiply, 63
	if (iKeyCode == 63) {
		sSoundFile = arrSoundFile[31];
	}
	// 32.small.keyboard.divide, 106
	if (iKeyCode == 106) {
		sSoundFile = arrSoundFile[32];
	}
	// 33.small.keyboard.numlock, 77
	if (iKeyCode == 77) {
		sSoundFile = arrSoundFile[33];
	}
	// 34.printscreen, 107
	if (iKeyCode == 107) {
		//sSoundFile = "sound-material/internet/camera-shutter.ogg";
		sSoundFile = arrSoundFile[34];
	}
	// 35.tab, 23
	if (iKeyCode == 23) {
		sSoundFile = arrSoundFile[35];
	}
	// 36.capslock, capslk, 66
	if (iKeyCode == 66) {
		sSoundFile = arrSoundFile[36];
	}
	// 37.insert, 118
	if (iKeyCode == 118) {
		sSoundFile = arrSoundFile[37];
	}
	// 38.delete, 66
	if (iKeyCode == 119) {
		sSoundFile = arrSoundFile[38];
	}
	
	// 播放
	if (sSoundFile != "") {
		signal(SIGCHLD, SIG_IGN);
		
		m_printf("igetpid,old = %d, %d\n", igetpid, igetpid_old);
		pid_t pid = fork();
		if ( pid == 0 && iKeyCode > 0 ) {
			//m_printf("sSoundFile = %s\n", sSoundFile);
			iPlaySoundFileCount++;
			//m_printf("iPlaySoundFileCount = %d\n",iPlaySoundFileCount);			
			sSoundFullFile = strJoin(sProgramPath, sSoundFile);
			m_printf("sSoundFullFile = %s\n", sSoundFullFile);
			
			igetpid = getpid();
			igetpid_old = igetpid;
			//m_printf("igetpid,old = %d, %d\n", igetpid, igetpid_old);
			
			int iExecl = execl(sPath, sProgram, sPara, sSoundFullFile, NULL);		//is ok
			
			m_printf("iExecl = %d\n",iExecl);
			//exit(EXIT_SUCCESS);
			exit(0);
		}
	}	
}

/***********************************************
		event_cb
***********************************************/
time_t updateTime,currentTime;
static void event_cb (XPointer user_data, XRecordInterceptData *hook) {
	if ( hook->category != XRecordFromServer ) {
		m_printf("Data not from server!");
		return;
	}
	xEvent * event = (xEvent *)hook->data; 
	switch (event->u.u.type) {
		case KeyPress:
			updateTime = time(NULL);
			m_printf("key press event! event->u.u.detail = %d\n", event->u.u.detail);
			playSoundFile(event->u.u.detail);
			
			iPressCount ++;
			printTime_hms();
			m_printf("iPressCount %d\n", iPressCount);
			break;
			
		case KeyRelease:
			updateTime = time(NULL);
			m_printf("key release event! %d\n", event->u.u.detail);
			break;
			
		case ButtonPress:
			updateTime = time(NULL);
			//m_printf("button press event! x = %d, y = %d\n", event->u.keyButtonPointer.rootX, event->u.keyButtonPointer.rootY);
			m_printf("button press event! x = %d,\n", event->u.u.detail);
			//鼠标滚轮, 前滚, 4			
			if (event->u.u.detail==4) {
				iScrollFrontCount ++;
				m_printf("iScrollFrontCount %d\n", iScrollFrontCount);
				playSoundFile(event->u.u.detail);
			}
			//鼠标滚轮, 后滚, 5
			if (event->u.u.detail==5) {
				iScrollBackCount ++;
				m_printf("iScrollBackCount %d\n", iScrollBackCount);
				playSoundFile(event->u.u.detail);
			}
			break;
 
		case ButtonRelease:
			updateTime = time(NULL);
			m_printf("button release event! x = %d, y = %d\n", event->u.keyButtonPointer.rootX, event->u.keyButtonPointer.rootY);
			break;
		default:
			break;
	}
}

/***********************************************
		monitor
***********************************************/
void * monitor(void *arg) {
	Display *ctrl_disp = XOpenDisplay(NULL);
	Display *data_disp = XOpenDisplay(NULL);
	
	if ( !ctrl_disp || !data_disp ) {
		fprintf(stderr,"Unable to connect to X11 display! \n");
		return NULL;
	}
	
	int major, minor, dummy;
	if ( !XQueryExtension (ctrl_disp, "XTEST", &major, &minor, &dummy) ) {
		fprintf(stderr,"XTest extension missing! \n");
		return NULL;
	}
	if ( !XRecordQueryVersion (ctrl_disp, &major, &minor) ) {
		fprintf(stderr,"Failed to obtain xrecord version! \n");
		return NULL;
	}
	XSynchronize(ctrl_disp, True);
	XFlush (ctrl_disp);
 
	XRecordRange *range = XRecordAllocRange ();
	
	/*
	 * 设定事件监听的范围
	 * 监听 first ~ last 之间的事件
	 */
	range->device_events.first = KeyPress;
	range->device_events.last = LASTEvent;
	XRecordClientSpec spec = XRecordAllClients;
	XRecordContext context = XRecordCreateContext (
								 data_disp, 0, &spec, 1, &range, 1 );
	if ( !context ) {
		fprintf(stderr,"Failed to create context! \n");
		return NULL;
	}
 
	if ( !XRecordEnableContext(data_disp, context, event_cb, NULL) ) {
		fprintf(stderr,"Failed to enable context! \n");
		return NULL;
	}
 
	XRecordDisableContext(data_disp, context);
	XRecordFreeContext (data_disp, context);
	XFree(range);
 
	XCloseDisplay (data_disp);
	XCloseDisplay (ctrl_disp);
	return NULL;
}

/***********************************************
		vMoniterDir
***********************************************/
int iOpenCount = 0;
int iGiGpathArrLenght;
void vMoniterDir(char *pArr[]){
	// 打开inotify实例
	int fd = inotify_init();
	if (fd == -1) {
		perror("inotify_init");
		//return 1;
	}
	m_printf("Moniter Directory Begin.\n");
	int pArrSize = iGiGpathArrLenght;
	m_printf("pArrSize = %d \n", pArrSize);
	
	// 添加需要监控的目录
	int wd[pArrSize];
	for (int i=0; i < pArrSize; i++){
		int iPathArr = iFileDirExist(pArr[i]);
		if (iPathArr){
			wd[i] = inotify_add_watch(fd, pArr[i], IN_ALL_EVENTS);
			if (wd[i] == -1) {
				perror("inotify_add_watch");
				close(fd);
				//return 1;
			}
			else{
				m_printf("Monitoring %s for changes...\n", pArr[i]);
			}
		}
	}
	
	// 分配足够大的缓冲区来接收inotify事件
	const int EVENT_BUF_LEN = sizeof(struct inotify_event) * 1024;
	char buffer[EVENT_BUF_LEN];
	
	FILE *fp; 
	// 无限循环，不断读取事件
	while (1) {
		ssize_t len = read(fd, buffer, EVENT_BUF_LEN);
		if (len == -1) {
			perror("read");
			close(fd);
			//return 1;
		}
		
		// 处理读取到的每个事件
		char *ptr;
		for (ptr = buffer; ptr < buffer + len;) {
			struct inotify_event *event = (struct inotify_event *)ptr;
			if (event->len) {
				if (event->mask & IN_ACCESS) {
					m_printf("File accessed: %s\n", event->name);
				}
				if (event->mask & IN_ATTRIB) {
					m_printf("Metadata changed: %s\n", event->name);
				}
				if (event->mask & IN_CLOSE_WRITE) {
					m_printf("File created IN_CLOSE_WRITE: %s\n", event->name);		// 以write方式打开文件并关闭
				}
				
				if (event->mask & IN_CLOSE_NOWRITE) {
					m_printf("File created IN_CLOSE_NOWRITE: %s, iOpenCount %d\n", event->name, iOpenCount);		// 以非write方式打开文件并关闭
					playSoundFile(221);
				}
				if (event->mask & IN_CREATE) {
					m_printf("File created: %s\n", event->name);
				}
				if (event->mask & IN_DELETE) {
					m_printf("File deleted: %s\n", event->name);
				}
				if (event->mask & IN_DELETE_SELF) {
					m_printf("File deleted IN_DELETE_SELF: %s\n", event->name);		// 被监测的文件或目录被删除（被监测的文件夹A被删除）
				}
				if (event->mask & IN_MODIFY) {
					m_printf("File modified: %s\n", event->name);
				}
				if (event->mask & IN_MOVE_SELF) {
					m_printf("File modified IN_MOVE_SELF: %s\n", event->name);		//被监测的文件或目录移动
				}
				if (event->mask & IN_MOVED_FROM) {
					m_printf("File modified IN_MOVED_FROM: %s,iOpenCount %d\n", event->name,iOpenCount);		//文件移出被监测的目录
					playSoundFile(221);
				}
				if (event->mask & IN_MOVED_TO) {
					m_printf("File moved IN_MOVED_TO: %s\n", event->name);		//文件移入被监测的目录
				}
				if (event->mask & IN_OPEN) {
					m_printf("File open IN_OPEN: %s\n", event->name);		//文件被打开
				};
			}
			ptr += sizeof(struct inotify_event) + event->len;
		}
	}
 
	// 清理, 根据数组长度进行遍历
	for (int i = 0; i < pArrSize; i++) {
		m_printf("%d ", pArr[i]);
		inotify_rm_watch(fd, wd[i]);
	}
	close(fd);
}


/***********************************************
		main
***********************************************/
int main (int argc, char *argv[]) {	
	for (int i = 1; i < argc; i++) {
		//m_printf("Argument %d: %s\n", i, argv[i]);
		if (argv[i][0] == '-') {
			if (argv[i][1] == 'p') {
				print_message = 1;
			}
		}
	}
	m_printf("Program name: %s\n", argv[0]);
	//程序所在目录	
	char *path = "/proc/self/exe";
	char sRealpath[1024];
	char *ret = realpath(path, sRealpath);
	if (ret == NULL) {
		perror("realpath error");
	} else {
		m_printf("The real path is A: %s\n", sRealpath);
	}
	sProgramPath = dirname(sRealpath);
	strcat(sProgramPath, "/");
	m_printf("sProgramPathA = %s\n",sProgramPath);
	
	//读 cfg 文件
	char *sFnConfig = strJoin(sProgramPath, sConfigFilename);
	readcfg(sFnConfig);
	
	// 需监控的目录, 是否存在
	char *pathArr []= {
			"/media/eclt/eflly/.Trash-1000",
			"/home/eclt/.local/share/Trash/files"
	};
	iGiGpathArrLenght = sizeof(pathArr) / sizeof(pathArr[0]);
	m_printf("iGpathArrLenght =%d\n", iGiGpathArrLenght);
	
	// 监控目录
	pthread_t trashid;
	pthread_create(&trashid, NULL, (void*)vMoniterDir, &pathArr);
	//log_message("建立pthread_create, vMoniterDir");
	
	//原代码, 监控键盘鼠标
	pthread_t tid;
	pthread_create(&tid, NULL, &monitor, NULL);
	//log_message("建立pthread_create, monitor");
	
	while(1) {
		sleep(1);
		currentTime = time(NULL);
	}
	
	
	//用 malloc 需释放
	free(sFnConfig);
	sFnConfig = NULL;
	free(sSoundFullFile);
	sSoundFullFile = NULL;
	
	free(sFnConfig);
	for ( int i =0; i <MAX_SOUNDS ; i++) {
		free(arrSoundFile[i]);
		arrSoundFile[i] = NULL;
	}
	return 0;
}
