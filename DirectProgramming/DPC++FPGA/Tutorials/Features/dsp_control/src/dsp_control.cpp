//==============================================================
// Copyright Intel Corporation
//
// SPDX-License-Identifier: MIT
// =============================================================
#include <CL/sycl.hpp>
#include <sycl/ext/intel/fpga_extensions.hpp>

// dpc_common.hpp can be found in the dev-utilities include folder.
// e.g., $ONEAPI_ROOT/dev-utilities/include/dpc_common.hpp
#include "dpc_common.hpp"

using namespace sycl;

// Forward declare the kernel names in the global scope.
// This FPGA best practice reduces name mangling in the optimization reports.
class GlobalControl;
class LocalControl;

// Runs the Kernel
void KernelRun(const std::vector<float> &input_data,
               std::vector<float> &output_data) {

#if defined(FPGA_EMULATOR)
  ext::intel::fpga_emulator_selector device_selector;
#else
  ext::intel::fpga_selector device_selector;
#endif

  try {
    // create the SYCL device queue
    queue q(device_selector, dpc_common::exception_handler,
            property::queue::enable_profiling{});

    buffer input_buffer(input_data);
    buffer output_buffer(output_data);

    auto e_g = q.submit([&](handler &h) {
      accessor input_a(input_buffer, h, read_only);
      accessor output_a(output_buffer, h, write_only, no_init);

      // Kernel that demonstrates DSP global control
      h.single_task<GlobalControl>([=]() [[intel::kernel_args_restrict]] {
        // Command-line option `-Xsdsp-mode=prefer-softlogic` controls the float
        // add to be implemented in soft-logic
        output_a[0] = input_a[0] + input_a[1];
      });
    });

    auto e_l = q.submit([&](handler &h) {
      accessor input_a(input_buffer, h, read_only);
      accessor output_a(output_buffer, h, write_only, no_init);

      // Kernel that demonstrates DSP local control
      h.single_task<LocalControl>([=]() [[intel::kernel_args_restrict]] {
        // The local control library function overrides the global control, and
        // makes the float add to be implemented in DSP
        ext::intel::math_dsp_control<ext::intel::Preference::DSP,
                                     ext::intel::Propagate::Off>(
            [&] { output_a[1] = input_a[0] + input_a[1]; });
      });
    });

  } catch (exception const &e) {
    // Catches exceptions in the host code
    std::cerr << "Caught a SYCL host exception:\n" << e.what() << "\n";

    // Most likely the runtime couldn't find FPGA hardware!
    if (e.get_cl_code() == CL_DEVICE_NOT_FOUND) {
      std::cerr << "If you are targeting an FPGA, please ensure that your "
                   "system has a correctly configured FPGA board.\n";
      std::cerr << "Run sys_check in the oneAPI root directory to verify.\n";
      std::cerr << "If you are targeting the FPGA emulator, compile with "
                   "-DFPGA_EMULATOR.\n";
    }
    std::terminate();
  }
}

int main() {
  std::vector<float> input_data = {1.234f, 2.345f};
  std::vector<float> output_data = {.0f, .0f};
  float golden = input_data[0] + input_data[1];

  KernelRun(input_data, output_data);

  bool passed = true;
  for (size_t i = 0; i < output_data.size(); i++) {
    if (output_data[i] != golden) {
      std::cout << "Output Mismatch: \n"
                << "output_data[" << i << "] vs golden" << i << " = "
                << output_data[i] << "," << golden << " \n";
      passed = false;
    }
  }

  if (passed) {
    std::cout << "PASSED: all kernel results are correct.\n";
  } else {
    std::cout << "FAILED\n";
  }
  return passed ? 0 : 1;
}

