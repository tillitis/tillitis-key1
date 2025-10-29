//======================================================================
//
// tk1_spi_master.v
// ----------------
// Minimal SPI master to be integrated into the tk1 module.
// The SPI master is able to generate a clock, and transfer,
// exchange a single byte with the slave.
//
// This master is compatible with the Winbond W25Q80DV memory.
// This means that MSB of a response from the memory is provided
// on the falling clock edge on the LSB of the command byte, not
// on a dummy byte. This means that the response spans the boundary
// of the bytes, The core handles this by sampling the MISO
// just prior to settint the positive clock flank at the start
// of a byte transfer.
//
//
// Author: Joachim Strombergson
// Copyright (C) 2023 - Tillitis AB
// SPDX-License-Identifier: BSD-2-Clause
//
//======================================================================

`default_nettype none

module tk1_spi_master (
    input wire clk,
    input wire reset_n,

    output wire spi_ss,
    output wire spi_sck,
    output wire spi_mosi,
    input  wire spi_miso,

    input  wire         spi_enable,
    input  wire         spi_enable_vld,
    input  wire         spi_start,
    input  wire [7 : 0] spi_tx_data,
    input  wire         spi_tx_data_vld,
    output wire [7 : 0] spi_rx_data,
    output wire         spi_ready
);


  //----------------------------------------------------------------
  // Internal constant and parameter definitions.
  //----------------------------------------------------------------
  localparam CTRL_IDLE = 3'h0;
  localparam CTRL_POS_FLANK = 3'h1;
  localparam CTRL_NEG_FLANK = 3'h2;
  localparam CTRL_NEXT = 3'h3;


  //----------------------------------------------------------------
  // Registers including update variables and write enable.
  //----------------------------------------------------------------
  reg         spi_ss_reg;

  reg         spi_csk_reg;
  reg         spi_csk_new;
  reg         spi_csk_we;

  reg [7 : 0] spi_tx_data_reg;
  reg [7 : 0] spi_tx_data_new;
  reg         spi_tx_data_nxt;
  reg         spi_tx_data_we;

  reg [7 : 0] spi_rx_data_reg;
  reg [7 : 0] spi_rx_data_new;
  reg         spi_rx_data_nxt;
  reg         spi_rx_data_we;

  reg         spi_miso_sample_reg;

  reg [2 : 0] spi_bit_ctr_reg;
  reg [2 : 0] spi_bit_ctr_new;
  reg         spi_bit_ctr_rst;
  reg         spi_bit_ctr_inc;
  reg         spi_bit_ctr_we;

  reg         spi_ready_reg;
  reg         spi_ready_new;
  reg         spi_ready_we;

  reg [2 : 0] spi_ctrl_reg;
  reg [2 : 0] spi_ctrl_new;
  reg         spi_ctrl_we;


  //----------------------------------------------------------------
  // Concurrent connectivity for ports etc.
  //----------------------------------------------------------------
  assign spi_ss      = spi_ss_reg;
  assign spi_sck     = spi_csk_reg;
  assign spi_mosi    = spi_tx_data_reg[7];
  assign spi_rx_data = spi_rx_data_reg;
  assign spi_ready   = spi_ready_reg;


  //----------------------------------------------------------------
  // reg_update
  //----------------------------------------------------------------
  always @(posedge clk) begin : reg_update
    if (!reset_n) begin
      spi_ss_reg          <= 1'h1;
      spi_csk_reg         <= 1'h0;
      spi_miso_sample_reg <= 1'h0;
      spi_tx_data_reg     <= 8'h0;
      spi_rx_data_reg     <= 8'h0;
      spi_bit_ctr_reg     <= 3'h0;
      spi_ready_reg       <= 1'h1;
      spi_ctrl_reg        <= CTRL_IDLE;
    end

    else begin
      spi_miso_sample_reg <= spi_miso;

      if (spi_enable_vld) begin
        spi_ss_reg <= ~spi_enable;
      end

      if (spi_csk_we) begin
        spi_csk_reg <= spi_csk_new;
      end

      if (spi_tx_data_we) begin
        spi_tx_data_reg <= spi_tx_data_new;
      end

      if (spi_rx_data_we) begin
        spi_rx_data_reg <= spi_rx_data_new;
      end

      if (spi_ready_we) begin
        spi_ready_reg <= spi_ready_new;
      end

      if (spi_bit_ctr_we) begin
        spi_bit_ctr_reg <= spi_bit_ctr_new;
      end

      if (spi_ctrl_we) begin
        spi_ctrl_reg <= spi_ctrl_new;
      end
    end
  end  // reg_update


  //----------------------------------------------------------------
  // bit_ctr
  //----------------------------------------------------------------
  always @* begin : bit_ctr
    spi_bit_ctr_new = 3'h0;
    spi_bit_ctr_we  = 1'h0;

    if (spi_bit_ctr_rst) begin
      spi_bit_ctr_new = 3'h0;
      spi_bit_ctr_we  = 1'h1;
    end

    else if (spi_bit_ctr_inc) begin
      spi_bit_ctr_new = spi_bit_ctr_reg + 1'h1;
      spi_bit_ctr_we  = 1'h1;
    end
  end


  //----------------------------------------------------------------
  // spi_tx_data_logic
  //
  // Logic for the tx_data shift register.
  // Either load or shift the data register.
  //----------------------------------------------------------------
  always @* begin : spi_tx_data_logic
    spi_tx_data_new = 8'h0;
    spi_tx_data_we  = 1'h0;

    if (spi_tx_data_vld) begin
      if (spi_ready_reg) begin
        spi_tx_data_new = spi_tx_data;
        spi_tx_data_we  = 1'h1;
      end
    end

    if (spi_tx_data_nxt) begin
      spi_tx_data_new = {spi_tx_data_reg[6 : 0], 1'h0};
      spi_tx_data_we  = 1'h1;
    end
  end


  //----------------------------------------------------------------
  // spi_rx_data_logic
  // Logic for the rx_data shift register.
  //----------------------------------------------------------------
  always @* begin : spi_rx_data_logic
    spi_rx_data_new = 8'h0;
    spi_rx_data_we  = 1'h0;

    if (spi_ss) begin
      spi_rx_data_new = 8'h0;
      spi_rx_data_we  = 1'h1;
    end

    else if (spi_rx_data_nxt) begin
      spi_rx_data_new = {spi_rx_data_reg[6 : 0], spi_miso_sample_reg};
      spi_rx_data_we  = 1'h1;
    end
  end


  //----------------------------------------------------------------
  // spi_master_ctrl
  //----------------------------------------------------------------
  always @* begin : spi_master_ctrl
    spi_rx_data_nxt = 1'h0;
    spi_tx_data_nxt = 1'h0;
    spi_csk_new     = 1'h0;
    spi_csk_we      = 1'h0;
    spi_bit_ctr_rst = 1'h0;
    spi_bit_ctr_inc = 1'h0;
    spi_ready_new   = 1'h0;
    spi_ready_we    = 1'h0;
    spi_ctrl_new    = CTRL_IDLE;
    spi_ctrl_we     = 1'h0;

    case (spi_ctrl_reg)
      CTRL_IDLE: begin
        if (spi_start) begin
          spi_csk_new     = 1'h0;
          spi_csk_we      = 1'h1;
          spi_bit_ctr_rst = 1'h1;
          spi_ready_new   = 1'h0;
          spi_ready_we    = 1'h1;
          spi_ctrl_new    = CTRL_POS_FLANK;
          spi_ctrl_we     = 1'h1;
        end
      end

      CTRL_POS_FLANK: begin
        spi_csk_new  = 1'h1;
        spi_csk_we   = 1'h1;
        spi_ctrl_new = CTRL_NEG_FLANK;
        spi_ctrl_we  = 1'h1;
      end

      CTRL_NEG_FLANK: begin
        spi_tx_data_nxt = 1'h1;
        spi_csk_new = 1'h0;
        spi_csk_we = 1'h1;
        spi_ctrl_new = CTRL_NEXT;
        spi_ctrl_we = 1'h1;
      end

      CTRL_NEXT: begin
        spi_rx_data_nxt = 1'h1;
        if (spi_bit_ctr_reg == 3'h7) begin
          spi_ready_new = 1'h1;
          spi_ready_we  = 1'h1;
          spi_ctrl_new  = CTRL_IDLE;
          spi_ctrl_we   = 1'h1;
        end
        else begin
          spi_bit_ctr_inc = 1'h1;
          spi_ctrl_new    = CTRL_POS_FLANK;
          spi_ctrl_we     = 1'h1;
        end
      end

      default: begin
      end
    endcase  // case (spi_ctrl_reg)
  end

endmodule  // tk1_spi_master

//======================================================================
// EOF tk1_spi_master.v
//======================================================================
