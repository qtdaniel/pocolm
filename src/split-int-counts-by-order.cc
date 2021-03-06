// split-int-counts.cc

// Copyright     2016  Johns Hopkins University (Author: Daniel Povey)

// See ../COPYING for clarification regarding multiple authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
// THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED
// WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE,
// MERCHANTABILITY OR NON-INFRINGEMENT.
// See the Apache 2 License for the specific language governing permissions and
// limitations under the License.

#include <cassert>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "pocolm-types.h"
#include "lm-state.h"



/*
   This program operates on 'int-counts' of the form that are output by
   'get-int-counts'.  It reads counts from its stdin (which must all have
   history-length >0), and outputs the counts to its outputs, splitting them up
   by order (empty history goes to the 1st output, length-1 history to the 2nd,
   etc.).  Note, we expect there to be no counts with empty history.  */


int main (int argc, const char **argv) {
  int num_outputs = argc - 1;
  if (num_outputs <= 0) {
    std::cerr << "split-int-counts-by-order: expected usage:\n"
              << "split-int-counts-by-order <output1> <output2> ... <outputN>  < input-int-counts\n"
              << "This program reads int-counts from its stdin, and distributes them\n"
              << "among the provided outputs according to the length of the history state\n";
    exit(1);
  }
  // note: if num_outputs is 1 and the input is of higher order, all data just goes
  // to the same file.
  std::ofstream *outputs = new std::ofstream[num_outputs];

  for (int32 i = 0; i < num_outputs; i++) {
    outputs[i].open(argv[i + 1], std::ios_base::binary|std::ios_base::out);
    if (!outputs[i]) {
      std::cerr << "split-int-counts-by-order: Failed to open '" << argv[i + 1] << "' for output.";
      exit(1);
    }
  }
  errno = 0;

  int32 num_states_written = 0;
  std::vector<int64> counts_written_per_output(num_outputs, 0);

  pocolm::IntLmState int_lm_state;

  while (std::cin.peek(),!std::cin.eof()) {
    int_lm_state.Read(std::cin);
    int32 history_size = int_lm_state.history.size();
    if (history_size >= num_outputs) {
      std::cerr << "split-int-counts-by-order: history-length " << history_size
                << " is >= num-outputs " << num_outputs << "\n";
      exit(1);
    }
    counts_written_per_output[history_size] += int_lm_state.counts.size();
    num_states_written++;
    int_lm_state.Write(outputs[history_size]);
  }

  std::ostringstream info_string;
  for (int32 i = 0; i < num_outputs; i++) {
    outputs[i].close();
    if (outputs[i].fail()) {
      std::cerr << "split-int-counts-by-order: failed to close file "
                << argv[i + 1] << " (disk full?)\n";
      exit(1);
    }
    info_string << " " << counts_written_per_output[i];
  }
  delete [] outputs;


  std::cerr << "split-int-counts-by-order: processed "
            << num_states_written << " LM states, with the counts "
            << "for each output respectively as: "
            << info_string.str() << "\n";

  return 0;
}

/*
  testing

 ( echo 11 12 13; echo 11 12 13 14 ) | get-text-counts 3 | sort -n | uniq -c | get-int-counts /dev/stdout /dev/null /dev/null | split-int-counts /dev/stdout /dev/null | print-int-counts

 */
