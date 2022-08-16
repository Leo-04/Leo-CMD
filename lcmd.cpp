#include <iostream>
#include <map>
#include <ctime>
#include <sstream>
#include <string>
#include <direct.h>
#include <signal.h>

#include <windows.h>
#define getcwd _getcwd

using namespace std;

// Ansi color codes
string color_prompt	= "\033[1;93m";
string color_cwd	= "\033[1;94m";
string color_error	= "\033[1;91m";
string color_info	= "\033[1;92m";
string color_normal	= "\033[1;97m";

//Alias variables
std::map<std::string, std::string> commands;

//Control key to exit
char exit_key = 'Z';


// Catch user pressing Contol-c and make sure it doesn't close Lcmd
bool ctrl_c_pressed = false;
void on_signal(int s){
	
	ctrl_c_pressed = true;
	cin.clear();
	
	// Reasign hook
	signal (SIGINT, on_signal);
}

//Enables ansi escape codes to be used for colors
void enable_color(){
	
	//Somtimes not defined
	#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
		#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
	#endif
	
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD dwMode = 0;
	GetConsoleMode(hOut, &dwMode);
	
	dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	SetConsoleMode(hOut, dwMode);
}

//Gets the directory of the current executeable
string make_exe_location(){
	char path[MAX_PATH];
	//Gets full path
    size_t len = GetModuleFileName(NULL, path, MAX_PATH);
	
	//Removes filename, leaving the file path
	path[len] = 0;
	for (size_t i = len; i >= 0 && path[i]!='\\'; i--)
		path[i] = 0;
	
	return path;
}

//Runs a command with current working directory
void run_command(string command, char* cwd){
	
	//Strutures to get prossess information
	STARTUPINFO info={sizeof(info)};
	PROCESS_INFORMATION processInfo;
	
	//Get duration of command
	int start = std::clock();
	int duration;
	
	//Executes command using 'cmd /c <command>'
	if (CreateProcess("C:/windows/system32/cmd.exe", const_cast<char *>(("/c "+command).c_str()), NULL, NULL, FALSE, 0, NULL, cwd, &info, &processInfo)){
		while (1){
			//Get exit code
			DWORD f;
			GetExitCodeProcess(processInfo.hProcess, &f);
			
			//Wait for it to finish
			if (f!=STILL_ACTIVE &&(WaitForSingleObject(processInfo.hProcess, 0)!=WAIT_TIMEOUT)){
				duration = ( std::clock() - start ) / (double) CLOCKS_PER_SEC; // Get duration
				
				//Show information about execution (exit code & duration)
				cout<<color_info<<"\nProsses Returned: "<<f<<" (0x"<<std::hex<<(f)<<") execution time: "<<std::dec<<duration<<" s"<<color_normal;
				break;
			}
		}
		//Clean up
		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);
	} else
		// Error happend
		cerr<<color_error<<"Can't Run: "<<command<<color_normal<<"\n";
}

//Inialize program
void init(){
	//Alias variables
	commands["ls"] = "dir /og /c /d /w /4 %LCMD ARGS%";
	commands["clear"] = "cls";
	commands["whereis"] = "where %LCMD ARGS%";
	commands["version"] = "echo LCMD version 4 on && ver";
	
	//Hook signals
	signal (SIGINT, on_signal);
	enable_color();
	
	//Set title to 'LCMD'
	system("title LCMD");
}

//Handles argv instructions
void handle_argv(int argc, char** argv){
	string arg;
	string goto_cwd = "";
	string prev_arg = "";
	string exe_location =  make_exe_location();
	
	//Start path
	std::stringstream ss;
	ss << getenv("PATH");
	ss << ";" << exe_location;
	ss << ";" << exe_location << "\\cmds\\";
	
	for (int i = 0; i < argc; i++){
		arg = argv[i];
		
		if (prev_arg == "-path")
			ss << ";" << arg;
		
		else if (prev_arg == "-cwd")
			goto_cwd = arg;
		
		else if (prev_arg == "-color_cwd")
			color_cwd = "\033[1;"+arg+"m";
		
		else if (prev_arg == "-color_error")
			color_error = "\033[1;"+arg+"m";
		
		else if (prev_arg == "-color_info")
			color_info = "\033[1;"+arg+"m";
		
		else if (prev_arg == "-color_normal")
			color_normal = "\033[1;"+arg+"m";
		
		else if (prev_arg == "-color_prompt")
			color_prompt = "\033[1;"+arg+"m";
		
		else if (arg == "-help" || arg == "-h")
			cout<<color_normal<<"\nCommand arguments:\n\t-path <folder to add to PATH>\n\t-cwd <directory to start in>\n\t-noansicolor\n\t-color <2 hex digits, first for background, then foreground>\n\t-exitkey <Key A-Z>\n\t-color_prompt <ansi color code>\n\t-color_cwd <ansi color code>\n\t-color_normal <ansi color code>\n\t-color_info <ansi color code>\n\t-color_error <ansi color code>\n";
		
		else if (arg == "-noansicolor"){
			color_prompt = "";
			color_cwd	= "";
			color_error	= "";
			color_info	= "";
			color_normal = "";
		}
		else if (prev_arg == "-color"){
			if (arg.size() == 2 && (isxdigit(arg[0])) && (isxdigit(arg[1]))){
				system(("color "+arg).c_str());
			}
		}
		else if (prev_arg == "-exitkey"){
			if (arg.size() == 1){
				exit_key = toupper(arg[0]);
			}
		}
		
		prev_arg = arg;
	}
	
	if (goto_cwd != ""){
		// Change Current Working Directory
		if (SetCurrentDirectory(goto_cwd.c_str()) == 0){
			cerr<<color_error<<"Cant Change current directory to '"<<goto_cwd<<"'"<<color_normal<<"\n";
		}
	}
	
	//End path & add it to env
	ss << '\0';
	std::string env = ss.str();
	//setenv("PATH", env.c_str(), 1);
	//_putenv(("PATH="+env).c_str());
	SetEnvironmentVariable("PATH", env.c_str());
}

//Main loop
int mainloop(){
	char cwd[MAX_PATH];
	string input, cmd, arg, striped_arg;
	
	while (true){
		if (!getcwd(cwd, sizeof(cwd)))
			cwd[0] = 0;
		
		//Ask for command
		cout<<"\n"<<color_cwd<<cwd<<">>:"<<color_prompt;
		ctrl_c_pressed = false;
		getline (cin, input);
		
		//Special case for ^Z as it cause EOF
		if (cin.fail() || cin.eof()){
			Sleep(100); // Give time for sigint
			
			cin.clear();
			
			if (exit_key == 'Z' && !ctrl_c_pressed){
				
				return 0xDECADE;
			}
		}
		
		cout << color_normal;
		
		if (input == "")
			continue;
		
		//Split command and arguments
		if (input.find(" ") == (unsigned int) -1){
			cmd = input;
			arg = "";
			striped_arg = "";
		} else {
			cmd = input.substr(0, input.find(" "));
			arg = input.substr(input.find(" ")+1, input.size());
			striped_arg = arg;
			
			//Strip white spaces and qoutes
			
			if (striped_arg.size()){
				size_t start_pos = 0, end_pos = striped_arg.size();
				while (start_pos < striped_arg.size() && (striped_arg[start_pos] == ' ' || striped_arg[start_pos] == '\r' || striped_arg[start_pos] == '\n' || striped_arg[start_pos] == '"' || striped_arg[start_pos] == '\''))
					start_pos ++;
				while (end_pos >= 0 && (striped_arg[end_pos - 1] == ' ' || striped_arg[end_pos - 1] == '\r' || striped_arg[end_pos - 1] == '\n' || striped_arg[end_pos - 1] == '"' || striped_arg[end_pos - 1] == '\''))
					end_pos--;
				if (start_pos > striped_arg.size() || end_pos < 0 || end_pos == start_pos)
					striped_arg = "";
				else
					striped_arg = striped_arg.substr(start_pos, end_pos - start_pos);
			}
		}
		
		if (cmd == "exit" || cmd[0] == (exit_key - '@')){
			//Exit the app
			try{
				return stoi(striped_arg);
			} catch (...){}
			break;
		} else if (cmd == "cd"){
			// Change Current Working Directory
			if (SetCurrentDirectory(striped_arg.c_str()) == 0){
				cerr<<color_error<<"Cant Change current directory to '"<<striped_arg<<"'"<<color_normal<<"\n";
			}
		} else if (cmd == "alias" || cmd == "aka"){
			//Give a command an alias
			
			cmd = arg.substr(0, arg.find(" "));
			arg = arg.substr(arg.find(" ")+1, arg.size());
			if (arg == ""){
				cout<<color_normal<<"Used to create alias of commands\n\t%LCMD ARGS% is replaced by the arguments passed to the alias\n\nCurrent aliases:\n";
				
				for (map<string, string>::iterator it = commands.begin(); it != commands.end(); it++)
				{
					cout << "\t" << color_prompt << it->first << color_normal << ": " << color_info << it->second << color_normal << "\n";
				}
			}
			else
				commands[cmd] = arg;
		} else if (commands.find(cmd) != commands.end()){
			//Find alias
			string command = commands[cmd];
			
			//Replace arguments
			int pos = command.find("%LCMD ARGS%");
			if (pos != -1)
				command.replace(pos, 11, arg);
			
			//Run it
			run_command(command, cwd);
		} else {
			//Run input
			run_command(input, cwd);
		}
	}
	return 0;
}

int main(int argc, char** argv){
	init();
	handle_argv(argc, argv);
	
	//Display header
	cout<<color_normal<<"LCMD version 4\n\tUse 'alias <name> <command>' it create alias of commands\n\tType 'exit' or '^"<< exit_key <<"' to exit\n";
	
	//Mainloop
	int ret = mainloop();
	
	//Reset colors
	cout<<color_normal;
	
	return ret;
}
