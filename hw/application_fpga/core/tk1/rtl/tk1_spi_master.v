//======================================================================
//
// tk1_spi_master.v
// ----------------
// SPI master integrated into the tk1 module.
//
//
// Author: Joachim Strombergson
// Copyright (C) 2023 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only
//
//======================================================================

`default_nettype none

module tk1_spi_master(
		      input wire           clk,
		      input wire           reset_n,

		      output wire          spi_ss,
		      output wire          spi_sck,
		      output wire          spi_mosi,
		      input wire           spi_miso,

		      input wire           spi_enable,
		      input wire           spi_enable_we,
		      input wire           spi_start,
		      input wire  [7 : 0]  spi_tx_data,
		      input wire           spi_tx_data_we,
		      input wire  [7 : 0]  spi_rx_data,
		      output wire          spi_ready
		     );


  //----------------------------------------------------------------
  // Internal constant and parameter definitions.
  //----------------------------------------------------------------


  //----------------------------------------------------------------
  // Registers including update variables and write enable.
  //----------------------------------------------------------------
  reg          spi_ss_reg;
  reg          spi_ss_we;

  reg          spi_csk_reg;
  reg          spi_csk_new;
  reg          spi_csk_we;

  reg [7 : 0]  spi_data_reg;
  reg [7 : 0]  spi_data_new;
  reg          spi_data_we;

  reg          spi_miso_sample0_reg;
  reg          spi_miso_sample1_reg;

  reg [3 : 0]  spi_clk_ctr_reg;
  reg [3 : 0]  spi_clk_ctr_new;
  reg          spi_clk_ctr_set;
  reg          spi_clk_ctr_we;

  reg [2 : 0]  spi_bit_ctr_reg;
  reg [2 : 0]  spi_bit_ctr_new;
  reg          spi_bit_ctr_rst;
  reg          spi_bit_ctr_we;

  reg          spi_ready_reg;
  reg          spi_ready_new;
  reg          spi_ready_we;

  reg [1 : 0]  spi_ctrl_reg;
  reg [1 : 0]  spi_ctrl_new;
  reg          spi_ctrl_we;


  //----------------------------------------------------------------
  // Wires.
  //----------------------------------------------------------------


  //----------------------------------------------------------------
  // Concurrent connectivity for ports etc.
  //----------------------------------------------------------------
  assign spi_ss   = spi_ss_reg;
  assign spi_sck  = spi_csk_reg;
  assign spi_mosi = spi_data_reg[7];


  //----------------------------------------------------------------
  // reg_update
  //----------------------------------------------------------------
  always @ (posedge clk)
    begin : reg_update
      if (!reset_n) begin
	spi_ss_reg      <= 1'h1;
	spi_csk_reg     <= 1'h0;
	spi_data_reg    <= 8'h0;
	spi_clk_ctr_reg <= 4'h0;
	spi_bit_ctr_reg <= 3'h0;
	spi_ready_reg   <= 1'h1;
      end

      else begin
	spi_miso_sample0_reg <= spi_miso;
	spi_miso_sample1_reg <= spi_miso_sample0_reg;

	if (spi_enable_we) begin
	  spi_ss_reg <= ~spi_enable;
	end

	if (spi_csk_we) begin
	  spi_csk_reg <= spi_csk_new;
	end

	if (spi_data_we) begin
	  spi_data_reg <= spi_data_new;
	end
      end
    end // reg_update


  //----------------------------------------------------------------
  // spi_master_ctrl
  //----------------------------------------------------------------
  always @*
    begin : spi_master_ctrl

    end

endmodule // tk1_spi_master

//======================================================================
// EOF tk1_spi_master.v
//======================================================================
