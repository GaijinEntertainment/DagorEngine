#include <EASTL/string.h>

eastl::string extract_file_name(const eastl::string& path)
{
	size_t last_slash = path.find_last_of('/');

	if (last_slash == eastl::string::npos)
		last_slash = path.find_last_of('\\');

	if (last_slash == eastl::string::npos)
		return path;

	return path.substr(last_slash + 1);
}


eastl::string extract_file_extension(const eastl::string& path)
{
	size_t last_dot = path.find_last_of('.');

	if (last_dot == eastl::string::npos)
		return "";

	return path.substr(last_dot);
}
