//======================================================================
//
// tb_tk1_spi_master.v
// -------------------
// Testbench for the TK1_SPI_MASTER core.
//
//
// Author: Joachim Strombergson
// Copyright (C) 2023 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only
//
//======================================================================

`default_nettype none

module tb_tk1_spi_master();

  //----------------------------------------------------------------
  // Internal constant and parameter definitions.
  //----------------------------------------------------------------
  parameter DEBUG     = 1;

  parameter CLK_HALF_PERIOD = 1;
  parameter CLK_PERIOD = 2 * CLK_HALF_PERIOD;


  //----------------------------------------------------------------
  // Register and Wire declarations.
  //----------------------------------------------------------------
  reg [31 : 0] cycle_ctr;
  reg [31 : 0] error_ctr;
  reg [31 : 0] tc_ctr;
  reg          monitor;

  reg           tb_clk;
  reg           tb_reset_n;
  wire          tb_spi_ss;
  wire          tb_spi_sck;
  wire          tb_spi_mosi;
  reg           tb_spi_miso;
  reg           tb_spi_enable;
  reg           tb_spi_enable_we;
  reg           tb_spi_start;
  reg  [7 : 0]  tb_spi_tx_data;
  reg           tb_spi_tx_data_we;
  wire  [7 : 0] tb_spi_rx_data;
  wire          tb_spi_ready;

  //----------------------------------------------------------------
  // Device Under Test.
  //----------------------------------------------------------------
  tk1_spi_master dut(
		     .clk(tb_clk),
		     .reset_n(tb_reset_n),

		     .spi_ss(tb_spi_ss),
		     .spi_sck(tb_spi_sck),
		     .spi_mosi(tb_spi_mosi),
		     .spi_miso(tb_spi_miso),

		     .spi_enable(tb_spi_enable),
		     .spi_enable_we(tb_spi_enable_we),
		     .spi_start(tb_spi_start),
		     .spi_tx_data(tb_spi_tx_data),
		     .spi_tx_data_we(tb_spi_tx_data_we),
		     .spi_rx_data(tb_spi_rx_data),
		     .spi_ready(tb_spi_ready)
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
      if (monitor)
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
      $display("State of DUT at cycle: %08d", cycle_ctr);
      $display("------------");
      $display("Inputs and outputs:");
      $display("");

      $display("Internal state:");
      $display("");

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
      cycle_ctr = 0;
      error_ctr = 0;
      tc_ctr    = 0;
      monitor   = 0;

      tb_clk            = 1'h0;
      tb_reset_n        = 1'h1;
      tb_spi_miso       = 1'h0;
      tb_spi_enable     = 1'h0;
      tb_spi_enable_we  = 1'h0;
      tb_spi_start      = 1'h0;
      tb_spi_tx_data    = 8'h0;
      tb_spi_tx_data_we = 1'h0;
    end
  endtask // init_sim


  //----------------------------------------------------------------
  // test1()
  //----------------------------------------------------------------
  task test1;
    begin
      tc_ctr = tc_ctr + 1;

      $display("");
      $display("--- test1: BLA BLA BLA started.");

      $display("--- test1: completed.");
      $display("");
    end
  endtask // test1


  //----------------------------------------------------------------
  // tk1_spi_master_test
  //----------------------------------------------------------------
  initial
    begin : tk1_spi_master_test
      $display("");
      $display("   -= Testbench for tk1_spi_master started =-");
      $display("     =======================================");
      $display("");

      init_sim();
      reset_dut();

      test1();

      display_test_result();
      $display("");
      $display("   -= Testbench for tk1_spi_master completed =-");
      $display("     =========================================");
      $display("");
      $finish;
    end // tk1_spi_master_test
endmodule // tb_tk1_spi_master

//======================================================================
// EOF tb_tk1_spi_master.v
//======================================================================
