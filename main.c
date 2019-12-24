#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// board information
#define BOARD_SIZE 8
#define EMPTY 0
#define BLACK_FLAG 1
#define BLACK_KING 3
#define WHITE_FLAG 2
#define WHITE_KING 4
#define JUMP_OPTIONS 10


// bool
typedef int BOOL;
#define TRUE 1
#define FALSE 0

#define MAX_STEP 12

#define START "START"
#define PLACE "PLACE"
#define TURN "TURN"
#define END "END"
#define DEPTH 8
#define INFINITE 897013703
#define TIME_TO_KILL 1.6

struct Command {
	int x[MAX_STEP];
	int y[MAX_STEP];
	int numStep;
};

clock_t time_start;
char board[BOARD_SIZE][BOARD_SIZE] = { 0 };
char ABboard[DEPTH + 15][BOARD_SIZE][BOARD_SIZE] = { 0 };
int moveBlackDir[2][2] = { {-1,-1},{-1,1} };
int moveWhiteDir[2][2] = { {1, -1}, {1, 1} };
int moveKingDir[4][2] = { {1, -1}, {1, 1}, {-1, -1}, {-1, 1} };
int jumpDir[4][2] = { {2, -2}, {2, 2}, {-2, -2}, {-2, 2} };
struct Command moveCmd = { .x = {0},.y = {0},.numStep = 2 };
struct Command jumpCmd = { .x = {0},.y = {0},.numStep = 0 };
struct Command longestJumpCmd = { .x = {0},.y = {0},.numStep = 1 };
struct Command jumpCmdOptions[JUMP_OPTIONS] = { {.x = {0},.y = {0},.numStep = 0} };
struct Command PVtempCmd = { .x = {0},.y = {0},.numStep = 0 };
int jumpOptions = 0;
int myFlag;
int myRound;
int FinalControl;
int show_alpha;
BOOL shutdown;

BOOL isInBound(int x, int y) {
	return x >= 0 && x < BOARD_SIZE && y >= 0 && y < BOARD_SIZE;
}

void printBoard() {
	printf("  0 1 2 3 4 5 6 7\n");
	for (int i = 0; i < BOARD_SIZE; i++) {
		printf("%d ", i);
		for (int j = 0; j < BOARD_SIZE; j++) {
			switch (board[i][j]) {
			case EMPTY:
				printf(". ");
				break;
			case WHITE_FLAG:
				printf("O ");
				break;
			case BLACK_FLAG:
				printf("X ");
				break;
			case WHITE_KING:
				printf("@ ");
				break;
			case BLACK_KING:
				printf("* ");
				break;
			default:
				break;
			}
		}
		printf("\n");
	}
	printf("\n");
}

void tryToJump(int x, int y, int currentStep, char Gameboard[BOARD_SIZE][BOARD_SIZE]) {
	int newX, newY, midX, midY;
	char tmpFlag;
	char tmp_Flag = Gameboard[x][y];
	if (tmp_Flag > 2) {
		tmp_Flag -= 2;
	}
	jumpCmd.x[currentStep] = x;
	jumpCmd.y[currentStep] = y;
	jumpCmd.numStep++;
	for (int i = 0; i < 4; i++) {
		newX = x + jumpDir[i][0];
		newY = y + jumpDir[i][1];
		midX = (x + newX) / 2;
		midY = (y + newY) / 2;
		if (isInBound(newX, newY) && (Gameboard[midX][midY] == 3 - tmp_Flag || Gameboard[midX][midY] == 5 - tmp_Flag) && (Gameboard[newX][newY] == EMPTY)) {
			Gameboard[newX][newY] = Gameboard[x][y];
			Gameboard[x][y] = EMPTY;
			tmpFlag = Gameboard[midX][midY];
			Gameboard[midX][midY] = EMPTY;
			tryToJump(newX, newY, currentStep + 1, Gameboard);
			Gameboard[x][y] = Gameboard[newX][newY];
			Gameboard[newX][newY] = EMPTY;
			Gameboard[midX][midY] = tmpFlag;
		}
	}
	if (jumpCmd.numStep > longestJumpCmd.numStep) {
		memcpy(&longestJumpCmd, &jumpCmd, sizeof(struct Command));
		memcpy(&jumpCmdOptions[0], &jumpCmd, sizeof(struct Command));
		jumpOptions = 0;
	}
	if (jumpCmd.numStep == longestJumpCmd.numStep && longestJumpCmd.numStep != 1) {
		memcpy(&jumpCmdOptions[jumpOptions], &jumpCmd, sizeof(struct Command));
		jumpOptions++;
	}
	jumpCmd.numStep--;
}

void place(struct Command cmd, char Gameboard[BOARD_SIZE][BOARD_SIZE]) {
	int midX, midY, curFlag;
	curFlag = Gameboard[cmd.x[0]][cmd.y[0]];
	for (int i = 0; i < cmd.numStep - 1; i++) {
		Gameboard[cmd.x[i]][cmd.y[i]] = EMPTY;
		Gameboard[cmd.x[i + 1]][cmd.y[i + 1]] = curFlag;
		if (abs(cmd.x[i] - cmd.x[i + 1]) == 2) {
			midX = (cmd.x[i] + cmd.x[i + 1]) / 2;
			midY = (cmd.y[i] + cmd.y[i + 1]) / 2;
			Gameboard[midX][midY] = EMPTY;
		}
	}
	for (int i = 0; i < BOARD_SIZE; i++) {
		if (Gameboard[0][i] == BLACK_FLAG) {
			Gameboard[0][i] = BLACK_KING;
		}
		if (Gameboard[BOARD_SIZE - 1][i] == WHITE_FLAG) {
			Gameboard[BOARD_SIZE - 1][i] = WHITE_KING;
		}
	}
}


int AlphaBeta(int flag, struct Command cmd, int alpha, int beta, int depth) {
	if (shutdown == TRUE) {
		return alpha;
	}
	clock_t time_now = clock();
	double time_passed = (float)(time_now - time_start) / CLOCKS_PER_SEC;
	if (time_passed >= TIME_TO_KILL) {
		shutdown = TRUE;
	}
	if (shutdown == TRUE) {
		return alpha;
	}	
	memcpy(ABboard[depth], ABboard[depth - 1], sizeof(ABboard[depth]));
	int midX, midY, curFlag;
	curFlag = ABboard[depth][cmd.x[0]][cmd.y[0]];
	for (int i = 0; i < cmd.numStep - 1; i++) {
		ABboard[depth][cmd.x[i]][cmd.y[i]] = EMPTY;
		ABboard[depth][cmd.x[i + 1]][cmd.y[i + 1]] = curFlag;
		if (abs(cmd.x[i] - cmd.x[i + 1]) == 2) {
			midX = (cmd.x[i] + cmd.x[i + 1]) / 2;
			midY = (cmd.y[i] + cmd.y[i + 1]) / 2;
			ABboard[depth][midX][midY] = EMPTY;
		}
	}
	for (int i = 0; i < BOARD_SIZE; i++) {
		if (ABboard[depth][0][i] == BLACK_FLAG) {
			ABboard[depth][0][i] = BLACK_KING;
		}
		if (ABboard[depth][BOARD_SIZE - 1][i] == WHITE_FLAG) {
			ABboard[depth][BOARD_SIZE - 1][i] = WHITE_KING;
		}
	}
	if (depth == FinalControl) {
		int mark = 0;
		for (int i = 0; i < BOARD_SIZE; i++)
		{
			for (int j = 0; j < BOARD_SIZE; j++)
			{
				if (ABboard[FinalControl][i][j] > 0)
				{
					if (ABboard[FinalControl][i][j] == 3 - myFlag) {
						mark -= 10;
					}
					if (ABboard[FinalControl][i][j] == myFlag) {
						mark += 10;
					}
					if (ABboard[FinalControl][i][j] == 5 - myFlag) {
						if(myRound<=40){
							mark -= 11;
						}
						else{
							mark -= 30;
						}
					}
					if (ABboard[FinalControl][i][j] == myFlag + 2) {
						if(myRound<=40){
							mark += 11;
						}
						else{
							mark += 30;
						}
					}
				}
			}
		}
		if ((FinalControl & 1) != 0) {
			return -mark;
		}
		else {
			return mark;
		}
	}
	struct Command command = {
	.x = {0},
	.y = {0},
	.numStep = 0
	};
	struct Command jumpCmdOption[JUMP_OPTIONS] = { {.x = {0},.y = {0},.numStep = 0} };
	BOOL can_jump_flag = FALSE;
	int maxStep = 1;
	int option = 0;
	for (int i = 0; i < BOARD_SIZE; i++) {
		for (int j = 0; j < BOARD_SIZE; j++) {
			if (ABboard[depth][i][j] == flag || ABboard[depth][i][j] == flag + 2) {
				longestJumpCmd.numStep = 1;
				memset(jumpCmdOptions, 0, sizeof(struct Command[JUMP_OPTIONS]));
				tryToJump(i, j, 0, ABboard[depth]);
				if (longestJumpCmd.numStep > maxStep) {
					can_jump_flag = TRUE;
					memcpy(&command, &longestJumpCmd, sizeof(struct Command));
					memcpy(&jumpCmdOption, &jumpCmdOptions, sizeof(struct Command[JUMP_OPTIONS]));
					option = jumpOptions;
					maxStep = longestJumpCmd.numStep;
				}
				else if (longestJumpCmd.numStep == maxStep && maxStep != 1) {
					int temp_options = 0;
					for (int k = 0; k < JUMP_OPTIONS; k++) {
						memcpy(&jumpCmdOption[k + option], &jumpCmdOptions[k], sizeof(struct Command));
						temp_options++;
						if (jumpCmdOptions[k].numStep != jumpCmdOptions[k + 1].numStep) {
							break;
						}
					}
					option += temp_options;
				}
			}
		}
	}
	if (can_jump_flag == TRUE && option == 1) {
		alpha = -AlphaBeta(3 - flag, command, -beta, -alpha, depth + 1);
		return alpha;
	}
	else if (can_jump_flag == TRUE) {
		for (int i = 0; i < option; i++) {
			int TempMark = -AlphaBeta(3 - flag, jumpCmdOption[i], -beta, -alpha, depth + 1);
			if (alpha < TempMark) {
				alpha = TempMark;
			}
			if (TempMark >= beta) {
				return TempMark;
			}
		}
		return alpha;
	}
	BOOL is_AlphaBeta_done=FALSE;
	for (int i = 0; i < BOARD_SIZE; i++) {
		for (int j = 0; j < BOARD_SIZE; j++) {
			if (ABboard[depth][i][j] > 0 && (ABboard[depth][i][j] == flag || ABboard[depth][i][j] == flag + 2)) {
				int newX, newY;
				if (ABboard[depth][i][j] == BLACK_FLAG) {
					for (int k = 0; k < 2; k++) {
						newX = i + moveBlackDir[k][0];
						newY = j + moveBlackDir[k][1];
						if (isInBound(newX, newY) && ABboard[depth][newX][newY] == EMPTY) {
							struct Command temp_command = { .x[0] = i,.y[0] = j,.x[1] = newX,.y[1] = newY,.numStep = 2 };
							int TempMark = -AlphaBeta(3 - flag, temp_command, -beta, -alpha, depth + 1);
							is_AlphaBeta_done= TRUE;
							if (alpha < TempMark) {
								alpha = TempMark;
							}
							if (TempMark >= beta) {
								return TempMark;
							}
						}
					}
				}
				else if (ABboard[depth][i][j] == WHITE_FLAG) {
					for (int k = 0; k < 2; k++) {
						newX = i + moveWhiteDir[k][0];
						newY = j + moveWhiteDir[k][1];
						if (isInBound(newX, newY) && ABboard[depth][newX][newY] == EMPTY) {
							struct Command temp_command = { .x[0] = i,.y[0] = j,.x[1] = newX,.y[1] = newY,.numStep = 2 };
							int TempMark = -AlphaBeta(3 - flag, temp_command, -beta, -alpha, depth + 1);
							is_AlphaBeta_done= TRUE;
							if (alpha < TempMark) {
								alpha = TempMark;
							}
							if (TempMark >= beta) {
								return TempMark;
							}
						}
					}
				}
				else if (ABboard[depth][i][j] == WHITE_KING || ABboard[depth][i][j] == BLACK_KING) {
					for (int k = 0; k < 4; k++) {
						newX = i + moveKingDir[k][0];
						newY = j + moveKingDir[k][1];
						if (isInBound(newX, newY) && ABboard[depth][newX][newY] == EMPTY) {
							struct Command temp_command = { .x[0] = i,.y[0] = j,.x[1] = newX,.y[1] = newY,.numStep = 2 };
							int TempMark = -AlphaBeta(3 - flag, temp_command, -beta, -alpha, depth + 1);
							is_AlphaBeta_done= TRUE;
							if (alpha < TempMark) {
								alpha = TempMark;
							}
							if (TempMark >= beta) {
								return TempMark;
							}
						}
					}
				}
			}
		}
	}
	if(is_AlphaBeta_done==FALSE){
		return -INFINITE;
	}
	return alpha;
}

struct Command aiTurn() {
	struct Command command = {
		.x = {0},
		.y = {0},
		.numStep = 0
	};
	int maxStep = 1;
	BOOL can_jump_flag = FALSE;
	struct Command jumpCmdOption[JUMP_OPTIONS] = { {.x = {0},.y = {0},.numStep = 0} };
	int option = 0;
	for (int i = 0; i < BOARD_SIZE; i++) {
		for (int j = 0; j < BOARD_SIZE; j++) {
			if (board[i][j] == myFlag || board[i][j] == myFlag + 2) {
				longestJumpCmd.numStep = 1;
				memset(jumpCmdOptions, 0, sizeof(struct Command[JUMP_OPTIONS]));
				tryToJump(i, j, 0, board);
				if (longestJumpCmd.numStep > maxStep) {
					can_jump_flag = TRUE;
					memcpy(&command, &longestJumpCmd, sizeof(struct Command));
					memcpy(&jumpCmdOption, &jumpCmdOptions, sizeof(struct Command[JUMP_OPTIONS]));
					option = jumpOptions;
					maxStep = longestJumpCmd.numStep;
				}
				else if (longestJumpCmd.numStep == maxStep && maxStep != 1) {
					int temp_options = 0;
					for (int k = 0; k < JUMP_OPTIONS; k++) {
						memcpy(&jumpCmdOption[k + option], &jumpCmdOptions[k], sizeof(struct Command));
						temp_options++;
						if (jumpCmdOptions[k].numStep != jumpCmdOptions[k + 1].numStep) {
							break;
						}
					}
					option += temp_options;
				}
			}
		}
	}
	int beta = INFINITE;
	int alpha = -INFINITE;
	memcpy(ABboard[0], board, sizeof(ABboard[0]));
	if (can_jump_flag == TRUE && option == 1) {
		return command;
	}
	else if (can_jump_flag == TRUE) {
		if (FinalControl >= DEPTH + 2 && abs(PVtempCmd.x[0] - PVtempCmd.x[1]) == 2) {
			int PVMark = -AlphaBeta(3 - myFlag, PVtempCmd, -beta, -alpha, 1);
			if (alpha < PVMark) {
				alpha = PVMark;
				memcpy(&command, &PVtempCmd, sizeof(struct Command));
			}
		}
		for (int i = 0; i < option; i++) {
			if (memcmp(&jumpCmdOption[i], &PVtempCmd, sizeof(struct Command)) == 0 && FinalControl >= DEPTH + 2) {
				continue;
			}
			int TempMark = -AlphaBeta(3 - myFlag, jumpCmdOption[i], -beta, -alpha, 1);
			if (alpha < TempMark) {
				alpha = TempMark;
				memcpy(&command, &jumpCmdOption[i], sizeof(struct Command));
			}
		}
		return command;
	}
	if (myFlag == WHITE_FLAG) {
		if (FinalControl >= DEPTH + 2 && abs(PVtempCmd.x[0] - PVtempCmd.x[1]) == 1) {
			int PVMark = -AlphaBeta(3 - myFlag, PVtempCmd, -beta, -alpha, 1);
			if (alpha < PVMark) {
				alpha = PVMark;
				moveCmd.x[0] = PVtempCmd.x[0];
				moveCmd.y[0] = PVtempCmd.y[0];
				moveCmd.x[1] = PVtempCmd.x[1];
				moveCmd.y[1] = PVtempCmd.y[1];
			}
		}
		for (int i = 0; i < BOARD_SIZE; i++) {
			for (int j = 0; j < BOARD_SIZE; j++) {
				if (board[i][j] == myFlag || board[i][j] == myFlag + 2) {
					int newX, newY;
					if (board[i][j] == BLACK_FLAG) {
						for (int k = 0; k < 2; k++) {
							newX = i + moveBlackDir[k][0];
							newY = j + moveBlackDir[k][1];
							if (isInBound(newX, newY) && board[newX][newY] == EMPTY) {
								struct Command temp_command = { .x[0] = i,.y[0] = j,.x[1] = newX,.y[1] = newY,.numStep = 2 };
								if (memcmp(&temp_command, &PVtempCmd, sizeof(struct Command)) == 0 && FinalControl >= DEPTH + 2) {
									continue;
								}
								int TempMark = -AlphaBeta(3 - myFlag, temp_command, -beta, -alpha, 1);
								if (alpha < TempMark) {
									alpha = TempMark;
									moveCmd.x[0] = i;
									moveCmd.y[0] = j;
									moveCmd.x[1] = newX;
									moveCmd.y[1] = newY;
								}
							}
						}
					}
					else if (board[i][j] == WHITE_FLAG) {
						for (int k = 0; k < 2; k++) {
							newX = i + moveWhiteDir[k][0];
							newY = j + moveWhiteDir[k][1];
							if (isInBound(newX, newY) && board[newX][newY] == EMPTY) {
								struct Command temp_command = { .x[0] = i,.y[0] = j,.x[1] = newX,.y[1] = newY,.numStep = 2 };
								if (memcmp(&temp_command, &PVtempCmd, sizeof(struct Command)) == 0 && FinalControl >= DEPTH + 2) {
									continue;
								}
								int TempMark = -AlphaBeta(3 - myFlag, temp_command, -beta, -alpha, 1);
								if (alpha < TempMark) {
									alpha = TempMark;
									moveCmd.x[0] = i;
									moveCmd.y[0] = j;
									moveCmd.x[1] = newX;
									moveCmd.y[1] = newY;
								}
							}
						}
					}
					else if (board[i][j] == WHITE_KING || board[i][j] == BLACK_KING) {
						for (int k = 0; k < 4; k++) {
							newX = i + moveKingDir[k][0];
							newY = j + moveKingDir[k][1];
							if (isInBound(newX, newY) && board[newX][newY] == EMPTY) {
								struct Command temp_command = { .x[0] = i,.y[0] = j,.x[1] = newX,.y[1] = newY,.numStep = 2 };
								if (memcmp(&temp_command, &PVtempCmd, sizeof(struct Command)) == 0 && FinalControl >= DEPTH + 2) {
									continue;
								}
								int TempMark = -AlphaBeta(3 - myFlag, temp_command, -beta, -alpha, 1);
								if (alpha < TempMark) {
									alpha = TempMark;
									moveCmd.x[0] = i;
									moveCmd.y[0] = j;
									moveCmd.x[1] = newX;
									moveCmd.y[1] = newY;
								}
							}
						}
					}
				}
			}
		}
	}
	else {
		if (FinalControl >= DEPTH + 2 && abs(PVtempCmd.x[0] - PVtempCmd.x[1]) == 1) {
			int PVMark = -AlphaBeta(3 - myFlag, PVtempCmd, -beta, -alpha, 1);
			if (alpha < PVMark) {
				alpha = PVMark;
				moveCmd.x[0] = PVtempCmd.x[0];
				moveCmd.y[0] = PVtempCmd.y[0];
				moveCmd.x[1] = PVtempCmd.x[1];
				moveCmd.y[1] = PVtempCmd.y[1];
			}
		}
		for (int i = BOARD_SIZE - 1; i >= 0; i--) {
			for (int j = BOARD_SIZE - 1; j >= 0; j--) {
				if (board[i][j] == myFlag || board[i][j] == myFlag + 2) {
					int newX, newY;
					if (board[i][j] == BLACK_FLAG) {
						for (int k = 0; k < 2; k++) {
							newX = i + moveBlackDir[k][0];
							newY = j + moveBlackDir[k][1];
							if (isInBound(newX, newY) && board[newX][newY] == EMPTY) {
								struct Command temp_command = { .x[0] = i,.y[0] = j,.x[1] = newX,.y[1] = newY,.numStep = 2 };
								if (memcmp(&temp_command, &PVtempCmd, sizeof(struct Command)) == 0 && FinalControl >= DEPTH + 2) {
									continue;
								}
								int TempMark = -AlphaBeta(3 - myFlag, temp_command, -beta, -alpha, 1);
								if (alpha < TempMark) {
									alpha = TempMark;
									moveCmd.x[0] = i;
									moveCmd.y[0] = j;
									moveCmd.x[1] = newX;
									moveCmd.y[1] = newY;
								}
							}
						}
					}
					else if (board[i][j] == WHITE_FLAG) {
						for (int k = 0; k < 2; k++) {
							newX = i + moveWhiteDir[k][0];
							newY = j + moveWhiteDir[k][1];
							if (isInBound(newX, newY) && board[newX][newY] == EMPTY) {
								struct Command temp_command = { .x[0] = i,.y[0] = j,.x[1] = newX,.y[1] = newY,.numStep = 2 };
								if (memcmp(&temp_command, &PVtempCmd, sizeof(struct Command)) == 0 && FinalControl >= DEPTH + 2) {
									continue;
								}
								int TempMark = -AlphaBeta(3 - myFlag, temp_command, -beta, -alpha, 1);
								if (alpha < TempMark) {
									alpha = TempMark;
									moveCmd.x[0] = i;
									moveCmd.y[0] = j;
									moveCmd.x[1] = newX;
									moveCmd.y[1] = newY;
								}
							}
						}
					}
					else if (board[i][j] == WHITE_KING || board[i][j] == BLACK_KING) {
						for (int k = 0; k < 4; k++) {
							newX = i + moveKingDir[k][0];
							newY = j + moveKingDir[k][1];
							if (isInBound(newX, newY) && board[newX][newY] == EMPTY) {
								struct Command temp_command = { .x[0] = i,.y[0] = j,.x[1] = newX,.y[1] = newY,.numStep = 2 };
								if (memcmp(&temp_command, &PVtempCmd, sizeof(struct Command)) == 0 && FinalControl >= DEPTH + 2) {
									continue;
								}
								int TempMark = -AlphaBeta(3 - myFlag, temp_command, -beta, -alpha, 1);
								if (alpha < TempMark) {
									alpha = TempMark;
									moveCmd.x[0] = i;
									moveCmd.y[0] = j;
									moveCmd.x[1] = newX;
									moveCmd.y[1] = newY;
								}
							}
						}
					}
				}
			}
		}
	}
	if (alpha == -INFINITE && shutdown == FALSE) {
		printf("Good Game!\n");
		fflush(stdout);
		exit(0);
	}
	if (shutdown == FALSE) {
		show_alpha = alpha;
	}
	memcpy(&command, &moveCmd, sizeof(struct Command));
	return command;
}

void start() {
	memset(board, 0, sizeof(board));
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 8; j += 2) {
			board[i][j + (i + 1) % 2] = WHITE_FLAG;
		}
	}
	for (int i = 5; i < 8; i++) {
		for (int j = 0; j < 8; j += 2) {
			board[i][j + (i + 1) % 2] = BLACK_FLAG;
		}
	}
}

void turn() {
	// AI
	time_start = clock();
	shutdown = FALSE;
	if (myRound <= 60 - DEPTH / 2) {
		FinalControl = DEPTH;
	}
	else {
		FinalControl = 2 * (60 - myRound) + 3 - myFlag;
	}
	struct Command command = aiTurn();
	struct Command return_command;
	memcpy(&return_command, &command, sizeof(struct Command));
	memcpy(&PVtempCmd, &command, sizeof(struct Command));
	if (myRound <= 60 - DEPTH / 2 - 1 ) {
		FinalControl = DEPTH + 2;
		command = aiTurn();
		if (shutdown == FALSE) {
			memcpy(&return_command, &command, sizeof(struct Command));
			memcpy(&PVtempCmd, &command, sizeof(struct Command));
		}
	}
	clock_t time_now = clock();
	double time_passed = (float)(time_now - time_start) / CLOCKS_PER_SEC;	
	if (myRound <= 60 - DEPTH / 2 - 2 && time_passed < 0.6) {
		FinalControl = DEPTH + 4;
		command = aiTurn();
		if (shutdown == FALSE) {
			memcpy(&return_command, &command, sizeof(struct Command));
			memcpy(&PVtempCmd, &command, sizeof(struct Command));
			printf("DEBUG depth_done=%d, time=%.3fs\n", FinalControl-2, time_passed);
		}
	}
	time_now = clock();
	time_passed = (float)(time_now - time_start) / CLOCKS_PER_SEC;
	if (myRound <= 60 - DEPTH / 2 - 3 && time_passed < 0.6) {
		FinalControl = DEPTH + 6;
		command = aiTurn();
		if (shutdown == FALSE) {
			memcpy(&return_command, &command, sizeof(struct Command));
			memcpy(&PVtempCmd, &command, sizeof(struct Command));
			printf("DEBUG depth_done=%d, time=%.3fs\n", FinalControl-2, time_passed);
		}
	}
	time_now = clock();
	time_passed = (float)(time_now - time_start) / CLOCKS_PER_SEC;
	if (myRound <= 60 - DEPTH / 2 - 4 && time_passed < 0.6) {
		FinalControl = DEPTH + 8;
		command = aiTurn();
		if (shutdown == FALSE) {
			memcpy(&return_command, &command, sizeof(struct Command));
			memcpy(&PVtempCmd, &command, sizeof(struct Command));
			printf("DEBUG depth_done=%d, time=%.3fs\n", FinalControl-2, time_passed);
		}
	}
	time_now = clock();
	time_passed = (float)(time_now - time_start) / CLOCKS_PER_SEC;
	if (myRound <= 60 - DEPTH / 2 - 5 && time_passed < 0.6) {
		FinalControl = DEPTH + 10;
		command = aiTurn();
		if (shutdown == FALSE) {
			memcpy(&return_command, &command, sizeof(struct Command));
			memcpy(&PVtempCmd, &command, sizeof(struct Command));
			printf("DEBUG depth_done=%d, time=%.3fs\n", FinalControl-2, time_passed);
		}
	}
	time_now = clock();
	time_passed = (float)(time_now - time_start) / CLOCKS_PER_SEC;
	if (myRound <= 60 - DEPTH / 2 - 6 && time_passed < 0.6) {
		FinalControl = DEPTH + 12;
		command = aiTurn();
		if (shutdown == FALSE) {
			memcpy(&return_command, &command, sizeof(struct Command));
			memcpy(&PVtempCmd, &command, sizeof(struct Command));
			printf("DEBUG depth_done=%d, time=%.3fs\n", FinalControl-2, time_passed);
		}
	}
	time_now = clock();
	time_passed = (float)(time_now - time_start) / CLOCKS_PER_SEC;
	if (myRound <= 60 - DEPTH / 2 - 7 && time_passed < 0.6) {
		FinalControl = DEPTH + 14;
		command = aiTurn();
		if (shutdown == FALSE) {
			memcpy(&return_command, &command, sizeof(struct Command));
			memcpy(&PVtempCmd, &command, sizeof(struct Command));
			printf("DEBUG depth_done=%d, time=%.3fs\n", FinalControl-2, time_passed);
		}
	}
	place(return_command, board);
	time_now = clock();
	time_passed = (float)(time_now - time_start) / CLOCKS_PER_SEC;
	if (shutdown == TRUE) {
		FinalControl -= 2;
	}
	printf("DEBUG alpha=%d, depth_done=%d, time=%.3fs\n", show_alpha, FinalControl, time_passed);
	printf("%d", return_command.numStep);
	for (int i = 0; i < return_command.numStep; i++) {
		printf(" %d,%d", return_command.x[i], return_command.y[i]);
	}
	printf("\n");
	fflush(stdout);
}


int main() {
	char tag[10] = { 0 };
	struct Command command = {
		.x = {0},
		.y = {0},
		.numStep = 0
	};
	while (TRUE) {
		memset(tag, 0, sizeof(tag));
		scanf("%s", tag);
		if (strcmp(tag, START) == 0) {
			scanf("%d", &myFlag);
			if (myFlag == BLACK_FLAG) {
				myRound = 1;
			}
			else {
				myRound = 0;
			}
			start();
			printf("OK\n");
			fflush(stdout);
		}
		else if (strcmp(tag, PLACE) == 0) {
			myRound++;
			scanf("%d", &command.numStep);
			for (int i = 0; i < command.numStep; i++) {
				scanf("%d,%d", &command.x[i], &command.y[i]);
			}
			place(command, board);
		}
		else if (strcmp(tag, TURN) == 0) {
			turn();
		}
//		printBoard();
	}
	return 0;
}