#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <termios.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

typedef uint32_t b_t;

#ifdef DEBUG
#define debug_puts(x) puts(x)
#else
#define debug_puts(x) 0
#endif

#define K_N 0
#define K_W 1
#define K_S 2
#define K_A 3
#define K_D 4

b_t b[4][4] /* =
{
    {
        0
    },
    {0,0,0,4},
    {0,0,0,2},
    {0,0,0,2}
} */; // 上一版本出错数据
int is_new[4][4];

#define sgetchar() ({int c; if ((c = getchar()) == EOF) exit(0); c;})

static struct termios g_saved_tty;
static int g_tty_saved = 0;

void restore_terminal(void) {
    if (g_tty_saved) {
        tcsetattr(STDIN_FILENO, TCSANOW, &g_saved_tty);
    }
}

void set_noncanonical(void) {
    struct termios tty;
    if (tcgetattr(STDIN_FILENO, &tty) < 0) return;

    g_saved_tty = tty;          // 保存原设置
    g_tty_saved = 1;
    atexit(restore_terminal);   // 注册退出时恢复

    tty.c_lflag &= ~ICANON;
    tty.c_lflag &= ~ECHO;
    tty.c_cc[VMIN]  = 1;
    tty.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
}

__attribute__((__noreturn__))
void * sig_thr(void * arg)
{
    int sig;
    sigset_t set;
    sigfillset(&set);
    int err;

    for (;;) {
        if ((err = sigwait(&set, &sig))) fprintf(stderr, "sigwait failed: %s\n", strerror(err));
        restore_terminal();

        printf("\ngot %d, exiting...\n", sig);
        exit(0);
    }
    // return NULL;
    
}


b_t gen_block_num()
{
    const b_t bs[] = { 2,2,2,2,2,2,2,2,2,4 };
    return bs[rand() % (sizeof(bs) / sizeof(b_t))];
}

b_t * get_empty()
{
    b_t * pb[16];
    b_t ** ppb = pb;
    for (size_t i = 0; i < 4; i++)
        for (size_t j = 0; j < 4; j++)
            if (!b[i][j]) *(ppb++) = &b[i][j];
    if (ppb == pb) return NULL;
    return pb[rand() % ((ppb - pb))];
}
void dump()
{
    fputs("\033[4A\r\033[J", stdout);
    for (size_t i = 0; i < 4; i++)
    {
        for (size_t j = 0; j < 4; j++)
            printf("%8u ", b[i][j]);
        putchar('\n');
    }
}
#define can_merge(i1,j1,i2,j2) (!is_new[i1][j1] && !is_new[i2][j2] && b[i1][j1] == b[i2][j2])
int is_move_up()
{
    debug_puts(__func__);
    for (size_t i = 1; i < 4; i++)
        for (size_t j = 0; j < 4; j++)
            if (b[i][j] && (b[i - 1][j] == 0 || can_merge(i,j,i-1,j)))
                return 1;
    return 0;
}
int is_move_down()
{
    debug_puts(__func__);
    for (int i = 2; i > -1; i--)
        for (size_t j = 0; j < 4; j++)
            if (b[i][j] && (b[i + 1][j] == 0 || can_merge(i,j,i+1,j)))
                return 1;
    return 0;
}
int is_move_left()
{
    debug_puts(__func__);
    for (size_t j = 1; j < 4; j++)
        for (size_t i = 0; i < 4; i++)
            if (b[i][j] && (b[i][j - 1] == 0 || can_merge(i,j,i,j-1)))
                return 1;
    return 0;
}
int is_move_right()
{
    debug_puts(__func__);
    for (int j = 2; j > -1; j--)
        for (size_t i = 0; i < 4; i++)
            if (b[i][j] && (b[i][j + 1] == 0 || can_merge(i,j,i,j+1)))
                return 1;
    return 0;
        
}
void move_up()
{
    debug_puts(__func__);
    for (size_t i = 1; i < 4; i++)
    {
        for (size_t j = 0; j < 4; j++)
        {
            if (!b[i][j]) continue;
            
            if (b[i - 1][j] == 0)
            {
                b[i - 1][j] = b[i][j];
                b[i][j] = 0;

                is_new[i - 1][j] = is_new[i][j];
                is_new[i][j] = 0;
            }
            else if (can_merge(i,j,i-1,j))
            {
                b[i - 1][j] *= 2;
                b[i][j] = 0;

                is_new[i - 1][j] = 1;
                is_new[i][j] = 0;
            }
        }
    }
}
void move_down()
{
    debug_puts(__func__);
    for (int i = 2; i > -1; i--)
    {
        for (size_t j = 0; j < 4; j++)
        {
            if (!b[i][j]) continue;
            if (b[i + 1][j] == 0)
            {
                b[i + 1][j] = b[i][j];
                b[i][j] = 0;

                is_new[i + 1][j] = is_new[i][j];
                is_new[i][j] = 0;
            }
            else if (can_merge(i,j,i+1,j))
            {
                b[i + 1][j] *= 2;
                b[i][j] = 0;

                is_new[i + 1][j] = 1;
                is_new[i][j] = 0;
            }
        }
    }
}
void move_left()
{
    debug_puts(__func__);
    for (size_t j = 1; j < 4; j++)
    {
        for (size_t i = 0; i < 4; i++)
        {
            if (!b[i][j]) continue;
            if (b[i][j - 1] == 0)
            {
                b[i][j - 1] = b[i][j];
                b[i][j] = 0;

                is_new[i][j - 1] = is_new[i][j];
                is_new[i][j] = 0;
            }
            else if (can_merge(i,j,i,j-1))
            {
                b[i][j - 1] *= 2;
                b[i][j] = 0;

                is_new[i][j - 1] = 1;
                is_new[i][j] = 0;
            }
        }
        
    }
    
}
void move_right()
{
    debug_puts(__func__);
    for (int j = 2; j > -1; j--)
    {
        for (size_t i = 0; i < 4; i++)
        {
            if (!b[i][j]) continue;
            if (b[i][j + 1] == 0)
            {
                b[i][j + 1] = b[i][j];
                b[i][j] = 0;

                is_new[i][j + 1] = is_new[i][j];
                is_new[i][j] = 0;
            }
            else if (can_merge(i,j,i,j+1))
            {
                b[i][j + 1] *= 2;
                b[i][j] = 0;

                is_new[i][j + 1] = 1;
                is_new[i][j] = 0;
            }
        }
        
    }
}

void exec_cmd(int cmd)
{
    debug_puts(__func__);
    switch (cmd)
    {
    case K_W:
        if (is_move_up())
        {
            do
                move_up();
            while (is_move_up());
            *get_empty() = gen_block_num();
            dump();
        }
        break;
    case K_S:
        if (is_move_down())
        {
            do
                move_down();
            while (is_move_down());
            
            *get_empty() = gen_block_num();
            dump();
        }
        break;
    case K_A:
        if (is_move_left())
        {
            do
                move_left();
            while (is_move_left());
            *get_empty() = gen_block_num();
            dump();
        }
        break;
    case K_D:
        if (is_move_right())
        {
            do
                move_right();
            while (is_move_right());
            *get_empty() = gen_block_num();
            dump();
        }
        break;
    default:
        break;
    }
    bzero(is_new, sizeof(is_new));
}

__attribute__((__noreturn__))
int main(int argc, char ** argv)
{

    sigset_t set;
    pthread_t tid;
    int c;
    int cmd;
    srand(time(NULL));

    sigfillset(&set);
    pthread_sigmask(SIG_BLOCK, &set, NULL);

    pthread_create(&tid, NULL, sig_thr, NULL);
    set_noncanonical();
    puts("Hello 2048!\nuse WASD or ↑↓←→ to control\n===================================\n\n\n\n");
    *get_empty() = gen_block_num();
    *get_empty() = gen_block_num();
    dump();
    

    while (is_move_up() || is_move_down() || is_move_left() || is_move_right())
    {
        c = sgetchar();
        if (c == 0x1B)
        {
            c = sgetchar(); // 若用户按下Esc，这里会导致下一次的方向键无效
            if (c == 0x5B)
            {
                switch ((c = sgetchar()))
                {
                case 'A':
                    c = 'W';
                    break;
                case 'B':
                    c = 'S';
                    break;
                case 'D':
                    c = 'A';
                    break;
                case 'C':
                    c = 'D';
                    break;
                default:
                    break;
                }
            }
        }
        switch (c)
        {
        case 'W':
        case 'w':
            cmd = K_W;
            break;
        case 'S':
        case 's':
            cmd = K_S;
            break;
        case 'A':
        case 'a':
            cmd = K_A;
            break;
        case 'D':
        case 'd':
            cmd = K_D;
            break;
        case ' ':
            dump();
        
        default:
            continue;;
        }


        exec_cmd(cmd);
    }
    puts("GAME OVER!");

    exit(0);
}
