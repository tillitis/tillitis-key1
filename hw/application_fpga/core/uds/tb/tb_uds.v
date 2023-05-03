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

  parameter CLK_HALF_PERIOD = 1;
  parameter CLK_PERIOD = 2 * CLK_HALF_PERIOD;

  localparam ADDR_UDS_FIRST = 8'h10;
  localparam ADDR_UDS_LAST  = 8'h17;


  //----------------------------------------------------------------
  // Register and Wire declarations.
  //----------------------------------------------------------------
  reg [31 : 0]  cycle_ctr;
  reg [31 : 0]  error_ctr;
  reg [31 : 0]  tc_ctr;
  reg           tb_monitor;

  reg           tb_clk;
  reg           tb_reset_n;
  reg           tb_fw_app_mode;
  reg           tb_cs;
  reg [7 : 0]   tb_address;
  wire [31 : 0] tb_read_data;


  //----------------------------------------------------------------
  // Device Under Test.
  //----------------------------------------------------------------
  uds dut(
          .clk(tb_clk),
          .reset_n(tb_reset_n),

	  .fw_app_mode(tb_fw_app_mode),

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
      $display("State of DUT at cycle: %08d", cycle_ctr);
      $display("------------");
      $display("Inputs and outputs:");
      $display("fw_app_mode: 0x%1x", tb_fw_app_mode);
      $display("cs: 0x%1x, address: 0x%02x, read_data: 0x%08x", tb_cs, tb_address, tb_read_data);
      $display("");

      $display("Internal state:");
      $display("tmp_read_ready: 0x%1x, tmp_read_data: 0x%08x", dut.tmp_ready, dut.tmp_read_data);
      for (i = 0 ; i < 8 ; i = i + 1) begin
        $display("uds_reg[%1d]: 0x%08x, uds_rd_reg[%1d]: 0x%1x", i, dut.uds_reg[i], i, dut.uds_rd_reg[i]);
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

      tb_clk         = 1'h0;
      tb_reset_n     = 1'h1;
      tb_fw_app_mode = 1'h0;
      tb_cs          = 1'h0;
      tb_address     = 8'h0;
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

      $display("");
      $display("--- test1: started.");

      $display("--- test1: Filling uds with known values.");
      dut.uds_reg[0] = 32'hf0f0f0f0;
      dut.uds_reg[1] = 32'he1e1e1e1;
      dut.uds_reg[2] = 32'hd2d2d2d2;
      dut.uds_reg[3] = 32'hc3c3c3c3;
      dut.uds_reg[4] = 32'hb4b4b4b4;
      dut.uds_reg[5] = 32'ha5a5a5a5;
      dut.uds_reg[6] = 32'h96969696;
      dut.uds_reg[7] = 32'h87878787;

      $display("--- test1: Dumping DUT state to show UDS contents");
      dump_dut_state();

      $display("--- test1: Reading UDS words.");
      read_word(ADDR_UDS_FIRST + 0, 32'hf0f0f0f0);
      read_word(ADDR_UDS_FIRST + 1, 32'he1e1e1e1);
      read_word(ADDR_UDS_FIRST + 2, 32'hd2d2d2d2);

      $display("--- test1: Dumping state again to see read bits.");
      dump_dut_state();

      $display("--- test1: Reading rest of the words.");
      read_word(ADDR_UDS_FIRST + 3, 32'hc3c3c3c3);
      read_word(ADDR_UDS_FIRST + 4, 32'hb4b4b4b4);
      read_word(ADDR_UDS_FIRST + 5, 32'ha5a5a5a5);
      read_word(ADDR_UDS_FIRST + 6, 32'h96969696);
      read_word(ADDR_UDS_FIRST + 7, 32'h87878787);

      $display("--- test1: Dumping state again to see read bits.");
      dump_dut_state();

      $display("--- test1: Reading UDS words again.");
      $display("--- test1: This should return all zeros.");
      read_word(ADDR_UDS_FIRST + 0, 32'h0);
      read_word(ADDR_UDS_FIRST + 1, 32'h0);
      read_word(ADDR_UDS_FIRST + 2, 32'h0);
      read_word(ADDR_UDS_FIRST + 3, 32'h0);
      read_word(ADDR_UDS_FIRST + 4, 32'h0);
      read_word(ADDR_UDS_FIRST + 5, 32'h0);
      read_word(ADDR_UDS_FIRST + 6, 32'h0);
      read_word(ADDR_UDS_FIRST + 7, 32'h0);

      $display("--- test1: Resetting DUT.");
      $display("--- test1: This should allow access again.");
      reset_dut();

      $display("--- test1: Filling uds with new known values.");
      dut.uds_reg[0] = 32'h0f0f0f0f;
      dut.uds_reg[1] = 32'h1e1e1e1e;
      dut.uds_reg[2] = 32'h2d2d2d2d;
      dut.uds_reg[3] = 32'h3c3c3c3c;
      dut.uds_reg[4] = 32'h4b4b4b4b;
      dut.uds_reg[5] = 32'h5a5a5a5a;
      dut.uds_reg[6] = 32'h69696969;
      dut.uds_reg[7] = 32'h78787878;


      $display("--- test1: Dumping state again to see read bits.");
      dump_dut_state();

      $display("--- test1: Reading UDS words in changed order.");
      read_word(ADDR_UDS_FIRST + 7, 32'h78787878);
      read_word(ADDR_UDS_FIRST + 6, 32'h69696969);
      read_word(ADDR_UDS_FIRST + 4, 32'h4b4b4b4b);
      read_word(ADDR_UDS_FIRST + 3, 32'h3c3c3c3c);
      read_word(ADDR_UDS_FIRST + 1, 32'h1e1e1e1e);
      read_word(ADDR_UDS_FIRST + 0, 32'h0f0f0f0f);
      read_word(ADDR_UDS_FIRST + 5, 32'h5a5a5a5a);
      read_word(ADDR_UDS_FIRST + 2, 32'h2d2d2d2d);

      $display("--- test1: Reading UDS words again.");
      $display("--- test1: This should return all zeros.");
      read_word(ADDR_UDS_FIRST + 0, 32'h0);
      read_word(ADDR_UDS_FIRST + 1, 32'h0);
      read_word(ADDR_UDS_FIRST + 2, 32'h0);
      read_word(ADDR_UDS_FIRST + 3, 32'h0);
      read_word(ADDR_UDS_FIRST + 4, 32'h0);
      read_word(ADDR_UDS_FIRST + 5, 32'h0);
      read_word(ADDR_UDS_FIRST + 6, 32'h0);
      read_word(ADDR_UDS_FIRST + 7, 32'h0);

      $display("--- test1: completed.");
      $display("");
    end
  endtask // test1


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
      $display("   -= Testbench for uds completed =-");
      $display("     =============================");
      $display("");
      $finish;
    end // uds_test
endmodule // tb_uds

//======================================================================
// EOF tb_uds.v
//======================================================================
