#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <termios.h>
#include <errno.h>
#include <sys/ioctl.h>

#define ABUF_INIT {NULL, 0}
#define CTRL_KEY(k) ((k) & 0x1f)

/*** terminal ***/

/*** data ***/
struct editorConfig {
    int cx, cy;
    int screenrows;
    int screencols;
    struct termios orig_termios;
};

//we will use this to build up the output before writing it to the terminal in one go, which is more efficient than writing each character individually.
struct abuf {
    char *b;
    int len;
};

struct editorConfig E;

/*** terminal ***/
void die(const char *s) 
{
    write(STDOUT_FILENO, "\x1b[2J", 4); // clear the screen
    write(STDOUT_FILENO, "\x1b[H", 3); // move the cursor to the top-left corner
    perror(s);
    exit(1);
}


void enableRawMode() {
    // This function would typically set the terminal to raw mode
    // using termios, but for simplicity, we will skip the implementation.
    struct termios raw;
    if(tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");
    raw = E.orig_termios; //save the original terminal settings for later restoration

    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN | BRKINT); // disable echo, canonical mode, signals, and extended input processing
    raw.c_iflag &= ~(IXON | ICRNL);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8); // set character size to 8 bits per byte
    //raw.c_cc[VMIN] = 0; // minimum number of bytes of input needed before read() can return
    //raw.c_cc[VTIME] = 1; // maximum amount of time to wait before read() returns, in tenths of a second

    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

void disableRawMode() {
    // This function would typically restore the terminal to its original state
    // using termios, but for simplicity, we will skip the implementation.
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1) die("tcsetattr");

}

char editorReadKey() {
    char x; 
    int nread; 
    while((nread = read(STDIN_FILENO, &x, 1)) != 1)
    {
        if(nread == -1 && errno != EAGAIN) die("read");
    }

    return x;
}

void editorProcessKeypress() {
    char c = editorReadKey();

    switch(c)
    {
        case CTRL_KEY('q'):
            disableRawMode();
            write(STDOUT_FILENO, "\x1b[2J", 4); // clear the screen
            write(STDOUT_FILENO, "\x1b[H", 3); // move the cursor to the top-left corner
            exit(0);
            break;
    }
}

int getCursorPosition(int *rows, int *cols)
{
    char buf[32];
    unsigned int i = 0;

    if(write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;// send the escape sequence to query the cursor position

    while(i < sizeof(buf) - 1)
    {
        if(read(STDIN_FILENO, &buf[i], 1) != 1) break; 
        if(buf[i] == 'R') break; // the response ends with 'R'
        i++;
    }

    buf[i] = '\0'; // null-terminate the response string

    if(buf[0] != '\x1b' || buf[1] != '[') return -1;// the response should start with "\x1b["
    if(sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1; // parse the row and column numbers from the response
    return 0;
    
}

int getWindowSize(int *rows, int *cols) {
    struct winsize ws;

    if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) //ioctl just gets the window size, and if it fails or returns 0 columns, we will use an alternative method to get the window size.
    {
        if(write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;// move the cursor to the bottom-right corner of the screen by sending escape sequences that move the cursor right and down a large number of times.
        return getCursorPosition(rows, cols); // then we query the cursor position, which will give us the actual size of the window.
    } 
    else 
    {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}
/*** output ***/
void editorDrawRows() {
    // This function would typically draw the rows of text in the editor,
    // but for simplicity, we will skip the implementation.
    int y; 
    for (y = 0; y < E.screenrows; y++)
    {
        write(STDOUT_FILENO, "~", 1);

        if(y < E.screenrows - 1) {
            write(STDOUT_FILENO, "\r\n", 2);
        }
    }

}

void editorRefreshScreen() {
    // This function would typically clear the screen and redraw the editor's interface,
    // but for simplicity, we will skip the implementation.
    write(STDOUT_FILENO, "\x1b[2J", 4); // clear the screen
    write(STDOUT_FILENO, "\x1b[H", 3); // move the cursor to the top-left corner

    editorDrawRows();

    write(STDOUT_FILENO, "\x1b[H", 3); // move the cursor to the top-left corner
}

/*** input ***/


/*** init ***/
void initEditor() {
    E.cx = 0;
    E.cy = 0;

    if(getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
}

int main(void) {
    enableRawMode();
    initEditor();
    while(1)
    {
        editorRefreshScreen();
        editorProcessKeypress();
    }


    return 0;
}