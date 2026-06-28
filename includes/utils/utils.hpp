#pragma once 

#include <sys/stat.h>
#include <iostream>
#include <dirent.h>
#include <sstream>
#include <map>

bool                                isError(int code);
bool                                isDir(const std::string& path);
bool	                            fileFound(const std::string& path);
bool                                isEncoded(const std::string& path);

std::string                         itos(int n);
std::string                         getReason(int code);
std::string	                        getType(const std::string& path);
std::string                         decodeHexa(const std::string& uri, bool plus);
std::string                         getParentDirectory(const std::string& path);

std::map<std::string, std::string>  init_mime_types();