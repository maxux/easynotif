#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

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
		case SIGWINCH:
			endwin();
			refresh();
		break;
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

void usage(char *argv0) {
	fprintf(stderr, "usage: %s [-h] file\n", argv0);
	exit(1);
}

int main(int argc, char *argv[]) {
	int i;
	char buffer[1024], *notiffile;
	FILE *fp;
	pthread_t waitkey;
	char newmsg;

	if(argc < 2)
		usage(argv[0]);
	if(!strcmp(argv[1], "-h"))
		usage(argv[0]);

	notiffile = argv[1];

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
	init_pair(3, COLOR_YELLOW, COLOR_BLUE);

	attron(COLOR_PAIR(2));
	bkgd(COLOR_PAIR(2));

	/* State Message */
	for(i = 0; i < getmaxy(stdscr); i++)
		printw("\n");

	/* Starting with 'no news messages' */
	attron(COLOR_PAIR(2));
	attron(A_BOLD);
	print_line("Waiting for notifications...");
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
		fp = fopen(notiffile, "r");
		if(!fp) {
			endwin();
			perror("fopen");
			return 1;
		}

		/* Waiting News */
		while(fgets(buffer, sizeof(buffer), fp) != NULL) {
			if(!newmsg) {
				/* Background blue */
				bkgd(COLOR_PAIR(1));
				refresh();

				/* Beep */
				printf("\x07");
			}

			/* New Message */
			attron(COLOR_PAIR(3));
			attron(A_BOLD);

			trim_text(buffer);
			print_line(buffer);

			newmsg++;
		}

		fclose(fp);
	}

	endwin();

	return 0;
}
