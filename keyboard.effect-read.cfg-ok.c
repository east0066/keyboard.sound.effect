
/*
	gcc keyboard.effect-read.cfg-ok.c  -o  keyboard.effect-read.cfg-ok	
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 1024
#define MAX_SOUNDS 39
char *sConfigFilename="keyboard.effect.cfg";
char *arrSoundFile[MAX_SOUNDS];
char *arrKeyName[] ={
"",
"1.esc",
"2.function.alone[f1-f10]",
"3.function.combination[fn+f1, fn+f2...]",
"4.tilde[~]",
"5.number[0-9]",
"6.alphabetic[a-zA-Z]",
"7.punctuation",
"8.backspace",
"9.enter",
"10.up",
"11.down",
"12.left",
"13.right",
"14.mouse.steel.scroll.front.up",
"15.mouse.steel.scroll.back",
"16.trash.clear",
"17.small.keyboard.0",
"18.small.keyboard.1",
"19.small.keyboard.2",
"20.small.keyboard.3",
"21.small.keyboard.4",
"22.small.keyboard.5",
"23.small.keyboard.6",
"24.small.keyboard.7",
"25.small.keyboard.8",
"26.small.keyboard.9",
"27.small.keyboard.enter",
"28.small.keyboard.dot",
"29.small.keyboard.add",
"30.small.keyboard.subtract",
"31.small.keyboard.multiply",
"32.small.keyboard.divide",
"33.small.keyboard.numlock",
"34.printscreen",
"35.tab",
"36.capslock",
"37.insert",
"38.delete"
};

char *rtrim(char *str)
{
    if (str == NULL || *str == '\0')
    {
        return str;
    }
    int len = strlen(str);
    char *p = str + len - 1;
    while (p >= str && isspace(*p))
    {
        *p = '\0';
        --p;
    }
    return str;
}

char *ltrim(char *str)
{
    if (str == NULL || *str == '\0')
    {
        return str;
    }
    int len = 0;
    char *p = str;
    while (*p != '\0' && isspace(*p))
    {
        ++p;
        ++len;
    }
    memmove(str, p, strlen(str) - len + 1);
    return str;
}

int contains(const char *str1, const char *str2) {
    return strstr(str1, str2) != NULL;
}

void cfgToArr(char* sKeyName, char* sSoundFile) {
	for (int i =1; i<MAX_SOUNDS; i++) {
		if (contains(sKeyName, arrKeyName[i])) {
			arrSoundFile[i] = (char*)malloc(strlen(sSoundFile)+1);
			strcpy(arrSoundFile[i], sSoundFile);
		}
	}
}

void readcfg(const char *fname) {
	FILE *file = fopen(fname, "r");
    if (file == NULL) {
        perror("Error opening file");
        exit(1);
    }
    char line[MAX_LINE_LENGTH];
    char *str1, *str2;
    while (fgets(line, sizeof(line), file) != NULL) {		
		str1 = "";
		str2 = "";
		rtrim(line);
		ltrim(line);
		
		if (line[0] == '#') {
			continue;
		}
		
		char *buf = line;
		char *str_p = NULL;
		char *a1 = "=";
		char *token;
		while((token = strtok_r(buf, a1, &str_p)) != NULL)
		{
			if(token != NULL)
				if (str1 == "") {
					str1 = token;
				}else if (str2 =="") {
					str2 = token;
					cfgToArr(str1, str2);
				}				
			buf = NULL;
		 }
    }
    fclose(file);
    //~ for (int i=1 ; i<MAX_SOUNDS; i++) {
		//~ if(arrSoundFile[i] != NULL) {
			//~ printf("arrSoundFile[%d] = %s\n", i, arrSoundFile[i]);
		//~ }
	//~ }
}

/*
int main() {
    readcfg(sConfigFilename);
    
	for ( int i =0; i <MAX_SOUNDS ; i++) {
		free(arrSoundFile[i]);
	}
	
    return 0;
}
*/
