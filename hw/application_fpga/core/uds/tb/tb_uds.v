//======================================================================
//
// tb_uds.v
// -----------
// Testbench for the UDS core.
//
//
// Author: Joachim Strombergson
// Copyright (C) 2022 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only
//
//======================================================================

`default_nettype none

module tb_uds();

  //----------------------------------------------------------------
  // Internal constant and parameter definitions.
  //----------------------------------------------------------------
  parameter DEBUG     = 1;
  parameter DUMP_WAIT = 0;

  parameter CLK_HALF_PERIOD = 1;
  parameter CLK_PERIOD = 2 * CLK_HALF_PERIOD;

  localparam ADDR_NAME0        = 8'h00;
  localparam ADDR_NAME1        = 8'h01;
  localparam ADDR_VERSION      = 8'h02;

  localparam ADDR_UDS_FIRST    = 8'h10;
  localparam ADDR_UDS_LAST     = 8'h17;


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
  reg [7 : 0]   tb_address;
  wire [31 : 0] tb_read_data;

  reg [31 : 0] read_data;


  //----------------------------------------------------------------
  // Device Under Test.
  //----------------------------------------------------------------
  uds dut(
           .clk(tb_clk),
           .reset_n(tb_reset_n),

           .cs(tb_cs),

           .address(tb_address),
           .read_data(tb_read_data)
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
      $display("State of DUT");
      $display("------------");
      $display("Cycle: %08d", cycle_ctr);
      for (i = 0 ; i < 8 ; i = i + 1) begin
        $display("uds_reg[%1d]: 0x%08x, uds_rd_reg[%1d]: 0x%1x",
                 i, dut.uds_reg[i], i, dut.uds_rd_reg[i]);
      end
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
      tb_address    = 8'h0;
    end
  endtask // init_sim


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
  // test1()
  //----------------------------------------------------------------
  task test1;
    begin
      tc_ctr = tc_ctr + 1;
      tb_monitor = 1'h0;

      $display("");
      $display("--- test1: started.");

      $display("--- test1: Reading NAME and version info.");
      read_word(ADDR_NAME0);
      read_word(ADDR_NAME1);
      read_word(ADDR_VERSION);

      $display("--- test1: Dumping DUT state to show UDS contents");
      dump_dut_state();

      $display("--- test1: Reading UDS words.");
      read_word(ADDR_UDS_FIRST + 0);
      read_word(ADDR_UDS_FIRST + 1);
      read_word(ADDR_UDS_FIRST + 2);

      $display("--- test1: Dumping state again to see read bits.");
      dump_dut_state();

      $display("--- test1: Reading rest of the words.");
      read_word(ADDR_UDS_FIRST + 3);
      read_word(ADDR_UDS_FIRST + 4);
      read_word(ADDR_UDS_FIRST + 5);
      read_word(ADDR_UDS_FIRST + 6);
      read_word(ADDR_UDS_FIRST + 7);

      $display("--- test1: Dumping state again to see read bits.");
      dump_dut_state();

      $display("--- test1: Reading UDS words again.");
      read_word(ADDR_UDS_FIRST + 0);
      read_word(ADDR_UDS_FIRST + 1);
      read_word(ADDR_UDS_FIRST + 2);
      read_word(ADDR_UDS_FIRST + 3);
      read_word(ADDR_UDS_FIRST + 4);
      read_word(ADDR_UDS_FIRST + 5);
      read_word(ADDR_UDS_FIRST + 6);
      read_word(ADDR_UDS_FIRST + 7);

      $display("--- test1: Resetting DUT.");
      reset_dut();

      $display("--- test1: Dumping state again to see read bits.");
      dump_dut_state();

      $display("--- test1: Reading UDS words.");
      read_word(ADDR_UDS_FIRST + 0);
      read_word(ADDR_UDS_FIRST + 1);
      read_word(ADDR_UDS_FIRST + 2);
      read_word(ADDR_UDS_FIRST + 3);
      read_word(ADDR_UDS_FIRST + 4);
      read_word(ADDR_UDS_FIRST + 5);
      read_word(ADDR_UDS_FIRST + 6);
      read_word(ADDR_UDS_FIRST + 7);

      $display("--- test1: Reading UDS words again.");
      read_word(ADDR_UDS_FIRST + 0);
      read_word(ADDR_UDS_FIRST + 1);
      read_word(ADDR_UDS_FIRST + 2);
      read_word(ADDR_UDS_FIRST + 3);
      read_word(ADDR_UDS_FIRST + 4);
      read_word(ADDR_UDS_FIRST + 5);
      read_word(ADDR_UDS_FIRST + 6);
      read_word(ADDR_UDS_FIRST + 7);

      $display("--- test1: completed.");
      $display("");
    end
  endtask // tes1


  //----------------------------------------------------------------
  // uds_test
  //----------------------------------------------------------------
  initial
    begin : uds_test
      $display("");
      $display("   -= Testbench for uds started =-");
      $display("     ===========================");
      $display("");

      init_sim();
      reset_dut();
      test1();

      display_test_result();
      $display("");
      $display("   -= Testbench for uds started =-");
      $display("     ===========================");
      $display("");
      $finish;
    end // uds_test
endmodule // tb_uds

//======================================================================
// EOF tb_uds.v
//======================================================================
