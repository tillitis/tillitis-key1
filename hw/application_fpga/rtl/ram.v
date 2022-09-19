//======================================================================
//
// ram.v
// -----
// Module that encapsulates the four SPRAM blocks in the Lattice
// iCE40UP 5K device. This creates a single 32-bit wide,
// 128 kByte large memory.
//
// Author: Joachim Strombergson
// Copyright (C) 2022 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only
//
//======================================================================

`default_nettype none

module ram(
	   input   wire          clk,
	   input   wire          reset_n,
           input   wire          cs,
	   input   wire [03 : 0] we,
	   input   wire [14 : 0] address,
	   input   wire [31 : 0] write_data,
	   output  wire [31 : 0] read_data,
	   output  wire          ready
          );


  //----------------------------------------------------------------
  // Registers and wires.
  //----------------------------------------------------------------
  reg ready_reg;

  reg          cs0;
  reg          cs1;
  reg [31 : 0] read_data0;
  reg [31 : 0] read_data1;
  reg [31 : 0] muxed_read_data;


  //----------------------------------------------------------------
  // Concurrent assignment of ports.
  //----------------------------------------------------------------
  assign read_data = muxed_read_data;
  assign ready     = ready_reg;


  //----------------------------------------------------------------
  // SPRAM instances.
  //----------------------------------------------------------------
  SB_SPRAM256KA spram0(
		       .ADDRESS(address[13:0]),
		       .DATAIN(write_data[15:0]),
		       .MASKWREN({we[1], we[1], we[0], we[0]}),
		       .WREN(we[1] | we[0]),
		       .CHIPSELECT(cs0),
		       .CLOCK(clk),
		       .STANDBY(1'b0),
		       .SLEEP(1'b0),
		       .POWEROFF(1'b1),
		       .DATAOUT(read_data0[15:0])
	              );

  SB_SPRAM256KA spram1(
		       .ADDRESS(address[13:0]),
		       .DATAIN(write_data[31:16]),
		       .MASKWREN({we[3], we[3], we[2], we[2]}),
		       .WREN(we[3] | we[2]),
		       .CHIPSELECT(cs0),
		       .CLOCK(clk),
		       .STANDBY(1'b0),
		       .SLEEP(1'b0),
		       .POWEROFF(1'b1),
		       .DATAOUT(read_data0[31:16])
	              );


  SB_SPRAM256KA spram2(
		       .ADDRESS(address[13:0]),
		       .DATAIN(write_data[15:0]),
		       .MASKWREN({we[1], we[1], we[0], we[0]}),
		       .WREN(we[1] | we[0]),
		       .CHIPSELECT(cs1),
		       .CLOCK(clk),
		       .STANDBY(1'b0),
		       .SLEEP(1'b0),
		       .POWEROFF(1'b1),
		       .DATAOUT(read_data1[15:0])
	              );

  SB_SPRAM256KA spram3(
		       .ADDRESS(address[13:0]),
		       .DATAIN(write_data[31:16]),
		       .MASKWREN({we[3], we[3], we[2], we[2]}),
		       .WREN(we[3] | we[2]),
		       .CHIPSELECT(cs1),
		       .CLOCK(clk),
		       .STANDBY(1'b0),
		       .SLEEP(1'b0),
		       .POWEROFF(1'b1),
		       .DATAOUT(read_data1[31:16])
	              );


  //----------------------------------------------------------------
  // reg_update
  //
  // Posedge triggered with synchronous, active low reset.
  // This simply creates a one cycle access latency to match
  // the latency of the spram blocks.
  //----------------------------------------------------------------
  always @(posedge clk)
    begin : reg_update
      if (!reset_n) begin
        ready_reg <= 1'h0;
      end
      else begin
        ready_reg <= cs;
      end
    end


  //----------------------------------------------------------------
  // mem_mux
  //----------------------------------------------------------------
  always @*
    begin : mem_mux
      cs0 = 1'h0;
      cs1 = 1'h0;

      if (address[14]) begin
        cs1             = cs;
        muxed_read_data = read_data1;
      end else begin
        cs0             = cs;
        muxed_read_data = read_data0;
      end
    end

endmodule // ram

//======================================================================
// EOF ram.v
//======================================================================
