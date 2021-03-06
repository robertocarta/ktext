/*** includes ***/
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <errno.h>

// nnoremap <leader>e :split<cr>:terminal make&&./kilo<cr>


/*** defines ***/
#define CTRL_K(k) ((k) & 0x1f)

/*** data ***/
struct termios orig_termios;

void die(const char *s) {
	perror(s);
	exit(1);
}

struct editorConfig {
	struct termios orig_termios;
	int screenrows;
	int screencols;
};

struct editorConfig E;

/*** terminal ***/

void disableRawMode() {
	/* write(STDOUT_FILENO, "\x1b[2J", 4); */
	/* write(STDOUT_FILENO, "\x1b[H", 3); */
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) ==-1) die("tcsetattr");
}

void enableRawMode() {
	if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcsetattr");
	atexit(disableRawMode);

	struct termios raw = E.orig_termios;

	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	raw.c_oflag &= ~(OPOST);
	raw.c_cflag |= (CS8);
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 1;

	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) ==1) die("tcsetattr");
}

char editorReadKey() {
	int nread;
	char c;
	while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
	if (nread == -1 && errno != EAGAIN) die("read");
	}
	return c;
}

void editorProcessKeyPress() {
	char c = editorReadKey();
	switch (c) {
		case CTRL_K('q'): 
			write(STDOUT_FILENO, "\x1b[2J", 4);
			write(STDOUT_FILENO, "\x1b[H", 3);
			exit(0); break;
	}

}

void editorDrawRows() {
	int y;
	for (y = 0; y < E.screenrows; y++) {
		write(STDOUT_FILENO, "~", 1);
		if (y < E.screenrows - 1) {
			write(STDOUT_FILENO, "\r\n", 2);
		}
	}
}

void editorRefreshScreen() {
	write(STDOUT_FILENO, "\x1b[2J", 4); // clear screen
	write(STDOUT_FILENO, "\x1b[H", 3); // move back cursor to column 1 row 1
	editorDrawRows();
	write(STDOUT_FILENO, "\x1b[H", 3); // move back cursor to column 1 row 1
}


int getCursorPosition(int *rows, int *cols) {
	char buf[32];
	unsigned int i = 0;

	if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;
	while (i < sizeof(buf) - 1) {
		if (read(STDIN_FILENO, &buf[i], 1) != 1) break; if (buf[i] == 'R') break;
		i++;
	}
	buf[i] = '\0';

	if (buf[0] != '\x1b' || buf[1] != '[') return -1;
	if (sscanf(&buf[2], "%d;%d", rows, cols) !=2) return -1;
	return 0;
}

int getWindowSize(int *rows, int *cols) {
	struct winsize ws;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col ==0) {
		if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
		return getCursorPosition(rows, cols);
	} else {
		*cols = ws.ws_col;
		*rows = ws.ws_row;
		return 0;
	}
}


void initEditor() {
	if (getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize");

}


/*** init ***/
int main() {
	char c;
	enableRawMode();
	initEditor();
	while (1) {
		editorRefreshScreen();
		editorProcessKeyPress();
	}
	return 0;
}
