#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <Windows.h>
#include <string.h>
#include <WinInet.h>
#include <errno.h>
#include <time.h>
#include <stdlib.h>
#include <direct.h>

#pragma warning(disable: 4996)

#define CMD_GREEN SetConsoleTextAttribute(hConsole, 10);
#define CMD_RED	  SetConsoleTextAttribute(hConsole, 12);
#define CMD_MGNT  SetConsoleTextAttribute(hConsole, 13);

#define LOG_OK 1
#define LOG_FAILURE -1

#define PATH_MAX		2048
#define BUFF_MAX		256
#define ERR_MAX			256
#define SMALLBUFF_MAX	16
#define CUT_LEN			8

#define INSTALL_SUCCESS "mcmctl=0"

#define IS_INSTALLED   0
#define NOT_INSTALLED -1

#define ssize_t SSIZE_T
#define EFOPEN -EXIT_FAILURE;

int8_t logStatus = 0;
uint8_t verbose = 0;

char LOGFILE[PATH_MAX];
const char *modPath = "mods.zip";

HANDLE hConsole;

ssize_t write_log(const char* message, ...) {

	if (logStatus != LOG_OK) {
		return 0;
	}

	va_list args;
	char* newline = "\n";

	va_start(args, message);
	FILE *errfp = fopen(LOGFILE, "ab");

	if (!errfp) {
		printf("[-]: Unable to create log file -> '%s'\n", LOGFILE);
		return EFOPEN;
	}

	size_t retsum;

	time_t now;
	time(&now);

	char tt_time[256];

	sprintf(tt_time, ctime(&now));
	size_t time_len = strlen(tt_time);
	if (tt_time[time_len - 1] == '\n') {
		tt_time[time_len - 3] = 0;
	}

	size_t _prefix = fprintf(errfp, "[%s]: ", tt_time);
	size_t _suffix = vfprintf(errfp, message, args);
	fprintf(errfp, newline);

	va_end(args);
	fclose(errfp);

	retsum = _prefix + _suffix + 1;

	return retsum;
}

/* err message formatter */
LPCSTR formatErrMsg(DWORD _Err) {
	LPCSTR lpMsgBuf;

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		_Err,
		MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
		/*MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),    // je ne parle pas francais */
		(LPTSTR)&lpMsgBuf,
		0, NULL);

	return lpMsgBuf;
}

bool checkDir(const char *pDir) {
	/* two-liner, it's these guys who're cool these days.. */

	DWORD dwAtrb = GetFileAttributes(pDir);
	return (dwAtrb != INVALID_FILE_ATTRIBUTES && (dwAtrb & FILE_ATTRIBUTE_DIRECTORY));
}

size_t getTotalMemory(void) {
	MEMORYSTATUSEX memstat;

	memstat.dwLength = sizeof(memstat);

	GlobalMemoryStatusEx(&memstat);

	return (size_t)memstat.ullTotalPhys;
}

void calcCheckRAM(void) {
	printf("[+] Checking RAM status ..\n");
	float mem = (float)getTotalMemory() / (float)1073741824;

	printf("[+] Total physical memory installed: %.2fGB\n", mem);

	if (mem < (float)8) {
		printf("\n[!] Your physical memory is below recommended minimum! (recommended: 16GB / minimum recommended: 8GB)\n");
		printf("[!] ..expect shit to hit the fan..\n");
	}
	printf("[+] Calculating optimal memory settings ..\n");

	unsigned long xmem = (mem / 2) - 1024;
	if (xmem > 8192) xmem = 8192;

	printf("[+] Optimal memory settings = %lu MB\n", xmem);
}

int checkInternetCon(void) {
	printf("[+] Checking internet connection..\n");

	bool netStatus = InternetCheckConnection("http://github.com/", FLAG_ICC_FORCE_CONNECTION, 0);

	if (!netStatus) {
		printf("[-] Unable to establish internet connection!\n");
		return EXIT_FAILURE;
	}

	printf("[+] Internet connection ..... [OK]\n\n");
	return EXIT_SUCCESS;
}

int createLogFile(void) {
	strcpy(LOGFILE, "xopack.log");
	printf("[+] Creating log file for XoPack installer at  '%s' ..\n", LOGFILE);
	printf("[+] Writing debug entry to log file at '%s' ..\n", LOGFILE);

	logStatus = LOG_OK;

	ssize_t spawnLogRes =
		write_log("[DBG]: LOG_INIT -> Hello World! I'm a log file!");

	if (spawnLogRes > 1) {
		printf("[+] Successfully initialized log file!\n");
		return EXIT_SUCCESS;
	}

	logStatus = LOG_FAILURE;
	return EXIT_FAILURE;
}

int downloadMods(void) {
	
	const char *modURL =  "https://github.com/x0reaxeax/MCModCtl/releases/download/1.0/xopack.zip";		/* mmc_mods.rar */
	//const char *modURL = "http://127.0.0.1/xopack.zip"; //dbg

	printf("[+] Downloading mods.. (this may take a while, grab a coffee..)\n");

	HRESULT modsDlRes = URLDownloadToFile(NULL, modURL, modPath, 0, NULL);

	if (modsDlRes != S_OK) {
		printf("[-] Unable to download mods!\n");
		write_log("[ERR]: Unable to download mods! HRESULT -> '%s'\n", formatErrMsg(GetLastError()));
		return EXIT_FAILURE;
	}
	
	printf("[+] Successfully downloaded mods -> '%s'\n", modPath);

	return EXIT_SUCCESS;
}

void printMenu(void) {
	CMD_RED;
	printf("***********************************\n");
	printf("**");
	printf("*");
	CMD_MGNT;
	printf(" XoPack installer by x0reaxeax ");
	CMD_RED;
	printf("*\n");
	printf("***********************************\n\n");
}

void extractMods(void) {
	char extractCMD[BUFF_MAX];
	snprintf(extractCMD, BUFF_MAX - 1, "7za.exe x %s", modPath);
	printf("[+] Extracting mods.. \n");

	printf("\n\n");
	system(extractCMD);
	printf("\n\n");
}


unsigned int dumpMods(const char *sDir, unsigned int itr) {
	WIN32_FIND_DATA fdFile;
	HANDLE hFind = NULL;

	char sPath[PATH_MAX];

	sprintf(sPath, "%s\\*", sDir);

	if ((hFind = FindFirstFile(sPath, &fdFile)) == INVALID_HANDLE_VALUE) {
		printf("[-] Unable to locate 'mods' directory!\n");
		return EXIT_FAILURE;
	}

	do
	{
		if (strcmp(fdFile.cFileName, ".") != 0 && strcmp(fdFile.cFileName, "..") != 0) {
			sprintf(sPath, "%s\\%s", sDir, fdFile.cFileName);

			if (fdFile.dwFileAttributes &FILE_ATTRIBUTE_DIRECTORY) {
			}
			else {

				size_t fNameLen = strlen(fdFile.cFileName);
				const char *extension = &fdFile.cFileName[fNameLen - 4];

				if (strncmp(extension, ".jar", 4) == 0) {
					++itr;
				}
			}
		}
	} while (FindNextFile(hFind, &fdFile));

	FindClose(hFind);

	return itr;
}

int doOopsieCheck(void) {

	const char modsDir[PATH_MAX] = "mods";
	const char flanDir[PATH_MAX] = "Flan";


	bool modsCheck = checkDir(modsDir);
	bool flanCheck = checkDir(flanDir);

	if (!modsCheck || !flanCheck) {
		/* 1337 check */
		char *ptr = !modsCheck ? modsDir : flanDir;

		printf("[-] Directory '%s' doesn't exist!\n", ptr);
		return EXIT_FAILURE;
	}

	bool taint = false;

	unsigned int expMods = 90;
	unsigned int expFlan = 1;

	unsigned int modsItr = dumpMods(modsDir, 0);
	unsigned int flanItr = dumpMods(flanDir, 0);

	if (modsItr != expMods) {
		printf("[!] Warning: Missing %u mods!\n"
			" * Installed mods: %u\n"
			" * Expected Mods: %u\n\n", expMods - modsItr, modsItr, expMods);

		taint = true;
	}

	if (flanItr != expFlan) {
		printf("[!] Warning: Missing %u Flan's packs!\n"
			" * Installed packs: %u\n"
			" * Expected packs: %u\n\n", expFlan - flanItr, flanItr, expFlan);
		taint = true;
	}
	printf("\n[1337 Report]:\n\n");
	if (!taint) {
		printf(" * [TAINT = %s]   Everything should be okay bruh..\n", taint ? "true" : "false");
	}
	else {
		printf(" * [TAINT = %s]   Oopsie.. You done fucked up..\n", taint ? "true" : "false");
	}

	printf("\n\n");

	return EXIT_SUCCESS;

}

void _cexit(void) {
	printf("[+] Press 'Enter' to continue..\n");
	char c = getchar();
}

int main(int argc, const char **argv) {
	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	printMenu();
	CMD_GREEN;
	createLogFile();

	calcCheckRAM();

	int statusNET = checkInternetCon();

	if (statusNET != EXIT_SUCCESS) {
		_cexit();
		return EXIT_FAILURE;
	}

	int dlMods = downloadMods();

	if (dlMods != EXIT_SUCCESS) {
		_cexit();
		return EXIT_FAILURE;
	}

	extractMods();

	printf("[+] Running 'Oopsie Check'.. \n\n");
	Sleep(500);

	int statusOOPSIE = doOopsieCheck();

	if (statusOOPSIE != EXIT_SUCCESS) {
		_cexit();
		return EXIT_FAILURE;
	}

	_cexit();

	return EXIT_SUCCESS;

}
