//======================================================================
//
// uds.v
// --------
// Top level wrapper for the uds core.
//
//
// Author: Joachim Strombergson
// SPDX-FileCopyrightText: 2022 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause
//
//======================================================================

`default_nettype none

module uds (
    input wire clk,
    input wire reset_n,

    input  wire          en,
    input  wire          cs,
    input  wire [ 2 : 0] address,
    output wire [31 : 0] read_data,
    output wire          ready
);


  //----------------------------------------------------------------
  // Registers including update variables and write enable.
  //----------------------------------------------------------------
  reg          uds_rd_reg    [0 : 7];
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
  // uds rom instance.
  //----------------------------------------------------------------
  uds_rom rom_i (
      .addr(address),
      .re  (uds_rd_we),
      .data(tmp_read_data)
  );



  //----------------------------------------------------------------
  // reg_update
  //----------------------------------------------------------------
  always @(posedge clk) begin : reg_update
    integer i;

    if (!reset_n) begin
      for (i = 0; i < 8; i = i + 1) begin
        uds_rd_reg[i] <= 1'h0;
      end
    end
    else begin
      if (uds_rd_we) begin
        uds_rd_reg[address[2 : 0]] <= 1'h1;
      end
    end
  end  // reg_update


  //----------------------------------------------------------------
  // api
  //
  // The interface command decoding logic.
  //----------------------------------------------------------------
  always @* begin : api
    uds_rd_we = 1'h0;
    tmp_ready = 1'h0;

    if (cs) begin
      tmp_ready = 1'h1;

      if (en) begin
        if (uds_rd_reg[address[2 : 0]] == 1'h0) begin
          uds_rd_we = 1'h1;
        end
      end
    end
  end
endmodule  // uds

//======================================================================
// EOF uds.v
//======================================================================
