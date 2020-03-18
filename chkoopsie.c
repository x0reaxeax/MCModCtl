#include <stdio.h>
#include <Windows.h>
#include <stdbool.h>
#include <string.h>
//#include <stdint.h>

/* fuck u microsoft */
#pragma warning(disable: 4996)

/* args */
#define CMP_MAX	 8

/* dirpath */
#define PATH_MAX 2048

/* clr macros */
#define CMD_GREEN SetConsoleTextAttribute(hConsole, 10);
#define CMD_RED	  SetConsoleTextAttribute(hConsole, 12);
#define CMD_MGNT  SetConsoleTextAttribute(hConsole, 13);

int dumpMods(const char *sDir, unsigned int itr) {
	WIN32_FIND_DATA fdFile;
	HANDLE hFind = NULL;
	
	char opDir[PATH_MAX] = "xodbg.txt";
	char sPath[PATH_MAX];

	FILE *fp = fopen(opDir, "ab");

	if (!fp) {
		printf("[-] Unable to open '%s' for writing (ab)!\n", opDir);
		return EXIT_FAILURE;
	}

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
				printf("[+] Omitting directory '%s' ..\n\n", sPath);
			}
			else {
				printf("[+] File: '%s'\n", fdFile.cFileName);

				size_t fNameLen = strlen(fdFile.cFileName);
				const char *extension = &fdFile.cFileName[fNameLen - 4];

				if (strncmp(extension, ".jar", 4) == 0) {
					printf("[+] Found mod: '%s' -- writing to '%s' ..\n", fdFile.cFileName, opDir);
					fprintf(fp, "[%u] %s\n", itr, fdFile.cFileName);
					itr++;
				}
			}
		}
	} while (FindNextFile(hFind, &fdFile));

	FindClose(hFind);

	fprintf(fp, "\n\n-----------EOF-----------\n\n");
	fclose(fp);

	return 0;
}

void printBanner(void) {
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	CMD_RED;
	printf("*****************************************\n");
	printf("*");
	CMD_MGNT;
	printf(" Minecraft Oopsie Checker by x0reaxeax ");
	//printf(" Mod Dumper by x0reaxeax ");
	CMD_RED;
	printf("*\n");
	printf("*****************************************\n\n");

	CMD_GREEN;
}

bool checkDir(const char *pDir) {
	/* two-liner, it's these guys who're cool these days.. */
	DWORD dwAtrb = GetFileAttributes(pDir);
	return (dwAtrb != INVALID_FILE_ATTRIBUTES && (dwAtrb & FILE_ATTRIBUTE_DIRECTORY));
}

void _cexit(void) {
	printf("[+] Press 'Enter' to continue..\n");
	char c = getchar();
}

int main(int argc, const char **argv) {

	if (argc > 1) {
		if (strncmp(argv[1], "-h", CMP_MAX) == 0 || strncmp(argv[1], "-v", CMP_MAX) == 0) {
			printf(" version 1.0 -- there's no auto update system, check github for updates mf..\n");
			printf("  :: https://github.com/x0reaxeax/MCModCtl/releases \n");
			return EXIT_SUCCESS;
		}
	}

	//atexit(_cexit);
	
	printBanner();

	char modsDir[PATH_MAX] = "mods";
	char flanDir[PATH_MAX] = "Flan";

	bool modsCheck = checkDir(modsDir);
	bool flanCheck = checkDir(flanDir);

	if (!modsCheck || !flanCheck) {
		/* 1337 check */
		char *ptr = !modsCheck ? modsDir : flanDir;

		printf("[-] Directory '%s' doesn't exist!\n", ptr);
		printf(" * Make sure '%s' is in '.minecraft' folder and try again!\n\n", argv[0]);
	
		_cexit();
		return EXIT_FAILURE;
	}
	
	//uint8_t taint = 0;
	bool taint = false;

	unsigned int modsItr = 0;
	unsigned int flanItr = 0;

	unsigned int expMods = 96;
	unsigned int expFlan = 2;

	dumpMods(modsDir, modsItr);
	dumpMods(flanDir, flanItr);

	if (modsItr != 0)
		modsItr++;

	if (flanItr != 0)
		flanItr++;

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
	_cexit();

	return EXIT_SUCCESS;
}
