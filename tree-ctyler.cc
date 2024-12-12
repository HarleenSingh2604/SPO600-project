/* Test pass
   Chris Tyler, Seneca Polytechnic College, 2024-11
   Modelled on tree-nrv.cc

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

GCC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

#define INCLUDE_MEMORY
#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "backend.h"
#include "tree.h"
#include "gimple.h"
#include "tree-pass.h"
#include "ssa.h"
#include "gimple-iterator.h"
#include "gimple-walk.h"
#include "internal-fn.h"
#include "gimple-pretty-print.h"

// Included for dump_printf:
#include "tree-pretty-print.h"
#include "diagnostic.h"
#include "dumpfile.h"
#include "builtins.h"

namespace {

const pass_data pass_data_ctyler =
{
  GIMPLE_PASS, /* type */
  "ctyler", /* name */
  OPTGROUP_NONE, /* optinfo_flags */
  TV_NONE, /* tv_id */
  PROP_cfg, /* properties_required */
  0, /* properties_provided */
  0, /* properties_destroyed */
  0, /* todo_flags_start */
  0, /* todo_flags_finish */
};

class pass_ctyler : public gimple_opt_pass
{
public:
  pass_ctyler (gcc::context *ctxt)
    : gimple_opt_pass (pass_data_ctyler, ctxt)
  {}

  /* opt_pass methods: */
  bool gate (function *)  final override {
        return true; // always execute pass
  }
  unsigned int execute (function *fun) final override;

}; // class pass_ctyler

unsigned int
pass_ctyler::execute (function *fun)
  {
    // Dummy pass with diagnostic dump
    if (dump_file)
      {
        fprintf (dump_file, "===== Dummy Pass Diagnostic Dump =====\n");
      }

    // Iterate through code being compiled
    basic_block bb;
    int bb_cnt = 0, stmt_cnt = 0;
    FOR_EACH_BB_FN (bb, fun)
      {
        bb_cnt++;
        if (dump_file)
          {
            fprintf (dump_file, "===== Basic block count: %d =====\n", bb_cnt);
          }

        for (gimple_stmt_iterator gsi = gsi_start_bb (bb); !gsi_end_p (gsi); gsi_next (&gsi))
          {
            gimple *g = gsi_stmt (gsi);
            stmt_cnt++;
            if (dump_file)
              {
                fprintf (dump_file, "----- Statement count: %d -----\n", stmt_cnt);
                print_gimple_stmt (dump_file, g, 0, TDF_VOPS|TDF_MEMSYMS);
              }
         }
       }

    // Attempt to Identify cloned functions
    // Map to store cloned functions
    std::map<tree, std::vector<function*> > cloned_functions;

    // Iterate over all functions in the current unit
    for (function *f = fun; f; f = f->next)
      {
        // Check if the function name has a suffix indicating it is a clone
        const char *name = IDENTIFIER_POINTER (DECL_NAME (f));
        if (strstr (name, "_clone") != NULL)
          {
            // Extract the base function name
            tree base_name = get_base_name (name);
            cloned_functions[base_name].push_back (f);
          }
      }

    // Get representation of functions and construct hash or perform comparison
    for (auto &pair : cloned_functions)
      {
        if (pair.second.size() > 1)
          {
            bool should_prune = true;
            function *base_func = pair.second;
            for (size_t i = 1; i < pair.second.size(); i++)
              {
                function *clone_func = pair.second[i];
                if (!are_functions_equivalent (base_func, clone_func))
                  {
                    should_prune = false;
                    break;
                  }
              }

            // Make a decision on pruning
            if (should_prune)
              {
                if (dump_file)
                  {
                    fprintf (dump_file, "PRUNE: %s\n", IDENTIFIER_POINTER (DECL_NAME (base_func)));
                  }
              }
            else
              {
                if (dump_file)
                  {
                    fprintf (dump_file, "NOPRUNE: %s\n", IDENTIFIER_POINTER (DECL_NAME (base_func)));
                  }
              }
          }
      }

    return 0;
  }

  // Helper function to extract the base function name from a cloned function name
  static tree get_base_name (const char *name)
  {
    // Simplified example, actual implementation may vary based on GCC's naming conventions
    const char *clone_suffix = "_clone";
    size_t suffix_len = strlen (clone_suffix);
    size_t name_len = strlen (name);
    if (name_len > suffix_len && strcmp (name + name_len - suffix_len, clone_suffix) == 0)
      {
        return get_identifier (name, name_len - suffix_len);
      }
    return get_identifier (name, name_len);
  }

  // Helper function to compare two functions for equivalence
  static bool are_functions_equivalent (function *f1, function *f2)
  {
    // Simplified example, actual implementation may involve more detailed comparison
    basic_block bb1, bb2;
    FOR_EACH_BB_FN (bb1, f1)
      {
        FOR_EACH_BB_FN (bb2, f2)
          {
            gimple_stmt_iterator gsi1, gsi2;
            for (gsi1 = gsi_start_bb (bb1), gsi2 = gsi_start_bb (bb2);
                 !gsi_end_p (gsi1) && !gsi_end_p (gsi2);
                 gsi_next (&gsi1), gsi_next (&gsi2))
              {
                gimple *stmt1 = gsi_stmt (gsi1);
                gimple *stmt2 = gsi_stmt (gsi2);
                if (!gimple_stmt_equal_p (stmt1, stmt2))
                  return false;
              }
          }
      }
    return true;
  }
}; // class pass_ctyler

gimple_opt_pass *
make_pass_ctyler (gcc::context *ctxt)
{
  return new pass_ctyler (ctxt);
}

} // anon namespace

