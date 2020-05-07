// WebCrawler.h
// CSCE 463-500
// Luke Grammer
// 9/03/19

#pragma once

const std::string AGENT_NAME = "CPPWebCrawler/1.1";
const unsigned int INITIAL_BUF_SIZE = 1024 * 8; // initial buffer size is 8KB
const unsigned int BUF_SIZE_THRESHOLD = 1024; // Size threshold for increasing buffer capacity
const unsigned int TIMEOUT_SECONDS = 10; // Default timeout length

class WebCrawler
{
	ParsedURL url;
	SOCKET sock;

	struct hostent* remote; // structure used in DNS lookups
	struct sockaddr_in server; // structure for connecting to server
	std::chrono::time_point<std::chrono::high_resolution_clock> start_time, stop_time; // For timers

public:
	WebCrawler(ParsedURL _url); // Basic constructor initializes winsock and opens a TCP socket

	~WebCrawler(); // Destructor cleans up winsock and closes socket

	int createConnection(); // Resolves DNS and creates a TCP connection to a server given a valid, parsed URL 

	int write(); // Writes a properly formatted HTTP query to the connected server

	int read(char* &buf, size_t &cur_size); // Receives HTTP response from server

	int parse(char* buf, size_t size); // Parses HTTP response and finds number of links and HTTP status code and header
};

