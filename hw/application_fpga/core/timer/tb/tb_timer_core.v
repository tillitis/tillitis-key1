//======================================================================
//
// tb_timer_core.v
// --------------
// Testbench for the timer core.
//
//
// Author: Joachim Strombergson
// Copyright (C) 2022 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only
//
//======================================================================

`default_nettype none

module tb_timer_core();

  //----------------------------------------------------------------
  // Internal constant and parameter definitions.
  //----------------------------------------------------------------
  parameter DEBUG     = 0;
  parameter DUMP_WAIT = 0;

  parameter CLK_HALF_PERIOD = 1;
  parameter CLK_PERIOD = 2 * CLK_HALF_PERIOD;


  //----------------------------------------------------------------
  // Register and Wire declarations.
  //----------------------------------------------------------------
  reg [31 : 0] cycle_ctr;
  reg [31 : 0] error_ctr;
  reg [31 : 0] tc_ctr;
  reg          tb_monitor;

  reg           tb_clk;
  reg           tb_reset_n;
  reg  [31 : 0] tb_prescaler_init;
  reg  [31 : 0] tb_timer_init;
  reg           tb_start;
  reg           tb_stop;
  wire [31 : 0] tb_curr_timer;
  wire          tb_reached;
  wire          tb_running;


  //----------------------------------------------------------------
  // Device Under Test.
  //----------------------------------------------------------------
  timer_core dut(
		 .clk(tb_clk),
                 .reset_n(tb_reset_n),
                 .prescaler_init(tb_prescaler_init),
		 .timer_init(tb_timer_init),
		 .start(tb_start),
		 .stop(tb_stop),
		 .curr_timer(tb_curr_timer),
		 .reached(tb_reached),
		 .running(tb_running)
                );


  //----------------------------------------------------------------
  // clk_gen
  //
  // Always running clock generator process.
  //----------------------------------------------------------------
  always
    begin : clk_gen
      #CLK_HALF_PERIOD;
      tb_clk = !tb_clk;
    end // clk_gen


  //----------------------------------------------------------------
  // sys_monitor()
  //
  // An always running process that creates a cycle counter and
  // conditionally displays information about the DUT.
  //----------------------------------------------------------------
  always
    begin : sys_monitor
      cycle_ctr = cycle_ctr + 1;
      #(CLK_PERIOD);
      if (tb_monitor)
        begin
          dump_dut_state();
        end
    end


  //----------------------------------------------------------------
  // dump_dut_state()
  //
  // Dump the state of the dump when needed.
  //----------------------------------------------------------------
  task dump_dut_state;
    begin
      $display("State of DUT");
      $display("------------");
      $display("Cycle: %08d", cycle_ctr);
      $display("");
      $display("Inputs and outputs:");
      $display("prescaler_init: 0x%08x, timer_init: 0x%08x",
	       dut.prescaler_init, dut.timer_init);
      $display("start: 0x%1x, stop: 0x%1x, reached: 0x%1x,  running: 0x%1x",
	       dut.start, dut.stop, dut.reached);
      $display("");
      $display("Internal state:");
      $display("prescaler_reg: 0x%08x, prescaler_new: 0x%08x",
	       dut.prescaler_reg, dut.prescaler_new);
      $display("prescaler_rst: 0x%1x, prescaler_inc: 0x%1x",
	       dut.prescaler_rst, dut.prescaler_inc);
      $display("");
      $display("timer_reg: 0x%08x, timer_new: 0x%08x",
	       dut.timer_reg, dut.timer_new);
      $display("timer_rst: 0x%1x, timer_inc: 0x%1x",
	       dut.timer_rst, dut.timer_inc);
      $display("");
      $display("core_ctrl_reg: 0x%02x, core_ctrl_new: 0x%02x, core_ctrl_we: 0x%1x",
	       dut.core_ctrl_reg, dut.core_ctrl_new, dut.core_ctrl_we);
      $display("");
      $display("");
    end
  endtask // dump_dut_state


  //----------------------------------------------------------------
  // reset_dut()
  //
  // Toggle reset to put the DUT into a well known state.
  //----------------------------------------------------------------
  task reset_dut;
    begin
      $display("--- DUT before reset:");
      dump_dut_state();
      $display("--- Toggling reset.");
      tb_reset_n = 0;
      #(2 * CLK_PERIOD);
      tb_reset_n = 1;
      $display("--- DUT after reset:");
      dump_dut_state();
    end
  endtask // reset_dut


  //----------------------------------------------------------------
  // display_test_result()
  //
  // Display the accumulated test results.
  //----------------------------------------------------------------
  task display_test_result;
    begin
      if (error_ctr == 0)
        begin
          $display("--- All %02d test cases completed successfully", tc_ctr);
        end
      else
        begin
          $display("--- %02d tests completed - %02d test cases did not complete successfully.",
                   tc_ctr, error_ctr);
        end
    end
  endtask // display_test_result


  //----------------------------------------------------------------
  // wait_done()
  //
  // Wait for the running flag in the dut to be dropped.
  //
  // Note: It is the callers responsibility to call the function
  // when the dut is actively processing and will in fact at some
  // point set the flag.
  //----------------------------------------------------------------
  task wait_done;
    begin
      #(2 * CLK_PERIOD);
      while (tb_running)
        begin
          #(CLK_PERIOD);
          if (DUMP_WAIT)
            begin
              dump_dut_state();
            end
        end
    end
  endtask // wait_ready


  //----------------------------------------------------------------
  // init_sim()
  //
  // Initialize all counters and testbed functionality as well
  // as setting the DUT inputs to defined values.
  //----------------------------------------------------------------
  task init_sim;
    begin
      cycle_ctr         = 0;
      error_ctr         = 0;
      tc_ctr            = 0;
      tb_monitor        = 0;

      tb_clk            = 0;
      tb_reset_n        = 1;

      tb_start          = 1'h0;
      tb_stop           = 1'h0;
      tb_prescaler_init = 32'h0;
      tb_timer_init     = 32'h0;
    end
  endtask // init_sim


  //----------------------------------------------------------------
  // test1()
  //
  // Test that the timer can count to a specified number of cycles.
  //----------------------------------------------------------------
  task test1;
    begin : test1
      reg [31 : 0] test1_cycle_ctr_start;
      reg [31 : 0] test1_counted_num_cycles;
      reg [31 : 0] test1_expected_num_cycles;

      tc_ctr = tc_ctr + 1;

      $display("--- test1: Run timer to set value started.");
      $display("--- test1: prescaler: 6, timer: 9. Should take 6*9 + 1 = 55 cycles.");
      tb_prescaler_init = 32'h6;
      tb_timer_init     = 32'h9;
      test1_expected_num_cycles = tb_prescaler_init * tb_timer_init + 1;

      #(CLK_PERIOD);
      tb_start = 1'h1;
      test1_cycle_ctr_start = cycle_ctr;
      #(CLK_PERIOD);
      tb_start = 1'h0;
      #(CLK_PERIOD);

      while (~tb_reached) begin
	#(CLK_PERIOD);
      end
      test1_counted_num_cycles = cycle_ctr - test1_cycle_ctr_start;


      if (test1_counted_num_cycles == test1_expected_num_cycles) begin
	$display("--- test1: Correct number of cycles counted: %0d", test1_counted_num_cycles);
      end
      else begin
	$display("--- test1: Error, expected %0d cycles, counted cycles: %0d",
		 test1_expected_num_cycles, test1_counted_num_cycles);
	error_ctr = error_ctr + 1;
      end

      tb_stop = 1'h1;
      #(CLK_PERIOD);
      tb_stop = 1'h0;

      $display("--- test1: Completed.");
      $display("");
    end
  endtask // test1



  //----------------------------------------------------------------
  // test2()
  //
  // Test that the free running functionality works.
  //----------------------------------------------------------------
  task test2;
    begin : test2
      reg [31 : 0] test1_cycle_ctr_start;
      reg [31 : 0] test1_counted_num_cycles;
      reg [31 : 0] test1_expected_num_cycles;

      tc_ctr = tc_ctr + 1;

      $display("--- test2: Run timer in free running mode started.");
      $display("--- test2: Set prescaler and timer to one, but wait more cycles.");

      tb_prescaler_init     = 32'h1;
      tb_timer_init         = 32'h1;
      tb_start              = 1'h1;
      test1_cycle_ctr_start = cycle_ctr;

      #(CLK_PERIOD);
      tb_start = 1'h0;
      test1_cycle_ctr_start = cycle_ctr;

      #(1337 * CLK_PERIOD);
      test1_expected_num_cycles = cycle_ctr - test1_cycle_ctr_start;

      if (tb_curr_timer == test1_expected_num_cycles) begin
	$display("--- test2: Correct number of cycles counted: %0d", tb_curr_timer);
      end
      else begin
	$display("--- test2: Error, expected %0d cycles, counted cycles: %0d",
		 test1_expected_num_cycles, tb_curr_timer);
	error_ctr = error_ctr + 1;
      end

      $display("--- test2: Completed.");
      $display("");
    end
  endtask // test2


  //----------------------------------------------------------------
  // timer_core_test
  //
  // Test vectors from:
  //----------------------------------------------------------------
  initial
    begin : timer_core_test
      $display("--- Simulation of timer core started.");
      $display("");

      init_sim();
      reset_dut();

      test1();
      test2();

      display_test_result();
      $display("");
      $display("--- Simulation of timer core completed.");
      $finish(error_ctr);
    end // timer_core_test
endmodule // tb_timer_core

//======================================================================
// EOF tb_timer_core.v
//======================================================================
