#include <iostream>
#include <map>
#include <ctime>
#include <sstream>
#include <string>
#include <direct.h>
#include <signal.h>

#include<windows.h>
#define getcwd _getcwd

using namespace std;

// Ansi color codes
const string color_prompt	= "\033[1;93m";
const string color_cwd		= "\033[1;94m";
const string color_error	= "\033[1;91m";
const string color_info		= "\033[1;92m";
const string color_normal	= "\033[1;97m";

// Catch user pressing Contol-c and make sure it doesn't close Lcmd
void on_signal(int s){
	if (s==2)
		//Reset input
		cin.clear();
	
	// Reasign hook
	signal (SIGINT, on_signal);
}

void enable_color(){
	//Enables ansi escape codes to be used for colors
	
	#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
	#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
	#endif // ENABLE_VIRTUAL_TERMINAL_PROCESSING
	
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD dwMode = 0;
	GetConsoleMode(hOut, &dwMode);
	
	dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	SetConsoleMode(hOut, dwMode);
}
string make_exe_location(){
	//Gets the directory of the current executeable
	
	char path[MAX_PATH];
	// gets full path, includeing name
    size_t len = GetModuleFileName(NULL, path, MAX_PATH);
	
	// Remove name
	path[len] = 0;
	for (size_t i = len; i >= 0 && path[i]!='\\'; i--)
		path[i] = 0;
	
	return path;
}
string make_env(string exe_loc){
	//Reconstructs the path variable to include the executeable location
	// and the "cmds" folder
	
	std::stringstream ss;
	ss << "PATH=" << getenv("PATH");
	ss << ";" << exe_loc;
	ss << ";" << exe_loc << "cmds/";
	ss << '\0';
	return ss.str();
}

void run_command(string command, string env, char* cwd){
	//Runs a command with envorments and current working directory
	
	STARTUPINFO info={sizeof(info)};
	PROCESS_INFORMATION processInfo;
	
	//Get duration of command
	int start = std::clock();
	int duration;
	
	if (CreateProcess("C:/windows/system32/cmd.exe", const_cast<char *>(("/c "+command).c_str()), NULL, NULL, FALSE, 0, const_cast<char *>(env.c_str()), cwd, &info, &processInfo)){
		while (1){
			DWORD f;
			GetExitCodeProcess(processInfo.hProcess, &f);
			
			//Wait for it to finish
			if (f!=STILL_ACTIVE &&(WaitForSingleObject(processInfo.hProcess, 0)!=WAIT_TIMEOUT)){
				duration = ( std::clock() - start ) / (double) CLOCKS_PER_SEC;
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


int main(int argc, char** argv){
	char cwd[MAX_PATH];
	string input, cmd, arg, striped_arg;
	int exit_code = 0;
	
	//Get variables
	string exe_loc = make_exe_location();
	string env = make_env(exe_loc);
	std::map<std::string, std::string> commands;
	
	//Basic commands from linux
	commands["ls"] = "dir /og /c /d /w /4 %%";
	commands["clear"] = "cls";
	commands["whereis"] = "where %%";
	commands["version"] = "echo LCMD on && ver";
	
	//Hook
	signal (SIGINT, on_signal);
	enable_color();
	
	system("title LCMD");
	
	//Handle Argv
	if (argc > 2){
		cerr<<color_error<<"To many arguments for LCMD"<<color_normal<<"\n";
	//Allow a starting directory
	} if (argc == 2){
		// Change Current Working Directory
		if (SetCurrentDirectory(argv[1]) == 0){
			cerr<<color_error<<"Cant Change current directory to '"<<argv[1]<<"'"<<color_normal<<"\n";
			return -1;
		}
	}
	
	while (true){
		if (!getcwd(cwd, sizeof(cwd)))
			cwd[0] = 0;
		
		//Ask for command
		cout<<"\n"<<color_cwd<<cwd<<">>:"<<color_prompt;
		getline (cin, input);
		cout << color_normal;
		
		if (input == "")
			continue;
		
		//Split command and arguments
		if (input.find(" ") == -1){
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
		
		if (cmd == "exit"){
			//Exit the app
			try{
				exit_code = stoi(striped_arg);
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
			
			commands[cmd] = arg;
		} else if (commands.find(cmd) != commands.end()){
			//Find alias
			string command = commands[cmd];
			
			//Replace arguments
			int pos = command.find("%%");
			if (pos != -1)
				command.replace(pos, 2, arg);
			
			//Run it
			run_command(command, env, cwd);
		} else {
			//Run input
			run_command(input, env, cwd);
		}
	}
	
	return exit_code;
}