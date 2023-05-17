//======================================================================
//
// tk1_spi_master.v
// ----------------
// Minimal SPI master to be integrated into the tk1 module.
// The SPI master is able to generate a clock, and transfer,
// exchange a single byte with the slave.
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
		      output wire  [7 : 0] spi_rx_data,
		      output wire          spi_ready
		     );


  //----------------------------------------------------------------
  // Internal constant and parameter definitions.
  //----------------------------------------------------------------
  parameter CTRL_IDLE   = 2'h0;
  parameter CTRL_WAIT0  = 2'h1;
  parameter CTRL_NEXT   = 2'h2;
  parameter CTRL_WAIT1  = 2'h3;


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
  reg          spi_data_load;
  reg          spi_data_shift;
  reg          spi_data_we;

  reg          spi_miso_sample0_reg;
  reg          spi_miso_sample1_reg;

  reg [3 : 0]  spi_clk_ctr_reg;
  reg [3 : 0]  spi_clk_ctr_new;
  reg          spi_clk_ctr_rst;

  reg [2 : 0]  spi_bit_ctr_reg;
  reg [2 : 0]  spi_bit_ctr_new;
  reg          spi_bit_ctr_rst;
  reg          spi_bit_ctr_inc;
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
  reg spi_tx_data_load;
  reg spi_tx_data_shift;


  //----------------------------------------------------------------
  // Concurrent connectivity for ports etc.
  //----------------------------------------------------------------
  assign spi_ss      = spi_ss_reg;
  assign spi_sck     = spi_csk_reg;
  assign spi_mosi    = spi_data_reg[7];
  assign spi_rx_data = spi_data_reg;
  assign spi_ready   = spi_ready_reg;


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
	spi_ctrl_reg    <= CTRL_IDLE;
      end

      else begin
	spi_miso_sample0_reg <= spi_miso;
	spi_miso_sample1_reg <= spi_miso_sample0_reg;
	spi_clk_ctr_reg      <= spi_clk_ctr_new;

	if (spi_enable_we) begin
	  spi_ss_reg <= ~spi_enable;
	end

	if (spi_csk_we) begin
	  spi_csk_reg <= spi_csk_new;
	end

	if (spi_data_we) begin
	  spi_data_reg <= spi_data_new;
	end

	if (spi_ready_we) begin
	  spi_ready_reg <= spi_ready_new;
	end

	if (spi_ctrl_we) begin
	  spi_ctrl_reg <= spi_ctrl_new;
	end
      end
    end // reg_update


  //----------------------------------------------------------------
  // clk_ctr
  //
  // Continuously running counter that can be reset to zero.
  //----------------------------------------------------------------
  always @*
    begin : clk_ctr
      if (spi_clk_ctr_rst) begin
	spi_clk_ctr_new = 4'h0;
      end
      else begin
	spi_clk_ctr_new = spi_clk_ctr_reg + 1'h1;
      end
    end


  //----------------------------------------------------------------
  // bit_ctr
  //----------------------------------------------------------------
  always @*
    begin : bit_ctr
      spi_bit_ctr_new = 3'h0;
      spi_bit_ctr_we  = 1'h0;

      if (spi_bit_ctr_rst) begin
	spi_bit_ctr_new = 3'h0;
	spi_bit_ctr_we  = 1'h1;
      end

      if (spi_bit_ctr_inc) begin
	spi_bit_ctr_new = spi_bit_ctr_reg + 1'h1;
	spi_bit_ctr_we  = 1'h1;
      end
    end


  //----------------------------------------------------------------
  // spi_data_mux
  //
  // Either load or shift the data  register.
  //----------------------------------------------------------------
  always @*
    begin : spi_data_mux
      spi_data_new = 8'h0;
      spi_data_we  = 1'h0;

      if (spi_tx_data_load) begin
	spi_data_new = spi_tx_data;
	spi_data_we  = 1'h1;
      end

      if (spi_data_shift) begin
	spi_data_new = {spi_data_reg[6 : 0], spi_miso_sample1_reg};
	spi_data_we  = 1'h1;
      end
    end


  //----------------------------------------------------------------
  // spi_master_ctrl
  //----------------------------------------------------------------
  always @*
    begin : spi_master_ctrl
      spi_tx_data_load  = 1'h0;
      spi_tx_data_shift = 1'h0;
      spi_clk_ctr_rst   = 1'h0;
      spi_csk_we        = 1'h0;
      spi_csk_new       = 1'h0;
      spi_bit_ctr_rst   = 1'h0;
      spi_bit_ctr_inc   = 1'h0;
      spi_ready_new     = 1'h0;
      spi_ready_we      = 1'h0;
      spi_ctrl_new      = CTRL_IDLE;
      spi_ctrl_we       = 1'h0;


      case (spi_ctrl_reg)
        CTRL_IDLE: begin
          if (spi_start) begin
	    spi_ready_new     = 1'h0;
	    spi_ready_we      = 1'h1;
	    spi_ctrl_new      = CTRL_WAIT1;
	    spi_ctrl_we       = 1'h1;
          end
	end


        CTRL_WAIT1: begin
	  spi_ready_new     = 1'h1;
	  spi_ready_we      = 1'h1;
	  spi_ctrl_new      = CTRL_IDLE;
	  spi_ctrl_we       = 1'h1;
	end

        default: begin
        end
      endcase // case (spi_ctrl_reg)
    end

endmodule // tk1_spi_master

//======================================================================
// EOF tk1_spi_master.v
//======================================================================
