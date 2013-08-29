#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <locale.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#define LAST_BEEP_DELAY		20	/* seconds */

pthread_t waitkey;
short newmessage = 0;

void diep(char *str) {
	endwin();
	perror(str);
	exit(EXIT_FAILURE);
}

void split() {
	int i = getmaxx(stdscr);
	
	while(i-- > 1)
		printw("─");
	
	printw("\n");
}

void *reset(void *dummy) {
	while(1) {
		// waiting user input
		getchar();
		split();

		// reset black color
		bkgd(COLOR_PAIR(2));
		refresh();

		newmessage = 0;
	}
	
	return dummy;
}

void notificate(char *data) {
	time_t rawtime;
	struct tm *timeinfo;

	time(&rawtime);
	timeinfo = localtime(&rawtime);
	
	attron(COLOR_PAIR(3));
	attron(A_BOLD);

	printw("[%02d:%02d:%02d] %s\n", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, data);
	refresh();
	
	attron(COLOR_PAIR(1));
	attroff(A_BOLD);
	
	newmessage++;
}

void trim(char *data) {
	int len;

	len = strlen(data);

	if(data[len - 1] == '\n')
		data[len - 1] = '\0';
}

void sighandler(int signal) {
	switch(signal) {
		case SIGWINCH:
			endwin();
			refresh();
		break;
		
		case SIGINT:
			endwin();
			exit(1);
		break;
	}
}

void usage(char *argv0) {
	fprintf(stderr, "usage: %s [-h] file\n", argv0);
	exit(1);
}

int main(int argc, char *argv[]) {
	int i;
	char buffer[1024], *notiffile;
	FILE *fp;
	time_t last_beep = 0;

	if(argc < 2)
		usage(argv[0]);
		
	if(!strcmp(argv[1], "-h"))
		usage(argv[0]);

	notiffile = argv[1];

	setlocale(LC_CTYPE, "");
	
	initscr();              // init ncurses
	cbreak();               // no break line
	noecho();               // no echo key
	start_color();          // enable color
	use_default_colors();
	curs_set(0);            // disable cursor
	
	keypad(stdscr, TRUE);   // use scroll
	scrollok(stdscr, 1);

	/* handling CTRL+C and console resize */
	signal(SIGWINCH, sighandler);
	signal(SIGINT, sighandler);

	init_pair(1, COLOR_WHITE, COLOR_BLUE);
	init_pair(2, COLOR_WHITE, -1);
	init_pair(3, COLOR_YELLOW, COLOR_BLUE);

	/* scrolling to the bottom */
	for(i = 0; i < getmaxy(stdscr); i++)
		printw("\n");

	/* starting message */
	bkgd(COLOR_PAIR(2));
	attron(COLOR_PAIR(2));
	attron(A_BOLD);
	
	printw("Waiting for notifications...\n");
	split();
	
	attroff(A_BOLD);
	attron(COLOR_PAIR(1));
	
	refresh();

	// starting user input thread
	if(pthread_create(&waitkey, NULL, reset, NULL) != 0)
		diep("[-] pthread");

	/* Waiting new messages */
	while(1) {
		if(!(fp = fopen(notiffile, "r"))) {
			if(errno == EINTR)
				continue;
				
			diep("[-] fopen");
		}
		

		/* Waiting News */
		while(fgets(buffer, sizeof(buffer), fp)) {
			if(!newmessage) {
				/* background blue */
				bkgd(COLOR_PAIR(1));
				refresh();
			}
			
			// checking last time beep
			if(last_beep + LAST_BEEP_DELAY < time(NULL)) {
				printf("\x07");
				time(&last_beep);
			}

			trim(buffer);
			notificate(buffer);
		}

		fclose(fp);
	}

	endwin();

	return 0;
}
