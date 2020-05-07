// hw1p1.cpp
// CSCE 463-500
// Luke Grammer
// 9/03/19

#include "pch.h"

/*
* #define _CRTDBG_MAP_ALLOC  
* #include <stdlib.h>  
* #include <crtdbg.h> // Libraries to check for memory leaks
*/

#pragma comment(lib, "ws2_32.lib")

int main(int argc, char** argv)
{
	// Debug flag to check for memory leaks
	//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF); 
	
	if (argc < 2 || argc > 2)
	{   // Check to make sure supplied arguments are valid
		printf("Error: ");
		(argc < 2) ? printf("Too few arguments!") : printf("Too many arguments!");
		printf("\nUsage: hw1p1.exe scheme://host[:port][/path][?query][#fragment]");
		return 1;
	}

	ParsedURL url = ParsedURL::parseURL(argv[1]);
	if (!url.valid)
		return(EXIT_FAILURE);

	WebCrawler crawler = WebCrawler(url);
	if (crawler.createConnection() < 0)
		return(EXIT_FAILURE);
	if (crawler.write() < 0)
		return(EXIT_FAILURE);

	char* buffer = nullptr; // Buffer for HTTP response
	size_t buf_size = 0; // Size of buffer

	if (crawler.read(buffer, buf_size) < 0)
		return(EXIT_FAILURE);
	if (crawler.parse(buffer, buf_size) < 0)
		return(EXIT_FAILURE);

	free(buffer); // Clean up and exit
	return(EXIT_SUCCESS);
}
