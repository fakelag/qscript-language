#pragma once

#ifdef QS_MEMLEAK_TEST
#define QS_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
#include <stdlib.h>
#include <crtdbg.h>
#else
#define QS_NEW new
#endif

#include <string>
#include <vector>
#include <algorithm>
#include <functional>
#include <exception>
#include <regex>
#include <map>
#include <unordered_map>
#include <iostream>
#include <iomanip>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <type_traits>
#include <fstream>
#include <cassert>

#include "QScript.h"
#include "Exception.h"
