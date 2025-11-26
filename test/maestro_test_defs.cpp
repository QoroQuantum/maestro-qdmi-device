/*------------------------------------------------------------------------------
Copyright 2024 Munich Quantum Software Stack Project
Copyright 2025 Qoro Quantum Ltd.

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
 * @brief This file is for testing whether a device implements all the required
 * functions.
 * @details It calls all the functions in the device interface to ensure that
 * they are implemented. During linking, when a function is not implemented this
 * will raise an error.
 * @note This file is not meant to be ever executed, only linked.
 */

#include "maestro_qdmi/device.h"

int main()
{
    MAESTRO_QDMI_Device_Session session = nullptr;
    MAESTRO_QDMI_Device_Job job = nullptr;
    MAESTRO_QDMI_Site site = nullptr;
    MAESTRO_QDMI_Operation operation = nullptr;
    MAESTRO_QDMI_device_initialize();
    MAESTRO_QDMI_device_finalize();
    MAESTRO_QDMI_device_session_alloc(&session);
    MAESTRO_QDMI_device_session_init(session);
    MAESTRO_QDMI_device_session_free(session);
    MAESTRO_QDMI_device_session_set_parameter(session, QDMI_DEVICE_SESSION_PARAMETER_MAX, 0,
                                              nullptr);
    MAESTRO_QDMI_device_session_create_device_job(session, &job);
    MAESTRO_QDMI_device_job_free(job);
    MAESTRO_QDMI_device_job_set_parameter(job, QDMI_DEVICE_JOB_PARAMETER_MAX, 0, nullptr);
    MAESTRO_QDMI_device_job_submit(job);
    MAESTRO_QDMI_device_job_cancel(job);
    MAESTRO_QDMI_device_job_check(job, nullptr);
    MAESTRO_QDMI_device_job_wait(job);
    MAESTRO_QDMI_device_job_get_results(job, QDMI_JOB_RESULT_MAX, 0, nullptr, nullptr);
    MAESTRO_QDMI_device_session_query_device_property(session, QDMI_DEVICE_PROPERTY_MAX, 0, nullptr,
                                                      nullptr);
    MAESTRO_QDMI_device_session_query_site_property(session, site, QDMI_SITE_PROPERTY_MAX, 0,
                                                    nullptr, nullptr);
    MAESTRO_QDMI_device_session_query_operation_property(session, operation, 0, nullptr, 0, nullptr,
                                                         QDMI_OPERATION_PROPERTY_MAX, 0, nullptr,
                                                         nullptr);
}
