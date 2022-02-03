// chlevelmap.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdio.h>
#include <windows.h>
#include <atlstr.h>

bool used[3][25];
CString fn;

void process(FILE *fp) {
	for (int idx=0; idx<16; idx++) {
		char buf[128];
		fgets(buf, 128, fp);
		for (int idx2=0; idx2<21; idx2++) {
			int pos=idx2*3+1;
			int page=buf[pos]-'1';
			if ((page<0)||(page>2)) {
				printf("Bad page! Line %d, pos %d, file %s\n", idx, idx2, (LPCSTR)fn);
				exit(-1);
			}
			if (page == 2) {
				// this one accounts for six tiles used on 2 pages:
				// the ground underneath the destructible, and the
				// destructible's animation sequence
				int page2=buf[pos+1];
				int tile2=toupper(page2)-'A';
				if (page2 >= 'a') page2=1; else page2=0;
				used[page2][tile2]=true;

				int tile=buf[pos+2]-'A';
				for (int idx3=0; idx3<5; idx3++) {
					used[page][tile+idx3]=true;
				}
			} else {
				// this is just one used item
				int tile=buf[pos+2]-'A';
				used[page][tile]=true;
			}
		}
	}
}

int _tmain(int argc, _TCHAR* argv[])
{
	if (argc < 2) {
		printf("Pass a partial filename to do level processing for CH\n");
		return -1;
	}

	CString basefn=argv[1];
	memset(used, 0, sizeof(used));

	printf("Checking maps...\n");

	fn=basefn+"a.txt";
	FILE *fp=fopen(fn, "r");
	if (NULL == fp) {
		printf("Can't open file %s\n", (LPCSTR)fn);
		return -1;
	}
	process(fp);
	fclose(fp);

	fn=basefn+"b.txt";
	fp=fopen(fn, "r");
	if (NULL == fp) {
		printf("Can't open file %s\n", (LPCSTR)fn);
		return -1;
	}
	process(fp);
	fclose(fp);

	fn=basefn+"c.txt";
	fp=fopen(fn, "r");
	if (NULL == fp) {
		printf("Can't open file %s\n", (LPCSTR)fn);
		return -1;
	}
	process(fp);
	fclose(fp);

	// we are not keeping the challenge stages, though
	// I think not scanning it only affects the toy factory
#if 0
	fn=basefn+"d.txt";
	fp=fopen(fn, "r");
	if (NULL == fp) {
		printf("Can't open file %s\n", (LPCSTR)fn);
		return -1;
	}
	process(fp);
	fclose(fp);
#endif

	printf("Deleting unused tiles...\n");

	// now we go through and delete the unused files
	for (int page=0; page<3; page++) {
		for (int pos=0; pos<25; pos++) {
			if (!used[page][pos]) {
				for (int frame=0; frame<4; frame++) {
					fn.Format("%c%d-%d.png", 'a'+page, frame+1, pos);
					printf("Deleting unused frame %s...\n", (LPCSTR)fn);
					DeleteFile(fn);
				}
			}
		}
	}

	printf("Done.\n");
	
	return 0;
}

