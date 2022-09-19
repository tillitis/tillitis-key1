//======================================================================
//
// spram.v
// -------
// Module that encapsulates two of the SPRAM blocks in the Lattice
// iCE40UP 5K device. This creates a single 32-bit wide,
// 64 kByte large memory.
//
//
// Author: Joachim Strombergson
// Copyright (C) 2022 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only
//
//======================================================================

`default_nettype none

module spram(
	     input   wire          clk,
	     input   wire          rst_n,
             input   wire          cs,
	     input   wire [03 : 0] wen,
	     input   wire [13 : 0] addr,
	     input   wire [31 : 0] wdata,
             output  wire          ready,
	     output  wire [31 : 0] rdata
            );

  //----------------------------------------------------------------
  // Registers and wires.
  //----------------------------------------------------------------
  reg ready_reg;
  reg ready_new;


  //----------------------------------------------------------------
  //----------------------------------------------------------------
  assign ready = ready_reg;


  //----------------------------------------------------------------
  // SPRAM instances.
  //----------------------------------------------------------------
  SB_SPRAM256KA spram0(
		       .ADDRESS(addr[13:0]),
		       .DATAIN(wdata[15:0]),
		       .MASKWREN({wen[1], wen[1], wen[0], wen[0]}),
		       .WREN(wen[1]|wen[0]),
		       .CHIPSELECT(cs),
		       .CLOCK(clk),
		       .STANDBY(1'b0),
		       .SLEEP(1'b0),
		       .POWEROFF(1'b1),
		       .DATAOUT(rdata[15:0])
	              );

  SB_SPRAM256KA spram1(
		       .ADDRESS(addr[13:0]),
		       .DATAIN(wdata[31:16]),
		       .MASKWREN({wen[3], wen[3], wen[2], wen[2]}),
		       .WREN(wen[3]|wen[2]),
		       .CHIPSELECT(cs),
		       .CLOCK(clk),
		       .STANDBY(1'b0),
		       .SLEEP(1'b0),
		       .POWEROFF(1'b1),
		       .DATAOUT(rdata[31:16])
	              );


  //----------------------------------------------------------------
  // reg_update.
  //
  // Posedge triggered with synchronous, active low reset.
  // This simply creates a one cycle access delay to allow the
  // memory access to complete.
  //----------------------------------------------------------------
  always @(posedge clk)
    begin : reg_update
      if (!rst_n) begin
        ready_reg <= 1'h0;
      end
      else begin
        ready_reg <= cs;
      end
    end

endmodule // spram

//======================================================================
// EOF spram.v
//======================================================================
