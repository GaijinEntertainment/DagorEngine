/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#pragma once

#include "defines.h"
#include <omm.h>
#include <shared/assert.h>

#include <cstdio>

namespace omm
{
	static const char* ToString(ommSpecialIndex specialIndex)
	{
		switch (specialIndex)
		{
		case ommSpecialIndex_FullyTransparent:			return "Fully Transparent";
		case ommSpecialIndex_FullyOpaque:				return "Fully Opaque";
		case ommSpecialIndex_FullyUnknownTransparent:	return "Fully Unknown Transparent";
		case ommSpecialIndex_FullyUnknownOpaque:		return "Fully Unknown Opaque";
		default: return "Unknown State";
		}
	}

	class Logger
	{
	public:

		Logger() : m_log(ommMessageInterfaceDefault()) { }

		explicit Logger(ommMessageInterface log) :m_log(log) { }

		bool HasLogger() const
		{
			return m_log.messageCallback != nullptr;
		}

		void Info(const char* msg) const
		{
			_Log(ommMessageSeverity_Info, msg);
		}

		template<int N = 256, typename... Args>
		void Infof(const char* format, Args&&... args) const
		{
			_Logf<N>(ommMessageSeverity_Info, format, std::forward<Args>(args)...);
		}

		void PerfWarn(const char* msg) const
		{
			_Log(ommMessageSeverity_PerfWarning, msg);
		}

		template<int N = 256, typename... Args>
		void PerfWarnf(const char* format, Args&&... args) const
		{
			_Logf<N>(ommMessageSeverity_PerfWarning, format, std::forward<Args>(args)...);
		}

		void Error(const char* msg) const
		{
			_Log(ommMessageSeverity_Error, msg);
		}

		template<int N = 256, typename... Args>
		void Errorf(const char* format, Args&&... args) const
		{
			_Logf<N>(ommMessageSeverity_Error, format, std::forward<Args>(args)...);
		}

		void Fatal(const char* msg) const
		{
			_Log(ommMessageSeverity_Fatal, msg);
		}

		// Helper versions that return specific error codes

		[[nodiscard]] ommResult InvalidArg(const char* msg) const
		{
			_Log(ommMessageSeverity_Fatal, msg);
			return ommResult_INVALID_ARGUMENT;
		}

		template<int N = 256, typename... Args>
		[[nodiscard]] ommResult InvalidArgf(const char* format, Args&&... args) const
		{
			RETURN_STATUS_IF_FAILED(_Logf<N>(ommMessageSeverity_Fatal, format, std::forward<Args>(args)...));
			return ommResult_INVALID_ARGUMENT;
		}

		[[nodiscard]] ommResult NotImplemented(const char* msg) const
		{
			_Log(ommMessageSeverity_Fatal, msg);
			return ommResult_NOT_IMPLEMENTED;
		}

		template<int N = 256, typename... Args>
		[[nodiscard]] ommResult ErrorArgf(const char* format, Args&&... args) const
		{
			RETURN_STATUS_IF_FAILED(_Logf<N>(ommMessageSeverity_Error, format, std::forward<Args>(args)...));
			return ommResult_FAILURE;
		}

	private:

		void _Log(ommMessageSeverity severity, const char* msg) const
		{
			if (m_log.messageCallback)
			{
				(*m_log.messageCallback)(severity, msg, m_log.userArg);
			}
		}

		template<int N, typename... Args>
		ommResult _Logf(ommMessageSeverity severity, const char* format, Args&&... args) const
		{
			if (m_log.messageCallback)
			{
				char buffer[N];
				int result = std::snprintf(buffer, sizeof(buffer), format, std::forward<Args>(args)...);
				if (result < 0) {
					return ommResult_FAILURE; // sprintf_s failed for some reason
				}

				(*m_log.messageCallback)(severity, buffer, m_log.userArg);
			}
			return ommResult_SUCCESS;
		}

	private:
		ommMessageInterface m_log;
	};


} // namespace omm
