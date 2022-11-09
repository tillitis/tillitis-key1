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
  reg [31 : 0] tmp_read_data;
  reg [31 : 0] mem_read_data;
  reg          ready_reg;
  reg          fw_app_cs;


  //----------------------------------------------------------------
  // Concurrent assignment of ports.
  //----------------------------------------------------------------
  assign read_data = tmp_read_data;
  assign ready     = ready_reg;
  assign fw_app_cs = cs && ~fw_app_mode;


  //----------------------------------------------------------------
  // Block RAM instances.
  //----------------------------------------------------------------
  SB_RAM40_4K fw_ram0(
		      .RDATA(mem_read_data[15 : 0]),
		      .RADDR({3'h0, address}),
		      .RCLK(clk),
		      .RCLKE(1'h1),
		      .RE(fw_app_cs),
		      .WADDR({3'h0, address}),
		      .WCLK(clk),
		      .WCLKE(1'h1),
		      .WDATA(write_data[15 : 0]),
		      .WE((|we && fw_app_cs)),
		      .MASK({{8{~we[1]}}, {8{~we[0]}}})
		     );


  SB_RAM40_4K fw_ram1(
		      .RDATA(mem_read_data[31 : 16]),
		      .RADDR({3'h0, address}),
		      .RCLK(clk),
		      .RCLKE(1'h1),
		      .RE(fw_app_cs),
		      .WADDR({3'h0, address}),
		      .WCLK(clk),
		      .WCLKE(1'h1),
		      .WDATA(write_data[31 : 16]),
		      .WE((|we && fw_app_cs)),
		      .MASK({{8{~we[3]}}, {8{~we[2]}}})
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


  //----------------------------------------------------------------
  // read_mux
  //----------------------------------------------------------------
  always @*
    begin : read_mux;
      if (fw_app_cs) begin
	tmp_read_data = mem_read_data;
      end else begin
	tmp_read_data = 32'h0;
      end
    end

endmodule // fw_ram

//======================================================================
// EOF fw_ram.v
//======================================================================
