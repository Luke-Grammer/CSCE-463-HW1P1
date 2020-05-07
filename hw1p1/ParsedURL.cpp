// ParsedURL.cpp
// CSCE 463-500
// Luke Grammer
// 9/03/19

#include "pch.h"

ParsedURL ParsedURL::parseURL(std::string url)
{   // Parses a given URL as a string and returns a URL struct. 
	//If input string is malformed, the 'valid' property on the returned struct will be false
	printf("URL: %s\n", url.c_str());

	printf("\tParsing URL... ");

	ParsedURL return_val = ParsedURL();

	if (url.empty())
	{   // Check if input string is empty
		printf("failed with empty URL\n");
		return return_val;
	}

	size_t scheme_loc = url.find("://");
	if (scheme_loc != std::string::npos)
	{   // Make sure scheme is found
		return_val.scheme = url.substr(0, scheme_loc);
		if (return_val.scheme != "http")
		{   // Only http supported (for now?)
			printf("failed with invalid scheme\n");
			return return_val;
		}

		// Strip scheme from input
		url = url.substr(scheme_loc + 3, url.length() - (scheme_loc + 3));
	}
	else
	{   // Scheme not found in input, return
		printf("failed with invalid scheme\n");
		return return_val;
	}

	size_t fragment_loc = url.find("#");
	if (fragment_loc != std::string::npos)
	{   // Strip fragment from input
		url = url.substr(0, fragment_loc);
	}

	size_t query_loc = url.find("?");
	if (query_loc != std::string::npos)
	{   // Query is found
		return_val.query = url.substr(query_loc);

		// Extract query from input
		url = url.substr(0, query_loc);
	}

	size_t path_loc = url.find("/");
	if (path_loc != std::string::npos)
	{   // File path is found 
		return_val.path = url.substr(path_loc);

		// Extract file path from input
		url = url.substr(0, path_loc);
	}

	return_val.request = return_val.path + return_val.query;

	size_t port_loc = url.find(":");
	if (port_loc != std::string::npos && port_loc + 1 < url.length())
	{   // Port found, put into string
		std::string port_string = url.substr(port_loc + 1);
		try
		{
			// Attempt to convert port to an integer
			return_val.port = stoi(port_string);
			// Check range of port
			if (return_val.port <= MIN_PORT || return_val.port > MAX_PORT)
				throw std::out_of_range("");
		}
		catch (...)
		{   // Specified port was invalid, return
			printf("failed with invalid port\n");
			return return_val;
		}

		// Strip port from input
		url = url.substr(0, port_loc);
	}
	else if (port_loc + 1 >= url.length())
	{   // ':' was found without a port specified, return
		printf("failed with invalid port\n");
		return return_val;
	}

	if (url.length() > 0) // Check remaining input for host portion
	{
		return_val.host = url;
	}
	else // If there is no host specified, return
	{
		printf("failed with invalid host\n");
		return return_val;
	}

	return_val.valid = true; // If no failures were encountered, URL is assumed valid
	printf("host %s, port %d, request %s\n", return_val.host.c_str(), return_val.port, return_val.request.c_str());
	return return_val;
}