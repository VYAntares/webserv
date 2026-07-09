#pragma once

#include <sys/stat.h>
#include <iostream>
#include <dirent.h>
#include <sstream>
#include <vector>
#include <map>

#include "../config/ConfigStruct.hpp"

// longest-prefix match de l'URI contre les locations du serveur
// (partagé entre Router et HttpParser pour la limite de body par location)
const Location*                     findLocation(const std::string& uri, const Server& server);

bool                                isError(int code);
bool                                isDir(const std::string& path);
bool	                            fileFound(const std::string& path);
bool                                isEncoded(const std::string& path);

std::string                         itos(int n);
std::string                         getReason(int code);
std::string                         normalizePath(const std::string& p, const std::string& root);
std::string	                        getType(const std::string& path);
std::string                         decodeHexa(const std::string& uri, bool plus);
std::string                         getParentDirectory(const std::string& path);

std::map<std::string, std::string>  init_mime_types();