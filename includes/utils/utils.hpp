#pragma once 

#include <sys/stat.h>
#include <iostream>
#include <dirent.h>
#include <sstream>
#include <map>

bool                                isDir(const std::string& path);
bool	                            fileFound(const std::string& path);

std::string                         itos(int n);
std::string                         getReason(int code);
std::string	                        getType(const std::string& path);
std::string                         getParentDirectory(const std::string& path);

std::map<std::string, std::string>  init_mime_types();