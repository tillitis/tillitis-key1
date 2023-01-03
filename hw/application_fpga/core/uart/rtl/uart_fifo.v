//======================================================================
//
// uart_fifo.v
// -----------
// FIFO for rx and tx data buffering in the UART.
// The code should allocate a single EBR in a iCE40UP device.
//
//
// Author: Joachim Strombergson
// Copyright (c) 2022, Tillitis AB
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

module uart_fifo(
		 input wire          clk,
		 input wire          reset_n,

		 input wire          in_syn,
		 input wire [7 : 0]  in_data,
		 output wire         in_ack,

		 output wire [8 : 0] fifo_bytes,

		 output wire         out_syn,
		 output wire [7 : 0] out_data,
		 input wire          out_ack
		 );


  //----------------------------------------------------------------
  // Registers including update variables and write enable.
  //----------------------------------------------------------------
  reg [7 : 0]  fifo_mem [0 : 511];
  reg          fifo_mem_we;

  reg [8: 0]   in_ptr_reg;
  reg [8: 0]   in_ptr_new;
  reg          in_ptr_we;

  reg [8: 0]   out_ptr_reg;
  reg [8: 0]   out_ptr_new;
  reg          out_ptr_we;

  reg [8: 0]   byte_ctr_reg;
  reg [8: 0]   byte_ctr_new;
  reg          byte_ctr_inc;
  reg          byte_ctr_dec;
  reg          byte_ctr_we;

  reg          in_ack_reg;
  reg          in_ack_new;


  //----------------------------------------------------------------
  // Wires
  //----------------------------------------------------------------
  reg fifo_empty;
  reg fifo_full;


  //----------------------------------------------------------------
  // Concurrent connectivity for ports etc.
  //----------------------------------------------------------------
  assign in_ack     = in_ack_reg;
  assign out_syn    = ~fifo_empty;
  assign out_data   = fifo_mem[out_ptr_reg];
  assign fifo_bytes = byte_ctr_reg;


  //----------------------------------------------------------------
  // reg_update
  //----------------------------------------------------------------
  always @ (posedge clk)
    begin: reg_update
      if (!reset_n) begin
	in_ptr_reg   <= 9'h0;
	out_ptr_reg  <= 9'h0;
	byte_ctr_reg <= 9'h0;
	in_ack_reg   <= 1'h0;
      end
      else begin
	in_ack_reg <= in_ack_new;

        if (fifo_mem_we) begin
          fifo_mem[in_ptr_reg] <= in_data;
        end

        if (in_ptr_we) begin
          in_ptr_reg <= in_ptr_new;
        end

        if (out_ptr_we) begin
          out_ptr_reg <= out_ptr_new;
        end

        if (byte_ctr_we) begin
          byte_ctr_reg  <= byte_ctr_new;
        end
      end
    end // reg_update


  //----------------------------------------------------------------
  // byte_ctr
  //----------------------------------------------------------------
  always @*
    begin : byte_ctr
      fifo_empty   = 1'h0;
      fifo_full    = 1'h0;
      byte_ctr_new = 9'h0;
      byte_ctr_we  = 1'h0;

      if (byte_ctr_reg == 9'h0) begin
	fifo_empty = 1'h1;
      end

      if (byte_ctr_reg == 9'h1ff) begin
	fifo_full = 1'h1;
      end

      if ((byte_ctr_inc) && (!byte_ctr_dec)) begin
	byte_ctr_new = byte_ctr_reg + 1'h1;
	byte_ctr_we = 1'h1;
      end

      else if ((!byte_ctr_inc) && (byte_ctr_dec)) begin
	byte_ctr_new = byte_ctr_reg - 1'h1;
	byte_ctr_we = 1'h1;
      end
    end


  //----------------------------------------------------------------
  // in_logic
  //----------------------------------------------------------------
  always @*
    begin : in_logic
      fifo_mem_we  = 1'h0;
      in_ack_new   = 1'h0;
      byte_ctr_inc = 1'h0;
      in_ptr_we    = 1'h0;

      in_ptr_new   = in_ptr_reg + 1'h1;

      if ((in_syn) && (!in_ack) && (!fifo_full)) begin
	fifo_mem_we  = 1'h1;
	in_ack_new   = 1'h1;
	byte_ctr_inc = 1'h1;
	in_ptr_we    = 1'h1;
      end
    end


  //----------------------------------------------------------------
  // out_logic
  //----------------------------------------------------------------
  always @*
    begin : out_logic
      byte_ctr_dec = 1'h0;
      out_ptr_we   = 1'h0;

      out_ptr_new  = out_ptr_reg + 1'h1;

      if ((out_ack) && (!fifo_empty)) begin
	byte_ctr_dec = 1'h1;
	out_ptr_we   = 1'h1;
      end
    end

endmodule // uart_fifo

//======================================================================
// EOF uart_fifo.v
//======================================================================
