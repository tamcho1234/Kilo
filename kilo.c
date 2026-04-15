/*** define ***/
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

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
#include <sys/types.h>


#define ABUF_INIT {NULL, 0}
#define CTRL_KEY(k) ((k) & 0x1f)

/*** terminal ***/

/*** data ***/
typedef struct erow {
    int size;
    char *chars;
} erow;

struct editorConfig {
    int cx, cy;
    int screenrows;
    int screencols;
    struct termios orig_termios;
    erow *row;
    int numrows;
    int rowoff;
    int coloff;
};

enum editorKey {
    ARROW_LEFT = 1000,
    ARROW_RIGHT ,
    ARROW_UP ,
    ARROW_DOWN,
    PAGE_UP, 
    PAGE_DOWN,
    HOME_KEY,
    END_KEY,
    DEL_KEY
};

#define KILO_VERSION "0.0.1" // Define the version of the Kilo editor
//we will use this to build up the output before writing it to the terminal in one go, which is more efficient than writing each character individually.
struct abuf {
    char *b;
    int len;
};

struct editorConfig E;



/*** row operation ***/
void editorAppendRow(char *s, size_t len) {
    // This function would typically append a new row of text to the editor's data structures,
    // but for simplicity, we will skip the implementation.
    E.row = realloc(E.row, sizeof(erow) * (E.numrows + 1));
    int at = E.numrows;

    E.row[at].size = len;
    E.row[at].chars = malloc(len + 1);
    memcpy(E.row[at].chars, s, len);
    E.row[at].chars[len] = '\0';
    E.numrows++;
}

/*** terminal ***/
void die(const char *s) 
{
    write(STDOUT_FILENO, "\x1b[2J", 4); // clear the screen
    write(STDOUT_FILENO, "\x1b[H", 3); // move the cursor to the top-left corner
    perror(s);
    exit(1);
}


void editorOpen(char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) die("fopen");
    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;

    while((linelen = getline(&line, &linecap, fp)) != -1) 
    {
        while (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r')) 
        {
            // This function would typically read the file line by line and store it in the editor's data structures,
            // but for simplicity, we will skip the implementation.
            linelen--;
        }
    editorAppendRow(line, linelen);
    }
    fclose(fp);
    free(line);
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
    raw.c_cc[VMIN] = 0; // minimum number of bytes of input needed before read() can return
    raw.c_cc[VTIME] = 1; // maximum amount of time to wait before read() returns, in tenths of a second

    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

void disableRawMode() {
    // This function would typically restore the terminal to its original state
    // using termios, but for simplicity, we will skip the implementation.
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1) die("tcsetattr");

}

int editorReadKey() {
    char x; 
    int nread; 
    while((nread = read(STDIN_FILENO, &x, 1)) != 1)
    {
        if(nread == -1 && errno != EAGAIN) die("read");
    }

    if (x == '\x1b')
    {
        char seq[3];
        if(read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
        if(read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

        if(seq[0] == '[')
        {
            if(seq[1] >= '0' && seq[1] <= '9')
            {
                if(read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
                if(seq[2] == '~')
                {
                    switch(seq[1])
                    {
                        case '1': return HOME_KEY;
                        case '3': return DEL_KEY;
                        case '4': return END_KEY;
                        case '5': return PAGE_UP;
                        case '6': return PAGE_DOWN;
                        case '7': return HOME_KEY;
                        case '8': return END_KEY;

                    }
                }
            } else
            {
                switch(seq[1])
                {
                    case 'A': return ARROW_UP;
                    case 'B': return ARROW_DOWN;
                    case 'C': return ARROW_RIGHT;
                    case 'D': return ARROW_LEFT;
                    case 'H': return HOME_KEY;
                    case 'F': return END_KEY;
                }

            }
        } else if(seq[0] == '0')
            {
                switch(seq[1])
                {
                    case 'H': return HOME_KEY;
                    case 'F': return END_KEY;
                }
            }

        return '\x1b';

        }
        else
        {
            return x;
        }
    }

void editorMoveCursor(int key) {
    erow *row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];

    switch(key)
    {
        case ARROW_UP:
            if(E.cy != 0) E.cy--;
            break;
        case ARROW_LEFT:
            if(E.cx != 0){ 
                E.cx--;
            }
            else if(E.cy > 0) {
                E.cy--;
                E.cx = E.row[E.cy].size;
            }
            break;
        case ARROW_DOWN:
            if(E.cy < E.numrows) E.cy++;
            break;
        case ARROW_RIGHT:
            if(row && E.cx < row->size) {
                E.cx++;
            }
            else if(row && E.cx == row->size && E.cy < E.numrows - 1) {
                E.cy++;
                E.cx = 0;
            }
            break;
    }

    row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
    int rowlen = row ? row->size : 0;
    if(E.cx > rowlen) {
        E.cx = rowlen;
    }


}

void editorProcessKeypress() {
    int c = editorReadKey();

    switch(c)
    {
        case CTRL_KEY('q'):
            disableRawMode();
            write(STDOUT_FILENO, "\x1b[2J", 4); // clear the screen
            write(STDOUT_FILENO, "\x1b[H", 3); // move the cursor to the top-left corner
            exit(0);
            break;

        case ARROW_UP:
        case ARROW_LEFT:
        case ARROW_DOWN:
        case ARROW_RIGHT:
            editorMoveCursor(c);
            break;

        case PAGE_UP:
        case PAGE_DOWN:
            {
                int times = E.screenrows;
                    while(times--)
                    {
                        editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
                    }
                
            }
            break;

        case HOME_KEY:
        case END_KEY:
        {
            int times = E.screencols;
            while(times--)
            {
                editorMoveCursor(c == HOME_KEY ? ARROW_LEFT : ARROW_RIGHT);
            }
            break;
        }
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

void abAppend(struct abuf *ab, const char *s, int len) {
    char *new = realloc(ab->b, ab->len + len);
    if(new == NULL) return;
    memcpy(&new[ab->len], s, len);
    ab->b = new;
    ab->len += len;
}   

void abFree(struct abuf *ab) {
    free(ab->b);
}


/*** output ***/
void editorScroll() {
    if(E.cy < E.rowoff) {
        E.rowoff = E.cy;
    }
    if(E.cy >= E.rowoff + E.screenrows) {
        E.rowoff = E.cy - E.screenrows + 1;
    }
    if(E.cx < E.coloff) {
        E.coloff = E.cx;
    }
    if(E.cx >= E.coloff + E.screencols) {
        E.coloff = E.cx - E.screencols + 1;
    }
}

void editorDrawRows(struct abuf *ab) {
    // This function would typically draw the rows of text in the editor,
    // but for simplicity, we will skip the implementation.
    int y; 
    for (y = 0; y < E.screenrows; y++)
    {
        int filerow = y + E.rowoff; // calculate the corresponding file row based on the current scroll offset (E.rowoff)
        if(filerow >= E.numrows) {
            if(y == E.screenrows / 3) {
                char welcome[80];
                int welcomelen = snprintf(welcome, sizeof(welcome), "Kilo editor -- version: %s", KILO_VERSION);
                if(welcomelen > E.screencols) welcomelen = E.screencols;
                int padding = (E.screencols - welcomelen) /2;
                if(padding)
                {
                    abAppend(ab, "~", 1);
                    padding--;
                }
                while(padding--) abAppend(ab, " ", 1);
                abAppend(ab, welcome, welcomelen);
            } else {
                abAppend(ab, "~", 1);
            }
        } else {
            int len = E.row[filerow].size - E.coloff; // calculate the length of the visible portion of the row based on the current horizontal scroll offset (E.coloff)
            if(len < 0) len = 0;
            if(len > E.screencols) len = E.screencols;
            abAppend(ab, &E.row[filerow].chars[E.coloff], len);
        }
        abAppend(ab, "\x1b[K", 3); // clear the line from the cursor to the end of the line]")
        if(y < E.screenrows - 1) {
            abAppend(ab, "\r\n", 2);
        }
    }
}

void editorRefreshScreen() {
    // This function would typically clear the screen and redraw the editor's interface,
    // but for simplicity, we will skip the implementation.
    editorScroll();
    struct abuf ab = ABUF_INIT;

    abAppend(&ab, "\x1b[?25l", 6); // hide the cursor
    abAppend(&ab, "\x1b[H", 3); // move the cursor to the top-left corner
    editorDrawRows(&ab);

    abAppend(&ab, "\x1b[?25h", 6); // show the cursor
    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", E.cy - E.rowoff + 1, E.cx - E.coloff + 1); // move the cursor to the current position (E.cy, E.cx) using an escape sequence
    abAppend(&ab, buf, strlen(buf));
    write(STDOUT_FILENO, ab.b, ab.len);
    abFree(&ab);
}

/*** input ***/

/*** init ***/
void initEditor() {
    E.cx = 0;
    E.cy = 0;
    E.rowoff = 0;
    E.coloff = 0;
    if(getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
}

int main(int argc, char *argv[]) {

    enableRawMode();
    initEditor();
    if(argc >= 2) {
        editorOpen(argv[1]);
    }

    while(1)
    {
        editorRefreshScreen();
        editorProcessKeypress();
    }


    return 0;
}

