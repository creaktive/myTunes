#include <err.h>
#include <dirent.h>
#include <fcntl.h>
#include <signal.h>
#include <sqlite3.h>
#include <stdio.h>
#include <string.h>
#include <sys/event.h>
#include <sys/stat.h>

#define MEDIA	"/private/var/mobile/Media"
#define LIBFILE	MEDIA "/iTunes_Control/iTunes/iTunes Library.itlp/Library.itdb"
#define LOCFILE	MEDIA "/iTunes_Control/iTunes/iTunes Library.itlp/Locations.itdb"
#define ITFILES	MEDIA "/iTunes_Control/Music"
#define MYTUNES	MEDIA "/myTunes"

int kq, pid;

int wait(char *file) {
	int fd;

	do {
		usleep(100000);
		fd = open(file, O_RDONLY);
	} while (fd == -1);

	return fd;
}

int mon(int fd) {
	struct kevent ke;
	int i = fd;

	if (i != -1)
		close(i);

	fd = wait(LIBFILE);

	/*
	if (i != -1)
		kill(pid, SIGUSR1);
	*/
	kill(pid, SIGUSR1);

	EV_SET(&ke, fd, EVFILT_VNODE, EV_ADD, NOTE_DELETE | NOTE_WRITE | NOTE_EXTEND | NOTE_ATTRIB | NOTE_LINK | NOTE_RENAME | NOTE_REVOKE, 0, NULL);
	if (kevent(kq, &ke, 1, NULL, 0, NULL) == -1)
		err(1, "kevent2");

	return fd;
}

void postpone(int sig) {
	alarm(5);
}

static int callback(void *empty, int argc, char **argv, char **col) {
	char *p;
	char title[256];
	unsigned int i;
	struct stat fs;

	if (stat(argv[0], &fs) == -1)
		return 0;

	memset(title, '\0', sizeof(title));
	for (i = 0, p = argv[1]; (i < sizeof(title) - 1) && (*p != '\0'); i++, p++)
		switch (*p) {
			case ':':
			case '*':
			case '?':
				title[i] = '_';
				break;
			default:
				title[i] = *p;
		}

	//printf("%s\t%s\n", argv[0], title);
	link(argv[0], title);
	return 0;
}

int query(sqlite3 *db, char *sql, void *cb) {
	int i;
	char *err;

	for (i = 0; i < 5; i++)
		if (sqlite3_exec(db, sql, cb, 0, &err) == SQLITE_OK)
			return 1;
		else {
			warn("sqlite3_exec %s", err);
			sqlite3_free(err);
			usleep(100000);
		}

	return 0;
}

void clean() {
	DIR *pd;
	struct dirent *pde;

	pd = opendir(MYTUNES);
	if (pd == NULL)
		return;

	for (pde = readdir(pd); pde != NULL; pde = readdir(pd))
		if (pde->d_type == DT_REG)
			unlink(pde->d_name);

	closedir(pd);
}

void fix(int sig) {
	int ok;
	sqlite3 *db;

	alarm(0);

	signal(SIGUSR1, SIG_IGN);
	signal(SIGALRM, SIG_IGN);

	close(wait(LOCFILE));
	close(wait(LIBFILE));

	mkdir(MYTUNES, S_IRWXU | S_IRWXG | S_IRWXO);
	chdir(MYTUNES);
	clean();

	if (sqlite3_open(LIBFILE, &db))
		warn("sqlite3_open %s", sqlite3_errmsg(db));
	else if (
		query(db, "ATTACH '" LOCFILE "' AS loc", NULL) &&
		query(db, "SELECT '" ITFILES "/' || location, title || SUBSTR(location, 9) FROM item LEFT JOIN loc.location ON (pid = item_pid) ORDER BY location ASC", callback) &&
		query(db, "DETACH DATABASE loc", NULL)
	)
		ok = 1;

	sqlite3_close(db);

	signal(SIGUSR1, postpone);
	signal(SIGALRM, fix);
}

int main() {	
	int fd, null;
	struct kevent ke;

	if ((null = open("/dev/null", O_RDWR)) == -1)
		err(1, "null");

	switch (fork()) {
		case -1:
			err(1, "fork1");
		case  0:
			setsid();
			switch (fork()) {
				case -1:
					err(1, "fork2");
				case  0:
					// chdir("/");
					umask(0);

					// close standart IO
					close(0);
					close(1);
					close(2);

					// open'em as /dev/null
					dup2(null, 0);
					dup2(null, 1);
					dup2(null, 2);

					break;
				default:
					return 0;
			}
			break;
		default:
			return 0;
	}

	pid = fork();
	if (pid == -1)
		err(1, "fork3");
	else if (pid == 0) {
		signal(SIGUSR1, postpone);
		signal(SIGALRM, fix);

		while (1)
			pause();
	} else {
		kq = kqueue();
		if (kq == -1)
			err(1, "kqueue");

		for (fd = mon(-1); 1; fd = mon(fd)) {
			memset(&ke, '\0', sizeof(ke));
			
			if (kevent(kq, NULL, 0, &ke, 1, NULL) == -1)
				err(1, "kevent1");
		}
	}
}