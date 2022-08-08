/*Thư viện cần thiết cho project*/
#include <stdio.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
/* define một số hằng số cần thiết*/
#define BUF_SIZE 200
#define OP_CHECK_NOTSP 0x15
#define OP_CHECK_BG 0x26	 // &
#define OP_FROMFILE 0x3C	 // <
#define OP_TOFILE 0x3E		 // >
#define OP_FROMFILE_APP 0xAB // <<
#define OP_TOFILE_APP 0xBB	 // >>
#define OP_CHECK_PIPE 0x7C
#define LINE 80
// Kích thước history
#define ARRAYS_HISTORY_SIZE 100
// Mảng lưu trữ lịch sử
static char *Arrays_History[ARRAYS_HISTORY_SIZE];
// Biến lịch sử
static int HISTORY_COUNNT = 0;
static void EXCUTE_TERMI(const char *args);
static void ADD_HISTORY(const char *Termial_CMD);
static void RUN_HISTORY_OLD(const char *Termial_CMD);
static void LIST_HISTORY();
static void SIGLENAL_HANDLE(const int Run_Case);
static void OUTPUT(char *Termial_CMD);
void PARSE_CMD(char *input, char **args1, unsigned *op, char **args2);
void CHILD_PROCESS(char *argv[]);
void CHILD_IMPORT_FILE(char **argv, char **dir);
void CHILD_EXPORT_FILE(char **argv, char **dir, bool is_append);
void CHILD_PIDE_CMD(char **ar_in, char **ar_out);
void CMDPARENT(pid_t child_pid, int wait);
int main()
{
	printf("Enter 'history' to see history\n");
	printf("Enter '!!' to run command again\n");
	printf("Enter 'exit' to exit\n");
	// Bắt tìn hiệu
	// cấp phát bộ đệm cho args
	size_t Args_Size = 80;
	char *args = (char *)malloc(sizeof(char) * Args_Size);
	int Flat_Run = 0; // Flat để nhập lệnh
	struct sigaction Action, Action_Old;
	int waiting;
	char *argv[BUF_SIZE];
	char *argv2[BUF_SIZE];
	unsigned op_check = 0;
	Action.sa_flags = 0;
	Action.sa_handler = SIGLENAL_HANDLE;
	sigemptyset(&Action.sa_mask);
	if ((sigaction(SIGINT, &Action, &Action_Old) == -1) || (sigaction(SIGCHLD, &Action, &Action_Old) == -1))
	{
		perror("signal");
		return 1;
	}

	if (args == NULL)
	{ // Lỗi khi cấp phát bộ nhớ đếm cho malloc
		perror("malloc");
		return 1;
	}

	while (1)
	{
		if (!Flat_Run)
			printf("osh> "); // in dấu hiệu để nhập lệnh
		if (getline(&args, &Args_Size, stdin) == -1)
		{ // đọc dòng để nhận đầu vào
			if (errno == EINTR)
			{					 //  Lỗi
				clearerr(stdin); // xóa chuỗi bị lỗi đi
				Flat_Run = 1;	 // gán cờ lại bằng 1
				continue;		 // tiếp tục
			}
			perror("getline");
			break;
		}
		Flat_Run = 0;					// gán cờ lạ bằng 0
		int Args_Length = strlen(args); // lấy độ dài của args
		if (Args_Length == 1)
		{			  // chỉ khi dòng mới được nhập
			continue; // tiếp tục
		}
		args[Args_Length - 1] = '\0'; // xóa hàng mới

		if (strcmp(args, "exit") == 0)
		{
			/* Nếu dòng được nhập là "exit", thoát khỏi vòng lặp và kết thúc chương trình*/
			break;
		}
		else if (strcmp(args, "history") == 0)
		{
			/* gọi List_H nếu dòng đã nhập là "history"*/
			LIST_HISTORY();
		}
		else if (args[0] == '!')
		{
			/* Ký tự đầu tiên của dòng được nhập là "!" gọi Run_History*/
			RUN_HISTORY_OLD(args);
		}
		else
		{
			/*Sau khi check thì tiến hành add lệnh vào trong mạng lịch sử*/
			ADD_HISTORY(args);
			/*Tiến hành check lại lệnh xem lệnh của người input vào là lệnh gì và sét các trường hợp*/
			PARSE_CMD(args, argv, &op_check, argv2);
			waiting = 0;
			switch (op_check)
			{
			case OP_CHECK_BG:
				waiting = 1;
				break;
			case OP_FROMFILE:
				break;
			case OP_TOFILE:
				break;
			case OP_CHECK_PIPE:
				break;
			case OP_CHECK_NOTSP:
				printf("op_check: Invalid or not supported for syntax\n");
				break;
			default:
				break;
			}
			pid_t pid_1 = fork();
			switch (pid_1)
			{
			case -1:
				perror("fork() failed!");
				exit(EXIT_FAILURE);
			case 0:
				switch (op_check)
				{
				case OP_FROMFILE:
					CHILD_IMPORT_FILE(argv, argv2);
					break;
				case OP_TOFILE:
				case OP_TOFILE_APP:
					CHILD_EXPORT_FILE(argv, argv2, (op_check == OP_TOFILE_APP));
					break;
				case OP_CHECK_PIPE:
					CHILD_PIDE_CMD(argv, argv2);
					break;
				default:
					CHILD_PROCESS(argv);
					break;
				}
				exit(EXIT_FAILURE);
			default:
			{
				CMDPARENT(pid_1, waiting);
			}
				//TODO: Handle exception
			}
		}
	}
	free(args); // kết thúc chương trình giải phóng bộ nhớ
	return 0;
}
// Nhận và process lệnh từ người dùng
static void EXCUTE_TERMI(const char *args)
{
	char *Terminal_CMD = strdup(args);
	char *Temp_arrays[10];
	int argc = 0;

	// Tham số lẹnh parse

	Temp_arrays[argc++] = strtok(Terminal_CMD, " ");
	while (Temp_arrays[argc - 1] != NULL)
	{ // Receives INf
		Temp_arrays[argc++] = strtok(NULL, " ");
	}

	argc--; // Giảm argc xuống một để nó không lấy null làm mã thông báo
			// điều khiển hoạt động nền

	int background = 0;
	if (strcmp(Temp_arrays[argc - 1], "&") == 0)
	{
		background = 1;				//thiết lập cho công việc nền
		Temp_arrays[--argc] = NULL; // Xóa "&" khỏi mảng Temp_arrays
	}

	int fd[2] = {-1, -1}; // mô tả input và out

	while (argc >= 3)
	{

		//

		if (strcmp(Temp_arrays[argc - 2], ">") == 0)
		{ // output
			// Mở tệp tin
			fd[1] = open(Temp_arrays[argc - 1], O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
			if (fd[1] == -1)
			{
				perror("open");
				free(Terminal_CMD);
				return;
			}

			//cập nhật mảng tham số

			Temp_arrays[argc - 2] = NULL;
			argc -= 2;
		}
		else if (strcmp(Temp_arrays[argc - 2], "<") == 0)
		{ // input
			fd[0] = open(Temp_arrays[argc - 1], O_RDONLY);
			if (fd[0] == -1)
			{
				perror("open");
				free(Terminal_CMD);
				return;
			}
			Temp_arrays[argc - 2] = NULL;
			argc -= 2;
		}
		else
		{
			break;
		}
	}

	int STT;
	pid_t Pid_1 = fork(); // tạo new fork
	switch (Pid_1)
	{
	case -1: // Nếu Pid_1 trả về -1 là lỗi
		perror("fork");
		break;
	case 0: // Thành công

		if (fd[0] != -1)
		{
			if (dup2(fd[0], STDIN_FILENO) != STDIN_FILENO)
			{
				perror("dup2");
				exit(1);
			}
		}
		if (fd[1] != -1)
		{
			if (dup2(fd[1], STDOUT_FILENO) != STDOUT_FILENO)
			{
				perror("dup2");
				exit(1);
			}
		}
		execvp(Temp_arrays[0], Temp_arrays);
		perror("execvp");
		exit(0);
	default: // Process cha chờ con hoàn thành
		//Chowf
		close(fd[0]);
		close(fd[1]);
		if (!background)
			waitpid(Pid_1, &STT, 0);
		break;
	}
	free(Terminal_CMD);
	// giai phong
}

// Thêm lịch sử vào trong *Termial_CMD

static void ADD_HISTORY(const char *Termial_CMD)
{

	if (HISTORY_COUNNT == (ARRAYS_HISTORY_SIZE - 1))
	{ // FUll lịch sử
		int i;
		free(Arrays_History[0]);
		// giải phóng bộ nhứ của cá đầu tiên

		// Gán lịch sử ngược lại
		for (i = 1; i < HISTORY_COUNNT; i++)
			Arrays_History[i - 1] = Arrays_History[i];
		HISTORY_COUNNT--;
	}
	Arrays_History[HISTORY_COUNNT++] = strdup(Termial_CMD);
	// sau khi xóa xong ta thêm lịch sử vào lại trong HISTORY_COUNNT
}

// list_ history
static void LIST_HISTORY()
{
	printf("Your History:\n");
	for (int i = 0; i <= HISTORY_COUNNT - 1; i++)
	{ // các lịch sử
		printf("\t[%i]: %s\n", i + 1, Arrays_History[i]);
	}
}

// Chức năng xử lý tín hiệu

static void SIGLENAL_HANDLE(const int Run_Case_In)
{
	switch (Run_Case_In)
	{
	case SIGTERM:
	case SIGINT:
		break;
	case SIGCHLD:
		/* chờ đợi cho tất cả các hoạt động chết và nếu child kết thúc trong một phần khác của chương trình
	   Sử dụng cuộc gọi không chặn để ngăn trình xử lý tín hiệu chặn quá trình */
		while (waitpid(-1, NULL, WNOHANG) > 0)
			;
		break; // Ngưng
	}
}
// Thực hiện chạy cách lệnh có trong lịch sử

static void RUN_HISTORY_OLD(const char *Termial_CMD)
{
	int Count_Temp = 0;
	if (HISTORY_COUNNT == 0)
	{
		printf("[info]: No commands in history.\n");
		return;
	}
	if (Termial_CMD[1] == '!') // lấy mốc của lệnh cuối cùng được nhập
		Count_Temp = HISTORY_COUNNT - 1;
	else
	{
		Count_Temp = atoi(&Termial_CMD[1]) - 1; // lấy kí tự thứ 2 từ người dùng cho vào chỉ mục lịch sử
		if ((Count_Temp < 0) || (Count_Temp > HISTORY_COUNNT))
		{ // xử lí lỗi nếu không có chỉ số như vậy trong lịch sử
			fprintf(stderr, "No such command in history.\n");
			return;
		}
	}
	printf("%s\n", Termial_CMD);			  // in ra lệnh vừa nhập trong lịch sử
	EXCUTE_TERMI(Arrays_History[Count_Temp]); // chạy lệnh đó
}
/*Tiên hành chạy lệnh được truyền vào*/
void CHILD_PROCESS(char *argvS[])
{
	if (execvp(argvS[0], argvS) == -1)
	{
		perror("[ERORR]: Failed to execute cmd, Please check again.");
		exit(EXIT_FAILURE);
	}
}
/*Đọc từ file và tiến hành xử lí. đường dẫn*/
void CHILD_IMPORT_FILE(char **argv, char **dir)
{
	int fds;
	fds = open(dir[0], O_RDONLY); //O_RDONLY: Mở file dưới dạng chỉ được phép đọc
	if (fds == -1)
	{
		perror("[ERORR]: Redirect to input file failed, Please check again.");
		exit(EXIT_FAILURE);
	}
	dup2(fds, STDIN_FILENO);
	if (close(fds) == -1)
	{
		perror("[ERORR]: Closing input file failed");
		exit(EXIT_FAILURE);
	}
	/*Chạy chương trình*/
	CHILD_PROCESS(argv);
}
/*Hàm này chịu trách nhận lệnh của người dùng và xuất kết quả ra file*/
void CHILD_EXPORT_FILE(char **argv, char **dir, bool check_append)
{
	int fds;
	if (check_append && access(dir[0], F_OK) != -1)
	{
		fds = open(dir[0], O_WRONLY | O_APPEND);
	}
	else
	{
		fds = creat(dir[0], S_IRWXU);
	}
	if (fds == -1)
	{
		perror("[ERORR]: Redirect to output file failed, Please check again.");
		exit(EXIT_FAILURE);
	}
	dup2(fds, STDOUT_FILENO);
	if (close(fds) == -1)
	{
		perror("[ERORR]: Closing output file failed");
		exit(EXIT_FAILURE);
	}
	CHILD_PROCESS(argv);
}
/* Hàm này sẽ tiến hành chạy các chương trình có lệnh nhập vào có in and out ví dụ ls -l|less (theo ví dụ của thầy trong đề bài)*/
void CHILD_PIDE_CMD(char **ar_in, char **ar_out)
{
	int fds[2]; //Với fd[0] sẽ là in and với fd[1] sẽ là out

	if (pipe(fds) == -1) //fd[0] là chuẩn bị để read, fd[1] là chuẩn bị writing
	{
		perror("[ERORR]: Failed to pipe with cmd");
		/*tiến hành out*/
		exit(EXIT_FAILURE);
	}
	//argv's fork thứ nhất
	if (fork() == 0)
	{
		dup2(fds[1], STDOUT_FILENO);

		if (close(fds[0]) == -1 || close(fds[1]) == -1)
		{
			perror("[ERORR]: Unexpected error when make a pipe I/O");
		}
		CHILD_PROCESS(ar_in);
		/*tiến hành out*/
		exit(EXIT_FAILURE); //ngăn chặn truy cập vào fork của argv thứ hai
	}
	//argv's fork thứ 2
	if (fork() == 0)
	{
		dup2(fds[0], STDIN_FILENO);
		if (close(fds[1]) == -1 || close(fds[0]) == -1)
		{
			perror("[ERORR]: Unexpected error when make a pipe Input or Output");
		}
		CHILD_PROCESS(ar_out);
		/*tiến hành out*/
		exit(EXIT_FAILURE);
	}
	close(fds[0]);
	close(fds[1]);
	wait(0);
	wait(0);
}

void CMDPARENT(pid_t child_pids, int waiting)
{
	static int count_bg_process = 0;
	int status_temp;
	switch (waiting)
	{
		/*cha và con chạy đồng thời, trường hợp bình thường.*/
	case 0:
	{
		waitpid(child_pids, &status_temp, 0);
		break;
	}
	// parrent chờ  child - khi child chấm dứt, parent chạy và tra về.
	// Trường hợp '&' | chạy trong chương trình.
	default:
		count_bg_process += 1;
		printf("[%d] %d\n", count_bg_process, child_pids);
		waitpid(child_pids, &status_temp, WUNTRACED);
		if (WIFEXITED(status_temp))
		/*Macro này trả về một giá trị khác không nếu quá trình con kết thúc bình thường bằng exit hoặc _exit.*/
		{
			count_bg_process -= 1;
			printf("'%d' is finished and exited with status: %d\n", child_pids, status_temp);
		}
		break;
	}
}
/* Hàm xác định lệnh truyền vào theo từng trường hợp của đề bài */
void PARSE_CMD(char *input_command, char **args1, unsigned *ops, char **args2)
{
	// xóa toàn bộ bộ nhớ
	memset(args1, 0, BUF_SIZE);
	memset(args2, 0, BUF_SIZE);
	/* Khởi tạo các biến*/
	*ops = 0;
	bool new_token_temp = true;
	bool args2_turn = false;
	unsigned args1_i = 0;
	unsigned args2_i = 0;
	unsigned op_tofrom_potentials = 0;
	unsigned length_input = strlen(input_command);
	for (unsigned i = 0; i < length_input; i++)
	{
		switch (input_command[i])
		{
		case ' ':
			input_command[i] = 0;
			new_token_temp = true;
			if (op_tofrom_potentials == 1)
				op_tofrom_potentials = 2; // >, <
			break;
		/*xet các trường hợp lệnh truyên mà gán OP cho đúng với từng trường hợp để Ru lệnh*/
		/*trường hợp |*/
		case '|':
			input_command[i] = 0;
			new_token_temp = true;
			args2_turn = true;
			*ops = (*ops == 0) ? OP_CHECK_PIPE : OP_CHECK_NOTSP;
			break;
		/*trường hợp có file input vào*/
		case '<':
			input_command[i] = 0;
			new_token_temp = true;
			args2_turn = true;
			switch (op_tofrom_potentials)
			{
			case 0:
				op_tofrom_potentials = 1;
				*ops = (*ops == 0) ? OP_FROMFILE : OP_CHECK_NOTSP;
				break;

			case 1:
				if (*ops == OP_FROMFILE)
				{
					op_tofrom_potentials = 2;
					*ops = OP_FROMFILE_APP;
				}
				else
					*ops = OP_CHECK_NOTSP;
				break;

			case 2:
				*ops = OP_CHECK_NOTSP;
				break;

			default:
				break;
			}
			break;
		/*Trường hợp xuất kết quả lệnh ra file*/
		case '>':
			input_command[i] = 0;
			new_token_temp = true;
			args2_turn = true;
			switch (op_tofrom_potentials)
			{
			case 0:
				op_tofrom_potentials = 1;
				*ops = (*ops == 0) ? OP_TOFILE : OP_CHECK_NOTSP;
				break;
			case 1:
				if (*ops == OP_TOFILE)
				{
					op_tofrom_potentials = 2;
					*ops = OP_TOFILE_APP;
				}
				else
					*ops = OP_CHECK_NOTSP;
				break;

			case 2:
				*ops = OP_CHECK_NOTSP;
				break;

			default:
				break;
			}
			break;
		default:
			if (new_token_temp)
			{
				if (args2_turn)
				{
					args2[args2_i++] = (input_command + i);
				}
				else
				{
					args1[args1_i++] = (input_command + i);
				}
				new_token_temp = false;
			}
			break;
		}
		if (*ops == OP_CHECK_NOTSP)
			break;
	}
	/* check hiệu lực của OP trong từng trường hợp*/
	switch (*ops)
	{
	/* |, <, >, <<, >>*/
	case OP_FROMFILE:
	case OP_TOFILE:
	case OP_FROMFILE_APP:
	case OP_TOFILE_APP:
	case OP_CHECK_PIPE:
		if (args1_i == 0 || args2_i == 0)
		{
			*ops = OP_CHECK_NOTSP;
		}
		break;
	case OP_CHECK_BG:
		if (args1_i == 0 || args2_i != 0)
		{
			*ops = OP_CHECK_NOTSP;
		}
		break;
	}
}
/*----------------Cảm ơn thầy cô đã xem code của nhóm em. :(-----------------*/
