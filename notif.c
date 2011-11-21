#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>

#include "notif.h"

void print_line(char *data) {
	time_t rawtime;
	struct tm * timeinfo;
	
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	
	printw("[%d:%d:%d] %s\n", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, data);
	refresh();
}

void trim_text(char *data) {
	int len;
	
	len = strlen(data);
	
	if(data[len - 1] == '\n')
		data[len - 1] = '\0';
}

void dummy(int signal) {
	
}

void mark_as_read() {
	int i;
	
	while(1) {
		getchar();
		
		i = getmaxx(stdscr);
		while(i-- > 1)
			printw("-");
		
		printw("\n");
		bkgd(COLOR_PAIR(2));
		refresh();
		bkgd(COLOR_PAIR(1));
	}
}

int main(int argc, const char *argv[]) {
	int i;
	char buffer[1024];
	FILE *fp;
	pthread_t waitkey;
	
	/* Init curses */
	initscr();		/* Init ncurses */
	cbreak();		/* No break line */
	noecho();		/* No echo key */
	start_color();		/* Enable color */
	curs_set(0);		/* Disable cursor */
	keypad(stdscr, TRUE);

	/* Init Colors */
	init_pair(1, COLOR_WHITE, COLOR_BLUE);
	init_pair(2, COLOR_WHITE, COLOR_BLACK);
	
	attron(COLOR_PAIR(1));
	bkgd(COLOR_PAIR(1));
	
	/* State Message */
	attron(COLOR_PAIR(1));
	attron(A_BOLD);
	
	scrollok(stdscr, 1);
	
	for(i = 0; i < getmaxy(stdscr); i++)
		printw("\n");
		
	print_line("Waiting notifications...");
	attroff(A_BOLD);
	
	attron(COLOR_PAIR(1));
	
	signal(SIGWINCH, dummy);
	
	refresh();
	
	if(pthread_create(&waitkey, NULL, (void *) mark_as_read, (void*) NULL) != 0)
		return 1;
	
	while(1) {
		/* Init Files */
		fp = fopen(NOTIF_FILE, "r");
		if(!fp) {
			perror("fopen");
			return 1;
		}
		
		/* Waiting News */
		while(fgets(buffer, sizeof(buffer), fp) != NULL) {
			trim_text(buffer);
			print_line(buffer);
		}
	
		fclose(fp);
	}
	
	endwin();

	return 0;
}
