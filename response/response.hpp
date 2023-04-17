#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include "../config/config.hpp"

class response
{
private:
	string		_initial_line;
	string		_headers;
	string		_body;
	Server		_server;
	Location	_location;
	mapstring	_code_map;
	mapstring	_extention_map;
public:
	string		get_content_type(string extention);
	string		get_extention(string content_type);
	string		&get_initial_line();
	string		&get_headers();
	string		error_page_builder(string code);
	void		fill_initial_line(const string version, const string error_code);
	void		fill_header(const string header, const string value);
	void		reset_values();
	void		_codes();
	int			get_file(Request & request, const string &file);
	int			simple_response(Request &Request, string code);
	int			redirection(Request & request, int flag);
	int			auto_index(Request &request, string &path);
	void		_extentions();
	int			Get_method(Request & Request);
	int 		Delete_method(Request &Request);
	int			Create_response(Request & Request, string code);
};

#endif