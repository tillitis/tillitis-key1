//======================================================================
//
// tb_uart.v
// ---------
// Testbench for the UART core.
//
//
// Author: Joachim Strombergson
// Copyright (c) 2014, Secworks Sweden AB
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or
// without modification, are permitted provided that the following
// conditions are met:
//
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in
//    the documentation and/or other materials provided with the
//    distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//======================================================================

module tb_uart();

  //----------------------------------------------------------------
  // Internal constant and parameter definitions.
  //----------------------------------------------------------------
  parameter DEBUG           = 0;
  parameter VERBOSE         = 0;

  parameter CLK_HALF_PERIOD = 1;
  parameter CLK_PERIOD      = CLK_HALF_PERIOD * 2;


  //----------------------------------------------------------------
  // Register and Wire declarations.
  //----------------------------------------------------------------
  reg [31 : 0] cycle_ctr;
  reg [31 : 0] error_ctr;
  reg [31 : 0] tc_ctr;

  reg           tb_clk;
  reg           tb_reset_n;
  reg           tb_rxd;
  wire          tb_txd;
  reg           tb_cs;
  reg           tb_we;
  reg [7 : 0]   tb_address;
  reg [31 : 0]  tb_write_data;
  wire [31 : 0] tb_read_data;
  wire          tb_ready;

  reg          txd_state;


  //----------------------------------------------------------------
  // Device Under Test.
  //----------------------------------------------------------------
  uart dut(
           .clk(tb_clk),
           .reset_n(tb_reset_n),

           .rxd(tb_rxd),
           .txd(tb_txd),

            // API interface.
            .cs(tb_cs),
            .we(tb_we),
            .address(tb_address),
            .write_data(tb_write_data),
            .read_data(tb_read_data),
            .ready(tb_ready)
          );


  //----------------------------------------------------------------
  // Concurrent assignments.
  //----------------------------------------------------------------


  //----------------------------------------------------------------
  // clk_gen
  //
  // Clock generator process.
  //----------------------------------------------------------------
  always
    begin : clk_gen
      #CLK_HALF_PERIOD tb_clk = !tb_clk;
    end // clk_gen


  //----------------------------------------------------------------
  // sys_monitor
  //----------------------------------------------------------------
  always
    begin : sys_monitor
      #(CLK_PERIOD);
      if (DEBUG)
        begin
          dump_rx_state();
          dump_tx_state();
          $display("");
        end
      if (VERBOSE)
        begin
          $display("cycle: 0x%016x", cycle_ctr);
        end
      cycle_ctr = cycle_ctr + 1;
    end


  //----------------------------------------------------------------
  // tx_monitor
  //
  // Observes what happens on the dut tx port and reports it.
  //----------------------------------------------------------------
  always @*
    begin : tx_monitor
      if ((!tb_txd) && txd_state)
        begin
          $display("txd going low.");
          txd_state = 0;
        end

      if (tb_txd && (!txd_state))
        begin
          $display("txd going high");
          txd_state = 1;
        end
    end


  //----------------------------------------------------------------
  // dump_dut_state()
  //
  // Dump the state of the dut when needed.
  //----------------------------------------------------------------
  task dump_dut_state;
    begin
      $display("State of DUT");
      $display("------------");
      $display("Inputs and outputs:");
      $display("rxd = 0x%01x, txd = 0x%01x,",
               dut.core.rxd, dut.core.txd);
      $display("");

      $display("Sample and data registers:");
      $display("rxd_reg = 0x%01x, rxd_byte_reg = 0x%01x",
               dut.core.rxd_reg, dut.core.rxd_byte_reg);
      $display("");

      $display("Counters:");
      $display("rxd_bit_ctr_reg = 0x%01x, rxd_bitrate_ctr_reg = 0x%02x",
               dut.core.rxd_bit_ctr_reg, dut.core.rxd_bitrate_ctr_reg);
      $display("");


      $display("Control signals and FSM state:");
      $display("erx_ctrl_reg = 0x%02x",
               dut.core.erx_ctrl_reg);
      $display("");
    end
  endtask // dump_dut_state



  //----------------------------------------------------------------
  // dump_rx_state()
  //
  // Dump the state of the rx engine.
  //----------------------------------------------------------------
  task dump_rx_state;
    begin
      $display("rxd = 0x%01x, rxd_reg = 0x%01x, rxd_byte_reg = 0x%01x, rxd_bit_ctr_reg = 0x%01x, rxd_bitrate_ctr_reg = 0x%02x, rxd_syn = 0x%01x, erx_ctrl_reg = 0x%02x",
               dut.core.rxd, dut.core.rxd_reg, dut.core.rxd_byte_reg, dut.core.rxd_bit_ctr_reg,
               dut.core.rxd_bitrate_ctr_reg, dut.core.rxd_syn, dut.core.erx_ctrl_reg);
    end
  endtask // dump_dut_state



  //----------------------------------------------------------------
  // dump_tx_state()
  //
  // Dump the state of the tx engine.
  //----------------------------------------------------------------
  task dump_tx_state;
    begin
    end
  endtask // dump_dut_state


  //----------------------------------------------------------------
  // reset_dut()
  //----------------------------------------------------------------
  task reset_dut;
    begin
      $display("*** Toggle reset.");
      tb_reset_n = 0;
      #(2 * CLK_PERIOD);
      tb_reset_n = 1;
    end
  endtask // reset_dut


  //----------------------------------------------------------------
  // init_sim()
  //
  // Initialize all counters and testbed functionality as well
  // as setting the DUT inputs to defined values.
  //----------------------------------------------------------------
  task init_sim;
    begin
      cycle_ctr     = 0;
      error_ctr     = 0;
      tc_ctr        = 0;

      tb_clk        = 0;
      tb_reset_n    = 1;
      tb_rxd        = 1;
      tb_cs         = 0;
      tb_we         = 0;
      tb_address    = 8'h0;
      tb_write_data = 32'h0;

      txd_state     = 1;
    end
  endtask // init_sim


  //----------------------------------------------------------------
  // transmit_byte
  //
  // Transmit a byte of data to the DUT receive port.
  //----------------------------------------------------------------
  task transmit_byte(input [7 : 0] data);
    integer i;
    begin
      $display("*** Transmitting byte 0x%02x to the dut.", data);

      #10;

      // Start bit
      $display("*** Transmitting start bit.");
      tb_rxd = 0;
      #(CLK_PERIOD * dut.DEFAULT_BIT_RATE);

      // Send the bits LSB first.
      for (i = 0 ; i < 8 ; i = i + 1)
        begin
          $display("*** Transmitting data[%1d] = 0x%01x.", i, data[i]);
          tb_rxd = data[i];
          #(CLK_PERIOD * dut.DEFAULT_BIT_RATE);
        end

      // Send two stop bits. I.e. two bit times high (mark) value.
      $display("*** Transmitting two stop bits.");
      tb_rxd = 1;
      #(2 * CLK_PERIOD * dut.DEFAULT_BIT_RATE * dut.DEFAULT_STOP_BITS);
      $display("*** End of transmission.");
    end
  endtask // transmit_byte


  //----------------------------------------------------------------
  // check_transmit
  //
  // Transmits a byte and checks that it was captured internally
  // by the dut.
  //----------------------------------------------------------------
  task check_transmit(input [7 : 0] data);
    begin
      tc_ctr = tc_ctr + 1;

      transmit_byte(data);

      if (dut.core.rxd_byte_reg == data)
        begin
          $display("*** Correct data: 0x%01x captured by the dut.",
                   dut.core.rxd_byte_reg);
        end
      else
        begin
          $display("*** Incorrect data: 0x%01x captured by the dut Should be: 0x%01x.",
                   dut.core.rxd_byte_reg, data);
          error_ctr = error_ctr + 1;
        end
    end
  endtask // check_transmit


  //----------------------------------------------------------------
  // test_transmit
  //
  // Transmit a number of test bytes to the dut.
  //----------------------------------------------------------------
  task test_transmit;
    begin
      check_transmit(8'h55);
      check_transmit(8'h42);
      check_transmit(8'hde);
      check_transmit(8'had);
    end
  endtask // test_transmit


  //----------------------------------------------------------------
  // display_test_result()
  //
  // Display the accumulated test results.
  //----------------------------------------------------------------
  task display_test_result;
    begin
      if (error_ctr == 0)
        begin
          $display("*** All %02d test cases completed successfully", tc_ctr);
        end
      else
        begin
          $display("*** %02d test cases did not complete successfully.", error_ctr);
        end
    end
  endtask // display_test_result


  //----------------------------------------------------------------
  // uart_test
  // The main test functionality.
  //----------------------------------------------------------------
  initial
    begin : uart_test
      $display("   -- Testbench for uart core started --");

      init_sim();
      dump_dut_state();
      reset_dut();
      dump_dut_state();

      test_transmit();

      display_test_result();
      $display("*** Simulation done.");
      $finish;
    end // uart_test
endmodule // tb_uart

//======================================================================
// EOF tb_uart.v
//======================================================================
