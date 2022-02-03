// ch_story.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdio.h>
#include <ctype.h>

int _tmain(int argc, _TCHAR* argv[])
{
	FILE *in, *out;
	char buf[128];

	in=fopen(argv[1], "r");
	out=fopen(argv[2], "w");

	while (!feof(in)) {
		if (NULL == fgets(buf, 128, in)) break;
		if ('\0' == buf[0]) {
			fputs(buf, out);
			continue;
		}
		if ('\n' == buf[0]) {
			fputs(buf, out);
			continue;
		}
		if (' ' == buf[0]) {
			fputs(buf, out);
			continue;
		}
		if ('/' == buf[0]) {
			// comment
			fputs(buf, out);
			continue;
		}
		if (isdigit(buf[0])) {
			// line of text
			// Remove trailing eol chars
			char *p1=buf+strlen(buf)-1;
			while (*p1 < ' ') {
				*p1='\0';
				p1--;
				if (p1 < buf) break;
			}
			// remove the spaces after the digits
			p1=&buf[4];
			if (*p1 != '\0') {
				char *p2=p1+1;
				while (isspace(*p2)) p2++;
				memmove(p1, p2, strlen(p2)+1);
				// now parse the text for 32 characters
				int col=0;
				char *pLast=NULL;
				while (*p1) {
					if (*p1 == ' ') pLast=p1+1;
					if ((*p1 == '\\') && (*(p1+1) == 'n')) {
						p1+=2;
						col=0;
						pLast=NULL;
						continue;
					}
					col++;
					if (col == 26) {
						if (pLast == NULL) {
							// there were no spaces
							memmove(p1+2, p1, strlen(p1)+1);
							*p1='\\';
							*(p1+1)='n';
							p1+=2; 
							col=0;
							continue;
						}
						// there was a space, line break there
						memmove(pLast+1, pLast, strlen(pLast)+1);
						*(pLast-1)='\\';	// space
						*(pLast)='n';
						col=0;
						p1=pLast+1;
						pLast=NULL;
					}
					p1++;
				}
			}
			// escape any inbed quotes
			p1 = buf;
			while (p1 = strchr(p1, '\"')) {
				memmove(p1+1, p1, strlen(p1)+1);
				*p1='\\';
				p1+=2;
			}
			// and convert unavailable characters to something we do have
			char szAvail[]="!?.,:;\"\'\\-@#$%^ ";
			// note: we don't really have \, that's for \n only
			p1 = buf;
			while (*p1) {
				if ((!isalnum(*p1)) && (NULL == strchr(szAvail, *p1))) {
					*p1='-';
				}
				p1++;
			}

			fprintf(out, "\"%s\\0\"\n", buf);

		} else {
			// command line
			// Remove trailing eol chars
			char *p1=buf+strlen(buf)-1;
			while (*p1 < ' ') {
				*p1='\0';
				p1--;
				if (p1 < buf) break;
			}
			fprintf(out, "\"`%s\\0\"\n", buf);
		}
	}

	fclose(out);
	fclose(in);

	return 0;
}

