#pragma once

#include <cstdlib>
#include <spdlog/spdlog.h>

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Log

#define TRACE SPDLOG_TRACE
#define DEBUG SPDLOG_DEBUG
#define INFO SPDLOG_INFO
#define WARN SPDLOG_WARN
#define ERROR SPDLOG_ERROR
#define CRITICAL SPDLOG_CRITICAL
#define ABORT_MSG(...) do { CRITICAL(__VA_ARGS__); std::abort(); } while (0)
#define ASSERT_MSG(expr, ...) do { if (expr) { } else { ABORT_MSG(__VA_ARGS__); } } while (0)
#define ASSERT(expr) ASSERT_MSG(expr, "Assertion failed: ({})", #expr)
#define ASSERT_GET(expr) [&]{ auto ret = (expr); ASSERT_MSG(ret, "Assertion failed: ({})", #expr); return ret; }()
#define DBG(expr) [&]{ auto ret = (expr); DEBUG("({}) = {{{}}}", #expr, ret); return ret; }()

