// the goal of this app is to connect the output of the gcc debug to a curse window
// this way, when the debugger outputs an error, the programer can immediately correct the
// line at fault instead of doing the tedious task of opening up an editor, going to the
// line, changing one thing, saving and executing again.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <curses.h>

#define TRUE 1
#define FALSE 0

#define CODE_PAIR 1
#define ERROR_PAIR 2
#define HELP_PAIR 3
#define MESSAGE_PAIR 4

#define MAIN_MENU 0
#define INSERT_MENU 1

#define MAIN_MENU_SHORTCUTS "RIGHT  Next error   i  Insert mode  w  Write changes\nLEFT   Prev error   r  Relaunch cmd"
#define INSERT_MENU_SHORTCUTS "RIGHT Next char  UP   First char  enter/esc Main menu    suppr Del next char\nLEFT  Prev char  DOWN backspace Del prev char"

// Error holding double linked list

typedef struct ErrorNode { // double linked node
	struct ErrorNode *next; // next node
	struct ErrorNode *prev; // previous node
	int number; // node number
	char *filename; // path to the file TODO : maybe split path and filename ?
	long line_nb; // line number
	char *function_name; // name of the function the code appears in
	int error_in_line; // number of error in the line
	struct StringList* error_msgs; // all error messages
	char *origin_code; // original code line
	struct CharList *user_code; // user-modified code line
	struct StringList *help_list; // help lines
} ErrorNode;


typedef struct ErrorList { // double linked list
	ErrorNode *head; // head of the double linked list
	ErrorNode *tail; // tail of the double linked list
	int size; // number of nodes
} ErrorList;



// string holding simple linked list

typedef struct StringNode { // simple linked list
	struct StringNode *next; // next node
	char *content; // content
} StringNode;


typedef struct StringList {
	struct StringNode *head; // head of the linked list
	struct StringNode *tail; // tail of the linked list
	int size; // number of nodes
} StringList;


// char-holding linked list

typedef struct CharNode { // double linked list
	struct CharNode *next; // next node
	struct CharNode *prev; // previous node
	char content; // each node contains a char
} CharNode;

typedef struct CharList { // struct pointing to a char linked list
	CharNode *head; // head of the linked list
	CharNode *tail; // tail of the linked list
	int size; // number of nodes
} CharList;



// HELPER FUNCTIONS //


void insert_after(CharNode *current, CharNode *newnode, CharList *cl)
{
	newnode->prev = current;
	newnode->next = current->next;
	current->next = newnode;
	if (newnode->next == NULL) // insertion after tail
		cl->tail = newnode; // attach the new tail
	else
		newnode->next->prev = newnode; // next's prev is new
	cl->size += 1;
}

void insert_before(CharNode *current, CharNode *newnode, CharList *cl)
{
	newnode->next = current;
	newnode->prev = current->prev;
	current->prev = newnode;
	if (current->prev == NULL) // insertion before head
		cl->head = newnode; // attach the new head
	else
		newnode->prev->next = newnode; // prev's next is new
	cl->size += 1;
}

void delete_after(CharNode *current, CharList *cl)
{
	CharNode *temp = current->next;
	if (temp == NULL) return; // can't delete after tail
	current->next = temp->next;
	if (temp->next == NULL) // temp is tail
		cl->tail = current; // attach the new tail
	else
		temp->next->prev = current;
	cl->size -= 1;
	free(temp);
}

void delete_before(CharNode *current, CharList *cl)
{
	CharNode *temp = current->prev;
	if (temp == NULL) return; // can't delete before head
	current->prev = temp->prev;
	if (temp->prev == NULL) // temp is head
		cl->head = current; // attach the new head
	else
		temp->prev->next = current;
	cl->size -= 1;
	free(temp);
}

void free_char_list(CharList *cl)
{
	CharNode *temp, *node;
	node = cl->head;
	while (node != NULL)
	{
		temp = node->next;
		free(node);
		node = temp;
	}
	free(cl);
}


CharList* string_to_cl(char *string)
{
	// convert a string to char-holding double linked list (charlist)
	CharList* charlist = NULL;
	charlist = malloc(sizeof(CharList));
	charlist->head = NULL;
	charlist->tail = NULL;
	charlist->size = 0;

	for (int i=0; i<strlen(string); i++)
	{
		charlist->size += 1;

		CharNode *node = NULL; // creating the node
		node = malloc(sizeof(CharNode));

		node->next = NULL; // next is null
		node->prev = charlist->tail; // prev is list's tail

		node->content = string[i]; // content is a char

		if (i == 0)
			charlist->head = node;
		else
			charlist->tail->next = node;

		charlist->tail = node;
	}
	return charlist;
}


char* cl_to_string(CharList *charlist)
{
	// convert a charlist content to a string
	int size = charlist->size+1;
	char* str = malloc(size);
	int i = 0;
	CharNode *node = charlist->head;
	while (node != NULL)
	{
		str[i++] = node->content;
		node = node->next;
	}
	str[size-1] = '\0'; // null-terminated
	return str;
}

void print_charlist(CharList *charlist)
{
	// display the content of a charlist as a string
	CharNode *node = charlist->head;
	printf("str is #");
	while (node != NULL)
	{
		printf("%c", node->content);
		node = node->next;
	}
	printf("#\n");
}


void free_error_list(ErrorList *error_list)
{
	// completely free an error list and its member
	ErrorNode *enode = error_list->head;
	ErrorNode *etemp = NULL;

	while (enode != NULL)
	{
		free(enode->filename);
		free(enode->function_name);

		StringNode *mnode = enode->error_msgs->head;
		StringNode *mtemp = NULL;

		while (mnode != NULL)
		{
			free(mnode->content);
			mtemp = mnode;
			mnode = mnode->next;
			free(mtemp);
		}
		free(enode->error_msgs);


		free(enode->origin_code);
		free_char_list(enode->user_code);

		StringNode *hnode = enode->help_list->head;
		StringNode *htemp = NULL;

		while (hnode != NULL)
		{
			free(hnode->content);
			htemp = hnode;
			hnode = hnode->next;
			free(htemp);
		}
		free(enode->help_list);


		etemp = enode;
		enode = enode->next;
		free(etemp);
	}

	free(error_list);

}


void quit_on_error(char* str, int status)
{
	// display an error before quitting
	endwin(); // restores terminal
	printf("%s\n", str);
	exit(status);
}


// MAIN PROGRAM //


ErrorList* runCommand(char* user_cmd)
{

	// run the given command and stores the output errrors in an ErrorList
	FILE *p;
	char* errors_cmd = " 2>&1 > /dev/null"; // outputs only the errors/warning on stdout
	int size = strlen(user_cmd)+strlen(errors_cmd)+1;
	char* cmd = malloc(size); // new string
	strcpy(cmd, user_cmd); // add the user command
	strcat(cmd, errors_cmd); // add the error command
	cmd[size-1] = '\0';

	p = popen(cmd, "r"); // execute the command
	free(cmd);

	char* tmp;
	char line[300]; // buffer for reading the output
	regex_t first_line_expr;
	regex_t error_expr; // all the error message
	regex_t code_line_expr;
	regex_t help_line_expr;
	regex_t error_expr_line; // the line number of the error
	int is_new_code = TRUE;


	// compile the regexes

	if (regcomp(&first_line_expr, "[a-zA-Z0-9]*\\.c: ", 0) != 0)
		quit_on_error("Error in regex compilation\n", 1);

	// WARNING : does the error_expr overlap with the first_line_expr ?
	// TODO : change the error_expr with \\.c instead of .c
	if (regcomp(&error_expr, "[a-zA-Z0-9]*.c:[0-9]*:[0-9]*:", 0) != 0)
		quit_on_error("Error in regex compilation\n", 1);

	if (regcomp(&error_expr_line, ".c:[0-9]*:", 0) != 0)
		quit_on_error("Error in regex compilation\n", 1);

	if (regcomp(&code_line_expr, " *[0-9] |", 0) != 0)
		quit_on_error("Error in regex compilation\n", 1);

	if (regcomp(&help_line_expr, " *[^0-9] |", 0) != 0)
		quit_on_error("Error in regex compilation\n", 1);


	regmatch_t matchptr[1];

	ErrorList *error_list = NULL; // creating the error list
	error_list = malloc(sizeof(ErrorList));
	error_list->size = 0;
	error_list->head = NULL;
	error_list->tail = NULL;

	char *filename = NULL; // name of the file where the error is
	char *function_name = NULL; // name of the function the error is


	for (int row=0; row<300; row++) // TODO : replace the for by a while, until EOF
	{
		tmp = fgets(line, 300, p);
		if (tmp == NULL) break;

		if (regexec(&first_line_expr, line, 1, matchptr, 0) == 0)
		{
			// finding a first line expression
			int delim = matchptr->rm_eo-1;

			if (filename != NULL)
				free(filename); // already used, free first
			filename = malloc(delim*sizeof(char));
			strncpy(filename, line, delim-1);
			filename[delim-1] = '\0'; // null char

			if (function_name != NULL)
				free(function_name); // already used, free first
			function_name = malloc((strlen(line)-delim-1)*sizeof(char));
			strncpy(function_name, line+delim, strlen(line)-delim-2);
			function_name[strlen(line)-delim-2] = '\0'; // null char
		}


		if (regexec(&error_expr, line, 1, matchptr, 0) == 0)
		{
			// finding an error expression
			is_new_code = TRUE;
			int delim = matchptr->rm_eo;
			long line_nb_from_str = -1;


			if (regexec(&error_expr_line, line, 1, matchptr, 0) == 0)
			{
				// finding the line number
				line_nb_from_str = strtoll(line+matchptr->rm_so+3, NULL, 10);
			}
			else
				printf("Error : did not match!\n");


			int isOnSameLine = FALSE;
			if (error_list->size >= 1)
			{
				if (strcmp(error_list->tail->filename, filename) == 0 && error_list->tail->line_nb == line_nb_from_str)
				{
					// the error is on the same line as the previous error
					isOnSameLine = TRUE;
				}
			}

			if (isOnSameLine == TRUE)
			{
				// add the error message to the StringList error_msgs
				StringList *sl = error_list->tail->error_msgs;
				StringNode *msg_node = NULL;
				msg_node = malloc(sizeof(StringNode));
				msg_node->next = NULL;

				char *msg_error = malloc(strlen(line)-delim+1);
				strncpy(msg_error, line+delim+1, strlen(line)-delim);
				msg_node->content = msg_error; // set the error msg of the node


				sl->tail->next = msg_node;
				sl->tail = msg_node;
				sl->size += 1;

			}
			else
			{

				error_list->size += 1; // increment the ErrorNode counter

				ErrorNode *error_node = NULL; // create the next error node;
				error_node = malloc(sizeof(ErrorNode));


				error_node->number = error_list->size; // set the error nb

				error_node->filename = malloc(strlen(filename)+1);
				strcpy(error_node->filename, filename); // set the filename

				error_node->function_name = malloc(strlen(function_name)+1);
				strcpy(error_node->function_name, function_name); // set the function_name

				error_node->line_nb = line_nb_from_str; // set the error line number

				// create a new StringList for the error messages
				StringList *error_msgs = NULL;
				error_msgs = malloc(sizeof(StringList));

				StringNode *msg_node = NULL;
				msg_node = malloc(sizeof(StringNode));
				msg_node->next = NULL;

				char *msg_error = malloc(strlen(line)-delim+1);
				strncpy(msg_error, line+delim+1, strlen(line)-delim);
				msg_node->content = msg_error; // set the error msg of the node

				error_msgs->head = msg_node;
				error_msgs->tail = msg_node;

				error_msgs->size = 1;
				error_node->error_msgs = error_msgs;


				error_node->prev = error_list->tail; // link the node to the prev one
				error_node->next = NULL; // next node is NULL

				if (error_list->size == 1)
				{
					error_list->head = error_node; // first error -> set the head
				}
				else
				{
					error_list->tail->next = error_node; // set the actual tail's next to the node
				}

				error_list->tail = error_node;

				StringList *help_list = NULL;
				help_list = malloc(sizeof(StringList));


				error_node->help_list = help_list; // allocating the help_list

				error_node->help_list->size = 0;
				error_node->help_list->head = NULL;
				error_node->help_list->tail = NULL;
			}
		}

		if (regexec(&code_line_expr, line, 1, matchptr, 0) == 0)
		{


			if (is_new_code == TRUE)
			{
				char *code_origin = malloc(strlen(line)-matchptr->rm_eo); // reserve space
				strcpy(code_origin, matchptr->rm_eo+line+1); // copy the code line
				error_list->tail->origin_code = code_origin;
				error_list->tail->user_code = string_to_cl(code_origin);
				is_new_code = FALSE;
			}else
			{
				StringList *hl = error_list->tail->help_list;
				hl->size += 1;

				StringNode *help_temp = NULL;
				help_temp = malloc(sizeof(StringNode));

				help_temp->next = NULL;
				char *help_line = malloc(strlen(line)-matchptr->rm_eo); // reserve space
				strcpy(help_line, matchptr->rm_eo+line+1); // copy the help line with an offset
				help_temp->content = help_line; // assigning the content

				if (hl->size == 1)
				{
					hl->head = help_temp;
				}
				else
				{
					hl->tail->next = help_temp;
				}

				hl->tail = help_temp;
			}

		}

		if (regexec(&help_line_expr, line, 1, matchptr, 0) == 0)
		{
			StringList *hl = error_list->tail->help_list;
			hl->size += 1;

			StringNode *help_temp = NULL;
			help_temp = malloc(sizeof(StringNode));

			char *help_line = malloc(strlen(line)-matchptr->rm_eo); // reserve space
			strcpy(help_line, matchptr->rm_eo+line+1); // copy the help line with an offset
			help_temp->content = help_line; // assigning the content

			help_temp->next = NULL;

			if (hl->size == 1)
			{
				hl->head = help_temp;
			}
			else
			{
				hl->tail->next = help_temp;
			}

			hl->tail = help_temp;
		}


	}

	// free the popen
	pclose(p);

	free(function_name);
	free(filename);

	// free the compiled regexes
	regfree(&first_line_expr);
	regfree(&error_expr);
	regfree(&code_line_expr);
	regfree(&help_line_expr);
	regfree(&error_expr_line);

	return error_list;
}


void display_error(ErrorNode *node, int total_error)
{
	// display the content of an ErrorNode
	int line_cmp = 0;
	char str_number[15];

	sprintf(str_number, "Error %d/%d", node->number, total_error);

	mvaddstr(line_cmp++, 0, str_number); // error number
	mvaddstr(line_cmp++, 0, node->filename); // filename
	mvaddstr(line_cmp++, 0, node->function_name); // function_name

	StringList *error_msg_list = node->error_msgs;
	StringNode *error_msg_node = error_msg_list->head;

	// print the error messages
	attron(COLOR_PAIR(ERROR_PAIR));
	for (int i=0; i<error_msg_list->size; i++)
	{
		sprintf(str_number, "%d : ", i+1);
		mvaddstr(line_cmp++, 0, str_number); // error nb
		addstr(error_msg_node->content); // error message
		error_msg_node = error_msg_node->next;
	}
	attroff(COLOR_PAIR(ERROR_PAIR));

	// print the code line
	attron(COLOR_PAIR(CODE_PAIR));
	char *str = cl_to_string(node->user_code);
	mvaddstr(line_cmp++, 0, str); // code with error, editable by user
	free(str);
	attroff(COLOR_PAIR(CODE_PAIR));
	line_cmp++; // jump a line
	mvaddstr(line_cmp++, 0, node->origin_code); // code with error, original

	StringList *help_list = node->help_list;
	StringNode *help_node = help_list->head;

	// print the help messages
	attron(COLOR_PAIR(HELP_PAIR));
	for (int i = 0; i < help_list->size; i++)
	{
		mvaddstr(line_cmp++, 0, help_node->content); // help line
		help_node = help_node->next;
	}
	attroff(COLOR_PAIR(HELP_PAIR));

	return;
}


void display_interface(int menuType)
{
	// draw the "interface" (shortcuts, etc)
	move(LINES-2,0);
	clrtobot(); // clear to bottom

	switch (menuType)
	{
		case (MAIN_MENU):
			mvaddstr(LINES-2,0,MAIN_MENU_SHORTCUTS);
			break;
		case (INSERT_MENU):
			mvaddstr(LINES-2,0,INSERT_MENU_SHORTCUTS);
			break;
		default:

	};

}

void display_message(char *str)
{
	// display a message
	move(LINES-5,0);
	clrtoeol (); // erase any previous message

	if (str != NULL)
	{
		attron(COLOR_PAIR(MESSAGE_PAIR));
		int x_cur = (COLS-strlen(str))/2;
		mvaddstr(LINES-5, x_cur, str);
		attroff(COLOR_PAIR(MESSAGE_PAIR));
	}
}


int edit(WINDOW *screen, ErrorNode *en)
{
	// edit the code containing an error and returns TRUE if at least one edit has been made
	noecho(); // don't display what is typed
	curs_set(1); // cursor visible

	int c;
	char chr;
	int is_over = FALSE;
	int cur_x = 0; // cursor x position
	int cur_y = 3+en->error_msgs->size; // edit line is after 3lines + nb of error messages from the top
	int hasEdit = FALSE; // no edit made

	move(cur_y,cur_x); // move cursor to (0,4)
	refresh();

	CharList *charlist = en->user_code;
	CharNode *charnode = charlist->head;
	attron(COLOR_PAIR(CODE_PAIR));

	while (!is_over)
	{
		c = getch();
		switch (c)
		{
			case KEY_LEFT:
				// move cursor one char to the left if possible
				if (cur_x-1 >= 0)
				{
					cur_x -= 1;
					charnode = charnode->prev;
					move(cur_y, cur_x);
					refresh();
				}
				break;

			case KEY_RIGHT:
				// move cursor one char to the right if possible
				if (cur_x+1 < charlist->size)
				{
					cur_x += 1;
					charnode = charnode->next;
					move(cur_y, cur_x);
					refresh();
				}
				break;

			case KEY_DOWN:
				// move cursor to the last char
				cur_x = charlist->size - 1;
				charnode = charlist->tail;
				move(cur_y, cur_x);
				refresh();
				break;

			case KEY_UP:
				// move cursor to the first char
				cur_x = 0;
				charnode = charlist->head;
				move(cur_y, cur_x);
				refresh();
				break;


			case 10: // enter key
			case 27: // Escape key
				is_over = TRUE;
				break;

			case 330: // suppr key
				//remove the charnode after the current
				if (cur_x != charlist->size-1) // remove tail
				{
					delete_after(charnode, charlist);
					mvdelch(cur_y, cur_x);
					move(cur_y,cur_x);
					refresh();
				}
				hasEdit = TRUE;
				break;

			case KEY_BACKSPACE:
			case 127:
			case '\b':
				// remove the charnode before the current node
				if (cur_x != 0)
				{
					delete_before(charnode, charlist);
					cur_x -= 1;
					mvdelch(cur_y, cur_x);
					refresh();
				}
				hasEdit = TRUE;
				break;


			default:
				// insert charnode before the current node

				// TODO: use the beautiful insert_before function

				chr = c;
				CharNode *newnode = NULL;
				newnode = malloc(sizeof(CharNode));

				newnode->content = chr;
				newnode->next = charnode;
				newnode->prev = charnode->prev;

				if (cur_x == 0)// head insertion
					charlist->head = newnode;
				else
					charnode->prev->next = newnode;

				charnode->prev = newnode;

				charlist->size += 1;
				mvinsch(cur_y, cur_x, chr);
				refresh();
				cur_x += 1;
				hasEdit = TRUE;
				break;
		};


	}
	attroff(COLOR_PAIR(CODE_PAIR));
	curs_set(0);
	noecho();
	return hasEdit;
}


int place_in_file(ErrorList *el)
{
	// for every node in the ErrorList, write the user-modified code at the specified line
	char cp_buf[300];
	int nb_line = 1;
	char *filename = NULL;
	char *temp_name = NULL; // temporary name for the destination file
	int shouldOpenFile = TRUE; // whether we should open the files
	int isError = FALSE; // no error has happened

	FILE *forigin = NULL;
	FILE *fnew = NULL;

	ErrorNode *node = el->head;

	while (node != NULL)
	{

		filename = node->filename;

		if (shouldOpenFile == TRUE)
		{
		        forigin = fopen(filename, "r"); // origin file
			temp_name = malloc(sizeof(filename)+6); // filename+".temp"
		        fnew = fopen(temp_name, "w"); // destination file*
			shouldOpenFile = FALSE;
		}


		while (nb_line < node->line_nb)
		{
			if (fgets(cp_buf, 300, forigin) != NULL)
			{
				if (strlen(cp_buf) != 300) // buffer not full
					nb_line ++; // we found an end of line
				fputs(cp_buf, fnew); // copy the line in the new file
			}
		}

		// put the user-modified code in the new file
		fputs(cl_to_string(node->user_code), fnew);
		nb_line ++;

	        // read the new line but don't write it in the new
	        do
	        {
	                fgets(cp_buf, 300, forigin);
	        } while (strlen(cp_buf) == 300);

		node = node->next; // change the node

		char *tmp = "NULL";
		if (node != NULL)
			tmp = node->filename;

		if ( strcmp(tmp, filename) != 0 )
		{
			// the error is in another file or this is the last node
		        // copy everything to the end in the new file
		        while ( !feof(forigin) )
		        {
		                if (fgets(cp_buf, 300, forigin) != NULL)
				{
		                        fputs(cp_buf, fnew); // copy the line in the new file
				}
		        }

		        // closing the files
		        fclose(forigin);
		        fclose(fnew);
			if (rename(temp_name,filename) != 0)
			{
				isError = TRUE; // an error has happened !
				break;
			}
			shouldOpenFile = TRUE; // we have to open new files
		}

	}

	return isError;
}


// MAIN FUNCTION //


int main(int argc, char* argv[])
{
	WINDOW *screen;

	if (argc == 1)
	{
		printf("Usage is ./exe arg1 arg2 arg3 ...\n");
		exit(1);
	}

	int size = 0;
	int i;

	// putting all the arguments into one string
	for (i=1; i<argc; i++)
	{
		size += strlen(argv[i])+1; // counting arglen+space
	}

	char *command = malloc(size*sizeof(char));
	strcpy(command, argv[1]); // copy the first arg

	for (i=2; i<argc; i++)
	{
		strcat(command, " "); // add a space
		strcat(command, argv[i]); // add all args in one string
	}
	command[size-1] = '\0'; // null-terminated string

	// curses initialization
        screen = initscr();
        noecho(); // don't echo keystrokes
        cbreak(); // keyboard input valid immediately, not after hit Enter
        keypad(screen, TRUE); // enable keypad
        curs_set(0); // no cursor
        timeout(-1); // blocking wait

	start_color(); // initialize use of colours in the terminal
	init_pair(CODE_PAIR, COLOR_BLACK, COLOR_WHITE);
	init_pair(ERROR_PAIR, COLOR_RED, COLOR_BLACK); //, COLOR_RED);
	init_pair(HELP_PAIR, COLOR_GREEN, COLOR_BLACK); //, COLOR_GREEN);
	init_pair(MESSAGE_PAIR, COLOR_BLACK, COLOR_WHITE);


	int isRelaunch = TRUE; // whether the loop will relaunch
	int isOver; // wether the menu is over
	int shouldClear = TRUE; // whether we shoud clear the screen
	int hasEdit = FALSE; // no edit for now
	int hasSaved = TRUE; // whether the origin files have been made
	char *message = NULL;

	ErrorList* error_list = NULL;
	ErrorNode* node = NULL;

	while (isRelaunch)
	{

		error_list = runCommand(command); // run the command
		node = error_list->head;


		if (error_list->size == 0)
		{
			endwin();
			printf("The compiled program shows no error!\n");
			exit(0);
		}

		message = "Launching : done";

		isRelaunch = FALSE; // the program will not relaunch if not told so
		isOver = FALSE;
		int c;


		while (!isOver) // main menu
		{

			if (shouldClear)
				clear(); // clears the window

			// display all info
			display_error(node, error_list->size);
			display_interface(MAIN_MENU);
			display_message(message);

			shouldClear = TRUE;
			message = NULL;

			c = getch();
			switch (c)
			{
				case KEY_RIGHT:
					if (node->next != NULL) node = node->next;
					break;
				case KEY_LEFT:
					if (node->prev != NULL) node = node->prev;
					break;
				case 105: // letter 'i' for imput
					display_interface(INSERT_MENU);
					hasEdit = edit(screen, node);
					if (hasEdit)
						hasSaved = FALSE; // edited but not saved
					break;

				case 114: // letter 'r' for re-launch
					if (hasSaved)
					{
						display_message("Relaunching the command");
						isOver = TRUE; // exit menu
						isRelaunch = TRUE; // relaunch command
					}
					else
					{
						message = "Please save the changes for relaunching";
					}
					break;

				case 119: // letter 'w' for write
					if (hasEdit)
					{
						display_message("Beginning to write");
						if (place_in_file(error_list))
						{
							message ="ERROR IN WRITE !";
						}
						else
						{
							message = "Successful write !";
						}
						hasEdit = FALSE; // reset the edit flag
						hasSaved = TRUE; // file(s) has been saved
					}
					else
					{
						message = "No change to write";
					}
					break;

				case 10: // enter key
				case 27: // Escape key
					if (hasEdit)
					{
						display_message("Warning : edits have been made. Exit ? Y/N");
						int cc = getch();
						switch (cc)
						{
							case 89: // 'Y'
							case 121: // 'y'
								isOver = TRUE;
								break;
							default:
						};
					}
					else
					{
						isOver = TRUE;
					}
					break;

				case 410: // resize
					break;

				default:
					// no default behavior

			};

		}
	}

	free(command);
	free_error_list(error_list);


	endwin(); // restore original window

	return 0;
}
