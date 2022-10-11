//======================================================================
//
// fw_ram.v
// --------
// A small 512 x 32 RAM for FW use. With support for access control.
//
// Author: Joachim Strombergson
// Copyright (C) 2022 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only
//
//======================================================================

`default_nettype none

module fw_ram(
	      input  wire          clk,
	      input  wire          reset_n,

	      input  wire          fw_app_mode,

              input  wire          cs,
	      input  wire [3 : 0]  we,
	      input  wire [7 : 0]  address,
	      input  wire [31 : 0] write_data,
	      output wire [31 : 0] read_data,
	      output wire          ready
             );


  //----------------------------------------------------------------
  // Registers and wires.
  //----------------------------------------------------------------
  reg          ready_reg;


  //----------------------------------------------------------------
  // Concurrent assignment of ports.
  //----------------------------------------------------------------
  assign ready     = ready_reg;


  //----------------------------------------------------------------
  // Block RAM instances.
  //----------------------------------------------------------------
  SB_RAM40_4K fw_ram0(
		      .RDATA(read_data[15:0]),
		      .RADDR(address),
		      .RCLK(clk),
		      .RCLKE(1'h1),
		      .RE(cs),
		      .WADDR(address),
		      .WCLK(clk),
		      .WCLKE(1'h1),
		      .WDATA(write_data[15:0]),
		      .WE(|we),
		      .MASK({{4{we[1]}}, {4{we[0]}}})
		     );


  SB_RAM40_4K fw_ram1(
		      .RDATA(read_data[31:16]),
		      .RADDR(address),
		      .RCLK(clk),
		      .RCLKE(1'h1),
		      .RE(cs),
		      .WADDR(address),
		      .WCLK(clk),
		      .WCLKE(1'h1),
		      .WDATA(write_data[31:16]),
		      .WE(|we),
		      .MASK({{4{we[3]}}, {4{we[2]}}})
		     );


  //----------------------------------------------------------------
  // reg_update
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

endmodule // fw_ram

//======================================================================
// EOF fw_ram.v
//======================================================================
