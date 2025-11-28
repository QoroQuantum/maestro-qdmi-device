/*------------------------------------------------------------------------------
Copyright 2024 Munich Quantum Software Stack Project
Copyright 2025 Qoro Quantum Ltd.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
------------------------------------------------------------------------------*/

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include "maestro_qdmi/device.h"

#define CHECK_DEVICE_STATUS(device_status, expected_value)                     \
  {                                                                            \
    ASSERT_EQ(MAESTRO_QDMI_device_session_query_device_property(               \
                  session, QDMI_DEVICE_PROPERTY_STATUS, sizeof(size_t),        \
                  device_status, nullptr),                                     \
              QDMI_SUCCESS);                                                   \
    ASSERT_TRUE((*device_status == expected_value));                           \
  }

#define CHECK_JOB_STATUS(job_status, expected_value)                           \
  {                                                                            \
    ASSERT_EQ(MAESTRO_QDMI_device_job_check(job, job_status), QDMI_SUCCESS);   \
    ASSERT_TRUE(*job_status == expected_value);                                \
  }

#define CREATE_JOB(job, n_shot, format, program)                               \
  {                                                                            \
    ASSERT_EQ(MAESTRO_QDMI_device_session_create_device_job(session, &job),    \
              QDMI_SUCCESS);                                                   \
    ASSERT_EQ(                                                                 \
        MAESTRO_QDMI_device_job_set_parameter(                                 \
            job, QDMI_DEVICE_JOB_PARAMETER_SHOTSNUM, sizeof(n_shot), &n_shot), \
        QDMI_SUCCESS);                                                         \
    ASSERT_EQ(MAESTRO_QDMI_device_job_set_parameter(                           \
                  job, QDMI_DEVICE_JOB_PARAMETER_PROGRAMFORMAT,                \
                  sizeof(format), &format),                                    \
              QDMI_SUCCESS);                                                   \
    ASSERT_EQ(                                                                 \
        MAESTRO_QDMI_device_job_set_parameter(                                 \
            job, QDMI_DEVICE_JOB_PARAMETER_PROGRAM, strlen(program), program), \
        QDMI_SUCCESS);                                                         \
  }

class QDMIImplementationTest : public ::testing::Test {
protected:
  MAESTRO_QDMI_Device_Session session = nullptr;

  void SetUp() override {
    ASSERT_EQ(MAESTRO_QDMI_device_initialize(), QDMI_SUCCESS)
        << "Failed to initialize the device";

    ASSERT_EQ(MAESTRO_QDMI_device_session_alloc(&session), QDMI_SUCCESS)
        << "Failed to allocate a session";

    ASSERT_EQ(MAESTRO_QDMI_device_session_init(session), QDMI_SUCCESS)
        << "Failed to initialize a session.";
  }

  void TearDown() override { MAESTRO_QDMI_device_finalize(); }
};

TEST_F(QDMIImplementationTest, SessionSetParameterAfterAllocated) {
  char dummy_hostname[] = "qlm.lrz.de";
  ASSERT_EQ(MAESTRO_QDMI_device_session_set_parameter(
                session, QDMI_DEVICE_SESSION_PARAMETER_BASEURL,
                sizeof(dummy_hostname), &dummy_hostname),
            QDMI_ERROR_NOTSUPPORTED);
}

namespace {
std::string Get_test_circuit() {
  return "OPENQASM 2.0;\n"
         "include \"qelib1.inc\";\n"
         "qreg q[2];\n"
         "creg c[2];\n"
         "h q[0];\n"
         "cx q[0], q[1];\n"
         "measure q -> c;\n";
}
} // namespace

TEST_F(QDMIImplementationTest, ControlSetShotParameterImplemented) {
  MAESTRO_QDMI_Device_Job job = nullptr;
  int nShot = 1024;
  ASSERT_EQ(MAESTRO_QDMI_device_session_create_device_job(session, &job),
            QDMI_SUCCESS);
  ASSERT_EQ(MAESTRO_QDMI_device_job_set_parameter(
                job, QDMI_DEVICE_JOB_PARAMETER_SHOTSNUM, sizeof(nShot), &nShot),
            QDMI_SUCCESS);
  MAESTRO_QDMI_device_job_free(job);
}

TEST_F(QDMIImplementationTest, ControlSetProgramFormatParameterImplemented) {
  MAESTRO_QDMI_Device_Job job = nullptr;
  int nShot = 1024;
  const QDMI_Program_Format qirFormat = QDMI_PROGRAM_FORMAT_QIRBASESTRING;
  const QDMI_Program_Format qasmFormat = QDMI_PROGRAM_FORMAT_QASM2;

  ASSERT_EQ(MAESTRO_QDMI_device_session_create_device_job(session, &job),
            QDMI_SUCCESS);
  ASSERT_EQ(MAESTRO_QDMI_device_job_set_parameter(
                job, QDMI_DEVICE_JOB_PARAMETER_PROGRAMFORMAT, sizeof(qirFormat),
                &qirFormat),
            QDMI_ERROR_NOTSUPPORTED);
  ASSERT_EQ(MAESTRO_QDMI_device_job_set_parameter(
                job, QDMI_DEVICE_JOB_PARAMETER_PROGRAMFORMAT,
                sizeof(qasmFormat), &qasmFormat),
            QDMI_SUCCESS);
  MAESTRO_QDMI_device_job_free(job);
}

TEST_F(QDMIImplementationTest, ControlSubmitAndCancelJob) {

  MAESTRO_QDMI_Device_Job job = nullptr;
  size_t nShot = 1024;
  const QDMI_Program_Format qasmFormat = QDMI_PROGRAM_FORMAT_QASM2;
  std::string test_circuit = Get_test_circuit();
  const char *c_t_c = test_circuit.c_str();

  QDMI_Job_Status *job_status =
      (QDMI_Job_Status *)malloc(sizeof(QDMI_Job_Status));
  CREATE_JOB(job, nShot, qasmFormat, c_t_c);

  ASSERT_EQ(MAESTRO_QDMI_device_job_submit(job), QDMI_SUCCESS);

  ASSERT_EQ(MAESTRO_QDMI_device_job_cancel(job), QDMI_SUCCESS);

  CHECK_JOB_STATUS(job_status, QDMI_JOB_STATUS_CANCELED);

  MAESTRO_QDMI_device_job_free(job);
}

TEST_F(QDMIImplementationTest, ControlSubmitAndWaitJob) {
  MAESTRO_QDMI_Device_Job job = nullptr;
  size_t nShot = 1024;
  const QDMI_Program_Format qasmFormat = QDMI_PROGRAM_FORMAT_QASM2;
  std::string test_circuit = Get_test_circuit();
  const char *c_t_c = test_circuit.c_str();

  QDMI_Job_Status *job_status =
      (QDMI_Job_Status *)malloc(sizeof(QDMI_Job_Status));

  QDMI_Device_Status *device_status =
      (QDMI_Device_Status *)malloc(sizeof(QDMI_Device_Status));

  CREATE_JOB(job, nShot, qasmFormat, c_t_c);

  CHECK_DEVICE_STATUS(device_status, QDMI_DEVICE_STATUS_IDLE);
  CHECK_JOB_STATUS(job_status, QDMI_JOB_STATUS_CREATED);

  ASSERT_EQ(MAESTRO_QDMI_device_job_submit(job), QDMI_SUCCESS);

  ASSERT_EQ(MAESTRO_QDMI_device_job_wait(job, 5000), QDMI_SUCCESS);

  CHECK_DEVICE_STATUS(device_status, QDMI_DEVICE_STATUS_IDLE);
  CHECK_JOB_STATUS(job_status, QDMI_JOB_STATUS_DONE);

  MAESTRO_QDMI_device_job_free(job);
}

TEST_F(QDMIImplementationTest, ControlGetDataHistogramKeys) {
  MAESTRO_QDMI_Device_Job job = nullptr;
  size_t nShot = 1024;
  const QDMI_Program_Format qasmFormat = QDMI_PROGRAM_FORMAT_QASM2;
  std::string test_circuit = Get_test_circuit();
  const char *c_t_c = test_circuit.c_str();

  QDMI_Job_Status *job_status =
      (QDMI_Job_Status *)malloc(sizeof(QDMI_Job_Status));
  size_t histogram_size;
  char *histogram_keys;
  CREATE_JOB(job, nShot, qasmFormat, c_t_c);
  ASSERT_EQ(MAESTRO_QDMI_device_job_submit(job), QDMI_SUCCESS);
  ASSERT_EQ(MAESTRO_QDMI_device_job_wait(job, 5000), QDMI_SUCCESS);

  ASSERT_EQ(MAESTRO_QDMI_device_job_get_results(job, QDMI_JOB_RESULT_HIST_KEYS,
                                                0, nullptr, &histogram_size),
            QDMI_SUCCESS);

  histogram_keys = (char *)malloc(histogram_size);

  ASSERT_EQ(MAESTRO_QDMI_device_job_get_results(job, QDMI_JOB_RESULT_HIST_KEYS,
                                                histogram_size, histogram_keys,
                                                nullptr),
            QDMI_SUCCESS);

  MAESTRO_QDMI_device_job_free(job);
}

TEST_F(QDMIImplementationTest, ControlGetDataHistogramValue) {
  MAESTRO_QDMI_Device_Job job = nullptr;
  size_t nShot = 1024;
  const QDMI_Program_Format qasmFormat = QDMI_PROGRAM_FORMAT_QASM2;
  std::string test_circuit = Get_test_circuit();
  const char *c_t_c = test_circuit.c_str();

  QDMI_Job_Status *job_status =
      (QDMI_Job_Status *)malloc(sizeof(QDMI_Job_Status));
  size_t histogram_values_size;
  int *histogram_values;
  CREATE_JOB(job, nShot, qasmFormat, c_t_c);
  ASSERT_EQ(MAESTRO_QDMI_device_job_submit(job), QDMI_SUCCESS);
  ASSERT_EQ(MAESTRO_QDMI_device_job_wait(job, 5000), QDMI_SUCCESS);

  ASSERT_EQ(
      MAESTRO_QDMI_device_job_get_results(job, QDMI_JOB_RESULT_HIST_VALUES, 0,
                                          nullptr, &histogram_values_size),
      QDMI_SUCCESS);
  histogram_values = (int *)malloc(histogram_values_size);

  ASSERT_EQ(MAESTRO_QDMI_device_job_get_results(
                job, QDMI_JOB_RESULT_HIST_VALUES, histogram_values_size,
                histogram_values, nullptr),
            QDMI_SUCCESS);

  MAESTRO_QDMI_device_job_free(job);
}

TEST_F(QDMIImplementationTest, ControlGetDataProbabilityKeys) {
  MAESTRO_QDMI_Device_Job job = nullptr;
  size_t nShot = 1024;
  const QDMI_Program_Format qasmFormat = QDMI_PROGRAM_FORMAT_QASM2;
  std::string test_circuit = Get_test_circuit();
  const char *c_t_c = test_circuit.c_str();

  QDMI_Job_Status *job_status =
      (QDMI_Job_Status *)malloc(sizeof(QDMI_Job_Status));
  size_t probability_keys_size;
  char *probability_keys;
  CREATE_JOB(job, nShot, qasmFormat, c_t_c);
  ASSERT_EQ(MAESTRO_QDMI_device_job_submit(job), QDMI_SUCCESS);
  ASSERT_EQ(MAESTRO_QDMI_device_job_wait(job, 5000), QDMI_SUCCESS);

  ASSERT_EQ(MAESTRO_QDMI_device_job_get_results(
                job, QDMI_JOB_RESULT_PROBABILITIES_SPARSE_KEYS, 0, nullptr,
                &probability_keys_size),
            QDMI_ERROR_NOTSUPPORTED);
}

TEST_F(QDMIImplementationTest, ControlGetDataProbabilityValues) {
  MAESTRO_QDMI_Device_Job job = nullptr;
  size_t nShot = 1024;
  const QDMI_Program_Format qasmFormat = QDMI_PROGRAM_FORMAT_QASM2;
  std::string test_circuit = Get_test_circuit();
  const char *c_t_c = test_circuit.c_str();

  QDMI_Job_Status *job_status =
      (QDMI_Job_Status *)malloc(sizeof(QDMI_Job_Status));
  size_t probability_values_size;
  double *probability_values;
  CREATE_JOB(job, nShot, qasmFormat, c_t_c);
  ASSERT_EQ(MAESTRO_QDMI_device_job_submit(job), QDMI_SUCCESS);
  ASSERT_EQ(MAESTRO_QDMI_device_job_wait(job, 5000), QDMI_SUCCESS);

  ASSERT_EQ(MAESTRO_QDMI_device_job_get_results(
                job, QDMI_JOB_RESULT_PROBABILITIES_SPARSE_VALUES, 0, nullptr,
                &probability_values_size),
            QDMI_ERROR_NOTSUPPORTED);
}

TEST_F(QDMIImplementationTest, ControlGetDataProbabilityDense) {
  MAESTRO_QDMI_Device_Job job = nullptr;
  size_t nShot = 1024;
  const QDMI_Program_Format qasmFormat = QDMI_PROGRAM_FORMAT_QASM2;
  std::string test_circuit = Get_test_circuit();
  const char *c_t_c = test_circuit.c_str();

  QDMI_Job_Status *job_status =
      (QDMI_Job_Status *)malloc(sizeof(QDMI_Job_Status));
  size_t probability_dense_size;
  double *probability_dense;
  CREATE_JOB(job, nShot, qasmFormat, c_t_c);
  ASSERT_EQ(MAESTRO_QDMI_device_job_submit(job), QDMI_SUCCESS);
  ASSERT_EQ(MAESTRO_QDMI_device_job_wait(job, 5000), QDMI_SUCCESS);

  ASSERT_EQ(MAESTRO_QDMI_device_job_get_results(
                job, QDMI_JOB_RESULT_PROBABILITIES_DENSE, 0, nullptr,
                &probability_dense_size),
            QDMI_ERROR_NOTSUPPORTED);

  MAESTRO_QDMI_device_job_free(job);
}

TEST_F(QDMIImplementationTest, QuerySitePropertyNotSupported) {

  size_t size = 0;
  ASSERT_EQ(MAESTRO_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_SITES, 0, nullptr, &size),
            QDMI_SUCCESS)
      << "Devices must provide a list of sites";
  std::vector<MAESTRO_QDMI_Site> sites(size / sizeof(MAESTRO_QDMI_Site));
  ASSERT_EQ(MAESTRO_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_SITES, size,
                static_cast<void *>(sites.data()), nullptr),
            QDMI_SUCCESS)
      << "Devices must provide a list of sites";

  ASSERT_EQ(MAESTRO_QDMI_device_session_query_site_property(
                session, sites.at(0), QDMI_SITE_PROPERTY_T1, sizeof(uint64_t),
                nullptr, nullptr),
            QDMI_ERROR_NOTSUPPORTED);
}
