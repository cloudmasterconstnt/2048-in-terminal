#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/file.h>
#include "save.h"

#define PATH_LEN 256

#define MAX_POSSIBLE_SCORE  3932156
#define MAX_POSSIBLE_TILE   17

static char filename[PATH_LEN];
static int  fd;

static enum {NORMAL, NO_SAVE} save_mode = NORMAL;


static int get_filename(void)
{
	char *home = getenv("HOME");
	if (!home || strlen(home) > PATH_LEN-7)
		return -1;

	strcpy(filename, home);
	strcat(filename, "/.2048"); 
	return 0;
}

static int lock_file(void)
{
	fd = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	if (fd == -1)
		return -1;
	
	if (flock(fd, LOCK_EX | LOCK_NB) == -1) {
		close(fd);
		return -1;
	}
	return 0;
}

static bool sane(board_t board, int score, int max_score)
{
	if (score < 0 || max_score < 0 ||
	    max_score > MAX_POSSIBLE_SCORE ||
	    score > max_score)
	    	return false;

	for (int y = 0; y < 4; y++) {
		for (int x = 0; x < 4; x++) {
			int tile = board[y][x];
			if (tile > MAX_POSSIBLE_TILE || tile < 0)
				return false;
		}
	}
	return true;
}

int load_game(board_t board, int *score, int *max_score)
{
	if (get_filename() != 0) {
		printf("didn't get filename\n");
		save_mode = NO_SAVE;
		return 0;
	}
	if (lock_file() != 0) {
		printf("couldn't lock file\n");
		save_mode = NO_SAVE;
		return -1;
	}
	
	ssize_t s;
	s = read(fd, score, sizeof(int));
	if (s != sizeof(int))
		return -1;

	s = read(fd, max_score, sizeof(int));
	if (s != sizeof(int))
		return -1;

	s = read(fd, board, sizeof(board_t));
	if (s != sizeof(board_t))
		return -1;

	if (!sane(board, *score, *max_score)) {
		printf("save is invalid\n");
		return -1;
	}

	printf("loaded\n");
	return 0;
}

int save_game(board_t board, int score, int max_score)
{
	if (save_mode == NO_SAVE) {
		printf("no save\n");
		return -1;
	}
	off_t off;
	off = lseek(fd, 0, SEEK_SET);
	if (off == -1)
		return -1;
	write(fd, &score,     sizeof(int));
	write(fd, &max_score, sizeof(int));
	write(fd, board,      sizeof(board_t));
	/* game is saved only once, close file */
	close(fd);
	printf("saved\n");
	return 0;
}