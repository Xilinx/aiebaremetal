/**<!--
 * Copyright (C) 2026 Advanced Micro Devices, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"). You may
 * not use this file except in compliance with the License. A copy of the
 * License is located at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 * -->
 */
#ifndef AEG_LOG_H
#define AEG_LOG_H

#ifdef __cplusplus
#include <cstdarg>
#include <cstdio>

namespace aeg_log_inline {

inline void aeg_log_write_stderr(const char *const fmt, ...)
{
	std::va_list ap{};

	va_start(ap, fmt);
	(void)std::fputs("[AEG_ERROR] ", stderr);
	(void)std::vfprintf(stderr, fmt, ap);
	(void)std::fputs("\n", stderr);
	va_end(ap);
}

inline void aeg_log_write_stdout(const char *const fmt, ...)
{
	std::va_list ap{};

	va_start(ap, fmt);
	(void)std::fputs("[AEG_LOG] ", stdout);
	(void)std::vfprintf(stdout, fmt, ap);
	(void)std::fputs("\n", stdout);
	va_end(ap);
}

inline void aeg_log_write_noop(const char *const, ...) {}
} // namespace aeg_log_inline
#endif /* __cplusplus */

#ifndef AEG_ERROR
#ifdef __cplusplus
#if defined(AEG_DEBUG) || defined(DEBUG) || defined(DEBUG_RUNTIME) || !defined(NDEBUG)
#define AEG_ERROR ::aeg_log_inline::aeg_log_write_stderr
#else
#define AEG_ERROR ::aeg_log_inline::aeg_log_write_noop
#endif
#else /* C */
#include <stdarg.h>
#include <stdio.h>
static inline void aeg_log_write_stderr(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	(void)fputs("[AEG_ERROR] ", stderr);
	(void)vfprintf(stderr, fmt, ap);
	(void)fputc('\n', stderr);
	va_end(ap);
}
static inline void aeg_log_write_noop(const char *fmt, ...)
{
	(void)fmt;
}
#if defined(AEG_DEBUG) || defined(DEBUG) || defined(DEBUG_RUNTIME) || !defined(NDEBUG)
#define AEG_ERROR aeg_log_write_stderr
#else
#define AEG_ERROR aeg_log_write_noop
#endif
#endif /* __cplusplus */
#endif /* !AEG_ERROR */

#ifndef AEG_LOG
#ifdef __cplusplus
#if defined(AEG_DEBUG) || defined(DEBUG) || defined(DEBUG_RUNTIME) || !defined(NDEBUG)
#define AEG_LOG ::aeg_log_inline::aeg_log_write_stdout
#else
#define AEG_LOG ::aeg_log_inline::aeg_log_write_noop
#endif
#else /* C */
#include <stdarg.h>
#include <stdio.h>
static inline void aeg_log_write_stdout(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	(void)fputs("[AEG_LOG] ", stdout);
	(void)vfprintf(stdout, fmt, ap);
	(void)fputc('\n', stdout);
	va_end(ap);
}
#if defined(AEG_DEBUG) || defined(DEBUG) || defined(DEBUG_RUNTIME) || !defined(NDEBUG)
#define AEG_LOG aeg_log_write_stdout
#else
#define AEG_LOG aeg_log_write_noop
#endif
#endif /* __cplusplus */
#endif /* !AEG_LOG */

#endif /* AEG_LOG_H */
