#pragma once

#include <ostream>

#include "types.h"

void printLocations(std::ostream& out, const std::set<std::string>& locations);
void printIndex(std::ostream& out, const ClicIndex& index);
