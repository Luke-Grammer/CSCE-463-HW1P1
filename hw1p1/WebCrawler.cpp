// WebCrawler.cpp
// CSCE 463-500
// Luke Grammer
// 9/03/19

#include "pch.h"

WebCrawler::WebCrawler(ParsedURL _url) : remote { nullptr }, server{ NULL }
{   // Basic constructor initializes winsock and opens a TCP socket
	WSADATA wsa_data;
	WORD w_ver_requested;
	
	if (!_url.valid)
	{
		printf("URL provided to WebCrawler constructor has not been successfully parsed\n");
		exit(EXIT_FAILURE);
	}
	url = _url;

	//Initialize WinSock
	w_ver_requested = MAKEWORD(2, 2);
	if (WSAStartup(w_ver_requested, &wsa_data) != 0) {
		printf("\tWSAStartup error %d\n", WSAGetLastError());
		WSACleanup();
		exit(EXIT_FAILURE);
	}

	// Open a TCP socket
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET)
	{
		printf("\tsocket() generated error %d\n", WSAGetLastError());
		WSACleanup();
		exit(EXIT_FAILURE);
	}
}

WebCrawler::~WebCrawler() 
{   // Clean up
	WSACleanup();
	closesocket(sock);
}

int WebCrawler::createConnection()
{   // Resolves DNS and creates a TCP connection to a server given a valid, parsed URL 
	char* host;
	struct in_addr addr;
	DWORD IP;

	printf("\tDoing DNS... ");
	
	host = (char*) malloc(MAX_HOST_LEN);
	if (host == NULL)
	{
		printf("malloc failure for hostname\n");
		return -1;
	}

	// Need to put the hostname in a C string
	strcpy_s(host, MAX_HOST_LEN, url.host.c_str()); 

	start_time = std::chrono::high_resolution_clock::now();
    
	// First assume that the hostname is an IP address
	IP = inet_addr(host); 
	if (IP == INADDR_NONE)
	{   // Host is not a valid IP, do a DNS lookup
		if ((remote = gethostbyname(host)) == NULL) 
		{
			printf("failed with %d on gethostbyname\n", WSAGetLastError());
			free(host);
			return -1;
		}
		else 
		{   // Take the first IP address and copy into sin_addr, stop timer and print
			stop_time = std::chrono::high_resolution_clock::now();
			addr.s_addr = *(u_long*)remote->h_addr;
			memcpy((char*) &(server.sin_addr), remote->h_addr, remote->h_length);
			printf("done in %" PRIu64 " ms, found %s\n", 
				   std::chrono::duration_cast<std::chrono::milliseconds>
				   (stop_time - start_time).count(), inet_ntoa(addr));
		}
	}
	else
	{
		// Host is a valid IP, directly drop its binary version into sin_addr, stop timer and print
		stop_time = std::chrono::high_resolution_clock::now();
		printf("done in %" PRIu64 " ms, found %s\n", 
			   std::chrono::duration_cast<std::chrono::milliseconds>
			   (stop_time - start_time).count(), host);
		server.sin_addr.S_un.S_addr = IP;
	}

	// Set up the port # and protocol type
	server.sin_family = AF_INET;

    // Host-to-network flips the byte order
	server.sin_port = htons(url.port); 

	printf("      * Connecting to page... ");

	// Start timer and connect to the server
	start_time = std::chrono::high_resolution_clock::now();
	if (connect(sock, (struct sockaddr*) & server, sizeof(struct sockaddr_in)) == SOCKET_ERROR)
	{
		printf("failed with %d on connect\n", WSAGetLastError());
		free(host);
		return -1;
	}

	// Stop timer and print result
	stop_time = std::chrono::high_resolution_clock::now();
	printf("done in %" PRIu64 " ms\n", 
		   std::chrono::duration_cast<std::chrono::milliseconds>
		   (stop_time - start_time).count());

	// Clean up and return
	free(host);
	return 0;
}

int WebCrawler::write()
{   // Writes a properly formatted HTTP query to the connected server
	std::string http_request = "GET " + url.request + " HTTP/1.1\r\nUser-agent: " + 
		        AGENT_NAME + "\r\nHost: " + url.host + "\r\nConnection: close\r\n\r\n";

	if (send(sock, http_request.c_str(), (int) strlen(http_request.c_str()), NULL) < 0)
	{
		printf("failed with %d on send\n", WSAGetLastError());
		return -1;
	}

	return 0;
}

int WebCrawler::read(char* &buf, size_t &cur_size)
{   // Receives HTTP response from server and load into initially empty buffer buf
	
	printf("\tLoading... ");

	if (buf != NULL)
	{
		printf("read failed because buffer is already allocated.\n"
			   "\t\t   to avoid memory leaks with read(), passed buffer should be null.\n");
		return -1;
	}

	if (cur_size != 0)
	{
		printf("read failed because the passed buffer size is nonzero.\n");
		return -1;
	}

	buf = (char*) malloc(INITIAL_BUF_SIZE);
	if (buf == NULL)
	{
		printf("malloc failed for buffer");
		return -1;
	}

	size_t allocated_size = INITIAL_BUF_SIZE;
	
	// Set timeout value
	struct timeval timeout; 
	timeout.tv_sec = TIMEOUT_SECONDS; 
	timeout.tv_usec = 0;

	int ret = 0;
	fd_set fd;
	FD_ZERO(&fd);

	// Start timer
	start_time = std::chrono::high_resolution_clock::now();
	while (true)
	{
		FD_SET(sock, &fd);
		// Wait to see if socket has any data
		ret = select((int) sock + 1, &fd, NULL, NULL, &timeout);
		if (ret > 0)
		{
			// New data available; now read the next segment
			int bytes = recv(sock, buf + cur_size, (int) (allocated_size - cur_size), NULL);

			if (bytes < 0)
			{
				printf("failed with %d on recv\n", WSAGetLastError());
				free(buf);
				return -1;
			}

			// Advance current position by number of bytes read
			cur_size += bytes; 
			
			if (allocated_size - cur_size < BUF_SIZE_THRESHOLD)
			{   // Buffer needs to be expanded
				
				if (2 * allocated_size < allocated_size) 
				{   // Make sure allocated_size will not overflow
					printf("failed with buffer overflow\n");
					free(buf);
					return -1;
				}

			    // Expand memory for buffer, making sure the expansion succeeds
				char* temp = (char*) realloc(buf, 2 * allocated_size); 
				if (temp == NULL)
				{
					printf("realloc failed for buffer\n");
					free(buf);
					return -1;
				}

                // Double allocated size with each expansion (higher overhead but faster)
				allocated_size *= 2; 
				buf = temp;
			}
			// Connection closed
			if (bytes == 0) 
			{
				if (buf == NULL)
				{
					printf("nothing written to buffer\n");
					return -1;
				}

				char* response_pos = strstr(buf, "HTTP/");
				if (response_pos == NULL)
				{   // Response is not found, clean up and return
					printf("failed with non-HTTP header\n");
					free(buf);
					return -1;
				}

				stop_time = std::chrono::high_resolution_clock::now();
				/* Null-terminate buffer
				 *
				 * Warning C6386 due to indexing by cur_size, but buffer overflow is not possible because 
				 * allocated_size is strictly > cur_size while BUF_SIZE_THRESHOLD > 0 
				 */
				buf[cur_size] = '\0'; 

				printf("done in %" PRIu64 " ms with %" PRIu64 " bytes\n", 
					   std::chrono::duration_cast<std::chrono::milliseconds>
					   (stop_time - start_time).count(), cur_size);
				return 0;
			}
		}
		else if (ret == 0) 
		{   // Socket timed out
			stop_time = std::chrono::high_resolution_clock::now();
			printf("socket timed out in %.2f s\n", 
				   std::chrono::duration_cast<std::chrono::milliseconds>
				   (stop_time - start_time).count() / 1000.0);
			free(buf);
			return -1;
		}
		else
		{
			printf("failed with %d on select\n", WSAGetLastError());
			free(buf);
			return -1;
		}
	}
}

int WebCrawler::parse(char* buf, size_t size)
{   // Parses HTTP response and finds number of links and HTTP status code and header
	int num_links = -1; // To hold number of links in HTTP response
	int response = -1;  // To hold HTTP response code
	char* base_url;   
	std::string base_url_string = url.scheme + "://" + url.host;
	HTMLParserBase parser = HTMLParserBase();

	base_url = (char*)malloc(MAX_URL_LEN); 
	if (base_url == NULL)
	{
		printf("malloc failure for base URL\n");
		return -1;
	}

	// Create C-style string with base URL
	strcpy_s(base_url, MAX_URL_LEN, base_url_string.c_str()); 

	printf("\tVerifying header... ");

	// Get response code from HTTP Header
	char* response_pos = strstr(buf, "HTTP/");
	if (response_pos != NULL) 
	{   // If the response is found
		if (sscanf_s(response_pos, "%*s %d", &response) <= 0) 
		{   // If the response code could not be extracted, clean up and return
			printf("failed with non-HTTP header\n");
			free(base_url);
			return -1;
		}
	}
	else
	{   // If the response is not found, clean up and return
		printf("failed with non-HTTP header\n");
		free(base_url);
		return -1;
	}
	printf("status code %d\n", response);

	if (response > 199 && response < 300)
	{
		printf("      + Parsing page... ");

		// Start timing the parse
		start_time = std::chrono::high_resolution_clock::now(); 

		// Get number of links from response
		char* link_buffer = parser.Parse(buf, (int)size, base_url, (int)strlen(base_url), &num_links);
		if (num_links < 0)
		{
			printf("HTML parsing error\n");
			free(base_url);
			return -1;
		}

		// Stop timer and print information
		stop_time = std::chrono::high_resolution_clock::now();
		printf("done in %" PRIu64 " ms with %d links\n", std::chrono::duration_cast<std::chrono::milliseconds>(stop_time - start_time).count(), num_links);
	}

	printf("___________________________________________________________________________________\n");

	// Extract and print HTTP header
	char* header_end = strstr(buf, "\r\n\r\n");
	if (header_end == NULL)
	{
		printf("unexpected error printing HTTP header\n");
		free(base_url);
		return -1;
	}

	*header_end = '\0';
	printf("%s\r\n", buf);

	// Clean up allocated memory
	free(base_url); 
	return 0;
}