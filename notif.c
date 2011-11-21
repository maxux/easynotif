#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

#include "notif.h"

void print_line(char *data) {
	time_t rawtime;
	struct tm * timeinfo;
	
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	
	printw("[%02d:%02d:%02d] %s\n", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, data);
	refresh();
}

void trim_text(char *data) {
	int len;
	
	len = strlen(data);
	
	if(data[len - 1] == '\n')
		data[len - 1] = '\0';
}

void dummy(int signal) {
	switch(signal) {
		// Nothing yet
	}
}

void mark_as_read(int *newmsg) {
	int i;
	
	while(1) {
		getchar();
		
		i = getmaxx(stdscr);
		while(i-- > 1)
			printw("-");
		
		printw("\n");
		
		bkgd(COLOR_PAIR(2));
		refresh();
		
		*newmsg = 0;
	}
}

void blink(void) {
	int i;
	
	usleep(200000);
	
	for(i = 0; i < 15; i++) {
		bkgd(COLOR_PAIR(2));
		refresh();
		
		usleep(100000);
		
		bkgd(COLOR_PAIR(1));
		refresh();
		
		usleep(100000);
	}
}

int main(void) {
	int i;
	char buffer[1024];
	FILE *fp;
	pthread_t waitkey, blinkwin;
	char newmsg;
	
	/* Init curses */
	initscr();		/* Init ncurses */
	cbreak();		/* No break line */
	noecho();		/* No echo key */
	start_color();		/* Enable color */
	curs_set(0);		/* Disable cursor */
	keypad(stdscr, TRUE);
	
	/* Init Scroll */
	scrollok(stdscr, 1);
	
	/* Skipping Resize Signal */
	signal(SIGWINCH, dummy);

	/* Init Colors */
	init_pair(1, COLOR_WHITE, COLOR_BLUE);
	init_pair(2, COLOR_WHITE, COLOR_BLACK);
	
	attron(COLOR_PAIR(2));
	bkgd(COLOR_PAIR(2));
	
	/* State Message */	
	for(i = 0; i < getmaxy(stdscr); i++)
		printw("\n");
	
	/* Starting with 'no news messages' */
	attron(COLOR_PAIR(2));
	attron(A_BOLD);	
	print_line("Waiting notifications...");
	attroff(A_BOLD);
	
	/* Waiting new messages */
	attron(COLOR_PAIR(1));
	refresh();
	
	/* Init Message Count */
	newmsg = 0;
	
	if(pthread_create(&waitkey, NULL, (void *) mark_as_read, (void*) &newmsg) != 0)
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
			if(!newmsg) {
				if(pthread_create(&blinkwin, NULL, (void *) blink, (void*) NULL) != 0) {
					return 1;
				}
			}
				
			trim_text(buffer);
			print_line(buffer);
			
			newmsg++;
		}
	
		fclose(fp);
	}
	
	endwin();

	return 0;
}
