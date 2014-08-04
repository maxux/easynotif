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
#include <sqlite3.h>

#define LAST_BEEP_DELAY		20	/* seconds */

pthread_t waitkey;
short newmessage = 0;
sqlite3 *db;

//
// database
//

sqlite3 *db_sqlite_init(char *filename) {
	sqlite3_config(SQLITE_CONFIG_SERIALIZED);
	
	if(sqlite3_open(filename, &db) != SQLITE_OK) {
		fprintf(stderr, "[-] sqlite: cannot open sqlite databse: <%s>\n", sqlite3_errmsg(db));
		return NULL;
	}
	
	sqlite3_busy_timeout(db, 30000);
	
	return db;
}

int db_query(sqlite3 *db, char *sql) {
	if(sqlite3_exec(db, sql, NULL, NULL, NULL) != SQLITE_OK) {
		fprintf(stderr, "[-] sqlite: query <%s> failed: %s\n", sql, sqlite3_errmsg(db));
		return 0;
	}
	
	return 1;
}

int db_insert(char *data) {
	int value;
	char *sqlquery;
	
	sqlquery = sqlite3_mprintf(
		"INSERT INTO notifications (timestamp, read, message) "
		"VALUES (%u, 0, '%q')",
		time(NULL),
		data
	);
	
	value = db_query(db, sqlquery);	
	sqlite3_free(sqlquery);
	
	return value;
}

//
// ncurses
//

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
		
		// mark all as read
		db_query(db, "UPDATE notifications SET read = 1");
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
	
	// insert message in backlog database
	db_insert(data);
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
	fprintf(stderr, "usage: %s [-h] fifo-path database-path\n", argv0);
	exit(1);
}

int main(int argc, char *argv[]) {
	int i;
	char buffer[1024], *notiffile;
	FILE *fp;
	time_t last_beep = 0;

	if(argc < 3)
		usage(argv[0]);
		
	if(!strcmp(argv[1], "-h"))
		usage(argv[0]);

	notiffile = argv[1];
	
	// initializing database
	db_sqlite_init(argv[2]);
	db_query(db, "CREATE TABLE IF NOT EXISTS notifications (timestamp integer, read integer, message text)");
	db_query(db, "CREATE INDEX IF NOT EXISTS index_ts ON notifications (timestamp)");
	db_query(db, "CREATE INDEX IF NOT EXISTS index_read ON notifications (read)");

	// initializing console
	setlocale(LC_CTYPE, "");
	
	initscr();              // init ncurses
	cbreak();               // no break line
	noecho();               // no echo key
	start_color();          // enable color
	use_default_colors();
	curs_set(0);            // disable cursor
	
	keypad(stdscr, TRUE);   // use scroll
	scrollok(stdscr, 1);

	// handling ctrl+c and console resize
	signal(SIGWINCH, sighandler);
	signal(SIGINT, sighandler);

	init_pair(1, COLOR_WHITE, COLOR_BLUE);
	init_pair(2, COLOR_WHITE, -1);
	init_pair(3, COLOR_YELLOW, COLOR_BLUE);

	// scrolling to the bottom
	for(i = 0; i < getmaxy(stdscr); i++)
		printw("\n");

	// starting message
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

	// waiting new messages
	while(1) {
		if(!(fp = fopen(notiffile, "r"))) {
			if(errno == EINTR)
				continue;
				
			diep("[-] fopen");
		}
		

		// waiting news
		while(fgets(buffer, sizeof(buffer), fp)) {
			if(!newmessage) {
				// blue background
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
