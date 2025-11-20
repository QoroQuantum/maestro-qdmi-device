/*------------------------------------------------------------------------------
Copyright 2024 Munich Quantum Software Stack Project

Licensed under the Apache License, Version 2.0 with LLVM Exceptions (the
"License"); you may not use this file except in compliance with the License.
You may obtain a copy of the License at

https://github.com/Munich-Quantum-Software-Stack/QDMI/blob/develop/LICENSE

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
License for the specific language governing permissions and limitations under
the License.

SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
------------------------------------------------------------------------------*/

/** @file
 * @brief A template of a device implementation in C++.
 * @details In the end, all QDMI_ERROR_NOTIMPLEMENTED return codes should be
 * replaced by proper return codes.
 * For the documentation of the functions, see the official documentation.
 */

#include "maestro_qdmi/device.h"

#ifdef __cplusplus
#include <cstddef>
#else
#include <stddef.h>
#endif

 // The following line ignores the unused parameters in the functions.
 // Please remove the following code block after populating the functions.
 // NOLINTBEGIN(misc-unused-parameters,clang-diagnostic-unused-parameter)
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iterator>
#include <limits>
#include <map>
#include <random>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <atomic>
#include <mutex>
#include <thread>


#include "Simulator.hpp"

enum class MAESTRO_QDMI_DEVICE_SESSION_STATUS : uint8_t { ALLOCATED, INITIALIZED };

/**
 * @brief Implementation of the MAESTRO_QDMI_Device_Session structure.
 * @details This structure can, e.g., be used to store a token to access an API.
 */
struct MAESTRO_QDMI_Device_Session_impl_d {
	std::string token;
	MAESTRO_QDMI_DEVICE_SESSION_STATUS status = MAESTRO_QDMI_DEVICE_SESSION_STATUS::ALLOCATED;
	int qubits_num = 64; // some reasonable default value

	int simType = 0; // 0 - aer, 1 - qcsim, 2, - composite aer, 3 - composite qcsim, 2 - gpu, any other value = whatever, auto if available
	int simExecType = 0; // 0 - statevector, 1 - mps, 2 - stabilizer, 3 - tensor network, any other value = whatever, auto if available
	int maxBondDim = 0; // no limit
};

/**
 * @brief Implementation of the MAESTRO_QDMI_Device_Job structure.
 * @details This structure can, e.g., be used to store the job id.
 */
struct MAESTRO_QDMI_Device_Job_impl_d {
	~MAESTRO_QDMI_Device_Job_impl_d()
	{
		delete[] program;
	}

	std::string GetConfigJson() const
	{
		std::string config = "{\"shots\": ";

		config += std::to_string(num_shots);

		if (maxBondDim != 0)
			config += ", \"matrix_product_state_max_bond_dimension\": " + std::to_string(maxBondDim);

		config += "}";

		return config;
	}

	// very dumb json parser, but we know exactly what to expect
	void ParseResults(const std::string& res)
	{
		// we care only about 'counts' for now
		auto pos = res.find("\"counts\":");
		if (pos == std::string::npos)
			return;

		results.clear();

		pos += 9;
		while (pos < res.length() && res[pos] != '{')
			++pos;

		++pos; // skip '{'

		std::string key;
		std::string value;
		while (pos < res.length() && res[pos] != '}')
		{
			if (res[pos] == '"')
			{
				++pos;
				key.clear();
				while (pos < res.length() && res[pos] != '"')
				{
					key += res[pos];
					++pos;
				}
				++pos; // skip '"'
			}
			else if (res[pos] == ':')
			{
				++pos; // skip ':'
				while (pos < res.length() && (res[pos] == ' ' || res[pos] == '\n'))
					++pos;
				value.clear();
				while (pos < res.length() && isdigit(res[pos]))
				{
					value += res[pos];
					++pos;
				}
				if (!key.empty() && !value.empty())
				{
					results[key] = static_cast<size_t>(std::stoull(value));
				}
			}
			else
			{
				++pos;
			}
		}
	}

	MAESTRO_QDMI_Device_Session session = nullptr;
	int id = 0;
	QDMI_Program_Format format = QDMI_PROGRAM_FORMAT_QASM2;
	char* program = nullptr;

	std::atomic<QDMI_Job_Status> status{ QDMI_JOB_STATUS_SUBMITTED };
	size_t num_shots = 1;
	int qubits_num = 64;

	int simType = 0; // 0 - aer, 1 - qcsim, 2, - composite aer, 3 - composite qcsim, 2 - gpu, any other value = whatever, auto if available
	int simExecType = 0; // 0 - statevector, 1 - mps, 2 - stabilizer, 3 - tensor network, any other value = whatever, auto if available
	int maxBondDim = 0; // no limit

	std::unordered_map<std::string, size_t> results;
};

struct MAESTRO_QDMI_Device_State {
	SimpleSimulator simulator;
	bool simulator_initialized{ false };
	std::atomic_bool simulator_busy{ false };
	std::mutex simulator_mutex;

	// TODO: implement a thread that takes jobs from the queue and executes them
	// set the status QDMI_DEVICE_STATUS_IDLE if there are no more jobs to be executed
	// and QDMI_DEVICE_STATUS_BUSY if there are jobs being executed

	void Initialize()
	{
		std::lock_guard<std::mutex> lock(simulator_mutex);
		if (!simulator_initialized)
		{
			simulator.Init("maestro.so");
			simulator_initialized = true;
		}
	}

	void CancelJob(MAESTRO_QDMI_Device_Job job)
	{
		std::lock_guard<std::mutex> lock(simulator_mutex);
		auto it = jobs.find(job->id);
		if (it != jobs.end())
			jobs.erase(it);
			
		job->status = QDMI_JOB_STATUS_CANCELED;

		if (current_job == job)
			current_job = nullptr;
	}

	void RemoveJob(MAESTRO_QDMI_Device_Job job)
	{
		CancelJob(job);

		delete job;
	}

	void AddJob(MAESTRO_QDMI_Device_Job job)
	{
		std::lock_guard<std::mutex> lock(simulator_mutex);
		jobs[job->id] = job;
		job->status = QDMI_JOB_STATUS_QUEUED;
	}

	QDMI_Device_Status status = QDMI_DEVICE_STATUS_OFFLINE;
	std::atomic<int> job_id{ 0 };

	// acts as a queue for submitted jobs, allows cancelling if they are not started yet
	std::map<int, MAESTRO_QDMI_Device_Job> jobs;
	MAESTRO_QDMI_Device_Job current_job{ nullptr };


	std::mt19937 gen{ 80333 }; // Seeded with a constant for reproducibility
	std::uniform_int_distribution<> dis =
		std::uniform_int_distribution<>(0, std::numeric_limits<int>::max());
	std::bernoulli_distribution dis_bin{ 0.5 };
	std::uniform_real_distribution<> dis_real =
		std::uniform_real_distribution<>(-1.0, 1.0);
};

/**
 * @brief Implementation of the MAESTRO_QDMI_Site structure.
 * @details This structure can, e.g., be used to store the site id.
 */
struct MAESTRO_QDMI_Site_impl_d {
	size_t id;
};

/**
 * @brief Implementation of the MAESTRO_QDMI_Operation structure.
 * @details This structure can, e.g., be used to store the operation id.
 */
struct MAESTRO_QDMI_Operation_impl_d {
	std::string name;
};


namespace {
	int MAESTRO_QDMI_device_job_get_results_hist(MAESTRO_QDMI_Device_Job job,
		const QDMI_Job_Result result,
		const size_t size, void* data,
		size_t* size_ret) {
		const auto& hist = job->results;

		if (result == QDMI_JOB_RESULT_HIST_KEYS) {
			const size_t bitstring_size =
				hist.empty() ? 0 : hist.begin()->first.length();

			const size_t req_size = hist.size() * (bitstring_size + 1);
			if (size_ret != nullptr) {
				*size_ret = req_size;
			}
			if (data != nullptr) {
				if (size < req_size) {
					return QDMI_ERROR_INVALIDARGUMENT;
				}
				char* data_ptr = static_cast<char*>(data);
				for (const auto& [bitstring, count] : hist) {
					std::copy(bitstring.begin(), bitstring.end(), data_ptr);
					data_ptr += bitstring.length();
					*data_ptr++ = ',';
				}
				*(data_ptr - 1) = '\0'; // Replace last comma with null terminator
			}
		}
		else {
			// case QDMI_JOB_RESULT_HIST_VALUES:
			const size_t req_size = hist.size() * sizeof(size_t);
			if (size_ret != nullptr) {
				*size_ret = req_size;
			}
			if (data != nullptr) {
				if (size < req_size) {
					return QDMI_ERROR_INVALIDARGUMENT;
				}
				auto* data_ptr = static_cast<size_t*>(data);
				for (const auto& [_, count] : hist) {
					*data_ptr++ = count;
				}
			}
		}
		return QDMI_SUCCESS;
	} /// [DOXYGEN FUNCTION END]

	/**
	 * @brief Static function to maintain the device state.
	 * @return a pointer to the device state.
	 * @note This function is considered private and should not be used outside of
	 * this file. Hence, it is not part of any header file.
	 */
	MAESTRO_QDMI_Device_State* MAESTRO_QDMI_get_device_state() {
		static MAESTRO_QDMI_Device_State device_state;
		return &device_state;
	}

	/**
	 * @brief Local function to read the device status.
	 * @return the current device status.
	 * @note This function is considered private and should not be used outside of
	 * this file. Hence, it is not part of any header file.
	 */
	QDMI_Device_Status MAESTRO_QDMI_get_device_status() {
		auto state = MAESTRO_QDMI_get_device_state();
		std::lock_guard<std::mutex> lock(state->simulator_mutex);
		auto status = state->status;
		return status;
	}

	/**
	 * @brief Local function to set the device status.
	 * @param status the new device status.
	 * @note This function is considered private and should not be used outside of
	 * this file. Hence, it is not part of any header file.
	 */
	void MAESTRO_QDMI_set_device_status(QDMI_Device_Status status) {
		auto state = MAESTRO_QDMI_get_device_state();
		std::lock_guard<std::mutex> lock(state->simulator_mutex);
		state->status = status;
	}

	/**
	 * @brief Generate a random job id.
	 * @return a random job id.
	 * @note This function is considered private and should not be used outside of
	 * this file. Hence, it is not part of any header file.
	 */
	int MAESTRO_QDMI_generate_job_id() {
		auto state = MAESTRO_QDMI_get_device_state();
		return state->job_id++;
	}

	/**
	 * @brief Generate a random bit.
	 * @return a random bit.
	 * @note This function is considered private and should not be used outside of
	 * this file. Hence, it is not part of any header file.
	 */
	bool MAESTRO_QDMI_generate_bit() {
		auto state = MAESTRO_QDMI_get_device_state();
		return state->dis_bin(state->gen);
	}

	/**
	 * @brief Generate a random real number.
	 * @return a random real number.
	 * @note This function is considered private and should not be used outside of
	 * this file. Hence, it is not part of any header file.
	 */
	double MAESTRO_QDMI_generate_real() {
		auto state = MAESTRO_QDMI_get_device_state();
		return state->dis_real(state->gen);
	}


	constexpr MAESTRO_QDMI_Site_impl_d SITE0{ 0 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE1{ 1 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE2{ 2 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE3{ 3 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE4{ 4 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE5{ 5 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE6{ 6 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE7{ 7 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE8{ 8 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE9{ 9 };

	constexpr MAESTRO_QDMI_Site_impl_d SITE10{ 10 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE11{ 11 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE12{ 12 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE13{ 13 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE14{ 14 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE15{ 15 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE16{ 16 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE17{ 17 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE18{ 18 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE19{ 19 };

	constexpr MAESTRO_QDMI_Site_impl_d SITE20{ 20 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE21{ 21 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE22{ 22 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE23{ 23 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE24{ 24 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE25{ 25 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE26{ 26 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE27{ 27 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE28{ 28 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE29{ 29 };

	constexpr MAESTRO_QDMI_Site_impl_d SITE30{ 30 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE31{ 31 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE32{ 32 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE33{ 33 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE34{ 34 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE35{ 35 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE36{ 36 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE37{ 37 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE38{ 38 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE39{ 39 };

	constexpr MAESTRO_QDMI_Site_impl_d SITE40{ 40 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE41{ 41 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE42{ 42 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE43{ 43 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE44{ 44 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE45{ 45 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE46{ 46 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE47{ 47 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE48{ 48 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE49{ 49 };

	constexpr MAESTRO_QDMI_Site_impl_d SITE50{ 50 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE51{ 51 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE52{ 52 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE53{ 53 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE54{ 54 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE55{ 55 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE56{ 56 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE57{ 57 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE58{ 58 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE59{ 59 };

	constexpr MAESTRO_QDMI_Site_impl_d SITE60{ 60 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE61{ 61 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE62{ 62 };
	constexpr MAESTRO_QDMI_Site_impl_d SITE63{ 63 };


	constexpr std::array<const MAESTRO_QDMI_Site_impl_d*, 64> MAESTRO_DEVICE_SITES = {
		&SITE0, &SITE1, &SITE2, &SITE3, &SITE4, &SITE5, &SITE6, &SITE7, &SITE8, &SITE9,
		&SITE10, &SITE11, &SITE12, &SITE13, &SITE14, &SITE15, &SITE16, &SITE17, &SITE18, &SITE19,
		&SITE20, &SITE21, &SITE22, &SITE23, &SITE24, &SITE25, &SITE26, &SITE27, &SITE28, &SITE29,
		&SITE30, &SITE31, &SITE32, &SITE33, &SITE34, &SITE35, &SITE36, &SITE37, &SITE38, &SITE39,
		&SITE40, &SITE41, &SITE42, &SITE43, &SITE44, &SITE45, &SITE46, &SITE47, &SITE48, &SITE49,
		&SITE50, &SITE51, &SITE52, &SITE53, &SITE54, &SITE55, &SITE56, &SITE57, &SITE58, &SITE59,
		&SITE60, &SITE61, &SITE62, &SITE63
	};

} // namespace

// NOLINTBEGIN(bugprone-macro-parentheses)
#define ADD_SINGLE_VALUE_PROPERTY(prop_name, prop_type, prop_value, prop,      \
                                  size, value, size_ret)                       \
  {                                                                            \
    if ((prop) == (prop_name)) {                                               \
      if ((value) != nullptr) {                                                \
        if ((size) < sizeof(prop_type)) {                                      \
          return QDMI_ERROR_INVALIDARGUMENT;                                   \
        }                                                                      \
        *static_cast<prop_type *>(value) = prop_value;                         \
      }                                                                        \
      if ((size_ret) != nullptr) {                                             \
        *size_ret = sizeof(prop_type);                                         \
      }                                                                        \
      return QDMI_SUCCESS;                                                     \
    }                                                                          \
  } /// [DOXYGEN MACRO END]

#define ADD_STRING_PROPERTY(prop_name, prop_value, prop, size, value,          \
                            size_ret)                                          \
  {                                                                            \
    if ((prop) == (prop_name)) {                                               \
      if ((value) != nullptr) {                                                \
        if ((size) < strlen(prop_value) + 1) {                                 \
          return QDMI_ERROR_INVALIDARGUMENT;                                   \
        }                                                                      \
        strncpy(static_cast<char *>(value), prop_value, size);                 \
        static_cast<char *>(value)[size - 1] = '\0';                           \
      }                                                                        \
      if ((size_ret) != nullptr) {                                             \
        *size_ret = strlen(prop_value) + 1;                                    \
      }                                                                        \
      return QDMI_SUCCESS;                                                     \
    }                                                                          \
  } /// [DOXYGEN MACRO END]

#define ADD_LIST_PROPERTY(prop_name, prop_type, prop_values, prop, size,       \
                          value, size_ret)                                     \
  {                                                                            \
    if ((prop) == (prop_name)) {                                               \
      if ((value) != nullptr) {                                                \
        if ((size) < (prop_values).size() * sizeof(prop_type)) {               \
          return QDMI_ERROR_INVALIDARGUMENT;                                   \
        }                                                                      \
        memcpy(static_cast<void *>(value),                                     \
               static_cast<const void *>((prop_values).data()),                \
               (prop_values).size() * sizeof(prop_type));                      \
      }                                                                        \
      if ((size_ret) != nullptr) {                                             \
        *size_ret = (prop_values).size() * sizeof(prop_type);                  \
      }                                                                        \
      return QDMI_SUCCESS;                                                     \
    }                                                                          \
  } /// [DOXYGEN MACRO END]
// NOLINTEND(bugprone-macro-parentheses)


int MAESTRO_QDMI_device_initialize() {
	MAESTRO_QDMI_set_device_status(QDMI_DEVICE_STATUS_IDLE);

	return QDMI_SUCCESS;
} /// [DOXYGEN FUNCTION END]

int MAESTRO_QDMI_device_finalize() {
	MAESTRO_QDMI_set_device_status(QDMI_DEVICE_STATUS_OFFLINE);
	
	return QDMI_SUCCESS;
} /// [DOXYGEN FUNCTION END]

int MAESTRO_QDMI_device_session_alloc(MAESTRO_QDMI_Device_Session* session) {
	if (session == nullptr) {
		return QDMI_ERROR_INVALIDARGUMENT;
	}
	*session = new MAESTRO_QDMI_Device_Session_impl_d();
	
	return QDMI_SUCCESS;
} /// [DOXYGEN FUNCTION END]

int MAESTRO_QDMI_device_session_init(MAESTRO_QDMI_Device_Session session) {
	if (session == nullptr) {
		return QDMI_ERROR_INVALIDARGUMENT;
	}
	switch (MAESTRO_QDMI_get_device_status()) {
	case QDMI_DEVICE_STATUS_ERROR:
	case QDMI_DEVICE_STATUS_OFFLINE:
	case QDMI_DEVICE_STATUS_MAINTENANCE:
		return QDMI_ERROR_FATAL;
	default:
		break;
	}
	//if (session->token.empty()) {
	//	return QDMI_ERROR_PERMISSIONDENIED;
	//}
	session->status = MAESTRO_QDMI_DEVICE_SESSION_STATUS::INITIALIZED;
	
	return QDMI_SUCCESS;
} /// [DOXYGEN FUNCTION END]

void MAESTRO_QDMI_device_session_free(MAESTRO_QDMI_Device_Session session) {
	delete session;
} /// [DOXYGEN FUNCTION END]

int MAESTRO_QDMI_device_session_set_parameter(MAESTRO_QDMI_Device_Session session,
	QDMI_Device_Session_Parameter param,
	const size_t size, const void* value) {
	if (session == nullptr || (value != nullptr && size == 0) ||
		(param >= QDMI_DEVICE_SESSION_PARAMETER_MAX &&
			param != QDMI_DEVICE_SESSION_PARAMETER_CUSTOM1 &&
			param != QDMI_DEVICE_SESSION_PARAMETER_CUSTOM2 &&
			param != QDMI_DEVICE_SESSION_PARAMETER_CUSTOM3 &&
			param != QDMI_DEVICE_SESSION_PARAMETER_CUSTOM4 &&
			param != QDMI_DEVICE_SESSION_PARAMETER_CUSTOM5)) {
		return QDMI_ERROR_INVALIDARGUMENT;
	}
	if (session->status != MAESTRO_QDMI_DEVICE_SESSION_STATUS::ALLOCATED) {
		return QDMI_ERROR_BADSTATE;
	}
	if (param != QDMI_DEVICE_SESSION_PARAMETER_TOKEN && param != QDMI_DEVICE_SESSION_PARAMETER_CUSTOM1
		&& param != QDMI_DEVICE_SESSION_PARAMETER_CUSTOM2 && param != QDMI_DEVICE_SESSION_PARAMETER_CUSTOM3 && param != QDMI_DEVICE_SESSION_PARAMETER_CUSTOM4) {
		return QDMI_ERROR_NOTSUPPORTED;
	}
	if (value != nullptr) {
		if (param == QDMI_DEVICE_SESSION_PARAMETER_TOKEN)
			session->token = std::string(static_cast<const char*>(value), size);
		else if (param == QDMI_DEVICE_SESSION_PARAMETER_CUSTOM1)
			session->qubits_num = *static_cast<const int*>(value);
		else if (param == QDMI_DEVICE_SESSION_PARAMETER_CUSTOM2)
			session->simType = *static_cast<const int*>(value);
		else if (param == QDMI_DEVICE_SESSION_PARAMETER_CUSTOM3)
			session->simExecType = *static_cast<const int*>(value);
		else if (param == QDMI_DEVICE_SESSION_PARAMETER_CUSTOM4)
			session->maxBondDim = *static_cast<const int*>(value);
	}

	return QDMI_SUCCESS;
} /// [DOXYGEN FUNCTION END]

int MAESTRO_QDMI_device_session_create_device_job(MAESTRO_QDMI_Device_Session session,
	MAESTRO_QDMI_Device_Job* job) {
	if (session == nullptr || job == nullptr) {
		return QDMI_ERROR_INVALIDARGUMENT;
	}
	if (session->status != MAESTRO_QDMI_DEVICE_SESSION_STATUS::INITIALIZED) {
		return QDMI_ERROR_BADSTATE;
	}

	*job = new MAESTRO_QDMI_Device_Job_impl_d;
	(*job)->session = session;
	// set job id to random number for demonstration purposes
	(*job)->id = MAESTRO_QDMI_generate_job_id();
	(*job)->status = QDMI_JOB_STATUS_CREATED;
	(*job)->qubits_num = session->qubits_num;

	(*job)->simType = session->simType;
	(*job)->simExecType = session->simExecType;
	(*job)->maxBondDim = session->maxBondDim;

	return QDMI_SUCCESS;
} /// [DOXYGEN FUNCTION END]

void MAESTRO_QDMI_device_job_free(MAESTRO_QDMI_Device_Job job) {
	if (job == nullptr) {
		return;
	}

	auto state = MAESTRO_QDMI_get_device_state();
	state->RemoveJob(job);
} /// [DOXYGEN FUNCTION END]

int MAESTRO_QDMI_device_job_set_parameter(MAESTRO_QDMI_Device_Job job,
	const QDMI_Device_Job_Parameter param,
	const size_t size, const void* value) {
	if (job == nullptr || (value != nullptr && size == 0) ||
		(param >= QDMI_DEVICE_JOB_PARAMETER_MAX &&
			param != QDMI_DEVICE_JOB_PARAMETER_CUSTOM1 &&
			param != QDMI_DEVICE_JOB_PARAMETER_CUSTOM2 &&
			param != QDMI_DEVICE_JOB_PARAMETER_CUSTOM3 &&
			param != QDMI_DEVICE_JOB_PARAMETER_CUSTOM4 &&
			param != QDMI_DEVICE_JOB_PARAMETER_CUSTOM5)) {
		return QDMI_ERROR_INVALIDARGUMENT;
	}
	if (job->status != QDMI_JOB_STATUS_CREATED) {
		return QDMI_ERROR_BADSTATE;
	}
	switch (param) {
	case QDMI_DEVICE_JOB_PARAMETER_PROGRAMFORMAT:
		if (value != nullptr) {
			const auto format = *static_cast<const QDMI_Program_Format*>(value);
			if (format >= QDMI_PROGRAM_FORMAT_MAX &&
				format != QDMI_PROGRAM_FORMAT_CUSTOM1 &&
				format != QDMI_PROGRAM_FORMAT_CUSTOM2 &&
				format != QDMI_PROGRAM_FORMAT_CUSTOM3 &&
				format != QDMI_PROGRAM_FORMAT_CUSTOM4 &&
				format != QDMI_PROGRAM_FORMAT_CUSTOM5) {
				return QDMI_ERROR_INVALIDARGUMENT;
			}
			if (format != QDMI_PROGRAM_FORMAT_QASM2) {
				return QDMI_ERROR_NOTSUPPORTED;
			}
			job->format = format;
		}
		return QDMI_SUCCESS;
	case QDMI_DEVICE_JOB_PARAMETER_PROGRAM:
		if (value != nullptr) {
			job->program = new char[size + 1];
			memcpy(job->program, value, size);
			job->program[size] = 0;
		}
		return QDMI_SUCCESS;
	case QDMI_DEVICE_JOB_PARAMETER_SHOTSNUM:
		if (value != nullptr) {
			job->num_shots = *static_cast<const size_t*>(value);
		}
		return QDMI_SUCCESS;
	case QDMI_DEVICE_JOB_PARAMETER_CUSTOM1:
		if (value != nullptr) {
			job->qubits_num = *static_cast<const int*>(value);
		}
		return QDMI_SUCCESS;
	case QDMI_DEVICE_JOB_PARAMETER_CUSTOM2:
		if (value != nullptr) {
			job->simType = *static_cast<const int*>(value);
		}
		return QDMI_SUCCESS;
	case QDMI_DEVICE_JOB_PARAMETER_CUSTOM3:
		if (value != nullptr) {
			job->simExecType = *static_cast<const int*>(value);
		}
		return QDMI_SUCCESS;
	case QDMI_DEVICE_JOB_PARAMETER_CUSTOM4:
		if (value != nullptr) {
			job->maxBondDim = *static_cast<const int*>(value);
		}
		return QDMI_SUCCESS;
	default:
		break;
	}

	return QDMI_ERROR_NOTSUPPORTED;
} /// [DOXYGEN FUNCTION END]

int MAESTRO_QDMI_device_job_query_property(MAESTRO_QDMI_Device_Job job,
	const QDMI_Device_Job_Property prop,
	const size_t size, void* value,
	size_t* size_ret) {
	if (job == nullptr || (value != nullptr && size == 0) ||
		(prop >= QDMI_DEVICE_JOB_PROPERTY_MAX &&
			prop != QDMI_DEVICE_JOB_PROPERTY_CUSTOM1 &&
			prop != QDMI_DEVICE_JOB_PROPERTY_CUSTOM2 &&
			prop != QDMI_DEVICE_JOB_PROPERTY_CUSTOM3 &&
			prop != QDMI_DEVICE_JOB_PROPERTY_CUSTOM4 &&
			prop != QDMI_DEVICE_JOB_PROPERTY_CUSTOM5)) {
		return QDMI_ERROR_INVALIDARGUMENT;
	}
	const auto str = std::to_string(job->id);
	ADD_STRING_PROPERTY(QDMI_DEVICE_JOB_PROPERTY_ID, str.c_str(), prop, size,
		value, size_ret);
	ADD_SINGLE_VALUE_PROPERTY(QDMI_DEVICE_JOB_PROPERTY_PROGRAMFORMAT,
		QDMI_Program_Format, job->format, prop, size, value,
		size_ret);
	ADD_SINGLE_VALUE_PROPERTY(QDMI_DEVICE_JOB_PROPERTY_SHOTSNUM, size_t,
		job->num_shots, prop, size, value, size_ret);

	ADD_SINGLE_VALUE_PROPERTY(QDMI_DEVICE_JOB_PROPERTY_CUSTOM1, int,
		job->qubits_num, prop, size, value, size_ret);

	ADD_SINGLE_VALUE_PROPERTY(QDMI_DEVICE_JOB_PROPERTY_CUSTOM2, int,
		job->simType, prop, size, value, size_ret);
	ADD_SINGLE_VALUE_PROPERTY(QDMI_DEVICE_JOB_PROPERTY_CUSTOM3, int,
		job->simExecType, prop, size, value, size_ret);
	ADD_SINGLE_VALUE_PROPERTY(QDMI_DEVICE_JOB_PROPERTY_CUSTOM4, int,
		job->maxBondDim, prop, size, value, size_ret);

	return QDMI_ERROR_NOTSUPPORTED;
} /// [DOXYGEN FUNCTION END]

int MAESTRO_QDMI_device_job_submit(MAESTRO_QDMI_Device_Job job) {
	if (job == nullptr || job->status == QDMI_JOB_STATUS_DONE) {
		return QDMI_ERROR_INVALIDARGUMENT;
	}

	// ensure that the device is ready
	auto state = MAESTRO_QDMI_get_device_state();
	state->Initialize();

	state->AddJob(job);

	return QDMI_SUCCESS;
}

int MAESTRO_QDMI_device_job_cancel(MAESTRO_QDMI_Device_Job job) {
	if (job == nullptr || job->status == QDMI_JOB_STATUS_DONE) {
		return QDMI_ERROR_INVALIDARGUMENT;
	}

	auto state = MAESTRO_QDMI_get_device_state();
	state->CancelJob(job);

	return QDMI_SUCCESS;
} /// [DOXYGEN FUNCTION END]

int MAESTRO_QDMI_device_job_check(MAESTRO_QDMI_Device_Job job, QDMI_Job_Status* status) {
	if (job == nullptr || status == nullptr) {
		return QDMI_ERROR_INVALIDARGUMENT;
	}
	// randomly decide whether job is done or not
	if (job->status == QDMI_JOB_STATUS_RUNNING && MAESTRO_QDMI_generate_bit()) {
		MAESTRO_QDMI_set_device_status(QDMI_DEVICE_STATUS_IDLE);
		job->status = QDMI_JOB_STATUS_DONE;
	}
	*status = job->status;
	return QDMI_SUCCESS;
} /// [DOXYGEN FUNCTION END]

int MAESTRO_QDMI_device_job_wait(MAESTRO_QDMI_Device_Job job, const size_t timeout) {
	if (job == nullptr) {
		return QDMI_ERROR_INVALIDARGUMENT;
	}
	job->status = QDMI_JOB_STATUS_DONE;
	MAESTRO_QDMI_set_device_status(QDMI_DEVICE_STATUS_IDLE);
	return QDMI_SUCCESS;
} /// [DOXYGEN FUNCTION END]

int MAESTRO_QDMI_device_job_get_results(MAESTRO_QDMI_Device_Job job,
	QDMI_Job_Result result, const size_t size,
	void* data, size_t* size_ret) {
	if (job == nullptr || job->status != QDMI_JOB_STATUS_DONE ||
		(data != nullptr && size == 0) ||
		(result >= QDMI_JOB_RESULT_MAX && result != QDMI_JOB_RESULT_CUSTOM1 &&
			result != QDMI_JOB_RESULT_CUSTOM2 && result != QDMI_JOB_RESULT_CUSTOM3 &&
			result != QDMI_JOB_RESULT_CUSTOM4 &&
			result != QDMI_JOB_RESULT_CUSTOM5)) {
		return QDMI_ERROR_INVALIDARGUMENT;
	}

	switch (result) {
	case QDMI_JOB_RESULT_HIST_KEYS:
	case QDMI_JOB_RESULT_HIST_VALUES:
		return MAESTRO_QDMI_device_job_get_results_hist(job, result, size, data, size_ret);
		break;
	default:
		break;
	}

	return QDMI_ERROR_NOTSUPPORTED;
}

int MAESTRO_QDMI_device_session_query_device_property(
	MAESTRO_QDMI_Device_Session session, const QDMI_Device_Property prop,
	const size_t size, void* value, size_t* size_ret) {
	if (session == nullptr || (value != nullptr && size == 0) ||
		(prop >= QDMI_DEVICE_PROPERTY_MAX &&
			prop != QDMI_DEVICE_PROPERTY_CUSTOM1 &&
			prop != QDMI_DEVICE_PROPERTY_CUSTOM2 &&
			prop != QDMI_DEVICE_PROPERTY_CUSTOM3 &&
			prop != QDMI_DEVICE_PROPERTY_CUSTOM4 &&
			prop != QDMI_DEVICE_PROPERTY_CUSTOM5)) {
		return QDMI_ERROR_INVALIDARGUMENT;
	}
	if (session->status != MAESTRO_QDMI_DEVICE_SESSION_STATUS::INITIALIZED) {
		return QDMI_ERROR_BADSTATE;
	}
	ADD_STRING_PROPERTY(QDMI_DEVICE_PROPERTY_NAME, "Maestro Device",
		prop, size, value, size_ret);
	ADD_STRING_PROPERTY(QDMI_DEVICE_PROPERTY_VERSION, "0.0.1", prop, size, value,
		size_ret);
	ADD_STRING_PROPERTY(QDMI_DEVICE_PROPERTY_LIBRARYVERSION, "0.0.1", prop, size,
		value, size_ret);
	ADD_SINGLE_VALUE_PROPERTY(QDMI_DEVICE_PROPERTY_STATUS, QDMI_Device_Status,
		MAESTRO_QDMI_get_device_status(), prop, size, value,
		size_ret);

	ADD_SINGLE_VALUE_PROPERTY(QDMI_DEVICE_PROPERTY_QUBITSNUM, int, session->qubits_num, prop,
		size, value, size_ret);
	
	ADD_LIST_PROPERTY(QDMI_DEVICE_PROPERTY_SITES, MAESTRO_QDMI_Site, MAESTRO_DEVICE_SITES,
	            prop, size, value, size_ret);
	
	//        ADD_LIST_PROPERTY(QDMI_DEVICE_PROPERTY_OPERATIONS, MAESTRO_QDMI_Operation,
	//            MAESTRO_DEVICE_OPERATIONS, prop, size, value, size_ret);


	// The example device never requires calibration
	ADD_SINGLE_VALUE_PROPERTY(QDMI_DEVICE_PROPERTY_NEEDSCALIBRATION, size_t, 0,
		prop, size, value, size_ret);

	ADD_SINGLE_VALUE_PROPERTY(
		QDMI_DEVICE_PROPERTY_PULSESUPPORT, QDMI_Device_Pulse_Support_Level,
		QDMI_DEVICE_PULSE_SUPPORT_LEVEL_NONE, prop, size, value, size_ret);

	return QDMI_ERROR_NOTSUPPORTED;
} /// [DOXYGEN FUNCTION END]

int MAESTRO_QDMI_device_session_query_site_property(MAESTRO_QDMI_Device_Session session,
	MAESTRO_QDMI_Site site,
	const QDMI_Site_Property prop,
	const size_t size, void* value,
	size_t* size_ret) {
	if (session == nullptr || site == nullptr ||
		(value != nullptr && size == 0) ||
		(prop >= QDMI_SITE_PROPERTY_MAX && prop != QDMI_SITE_PROPERTY_CUSTOM1 &&
			prop != QDMI_SITE_PROPERTY_CUSTOM2 &&
			prop != QDMI_SITE_PROPERTY_CUSTOM3 &&
			prop != QDMI_SITE_PROPERTY_CUSTOM4 &&
			prop != QDMI_SITE_PROPERTY_CUSTOM5)) {
		return QDMI_ERROR_INVALIDARGUMENT;
	}
	ADD_SINGLE_VALUE_PROPERTY(QDMI_SITE_PROPERTY_INDEX, uint64_t, site->id, prop,
		size, value, size_ret);
	ADD_SINGLE_VALUE_PROPERTY(QDMI_SITE_PROPERTY_MODULEINDEX, uint64_t, 0, prop,
		size, value, size_ret);

	return QDMI_ERROR_NOTSUPPORTED;
} /// [DOXYGEN FUNCTION END]

int MAESTRO_QDMI_device_session_query_operation_property(
	MAESTRO_QDMI_Device_Session session, MAESTRO_QDMI_Operation operation,
	const size_t num_sites, const MAESTRO_QDMI_Site* sites, const size_t num_params,
	const double* params, const QDMI_Operation_Property prop, const size_t size,
	void* value, size_t* size_ret) {
	if (session == nullptr || operation == nullptr ||
		(sites != nullptr && num_sites == 0) ||
		(params != nullptr && num_params == 0) ||
		(value != nullptr && size == 0) ||
		(prop >= QDMI_OPERATION_PROPERTY_MAX &&
			prop != QDMI_OPERATION_PROPERTY_CUSTOM1 &&
			prop != QDMI_OPERATION_PROPERTY_CUSTOM2 &&
			prop != QDMI_OPERATION_PROPERTY_CUSTOM3 &&
			prop != QDMI_OPERATION_PROPERTY_CUSTOM4 &&
			prop != QDMI_OPERATION_PROPERTY_CUSTOM5)) {
		return QDMI_ERROR_INVALIDARGUMENT;
	}
	// General properties
	//ADD_STRING_PROPERTY(QDMI_OPERATION_PROPERTY_NAME,
	//	OPERATION_PROPERTIES.at(operation).first.c_str(), prop,
	//	size, value, size_ret);
	ADD_SINGLE_VALUE_PROPERTY(QDMI_OPERATION_PROPERTY_ISZONED, bool, false, prop,
		size, value, size_ret);
	/*
	if (operation == MAESTRO_DEVICE_OPERATIONS[3]) {
		if (sites != nullptr && num_sites != 2) {
			return QDMI_ERROR_INVALIDARGUMENT;
		}
		ADD_SINGLE_VALUE_PROPERTY(QDMI_OPERATION_PROPERTY_PARAMETERSNUM, size_t, 0,
			prop, size, value, size_ret);
		ADD_SINGLE_VALUE_PROPERTY(QDMI_OPERATION_PROPERTY_DURATION, uint64_t,
			OPERATION_PROPERTIES.at(operation).second, prop,
			size, value, size_ret);
		ADD_LIST_PROPERTY(QDMI_OPERATION_PROPERTY_SITES, MAESTRO_QDMI_Site,
			DEVICE_COUPLING_MAP, prop, size, value, size_ret);
		if (sites == nullptr) {
			ADD_SINGLE_VALUE_PROPERTY(QDMI_OPERATION_PROPERTY_QUBITSNUM, size_t, 2,
				prop, size, value, size_ret);
			return QDMI_ERROR_NOTSUPPORTED;
		}

		const std::pair site_pair = { sites[0], sites[1] };
		if (site_pair.first == site_pair.second) {
			return QDMI_ERROR_INVALIDARGUMENT;
		}
		const auto it = OPERATION_FIDELITIES.find(operation);
		if (it == OPERATION_FIDELITIES.end()) {
			return QDMI_ERROR_INVALIDARGUMENT;
		}
		const auto fit = it->second.find(site_pair);
		if (fit == it->second.end()) {
			return QDMI_ERROR_INVALIDARGUMENT;
		}
		ADD_SINGLE_VALUE_PROPERTY(QDMI_OPERATION_PROPERTY_FIDELITY, double,
			fit->second, prop, size, value, size_ret);
	}
	else if (operation == MAESTRO_DEVICE_OPERATIONS[0] ||
		operation == MAESTRO_DEVICE_OPERATIONS[1] ||
		operation == MAESTRO_DEVICE_OPERATIONS[2]) {
		if ((sites != nullptr && num_sites != 1) ||
			(params != nullptr && num_params != 1)) {
			return QDMI_ERROR_INVALIDARGUMENT;
		}
		ADD_SINGLE_VALUE_PROPERTY(QDMI_OPERATION_PROPERTY_DURATION, double, 0.01,
			prop, size, value, size_ret);
		ADD_SINGLE_VALUE_PROPERTY(QDMI_OPERATION_PROPERTY_QUBITSNUM, size_t, 1,
			prop, size, value, size_ret);
		ADD_SINGLE_VALUE_PROPERTY(QDMI_OPERATION_PROPERTY_PARAMETERSNUM, size_t, 1,
			prop, size, value, size_ret);
		ADD_SINGLE_VALUE_PROPERTY(QDMI_OPERATION_PROPERTY_FIDELITY, double, 1,
			prop, size, value, size_ret);
		ADD_LIST_PROPERTY(QDMI_OPERATION_PROPERTY_SITES, MAESTRO_QDMI_Site,
			MAESTRO_DEVICE_SITES, prop, size, value, size_ret);
	}
	*/
	return QDMI_ERROR_NOTSUPPORTED;
} /// [DOXYGEN FUNCTION END]

// The following line ignores the unused parameters in the functions.
// Please remove the following code block after populating the functions.
// NOLINTEND(misc-unused-parameters,clang-diagnostic-unused-parameter)
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
