//======================================================================
//
// tb_trng.v
// -----------
// Testbench for the TRNG core.
//
//
// Author: Joachim Strombergson
// Copyright (C) 2022 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only
//
//======================================================================

`default_nettype none

module tb_trng();

  //----------------------------------------------------------------
  // Internal constant and parameter definitions.
  //----------------------------------------------------------------
  parameter DEBUG     = 1;

  parameter CLK_HALF_PERIOD = 1;
  parameter CLK_PERIOD = 2 * CLK_HALF_PERIOD;

  // API
  localparam ADDR_STATUS       = 8'h09;
  localparam STATUS_READY_BIT  = 0;
  localparam ADDR_ENTROPY      = 8'h20;


  //----------------------------------------------------------------
  // Register and Wire declarations.
  //----------------------------------------------------------------
  reg [31 : 0]  cycle_ctr;
  reg [31 : 0]  error_ctr;
  reg [31 : 0]  tc_ctr;
  reg           tb_monitor;

  reg           tb_clk;
  reg           tb_reset_n;
  reg           tb_cs;
  reg           tb_we;
  reg [7 : 0]   tb_address;
  reg [31 : 0]  tb_write_data;
  wire [31 : 0] tb_read_data;
  wire          tb_ready;


  //----------------------------------------------------------------
  // Device Under Test.
  //----------------------------------------------------------------
  rosc dut(
           .clk(tb_clk),
           .reset_n(tb_reset_n),

           .cs(tb_cs),
           .we(tb_cs),
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
    begin : dump_dut_state
      integer i;
      $display("State of DUT at cycle: %08d", cycle_ctr);
      $display("------------");
      $display("Inputs and outputs:");
      $display("cs: 0x%1x, address: 0x%02x, read_data: 0x%08x", tb_cs, tb_address, tb_read_data);
      $display("");

      $display("Internal state:");
      $display("tmp_read_ready: 0x%1x, tmp_read_data: 0x%08x", dut.tmp_ready, dut.tmp_read_data);
      $display("cycle_ctr_done: 0x%1x, cycle_ctr_rst: 0x%1x, cycle_ctr: 0x%04x",
	       dut.cycle_ctr_done, dut.cycle_ctr_rst, dut.cycle_ctr_reg);
      $display("bit_ctr: 0x%02x", dut.bit_ctr_reg);
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

      tb_clk         = 1'h0;
      tb_reset_n     = 1'h1;
      tb_cs          = 1'h0;
      tb_cs          = 1'h0;
      tb_address     = 8'h0;
      tb_write_data  = 32'h0;
    end
  endtask // init_sim


  //----------------------------------------------------------------
  // read_word()
  //
  // Read a data word from the given address in the DUT.
  // the word read will be available in the global variable
  // read_data.
  //----------------------------------------------------------------
  task read_word(input [11 : 0]  address, input [31 : 0] expected);
    begin : read_word
      reg [31 : 0] read_data;

      tb_address   = address;
      tb_cs        = 1'h1;

      #(CLK_HALF_PERIOD);
      read_data = tb_read_data;

      #(CLK_HALF_PERIOD);
      tb_cs        = 1'h0;

      if (DEBUG)
        begin
	  if (read_data == expected) begin
            $display("--- Reading 0x%08x from 0x%02x.", read_data, address);
	  end else begin
            $display("--- Error: Got 0x%08x when reading from 0x%02x, expected 0x%08x",
		     read_data, address, expected);
	    error_ctr = error_ctr + 1;
	  end
          $display("");
        end
    end
  endtask // read_word


  //----------------------------------------------------------------
  // test1()
  //----------------------------------------------------------------
  task test1;
    begin
      tc_ctr = tc_ctr + 1;
      tb_monitor = 1;

      $display("");
      $display("--- test1: started.");
      read_word(ADDR_STATUS, 32'h0);
      $display("--- test1: completed.");
      $display("");
    end
  endtask // test1


  //----------------------------------------------------------------
  // trng_test
  //----------------------------------------------------------------
  initial
    begin : trng_test
      $display("");
      $display("   -= Testbench for trng started =-");
      $display("     ============================");
      $display("");

      init_sim();
      reset_dut();
      test1();

      display_test_result();
      $display("");
      $display("   -= Testbench for trng completed =-");
      $display("     ==============================");
      $display("");
      $finish;
    end // trng_test
endmodule // tb_trng

//======================================================================
// EOF tb_trng.v
//======================================================================
