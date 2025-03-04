#include "../../includes/Response/Response.hpp"

int	Response::redirection(Request & Request, int flag)
{
	fill_initial_line(Request.http_version, "301");
	if (flag == 0)
		fill_header("Location", Request._location.get_element("return"));
	else
		fill_header("Location", Request.path);
	fill_header("Content-Type", "text/html");
	fill_header("Content-Length","0");
	_headers += "\r\n";
	Request._buffer = _initial_line + _headers;
	send(Request.socket, Request._buffer.c_str(), Request._buffer.size(), 0);
	return 0;
}

int	Response::get_file(Request & Request, const string &file)
{
	std::ifstream  file_stream(file);
	if (!file_stream)
		return simple_Response(Request, "404");
	string	buffer;
	buffer.resize(2000);
	int i = 0;
	int j = 0;
	if (!Request._first)
	{
		fill_initial_line(Request.http_version, "200");
		if (file.find_last_of(".") == string::npos)
			fill_header("Content-Type", get_content_type(""));
		else
			fill_header("Content-Type", get_content_type(file.substr(file.find_last_of("."))));
		file_stream.seekg(0, file_stream.end);
		fill_header("Content-Length", std::to_string(file_stream.tellg()));
		_headers += "\r\n";
		Request._file_size = file_stream.tellg();
		file_stream.seekg(0, file_stream.beg);
		file_stream.close();
		Request._buffer = _initial_line + _headers;
		Request._first = 1;
		Request._fd = open(file.c_str(), O_RDONLY);
		Request._size_to_write = Request._buffer.size() + Request._file_size;
		i = Request._buffer.size();
	}
	if (!Request._buffer_state)
	{
		j = read(Request._fd, &buffer[0], 2000);
		if (j < 0)
			return simple_Response(Request, "404");
		Request._buffer += buffer;
		buffer.clear();
		i += j;
		if(!j)
			close(Request._fd);
	}
	int a;
	a = send(Request.socket,&Request._buffer[0], i, 0);
	if (a < 0)
	{
		close(Request._fd);
		return 0;
	}
	Request._amount_written += a;
	if (a != (int)Request._buffer.size())
	{
		Request._buffer = Request._buffer.substr(a);
		Request._buffer_state = 1;
	}
	else
	{
		Request._buffer.clear();
		Request._buffer_state = 0;
	}
	if (Request._amount_written == Request._size_to_write)
	{
		close(Request._fd);
		return 0;
	}
	return 1;
}

int	Response::auto_index(Request &request, string &path)
{
	DIR *dir  = opendir(path.c_str());
	struct dirent *ent;
	std::ostringstream ss;
	ss << "<!DOCTYPE html>\n";
	ss << "<html>\n";
	ss << "<head>\n";
	ss << "  <title>Index of " << request.path << "</title>\n";
	ss << "</head>\n";
	ss << "<body>\n";
	ss << "  <h1>Index of " << request.path << "</h1>\n";
	ss << "  <ul>\n";
	while ((ent = readdir(dir)) != NULL) {
		std::string filename(ent->d_name);
		if (filename != "." && filename != "..") {
			ss << "	<li><a href=\"" << filename << "\">" << filename << "</a></li>\n";
		}
	}
	ss << "  </ul>\n";
	ss << "</body>\n";
	ss << "</html>\n";
	fill_initial_line(request.http_version, "200");
	fill_header("Content-Type", "text/html");
	fill_header("Content-Length", std::to_string(ss.str().length()));
	_headers += "\r\n";
	request._buffer = _initial_line + _headers + ss.str() + "\r\n";
	send(request.socket, request._buffer.c_str(), request._buffer.size(), 0);
	closedir(dir);
	return 0;
}

int	Response::Get_method(Request & Request)
{
	string fullpath = Request.fullpath;
	DIR *dir = opendir(fullpath.c_str());
	string exten;
	string temp = Request._location.get_element("index");
	if (temp != "")
	{
		size_t pos = temp.find_last_of(".");
		if (pos != string::npos)
			exten = temp.substr(pos);
	}
	// adding CGI if we found dir
	if (dir)
	{
		closedir(dir);
		if (Request.path.back() != '/')
		{
			Request.path += '/';
			return redirection(Request, 1);
		}
		else if (Request._location.get_element("index") != "")
		{
			if (Request._location.get_CgiFlag() && (exten == ".php" || exten == ".py"))
				return handle_cgi(Request, fullpath + Request._location.get_element("index"), exten);
			return get_file(Request, fullpath + Request._location.get_element("index"));
		}
			
		else if (Request._location.get_element("auto_index") == "on")
			return auto_index(Request, fullpath);
		else if (Request._location.get_element("auto_index") == "off")
			return simple_Response(Request, "403");
	}
	int fd = open(fullpath.c_str(), O_RDONLY);
	if (fd >= 0)
	{
		string extention;
		if (fullpath.find_last_of(".") != std::string::npos)
			extention = fullpath.substr(fullpath.find_last_of("."));
		else
			extention = "";
		close(fd);
		if (Request._location.get_CgiFlag() && (extention == ".php" || extention == ".py"))
			return handle_cgi(Request, fullpath, extention);
		return get_file(Request, fullpath);
	}
	return simple_Response(Request, "404");
}