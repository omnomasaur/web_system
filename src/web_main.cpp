////////////////////////////////////////////////////////////
// 
// This project is a very small executable designed only to
// be used as a subprocess for Cef.  The advantage is that
// it is very small and will therefore load quickly.  
//
// If the program attempting to use this executable for it's
// subprocess is started from a location other than the home
// of this executable, the path to this executable will likely
// be incorrect.  The main process will then fallback to 
// attempting to use copies of itself as subprocesses.
// 
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "WebSystem.h"

////////////////////////////////////////////////////////////
// Entry point
////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
	//Run the process and capture the exit code.
	int exit_code = WebSystem::Main();
	
	//When we are finished running, exit with the captured exit code.
	if (exit_code >= 0)
	{
		return exit_code;
	}

	//It should not be possible to reach this location.

	//This program can also be written simply as:
	//return WebSystem::Main();
}