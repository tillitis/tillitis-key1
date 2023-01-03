//======================================================================
//
// uart.v
// ------
// Top level wrapper for the uart core.
//
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
// SPDX-License-Identifier: BSD-2-Clause
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

module uart(
            input wire           clk,
            input wire           reset_n,

            input wire           rxd,
            output wire          txd,

            input wire           cs,
            input wire           we,
            input wire [7 : 0]   address,
  /* verilator lint_off UNUSED */
            input wire [31 : 0]  write_data,
  /* verilator lint_on UNUSED */
            output wire [31 : 0] read_data,
            output wire          ready
           );


  //----------------------------------------------------------------
  // Internal constant and parameter definitions.
  //----------------------------------------------------------------
  localparam ADDR_BIT_RATE     = 8'h10;
  localparam ADDR_DATA_BITS    = 8'h11;
  localparam ADDR_STOP_BITS    = 8'h12;

  localparam ADDR_RX_STATUS    = 8'h20;
  localparam ADDR_RX_DATA      = 8'h21;
  localparam ADDR_RX_BYTES     = 8'h22;

  localparam ADDR_TX_STATUS    = 8'h40;
  localparam ADDR_TX_DATA      = 8'h41;

  // The default bit rate is based on target clock frequency
  // divided by the bit rate times in order to hit the
  // center of the bits. I.e.
  // Clock: 18 MHz, 62500 bps
  // Divisor = 18E6 / 62500 = 288
  // This also satisfies 1E6 % bps == 0 for the CH552 MCU used for USB-serial
  localparam DEFAULT_BIT_RATE  = 16'd288;
  localparam DEFAULT_DATA_BITS = 4'h8;
  localparam DEFAULT_STOP_BITS = 2'h1;


  //----------------------------------------------------------------
  // Registers including update variables and write enable.
  //----------------------------------------------------------------
  reg [15 : 0] bit_rate_reg;
  reg          bit_rate_we;

  reg [3 : 0]  data_bits_reg;
  reg          data_bits_we;

  reg [1 : 0]  stop_bits_reg;
  reg          stop_bits_we;


  //----------------------------------------------------------------
  // Wires.
  //----------------------------------------------------------------
  wire          core_rxd_syn;
  wire [7 : 0]  core_rxd_data;
  wire          core_rxd_ack;

  reg           core_txd_syn;
  reg [7 : 0]   core_txd_data;
  wire          core_txd_ready;

  wire          fifo_out_syn;
  wire [7 : 0]  fifo_out_data;
  reg           fifo_out_ack;
  wire [8 : 0]  fifo_bytes;

  reg [31 : 0]  tmp_read_data;
  reg           tmp_ready;


  //----------------------------------------------------------------
  // Concurrent connectivity for ports etc.
  //----------------------------------------------------------------
  assign read_data = tmp_read_data;
  assign ready     = tmp_ready;


  //----------------------------------------------------------------
  // Module instantiations.
  //----------------------------------------------------------------
  uart_core core(
                 .clk(clk),
                 .reset_n(reset_n),

                 // Configuration parameters
                 .bit_rate(bit_rate_reg),
                 .data_bits(data_bits_reg),
                 .stop_bits(stop_bits_reg),

                 // External data interface
                 .rxd(rxd),
                 .txd(txd),

                 // Internal receive interface.
                 .rxd_syn(core_rxd_syn),
                 .rxd_data(core_rxd_data),
                 .rxd_ack(core_rxd_ack),

                 // Internal transmit interface.
                 .txd_syn(core_txd_syn),
                 .txd_data(core_txd_data),
                 .txd_ready(core_txd_ready)
                 );


  uart_fifo fifo(
		 .clk(clk),
		 .reset_n(reset_n),

		 .in_syn(core_rxd_syn),
		 .in_data(core_rxd_data),
		 .in_ack(core_rxd_ack),

		 .fifo_bytes(fifo_bytes),

		 .out_syn(fifo_out_syn),
		 .out_data(fifo_out_data),
		 .out_ack(fifo_out_ack)
		);


  //----------------------------------------------------------------
  // reg_update
  //
  // Update functionality for all registers in the core.
  // All registers are positive edge triggered with synchronous
  // active low reset.
  //----------------------------------------------------------------
  always @ (posedge clk)
    begin: reg_update
      if (!reset_n) begin
        bit_rate_reg  <= DEFAULT_BIT_RATE;
        data_bits_reg <= DEFAULT_DATA_BITS;
        stop_bits_reg <= DEFAULT_STOP_BITS;
      end
      else begin
        if (bit_rate_we) begin
          bit_rate_reg  <= write_data[15 : 0];
        end

        if (data_bits_we) begin
          data_bits_reg  <= write_data[3 : 0];
        end

        if (stop_bits_we) begin
          stop_bits_reg  <= write_data[1 : 0];
        end
      end
    end // reg_update


  //----------------------------------------------------------------
  // api
  //
  // The core API that allows an internal host to control the
  // core functionality.
  //----------------------------------------------------------------
  always @*
    begin: api
      // Default assignments.
      bit_rate_we   = 1'h0;
      data_bits_we  = 1'h0;
      stop_bits_we  = 1'h0;
      core_txd_syn  = 1'h0;
      fifo_out_ack  = 1'h0;
      tmp_read_data = 32'h0;
      tmp_ready     = 1'h0;

      core_txd_data = write_data[7 : 0];

      if (cs) begin
	tmp_ready = 1'h1;

        if (we) begin
          case (address)
            ADDR_BIT_RATE: begin
              bit_rate_we  = 1;
            end

            ADDR_DATA_BITS: begin
              data_bits_we  = 1;
            end

            ADDR_STOP_BITS: begin
              stop_bits_we  = 1;
            end

	    ADDR_TX_DATA: begin
	      if (core_txd_ready) begin
		core_txd_syn = 1'h1;
	      end
	    end

            default: begin
            end
          endcase // case (address)
        end

	else begin
          case (address)
            ADDR_BIT_RATE: begin
              tmp_read_data = {16'h0, bit_rate_reg};
            end

            ADDR_DATA_BITS: begin
              tmp_read_data = {28'h0, data_bits_reg};
            end

            ADDR_STOP_BITS: begin
              tmp_read_data = {30'h0, stop_bits_reg};
            end

	    ADDR_RX_STATUS: begin
              tmp_read_data = {31'h0, fifo_out_syn};
	    end

	    ADDR_RX_DATA: begin
	      fifo_out_ack  = 1'h1;
              tmp_read_data = {24'h0, fifo_out_data};
	    end

	    ADDR_RX_BYTES: begin
              tmp_read_data = {23'h0, fifo_bytes};
	    end

	    ADDR_TX_STATUS: begin
              tmp_read_data = {31'h0, core_txd_ready};
	    end

            default: begin
            end
          endcase // case (address)
        end
      end
    end

endmodule // uart

//======================================================================
// EOF uart.v
//======================================================================
