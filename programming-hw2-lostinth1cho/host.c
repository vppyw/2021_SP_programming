#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <math.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <sys/stat.h>

int cmd_opt = 0;
int host_id;
int luckynumber;
int depth;
int player_ID[8];
int child_pid[2];
int pipeRead[2][2];
int pipeWrite[2][2];
FILE *readfp[2];
FILE *writefp[2];
int status;
int point[13] = {0};

int main()
{
    //./host -m [host_id] -d [depth] -l [lucky_number]
    while (1)
    {
        cmd_opt = getopt(argc, argv, "m:d:l:");
        if (cmd_opt == -1)
            break;
        switch (cmd_opt)
        {
        case m:
            host_id = atoi(optarg);
            break;
        case d:
            depth = atoi(optarg);
            break;
        case l:
            luckynumber = atoi(optarg);
            break;
        default:
            fprintf(stderr, "Not supported option\n");
            break;
        }
    }
    int hostpid = getpid();
    char readfilename[512];
    if (depth == 0)
    {
        sprintf(readfilename, "fifo_%d.tmp", host_id);
        FILE *read_fifo = fopen(readfilename, "r+");
        FILE *write_fifo = fopen("fifo_0.cmp", "r+");

        while (1)
        {
            fscanf(read_fifo, "%d %d %d %d %d %d %d %d", &player_ID[0], &player_ID[1], &player_ID[2], &player_ID[3], &player_ID[4], &player_ID[5], &player_ID[6], &player_ID[7]);
            if (player_ID[0] == -1)
            { //輸入結束
                exit(-1);
            }
            for (int i = 0; i < 2; i++)
            {
                pipe(pipeWrite[i]);
                pipe(pipeRead[i]);
                if (getpid() == hostpid)
                {
                    child_pid[i] = fork();
                }
                

                if (child_pid[i] < 0)
                {
                    fprintf(stderr, "fork error.\n", 12);
                }

                if (child_pid[i] == 0) // child
                {
                    dup2(pipeWrite[i][0], STDIN_FILENO);
                    dup2(pipeRead[i][0], STDOUT_FILENO);
                    for (int j = 0; j < 2; j++)
                    {
                        close(pipeWrite[i][j]);
                        close(pipeRead[i][j]);
                    }
                    char hostidbuf[512];
                    char LuckyNbuf[512];
                    sprintf(hostidbuf, "%d", host_id);
                    sprintf(LuckyNbuf, "%d", luckynumber);
                    execlp("./host", "./host", "-m", hostidbuf, "-d", "1", "-l", LuckyNbuf, NULL);
                }
                else
                {
                    readfp[i] = fdopen(pipeRead[i][0], "r");
                    writefp[i] = fdopen(pipeWrite[i][1], "w");
                    close(pipeWrite[i][0]);
                    close(pipeRead[i][1]);
                    fprintf(writefp[i], "%d %d %d %d\n", player_ID[0], player_ID[1], player_ID[2], player_ID[3]);
                    fflush(writefp[i]);

                    waitpid(child_pid[i], &status, 0);
                    fclose(readfp[i]);
                    fclose(writefp[i]);
                }
            }
        }
        exit(0);
    }
    else if (depth == 1)
    {
        for (int i = 0; i < 4; i++)
        {
            scanf("%d", &player_ID[i]);
        }
        for (int i = 0; i < 2; i++)
        {
            pipe(pipeRead[i]);
            pipe(pipeWrite[i]);
        }
        if ((child_pid[0] = fork()) && (child_pid[1] = fork()))
        {
            fprintf(stderr, "fork error.\n", 12);
        }

        if (child_pid[0] == 0)
        {
            dup2(pipeWrite[0][0], STDIN_FILENO);
            dup2(pipeRead[0][1], STDOUT_FILENO);
            for (int i = 0; i < 2; i++)
            {
                for (int j = 0; j < 2; j++)
                {
                    close(pipeRead[i][j]);
                    close(pipeWrite[i][j]);
                }
            }
            char hostidBuf[512];
            char luckyNBuf[512];
            sprintf(hostidBuf, "%d", host_id);
            sprintf(luckyNBuf, "%d", luckynumber);
            execlp("./host", "./host", "-m", hostidBuf, "-d", "2", "-l", luckyNBuf, NULL);
        }
        else if (child_pid[1] == 0)
        {
            dup2(pipeWrite[1][0], STDIN_FILENO);
            dup2(pipeRead[1][1], STDOUT_FILENO);
            for (int i = 0; i < 2; i++)
            {
                for (int j = 0; j < 2; j++)
                {
                    close(pipeRead[i][j]);
                    close(pipeWrite[i][j]);
                }
            }
            char hostidBuf[512];
            char luckyNBuf[512];
            sprintf(hostidBuf, "%d", host_id);
            sprintf(luckyNBuf, "%d", luckynumber);
            execlp("./host", "./host", "-m", hostidBuf, "-d", "2", "-l", luckyNBuf, NULL);
        }
        else
        {
            for (int i = 0; i < 2; i++)
            {
                readfp[i] = fdopen(pipeRead[i][0], "r");
                writefp[i] = fopen(pipeWrite[i][1], "w");
                close(pipeRead[i][1]);
                close(pipeWrite[i][0]);
            }
            fprintf(writefp[0], "%d %d\n", player_ID[0], player_ID[1]);
            fflush(writefp[0]);
            fprintf(writefp[1], "%d %d\n", player_ID[2], player_ID[3]);
            fflush(writefp[1]);

            for (int i = 0; i < 10; i++)
            {
                int id1, id2, guess1, guess2;
                fscanf(readfp[0], "%d%d", &id1, &guess1);
                fscanf(readfp[1], "%d%d", &id2, &guess2);
                int diff1 = abs(luckynumber - guess1);
                int diff2 = abs(luckynumber - guess2);
                if (diff1 <= diff2)
                {
                    id2 = id1;
                    guess2 = guess1;
                }
                printf("%d %d\n", id2, guess2);
                fflush(stdout);
            }

            for (int i = 0; i < 2; i++)
            {
                fclose(readfp[i]);
                fclose(writefp[i]);
                waitpid(child_pid[i], &status, 0);
            }
        }
        exit(0);
    }
    else if (depth == 2)
    {
        for (int i = 0; i < 2; i++)
        {
            scanf("%d", &player_ID[i]);
        }
        int pid1;
        int pid2;
        int pipe1Read[2];
        int pipe2Read[2];
        pipe(pipe1Read);
        pipe(pipe2Read);
        if ((pid1 = fork()) && (pid2 = fork()) < 0)
        {
            fprintf(stderr, "fork error.\n", 12);
        }
        if (pid1 == 0)
        {
            dup2(pipe1Read[1], STDOUT_FILENO);
            close(pipe1Read[0]);
            close(pipe1Read[1]);
            close(pipe2Read[0]);
            close(pipe2Read[1]);
            char playerID[512];
            sprintf(playerID, "%d", player_ID[0]);
            execlp("./player", "./player", "-n", buf, NULL);
        }
        else if (pid2 == 0)
        {
            dup2(pipe2Read[1], STDOUT_FILENO);
            close(pipe1Read[0]);
            close(pipe1Read[1]);
            close(pipe2Read[0]);
            close(pipe2Read[1]);
            char playerID[512];
            sprintf(playerID, "%d", player_ID[1]);
            execlp("./player", "./player", "-n", buf, NULL);
        }
        else
        {
            readfp[0] = fdopen(pipe1Read[0], "r");
            readfp[1] = fdopen(pipe2Read[0], "r");
            close(pipe1Read[1]);
            close(pipe2Read[1]);
            for (int i = 0; i < 10; i++)
            {
                int id1, id2, guess1, guess2;
                fscanf(readfp[0], "%d%d", &id1, &guess1);
                fscanf(readfp[1], "%d%d", &id2, &guess2);
                int diff1 = abs(luckynumber - guess1);
                int diff2 = abs(luckynumber - guess2);
                if (diff1 <= diff2)
                {
                    id2 = id1;
                    guess2 = guess1;
                }
                printf("%d %d\n", id2, guess2);
                fflush(stdout);
            }
            fclose(readfp[0]);
            fclose(readfp[1]);
            waitpid(pid1, &status, 0);
            waitpid(pid2, &status, 0);
        }
        exit(0);
    }
    // todo: 比分數 95行
}