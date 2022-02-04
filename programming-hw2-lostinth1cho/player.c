/*
 * @Author: your name
 * @Date: 2021-11-23 14:46:29
 * @LastEditTime: 2021-11-23 17:52:02
 * @LastEditors: Please set LastEditors
 * @Description: 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 * @FilePath: \Coding\SP2021progHW2\player.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
//./player -n [player_id]
int main(int argc, char *argv[])
{
    if (argc != 3 )
    {
        fprintf(stderr, "argc error7414\n");
        exit(1);
    }
    int player_id = atoi(argv[2]);
    int guess;
    for (int i = 1; i <= 10; i++)
    {
        /* initialize random seed: */
        srand((player_id + i) * 323);
        /* generate guess between 1 and 1000: */
        guess = rand() % 1001;
        printf("%d %d\n", player_id, guess);
        fflush(stdout);
    }
    exit(0);
}