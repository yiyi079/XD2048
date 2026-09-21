#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <termios.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

typedef uint32_t block2048_t;

#ifdef DEBUG
#define debug_puts(x) puts(x)
#else
#define debug_puts(x) ((void)(x),0)
#endif

#define K_N 0
#define K_W 1
#define K_S 2
#define K_A 3
#define K_D 4

block2048_t b[4][4] /* = 
{
    {64,  4,  8,   2},
    {32,  8, 32, 256},
    { 8, 64,  0,   8},
    { 2,  4, 16,   0}
} */; // 测试用 GAME OVER 数据
char is_new[4][4];

inline
static char sgetchar()
{
    char buf[16];
    int ret;
    for (;;)
    {
        ret = read(STDIN_FILENO, buf, sizeof(buf));
        if (ret <= 0) exit(0);
        debug_puts(__func__);
        // printf("\n\n\n%d:%i%i%i\n", ret, buf[0], buf[1], buf[2]);
        if (ret < 3) return *buf & ~0x20;
        if (buf[0] == 0x1B && buf[1] == 0x5B)
        {
            switch (buf[2])
            {
            case 'A': // UP
                return 'W';
            case 'B': // DOWN
                return 'S';
            case 'D': // LEFT
                return'A';
            case 'C': // RIGHT
                return 'D';
            }
        }
    }
}



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
    if ((err = sigwait(&set, &sig)))
        fprintf(stderr, "sigwait failed: %s\n", strerror(err));
    else
        printf("\ngot %d, exiting...\n", sig);
    
    restore_terminal();
    exit(0);
    
}

block2048_t gen_block_num()
{
    const block2048_t bs[] = { 2,2,2,2,2,2,2,2,2,4 };
    return bs[rand() % (sizeof(bs) / sizeof(block2048_t))];
}

block2048_t * get_empty()
{
    block2048_t * pb[16];
    block2048_t ** ppb = pb;
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
    int is_updated, need_gen = 0;
    do
    {
        is_updated  = 0;
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
                    is_updated = 1;
                    need_gen = 1;
                }
                else if (can_merge(i,j,i-1,j))
                {
                    b[i - 1][j] *= 2;
                    b[i][j] = 0;

                    is_new[i - 1][j] = 1;
                    is_new[i][j] = 0;
                    is_updated = 1;
                    need_gen = 1;
                }
            }
        }
    } while (is_updated);
    if (need_gen) {
        *get_empty() = gen_block_num();
        dump();
        bzero(is_new, sizeof(is_new));
    }
    
}
void move_down()
{
    debug_puts(__func__);
    int is_updated, need_gen = 0;
    do
    {
        is_updated  = 0;
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
                    is_updated = 1;
                    need_gen = 1;
                }
                else if (can_merge(i,j,i+1,j))
                {
                    b[i + 1][j] *= 2;
                    b[i][j] = 0;

                    is_new[i + 1][j] = 1;
                    is_new[i][j] = 0;
                    is_updated = 1;
                    need_gen = 1;
                }
            }
        }
    } while (is_updated);
    if (need_gen) {
        *get_empty() = gen_block_num();
        dump();
        bzero(is_new, sizeof(is_new));
    }
}
void move_left()
{
    debug_puts(__func__);
    int is_updated, need_gen = 0;
    do
    {
        is_updated  = 0;
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
                    is_updated = 1;
                    need_gen = 1;
                }
                else if (can_merge(i,j,i,j-1))
                {
                    b[i][j - 1] *= 2;
                    b[i][j] = 0;

                    is_new[i][j - 1] = 1;
                    is_new[i][j] = 0;
                    is_updated = 1;
                    need_gen = 1;
                }
            }
            
        }
    } while (is_updated);
    if (need_gen) {
        *get_empty() = gen_block_num();
        dump();
        bzero(is_new, sizeof(is_new));
    }
    
}
void move_right()
{
    debug_puts(__func__);
    int is_updated, need_gen = 0;
    do
    {
        is_updated  = 0;
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

                    is_updated = 1;
                    need_gen = 1;
                }
                else if (can_merge(i,j,i,j+1))
                {
                    b[i][j + 1] *= 2;
                    b[i][j] = 0;

                    is_new[i][j + 1] = 1;
                    is_new[i][j] = 0;

                    is_updated = 1;
                    need_gen = 1;
                }
            }
            
        }
    } while (is_updated);
    if (need_gen) {
        *get_empty() = gen_block_num();
        dump();
        bzero(is_new, sizeof(is_new));
    }
}
#undef can_merge

void init_env()
{
    sigset_t set;
    pthread_t tid;
    srand(time(NULL));

    sigfillset(&set);
    pthread_sigmask(SIG_BLOCK, &set, NULL);

    pthread_create(&tid, NULL, sig_thr, NULL);
    set_noncanonical();

    puts("Hello 2048!\nuse WASD or ↑↓←→ to control\n===================================\n\n\n\n");

    *get_empty() = gen_block_num();
    *get_empty() = gen_block_num();
    dump();
}

void restart()
{
    bzero(b, sizeof(b));
    *get_empty() = gen_block_num();
    *get_empty() = gen_block_num();
    dump();
}

void game_loop()
{
    char c;
    while (is_move_up() || is_move_down() || is_move_left() || is_move_right())
    {
        c = sgetchar();
        switch (c)
        {
        case 'W':
            move_up();
            break;
        case 'S':
            move_down();
            break;
        case 'A':
            move_left();
            break;
        case 'D':
            move_right();
            break;
        case 'R':
            printf("Do you really want to restart? ( Click R again )\r");
            fflush(stdout);
            if (sgetchar() == 'R')
                restart();
            else 
            {
                fputs("\033[J", stdout);
                fflush(stdout);
            }
            
        }
    }
}

__attribute__((__noreturn__))
int main(int argc, char ** argv)
{
    init_env();

    for (;;)
    {
        game_loop();

        printf("GAME OVER!\nDo you want to restart? ( Click R )\033[1A\r");
        fflush(stdout);
        if (sgetchar() == 'R')
            restart();
        else exit(0);

    }
}
