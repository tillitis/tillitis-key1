//======================================================================
//
// uart_core.v
// -----------
// A simple universal asynchronous receiver/transmitter (UART)
// interface. The interface contains 16 byte wide transmit and
// receivea buffers and can handle start and stop bits. But in
// general is rather simple. The primary purpose is as host
// interface for the coretest design. The core also has a
// loopback mode to allow testing of a serial link.
//
// Note that the UART has a separate API interface to allow
// a control core to change settings such as speed. But the core
// has default values to allow it to start operating directly
// after reset. No config should be needed.
//
//
// Author: Joachim Strombergson
// Copyright (c) 2014, Secworks Sweden AB
//
// SPDX-License-Identifier: BSD-2-Clause
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

module uart_core(
                 input wire          clk,
                 input wire          reset_n,

                 // Configuration parameters
                 input wire [15 : 0] bit_rate,
                 input wire [3 : 0]  data_bits,
                 input wire [1 : 0]  stop_bits,

                 // External data interface
                 input wire          rxd,
                 output wire         txd,

                 // Internal receive interface.
                 output wire         rxd_syn,
                 output wire [7 : 0] rxd_data,
                 input wire          rxd_ack,

                 // Internal transmit interface.
                 input wire          txd_syn,
                 input wire [7 : 0]  txd_data,
                 output wire         txd_ready
                );


  //----------------------------------------------------------------
  // Internal constant and parameter definitions.
  //----------------------------------------------------------------
  parameter ERX_IDLE  = 0;
  parameter ERX_START = 1;
  parameter ERX_BITS  = 2;
  parameter ERX_STOP  = 3;
  parameter ERX_SYN   = 4;

  parameter ETX_IDLE  = 0;
  parameter ETX_ACK   = 1;
  parameter ETX_START = 2;
  parameter ETX_BITS  = 3;
  parameter ETX_STOP  = 4;


  //----------------------------------------------------------------
  // Registers including update variables and write enable.
  //----------------------------------------------------------------
  reg          rxd0_reg;
  reg          rxd_reg;

  reg [7 : 0]  rxd_byte_reg;
  reg          rxd_byte_we;

  reg [3 : 0]  rxd_bit_ctr_reg;
  reg [3 : 0]  rxd_bit_ctr_new;
  reg          rxd_bit_ctr_we;
  reg          rxd_bit_ctr_rst;
  reg          rxd_bit_ctr_inc;

  reg [15 : 0] rxd_bitrate_ctr_reg;
  reg [15 : 0] rxd_bitrate_ctr_new;
  reg          rxd_bitrate_ctr_we;
  reg          rxd_bitrate_ctr_rst;
  reg          rxd_bitrate_ctr_inc;

  reg          rxd_syn_reg;
  reg          rxd_syn_new;
  reg          rxd_syn_we;

  reg [2 : 0]  erx_ctrl_reg;
  reg [2 : 0]  erx_ctrl_new;
  reg          erx_ctrl_we;

  reg          txd_reg;
  reg          txd_new;
  reg          txd_we;

  reg [7 : 0]  txd_byte_reg;
  reg [7 : 0]  txd_byte_new;
  reg          txd_byte_we;

  reg [3 : 0]  txd_bit_ctr_reg;
  reg [3 : 0]  txd_bit_ctr_new;
  reg          txd_bit_ctr_we;
  reg          txd_bit_ctr_rst;
  reg          txd_bit_ctr_inc;

  reg [15 : 0] txd_bitrate_ctr_reg;
  reg [15 : 0] txd_bitrate_ctr_new;
  reg          txd_bitrate_ctr_we;
  reg          txd_bitrate_ctr_rst;
  reg          txd_bitrate_ctr_inc;

  reg          txd_ready_reg;
  reg          txd_ready_new;
  reg          txd_ready_we;

  reg [2 : 0]  etx_ctrl_reg;
  reg [2 : 0]  etx_ctrl_new;
  reg          etx_ctrl_we;


  //----------------------------------------------------------------
  // Wires.
  //----------------------------------------------------------------
  wire [15 : 0] half_bit_rate;


  //----------------------------------------------------------------
  // Concurrent connectivity for ports etc.
  //----------------------------------------------------------------
  assign txd      = txd_reg;
  assign rxd_syn  = rxd_syn_reg;
  assign rxd_data = rxd_byte_reg;
  assign txd_ready  = txd_ready_reg;

  assign half_bit_rate = {1'b0, bit_rate[15 : 1]};


  //----------------------------------------------------------------
  // reg_update
  //
  // Update functionality for all registers in the core.
  // All registers are positive edge triggered with
  // synchronous active low reset.
  //----------------------------------------------------------------
  always @ (posedge clk)
    begin: reg_update
      if (!reset_n) begin
        rxd0_reg            <= 1'b0;
        rxd_reg             <= 1'b0;
        rxd_byte_reg        <= 8'h0;
        rxd_bit_ctr_reg     <= 4'h0;
        rxd_bitrate_ctr_reg <= 16'h0;
        rxd_syn_reg         <= 0;
        erx_ctrl_reg        <= ERX_IDLE;

        txd_reg             <= 1'b1;
        txd_byte_reg        <= 8'h0;
        txd_bit_ctr_reg     <= 4'h0;
        txd_bitrate_ctr_reg <= 16'h0;
        txd_ready_reg       <= 1'b1;
        etx_ctrl_reg        <= ETX_IDLE;
      end

      else begin
        rxd0_reg <= rxd;
        rxd_reg  <= rxd0_reg;

        if (rxd_byte_we) begin
          rxd_byte_reg <= {rxd_reg, rxd_byte_reg[7 : 1]};
        end

        if (rxd_bit_ctr_we) begin
          rxd_bit_ctr_reg <= rxd_bit_ctr_new;
        end

        if (rxd_bitrate_ctr_we) begin
          rxd_bitrate_ctr_reg <= rxd_bitrate_ctr_new;
        end

        if (rxd_syn_we) begin
          rxd_syn_reg <= rxd_syn_new;
        end

        if (erx_ctrl_we) begin
          erx_ctrl_reg <= erx_ctrl_new;
        end

        if (txd_we) begin
          txd_reg <= txd_new;
        end

        if (txd_byte_we) begin
          txd_byte_reg <= txd_byte_new;
        end

        if (txd_bit_ctr_we) begin
          txd_bit_ctr_reg <= txd_bit_ctr_new;
        end

        if (txd_bitrate_ctr_we) begin
          txd_bitrate_ctr_reg <= txd_bitrate_ctr_new;
        end

        if (txd_ready_we) begin
          txd_ready_reg <= txd_ready_new;
        end

        if (etx_ctrl_we) begin
          etx_ctrl_reg <= etx_ctrl_new;
        end
      end
    end // reg_update


  //----------------------------------------------------------------
  // rxd_bit_ctr
  //
  // Bit counter for receiving data on the external
  // serial interface.
  //----------------------------------------------------------------
  always @*
    begin: rxd_bit_ctr
      rxd_bit_ctr_new = 4'h0;
      rxd_bit_ctr_we  = 1'b0;

      if (rxd_bit_ctr_rst) begin
        rxd_bit_ctr_new = 4'h0;
        rxd_bit_ctr_we  = 1'b1;
      end

      else if (rxd_bit_ctr_inc) begin
        rxd_bit_ctr_new = rxd_bit_ctr_reg + 1'h1;
        rxd_bit_ctr_we  = 1'b1;
      end
    end // rxd_bit_ctr


  //----------------------------------------------------------------
  // rxd_bitrate_ctr
  //
  // Bitrate counter for receiving data on the external
  // serial interface.
  //----------------------------------------------------------------
  always @*
    begin: rxd_bitrate_ctr
      rxd_bitrate_ctr_new = 16'h0;
      rxd_bitrate_ctr_we  = 1'h0;

      if (rxd_bitrate_ctr_rst) begin
        rxd_bitrate_ctr_new = 16'h0;
        rxd_bitrate_ctr_we  = 1'b1;
      end

      else if (rxd_bitrate_ctr_inc) begin
        rxd_bitrate_ctr_new = rxd_bitrate_ctr_reg + 1'h1;
        rxd_bitrate_ctr_we  = 1'b1;
      end
    end // rxd_bitrate_ctr



  //----------------------------------------------------------------
  // txd_bit_ctr
  //
  // Bit counter for transmitting data on the external
  // serial interface.
  //----------------------------------------------------------------
  always @*
    begin: txd_bit_ctr
      txd_bit_ctr_new = 4'h0;
      txd_bit_ctr_we  = 1'h0;

      if (txd_bit_ctr_rst) begin
        txd_bit_ctr_new = 4'h0;
        txd_bit_ctr_we  = 1'h1;
      end

      else if (txd_bit_ctr_inc) begin
        txd_bit_ctr_new = txd_bit_ctr_reg + 1'h1;
        txd_bit_ctr_we  = 1'b1;
      end
    end // txd_bit_ctr


  //----------------------------------------------------------------
  // txd_bitrate_ctr
  //
  // Bitrate counter for transmitting data on the external
  // serial interface.
  //----------------------------------------------------------------
  always @*
    begin: txd_bitrate_ctr
      txd_bitrate_ctr_new = 16'h0;
      txd_bitrate_ctr_we  = 0;

      if (txd_bitrate_ctr_rst) begin
        txd_bitrate_ctr_new = 16'h0;
        txd_bitrate_ctr_we  = 1;
      end

      else if (txd_bitrate_ctr_inc) begin
        txd_bitrate_ctr_new = txd_bitrate_ctr_reg + 1'h1;
        txd_bitrate_ctr_we  = 1;
      end
    end // txd_bitrate_ctr


  //----------------------------------------------------------------
  // external_rx_engine
  //
  // Logic that implements the receive engine towards
  // the external interface. Detects incoming data, collects it,
  // if required checks parity and store correct data into
  // the rx buffer.
  //----------------------------------------------------------------
  always @*
    begin: external_rx_engine
      rxd_bit_ctr_rst     = 0;
      rxd_bit_ctr_inc     = 0;
      rxd_bitrate_ctr_rst = 0;
      rxd_bitrate_ctr_inc = 0;
      rxd_byte_we         = 0;
      rxd_syn_new         = 0;
      rxd_syn_we          = 0;
      erx_ctrl_new        = ERX_IDLE;
      erx_ctrl_we         = 0;

      case (erx_ctrl_reg)
        ERX_IDLE: begin
          if (!rxd_reg) begin
            // Possible start bit detected.
            rxd_bitrate_ctr_rst = 1;
            erx_ctrl_new        = ERX_START;
            erx_ctrl_we         = 1;
          end
        end

        ERX_START: begin
          rxd_bitrate_ctr_inc = 1;
          if (rxd_reg) begin
            // Just a glitch.
            erx_ctrl_new = ERX_IDLE;
            erx_ctrl_we  = 1;
          end

          else begin
            if (rxd_bitrate_ctr_reg == half_bit_rate) begin
              // start bit assumed. We start sampling data.
              rxd_bit_ctr_rst     = 1;
              rxd_bitrate_ctr_rst = 1;
              erx_ctrl_new        = ERX_BITS;
              erx_ctrl_we         = 1;
            end
          end
        end


        ERX_BITS: begin
          if (rxd_bitrate_ctr_reg < bit_rate) begin
            rxd_bitrate_ctr_inc = 1;
          end

          else begin
            rxd_byte_we         = 1;
            rxd_bit_ctr_inc     = 1;
            rxd_bitrate_ctr_rst = 1;
            if (rxd_bit_ctr_reg == (data_bits - 1)) begin
              erx_ctrl_new = ERX_STOP;
              erx_ctrl_we  = 1;
            end
          end
        end


        ERX_STOP: begin
          rxd_bitrate_ctr_inc = 1;
          if (rxd_bitrate_ctr_reg == bit_rate * stop_bits) begin
            rxd_syn_new  = 1;
            rxd_syn_we   = 1;
            erx_ctrl_new = ERX_SYN;
            erx_ctrl_we  = 1;
          end
        end


        ERX_SYN: begin
          if (rxd_ack) begin
            rxd_syn_new  = 0;
            rxd_syn_we   = 1;
            erx_ctrl_new = ERX_IDLE;
            erx_ctrl_we  = 1;
            end
        end

        default: begin
        end

      endcase // case (erx_ctrl_reg)
    end // external_rx_engine


  //----------------------------------------------------------------
  // external_tx_engine
  //
  // Logic that implements the transmit engine towards
  // the external interface.
  //----------------------------------------------------------------
  always @*
    begin: external_tx_engine
      txd_new             = 0;
      txd_we              = 0;
      txd_byte_new        = 0;
      txd_byte_we         = 0;
      txd_bit_ctr_rst     = 0;
      txd_bit_ctr_inc     = 0;
      txd_bitrate_ctr_rst = 0;
      txd_bitrate_ctr_inc = 0;
      txd_ready_new       = 0;
      txd_ready_we        = 0;
      etx_ctrl_new        = ETX_IDLE;
      etx_ctrl_we         = 0;

      case (etx_ctrl_reg)
        ETX_IDLE: begin
          txd_new = 1;
          txd_we  = 1;
          if (txd_syn) begin
            txd_byte_new        = txd_data;
            txd_byte_we         = 1;
            txd_ready_new       = 0;
            txd_ready_we        = 1;
            txd_bitrate_ctr_rst = 1;
            etx_ctrl_new        = ETX_ACK;
            etx_ctrl_we         = 1;
          end
        end


        ETX_ACK: begin
          if (!txd_syn) begin
            txd_new      = 0;
            txd_we       = 1;
            etx_ctrl_new = ETX_START;
            etx_ctrl_we  = 1;
          end
        end


        ETX_START: begin
          if (txd_bitrate_ctr_reg == bit_rate) begin
            txd_bit_ctr_rst     = 1;
            etx_ctrl_new        = ETX_BITS;
            etx_ctrl_we         = 1;
          end

          else begin
            txd_bitrate_ctr_inc = 1;
          end
        end


        ETX_BITS: begin
          if (txd_bitrate_ctr_reg < bit_rate) begin
            txd_bitrate_ctr_inc = 1;
          end

          else begin
            txd_bitrate_ctr_rst = 1;
            if (txd_bit_ctr_reg == data_bits) begin
              txd_new      = 1;
              txd_we       = 1;
              etx_ctrl_new = ETX_STOP;
              etx_ctrl_we  = 1;
            end

            else begin
              txd_new         = txd_byte_reg[txd_bit_ctr_reg[2 : 0]];
              txd_we          = 1;
              txd_bit_ctr_inc = 1;
            end
          end
        end


        ETX_STOP: begin
          txd_bitrate_ctr_inc = 1;
          if (txd_bitrate_ctr_reg == bit_rate * stop_bits) begin
            txd_ready_new = 1;
            txd_ready_we  = 1;
            etx_ctrl_new  = ETX_IDLE;
            etx_ctrl_we   = 1;
          end
        end

        default: begin
        end

      endcase // case (etx_ctrl_reg)
    end // external_tx_engine

endmodule // uart

//======================================================================
// EOF uart.v
//======================================================================
