//======================================================================
//
// uds.v
// --------
// Top level wrapper for the uds core.
//
//
// Author: Joachim Strombergson
// Copyright (C) 2022 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only
//
//======================================================================

`default_nettype none

module uds(
           input wire           clk,
           input wire           reset_n,

	   input wire           fw_app_mode,

           input wire           cs,
           input wire  [7 : 0]  address,
           output wire [31 : 0] read_data,
           output wire          ready
          );


  //----------------------------------------------------------------
  // Internal constant and parameter definitions.
  //----------------------------------------------------------------
  localparam ADDR_NAME0        = 8'h00;
  localparam ADDR_NAME1        = 8'h01;
  localparam ADDR_VERSION      = 8'h02;

  localparam ADDR_UDS_FIRST    = 8'h10;
  localparam ADDR_UDS_LAST     = 8'h17;

  localparam CORE_NAME0        = 32'h7564735f; // "uds_"
  localparam CORE_NAME1        = 32'h6d656d20; // "mem "
  localparam CORE_VERSION      = 32'h00000001;


  //----------------------------------------------------------------
  // Registers including update variables and write enable.
  //----------------------------------------------------------------
  reg [31 : 0] uds_reg [0 : 7];
  initial $readmemh(`UDS_HEX, uds_reg);

  reg          uds_rd_reg [0 : 7];
  reg          uds_rd_we;


  //----------------------------------------------------------------
  // Wires.
  //----------------------------------------------------------------
  reg [31 : 0] tmp_read_data;
  reg          tmp_ready;


  //----------------------------------------------------------------
  // Concurrent connectivity for ports etc.
  //----------------------------------------------------------------
  assign read_data = tmp_read_data;
  assign ready     = tmp_ready;


  //----------------------------------------------------------------
  // reg_update
  //----------------------------------------------------------------
  always @ (posedge clk)
    begin : reg_update
      integer i;

      if (!reset_n) begin
        for (i = 0 ; i < 8 ; i = i + 1) begin
	  uds_rd_reg[i] <= 1'h0;;
        end
      end
      else begin
	if (uds_rd_we) begin
	  uds_rd_reg[address[2 : 0]] <= 1'h1;
	end
      end
    end // reg_update


  //----------------------------------------------------------------
  // api
  //
  // The interface command decoding logic.
  //----------------------------------------------------------------
  always @*
    begin : api
      uds_rd_we     = 1'h0;
      tmp_read_data = 32'h0;
      tmp_ready     = 1'h0;

      if (cs) begin
	tmp_ready = 1'h1;

	if (address == ADDR_NAME0) begin
	  tmp_read_data = CORE_NAME0;
        end

	if (address == ADDR_NAME1) begin
	  tmp_read_data = CORE_NAME1;
	end

	if (address == ADDR_VERSION) begin
	  tmp_read_data = CORE_VERSION;
	end

	if ((address >= ADDR_UDS_FIRST) && (address <= ADDR_UDS_LAST)) begin
	  if (!fw_app_mode) begin
            if (uds_rd_reg[address[2 : 0]] == 1'h0) begin
              tmp_read_data = uds_reg[address[2 : 0]];
              uds_rd_we     = 1'h1;
            end
	  end
        end
      end
    end

endmodule // uds

//======================================================================
// EOF uds.v
//======================================================================
