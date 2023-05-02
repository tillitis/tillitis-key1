//======================================================================
//
// tb_timer.v
// -----------
// Testbench for the timer top level wrapper.
//
//
// Author: Joachim Strombergson
// Copyright (C) 2022 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only
//
//======================================================================

`default_nettype none

module tb_timer();

  //----------------------------------------------------------------
  // Internal constant and parameter definitions.
  //----------------------------------------------------------------
  parameter DEBUG     = 0;
  parameter DUMP_WAIT = 0;

  parameter CLK_HALF_PERIOD = 1;
  parameter CLK_PERIOD = 2 * CLK_HALF_PERIOD;

  localparam ADDR_CTRL          = 8'h08;
  localparam CTRL_START_BIT     = 0;
  localparam CTRL_STOP_BIT      = 1;

  localparam ADDR_STATUS        = 8'h09;
  localparam STATUS_RUNNING_BIT = 0;

  localparam ADDR_PRESCALER     = 8'h0a;
  localparam ADDR_TIMER         = 8'h0b;


  //----------------------------------------------------------------
  // Register and Wire declarations.
  //----------------------------------------------------------------
  reg [31 : 0] cycle_ctr;
  reg [31 : 0] error_ctr;
  reg [31 : 0] tc_ctr;
  reg          tb_monitor;

  reg           tb_clk;
  reg           tb_reset_n;
  reg           tb_cs;
  reg           tb_we;
  reg [7 : 0]   tb_address;
  reg [31 : 0]  tb_write_data;
  wire [31 : 0] tb_read_data;
  wire          tb_ready;

  reg [31 : 0] read_data;


  //----------------------------------------------------------------
  // Device Under Test.
  //----------------------------------------------------------------
  timer dut(
           .clk(tb_clk),
           .reset_n(tb_reset_n),

           .cs(tb_cs),
           .we(tb_we),

           .address(tb_address),
           .write_data(tb_write_data),
           .read_data(tb_read_data),
	   .ready(tb_ready)
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
      $display("cs: 0x%1x, we: 0x%1x, address: 0x%02x, write_data: 0x%08x, read_data: 0x%08x, ready: 0x%1x",
	       tb_cs, tb_we, tb_address, tb_write_data, tb_read_data, tb_ready);
      $display("");
      $display("Internal state:");
      $display("prescaler_reg: 0x%08x, timer_reg: 0x%08x", dut.prescaler_reg, dut.timer_reg);
      $display("start_reg: 0x%1x, stop_reg: 0x%1x", dut.start_reg, dut.stop_reg);
      $display("core_running: 0x%1x, core_curr_timer: 0x%08x", dut.core_running, dut.core_curr_timer);
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
      $display("--- Toggle reset.");
      tb_reset_n = 0;
      #(2 * CLK_PERIOD);
      tb_reset_n = 1;
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
  // init_sim()
  //
  // Initialize all counters and testbed functionality as well
  // as setting the DUT inputs to defined values.
  //----------------------------------------------------------------
  task init_sim;
    begin
      cycle_ctr  = 0;
      error_ctr  = 0;
      tc_ctr     = 0;
      tb_monitor = 0;

      tb_clk        = 1'h0;
      tb_reset_n    = 1'h1;
      tb_cs         = 1'h0;
      tb_we         = 1'h0;
      tb_address    = 8'h0;
      tb_write_data = 32'h0;
    end
  endtask // init_sim


  //----------------------------------------------------------------
  // write_word()
  //
  // Write the given word to the DUT using the DUT interface.
  //----------------------------------------------------------------
  task write_word(input [11 : 0] address,
                  input [31 : 0] word);
    begin
      if (DEBUG)
        begin
          $display("--- Writing 0x%08x to 0x%02x.", word, address);
          $display("");
        end

      tb_address = address;
      tb_write_data = word;
      tb_cs = 1;
      tb_we = 1;
      #(2 * CLK_PERIOD);
      tb_cs = 0;
      tb_we = 0;
    end
  endtask // write_word


  //----------------------------------------------------------------
  // read_word()
  //
  // Read a data word from the given address in the DUT.
  // the word read will be available in the global variable
  // read_data.
  //----------------------------------------------------------------
  task read_word(input [11 : 0]  address);
    begin
      tb_address = address;
      tb_cs = 1;
      tb_we = 0;
      #(CLK_PERIOD);
      read_data = tb_read_data;
      tb_cs = 0;

      if (DEBUG)
        begin
          $display("--- Reading 0x%08x from 0x%02x.", read_data, address);
          $display("");
        end
    end
  endtask // read_word


  //----------------------------------------------------------------
  // wait_ready()
  //
  // Wait for the ready flag to be set in dut.
  //----------------------------------------------------------------
  task wait_ready;
    begin : wready
      read_word(ADDR_STATUS);
      while (read_data == 0)
        read_word(ADDR_STATUS);
    end
  endtask // wait_ready


  //----------------------------------------------------------------
  // test1()
  // Set timer and scaler and then start the timer. Wait
  // for the ready flag to be asserted again.
  //----------------------------------------------------------------
  task test1;
    begin : test1
      reg [31 : 0] time_start;
      reg [31 : 0] time_stop;

      tc_ctr = tc_ctr + 1;
      tb_monitor = 0;

      $display("");
      $display("--- test1: started.");

      write_word(ADDR_PRESCALER, 8'h02);
      write_word(ADDR_TIMER, 8'h10);

      time_start = cycle_ctr;
      write_word(ADDR_CTRL, 8'h01);

      #(2 * CLK_PERIOD);
      read_word(ADDR_STATUS);
      while (read_data) begin
	read_word(ADDR_STATUS);
      end

      time_stop = cycle_ctr;
      write_word(CTRL_START_BIT, 8'h02);

      $display("--- test1: Cycles between start and stop: %d", (time_stop - time_start));
      #(CLK_PERIOD);
      tb_monitor = 0;

      $display("--- test1: completed.");
      $display("");
    end
  endtask // tes1


  //----------------------------------------------------------------
  // timer_test
  //----------------------------------------------------------------
  initial
    begin : timer_test
      $display("");
      $display("   -= Testbench for timer started =-");
      $display("     =============================");
      $display("");

      init_sim();
      reset_dut();
      test1();

      display_test_result();
      $display("");
      $display("   -= Testbench for timer completed =-");
      $display("     ===============================");
      $display("");
      $finish;
    end // timer_test
endmodule // tb_timer

//======================================================================
// EOF tb_timer.v
//======================================================================
