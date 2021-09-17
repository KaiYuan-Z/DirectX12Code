#pragma once

#pragma warning(disable:4201) // nonstandard extension used : nameless struct/union
#pragma warning(disable:4328) // nonstandard extension used : class rvalue used as lvalue
#pragma warning(disable:4324) // structure was padded due to __declspec(align())

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <atlbase.h>

#include <stdlib.h>
#include <sstream>
#include <iomanip>

#include <list>
#include <string>
#include <shellapi.h>
#include <unordered_map>
#include <assert.h>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdarg>
#include <vector>
#include <memory>
#include <exception>
#include <map>
#include <set>
#include <queue>

#include <wrl.h>
#include <ppltasks.h>
#include <comdef.h>