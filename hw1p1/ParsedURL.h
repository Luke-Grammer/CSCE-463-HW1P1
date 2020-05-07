// ParsedURL.h
// CSCE 463-500
// Luke Grammer
// 9/03/19

#pragma once

const unsigned MAX_PORT = 65535;
const unsigned MIN_PORT = 1;
const unsigned DEFAULT_PORT = 80;

struct ParsedURL
{   // Basic struct for containing a URL
	std::string scheme, host, query, path, request;
	int port;
	bool valid; // To determine is a given URL is properly formed

	ParsedURL() : scheme{ "" }, host{ "" }, port{ DEFAULT_PORT }, query{ "" }, path{ "/" }, request{ "" }, valid{ false } {} // Basic default constructor

	static ParsedURL parseURL(std::string url); // Parses a given URL as a string and returns a URL struct. 
};