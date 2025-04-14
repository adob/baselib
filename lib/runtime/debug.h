#pragma once

namespace lib::runtime {
    // num_cpu returns the number of logical CPUs usable by the current process.
    //
    // The set of available CPUs is checked by querying the operating system
    // at process startup. Changes to operating system CPU allocation after
    // process startup are not reflected.
    int num_cpu();
}