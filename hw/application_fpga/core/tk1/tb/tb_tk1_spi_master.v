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
  parameter DEBUG = 1;

  parameter CLK_HALF_PERIOD = 1;
  parameter CLK_PERIOD      = 2 * CLK_HALF_PERIOD;

  parameter MISO_ALL_ZERO = 0;
  parameter MISO_ALL_ONE  = 1;
  parameter MISO_MOSI     = 2;
  parameter MISO_INV_MOSI = 3;


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
  reg           tb_spi_enable_vld;
  reg           tb_spi_start;
  reg  [7 : 0]  tb_spi_tx_data;
  reg           tb_spi_tx_data_vld;
  wire  [7 : 0] tb_spi_rx_data;
  wire          tb_spi_ready;

  reg [1 : 0]   tb_miso_mux_ctrl;


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
		     .spi_enable_vld(tb_spi_enable_vld),
		     .spi_start(tb_spi_start),
		     .spi_tx_data(tb_spi_tx_data),
		     .spi_tx_data_vld(tb_spi_tx_data_vld),
		     .spi_rx_data(tb_spi_rx_data),
		     .spi_ready(tb_spi_ready)
		     );

  //----------------------------------------------------------------
  // miso_mux
  //
  // This is a MUX that controls the input to the miso signal.
  //----------------------------------------------------------------
  always @*
    begin : miso_mux
      case (tb_miso_mux_ctrl)
	MISO_ALL_ZERO: tb_spi_miso = 1'h0;
	MISO_ALL_ONE:  tb_spi_miso = 1'h1;
	MISO_MOSI:     tb_spi_miso = tb_spi_mosi;
	MISO_INV_MOSI: tb_spi_miso = ~tb_spi_mosi;
	default: begin end
      endcase // case (miso_mux_ctrl)
    end

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
      $display("");
      $display("State of DUT at cycle: %08d", cycle_ctr);
      $display("------------");
      $display("Inputs and outputs:");
      $display("spi_ss: 0x%1x, spi_sck: 0x%1x, spi_mosi: 0x%1x, spi_miso:0x%1x",
	       dut.spi_ss, dut.spi_sck, dut.spi_mosi, dut.spi_miso);
      $display("spi_enable_vld: 0x%1x, spi_enable: 0x%1x",
	       dut.spi_enable_vld, dut.spi_enable);
      $display("spi_tx_data_vld: 0x%1x, spi_tx_data: 0x%02x",
	       dut.spi_tx_data_vld, dut.spi_tx_data);
      $display("spi_start: 0x%1x, spi_ready: 0x%1x, spi_rx_data: 0x%02x",
	       dut.spi_start, dut.spi_ready, dut.spi_rx_data);
      $display("");


      $display("");
      $display("Internal state:");
      $display("spi_clk_ctr_rst: 0x%1x, spi_clk_ctr_reg: 0x%02x",
	       dut.spi_clk_ctr_rst, dut.spi_clk_ctr_reg);
      $display("");
      $display("spi_bit_ctr_rst: 0x%1x, spi_bit_ctr_inc: 0x%1x, spi_bit_ctr_reg: 0x%02x",
	       dut.spi_bit_ctr_rst, dut.spi_bit_ctr_inc, dut.spi_bit_ctr_reg);
      $display("");
      $display("spi_ctrl_reg: 0x%02x, spi_ctrl_new: 0x%02x, spi_ctrl_we: 0x%1x",
	       dut.spi_ctrl_reg, dut.spi_ctrl_new, dut.spi_ctrl_we);

      $display("");
      $display("spi_tx_data_new: 0x%1x, spi_tx_data_nxt: 0x%1x, spi_tx_data_we: 0x%1x",
	       dut.spi_tx_data_new, dut.spi_tx_data_nxt, dut.spi_tx_data_we);
      $display("spi_tx_data_reg: 0x%02x, spi_tx_data_new: 0x%02x",
	       dut.spi_tx_data_reg, dut.spi_tx_data_new);
      $display("");
      $display("spi_miso_sample0_reg: 0x%1x, spi_miso_sample1_reg: 0x%1x",
	       dut.spi_miso_sample0_reg, dut.spi_miso_sample1_reg);
      $display("");
      $display("spi_rx_data_nxt: 0x%1x, spi_rx_data_we: 0x%1x",
	       dut.spi_rx_data_nxt, dut.spi_rx_data_we);
      $display("spi_rx_data_reg: 0x%02x, spi_rx_data_new: 0x%02x",
	       dut.spi_rx_data_reg, dut.spi_rx_data_new);
      $display("spi_rx_data_reg0: 0x%1x, spi_rx_data_new0: 0x%1x",
	       dut.spi_rx_data_reg[0], dut.spi_rx_data_new[0]);
      $display("spi_rx_data_reg1: 0x%1x, spi_rx_data_new1: 0x%1x",
	       dut.spi_rx_data_reg[1], dut.spi_rx_data_new[1]);
      $display("spi_rx_data_reg2: 0x%1x, spi_rx_data_new2: 0x%1x",
	       dut.spi_rx_data_reg[2], dut.spi_rx_data_new[2]);
      $display("spi_rx_data_reg3: 0x%1x, spi_rx_data_new3: 0x%1x",
	       dut.spi_rx_data_reg[3], dut.spi_rx_data_new[3]);
      $display("spi_rx_data_reg4: 0x%1x, spi_rx_data_new4: 0x%1x",
	       dut.spi_rx_data_reg[4], dut.spi_rx_data_new[4]);
      $display("spi_rx_data_reg5: 0x%1x, spi_rx_data_new5: 0x%1x",
	       dut.spi_rx_data_reg[5], dut.spi_rx_data_new[5]);
      $display("spi_rx_data_reg6: 0x%1x, spi_rx_data_new6: 0x%1x",
	       dut.spi_rx_data_reg[6], dut.spi_rx_data_new[6]);
      $display("spi_rx_data_reg7: 0x%1x, spi_rx_data_new7: 0x%1x",
	       dut.spi_rx_data_reg[7], dut.spi_rx_data_new[7]);
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

      tb_clk             = 1'h0;
      tb_reset_n         = 1'h1;
      tb_spi_miso        = 1'h0;
      tb_spi_enable      = 1'h0;
      tb_spi_enable_vld  = 1'h0;
      tb_spi_start       = 1'h0;
      tb_spi_tx_data     = 8'h0;
      tb_spi_tx_data_vld = 1'h0;
      tb_miso_mux_ctrl   = MISO_MOSI;
    end
  endtask // init_sim


  //----------------------------------------------------------------
  // test1()
  //----------------------------------------------------------------
  task test1;
    begin
      tc_ctr = tc_ctr + 1;
      monitor = 1;

      tb_miso_mux_ctrl = MISO_INV_MOSI;

      $display("");
      $display("--- test1: Send a byte, receive a byte.");

      $display("--- test1: State at idle for 4 cycles.");
      #(4 * CLK_PERIOD);

      $display("--- test1: Enable the SPI master. Should drop cs");
      tb_spi_enable     = 1'h1;
      tb_spi_enable_vld  = 1'h1;
      #(CLK_PERIOD);
      tb_spi_enable_vld  = 1'h0;

      $display("--- test1: Load a 0xa7 byte to transmit.");
      #(CLK_PERIOD);
      tb_spi_tx_data    = 8'ha7;
      tb_spi_tx_data_vld = 1'h1;
      #(CLK_PERIOD);
      tb_spi_tx_data_vld = 1'h0;

      $display("--- test1: Start the transfer. This should drop ready.");
      #(CLK_PERIOD);
      tb_spi_start = 1'h1;
      #(CLK_PERIOD);
      tb_spi_start = 1'h0;
      #(2 * CLK_PERIOD);

      $display("--- test1: Wait for ready to be set.");
      while (!tb_spi_ready) begin
	#(CLK_PERIOD);
      end
      $display("--- test1: Ready has been set.");
      #(CLK_PERIOD);

      $display("--- test1: Disable the SPI master. Should raise cs");
      tb_spi_enable     = 1'h0;
      tb_spi_enable_vld  = 1'h1;
      #(CLK_PERIOD);
      tb_spi_enable_vld  = 1'h0;
      #(CLK_PERIOD);

      if (tb_miso_mux_ctrl == MISO_ALL_ZERO) begin
	if (tb_spi_rx_data == 8'h0) begin
	  $display("--- test1: Correct all zero data received.");
	end else begin
	  $display("--- test1: Error, Incorrect all zero data data received.");
	end
      end

      if (tb_miso_mux_ctrl == MISO_ALL_ONE) begin
	if (tb_spi_rx_data == 8'hff) begin
	  $display("--- test1: Correct all one data received.");
	end else begin
	  $display("--- test1: Error, Incorrect all one data data received.");
	end
      end

      if (tb_miso_mux_ctrl == MISO_MOSI) begin
	if (tb_spi_rx_data == tb_spi_tx_data) begin
	  $display("--- test1: Correct MOSI data received.");
	end else begin
	  $display("--- test1: Error, Incorrect MOSI data received.");
	end
      end

      if (tb_miso_mux_ctrl == MISO_INV_MOSI) begin
	if (tb_spi_rx_data == ~tb_spi_tx_data) begin
	  $display("--- test1: Correct inverse MOSI data received.");
	end else begin
	  $display("--- test1: Error, Incorrect inverse MOSI data received.");
	end
      end

      $display("--- test1: completed.");
      monitor = 0;

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
