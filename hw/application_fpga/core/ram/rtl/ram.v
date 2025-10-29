//======================================================================
//
// ram.v
// -----
// Module that encapsulates the four SPRAM blocks in the Lattice
// iCE40UP 5K device. This creates a single 32-bit wide,
// 128 kByte large memory.
//
// The block also implements data and address scrambling controlled
// by the ram_addr_rand and ram_data_rand seeds.
//
//
// Author: Joachim Strombergson
// Copyright (C) 2022 - Tillitis AB
// SPDX-License-Identifier: BSD-2-Clause
//
//======================================================================

`default_nettype none

module ram (
    input wire clk,
    input wire reset_n,

    input wire [14 : 0] ram_addr_rand,
    input wire [31 : 0] ram_data_rand,

    input  wire          cs,
    input  wire [ 3 : 0] we,
    input  wire [15 : 0] address,
    input  wire [31 : 0] write_data,
    output wire [31 : 0] read_data,
    output wire          ready
);


  //----------------------------------------------------------------
  // Registers and wires.
  //----------------------------------------------------------------
  reg          ready_reg;

  reg          cs0;
  reg          cs1;
  reg [31 : 0] read_data0;
  reg [31 : 0] read_data1;
  reg [31 : 0] muxed_read_data;

  reg [14 : 0] scrambled_ram_addr;
  reg [31 : 0] scrambled_write_data;
  reg [31 : 0] descrambled_read_data;


  //----------------------------------------------------------------
  // Concurrent assignment of ports.
  //----------------------------------------------------------------
  assign read_data = descrambled_read_data;
  assign ready     = ready_reg;


  //----------------------------------------------------------------
  // SPRAM instances.
  //----------------------------------------------------------------
  SB_SPRAM256KA spram0 (
      .ADDRESS(scrambled_ram_addr[13:0]),
      .DATAIN(scrambled_write_data[15:0]),
      .MASKWREN({we[1], we[1], we[0], we[0]}),
      .WREN(we[1] | we[0]),
      .CHIPSELECT(cs0),
      .CLOCK(clk),
      .STANDBY(1'b0),
      .SLEEP(1'b0),
      .POWEROFF(1'b1),
      .DATAOUT(read_data0[15:0])
  );

  SB_SPRAM256KA spram1 (
      .ADDRESS(scrambled_ram_addr[13:0]),
      .DATAIN(scrambled_write_data[31:16]),
      .MASKWREN({we[3], we[3], we[2], we[2]}),
      .WREN(we[3] | we[2]),
      .CHIPSELECT(cs0),
      .CLOCK(clk),
      .STANDBY(1'b0),
      .SLEEP(1'b0),
      .POWEROFF(1'b1),
      .DATAOUT(read_data0[31:16])
  );


  SB_SPRAM256KA spram2 (
      .ADDRESS(scrambled_ram_addr[13:0]),
      .DATAIN(scrambled_write_data[15:0]),
      .MASKWREN({we[1], we[1], we[0], we[0]}),
      .WREN(we[1] | we[0]),
      .CHIPSELECT(cs1),
      .CLOCK(clk),
      .STANDBY(1'b0),
      .SLEEP(1'b0),
      .POWEROFF(1'b1),
      .DATAOUT(read_data1[15:0])
  );

  SB_SPRAM256KA spram3 (
      .ADDRESS(scrambled_ram_addr[13:0]),
      .DATAIN(scrambled_write_data[31:16]),
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
  always @(posedge clk) begin : reg_update
    if (!reset_n) begin
      ready_reg <= 1'h0;
    end
    else begin
      ready_reg <= cs;
    end
  end


  //----------------------------------------------------------------
  // scramble_descramble
  //
  // Scramble address and write data, and descramble read data using
  // the ram_addr_rand and ram_data_rand seeds.
  //----------------------------------------------------------------
  always @* begin : scramble_descramble
    scrambled_ram_addr    = address[14 : 0]  ^ ram_addr_rand;
    scrambled_write_data  = write_data ^ ram_data_rand ^ {2{address}};
    descrambled_read_data = muxed_read_data ^ ram_data_rand ^ {2{address}};
  end


  //----------------------------------------------------------------
  // mem_mux
  //
  // Select which of the data read from the banks should be
  // returned during a read access.
  //----------------------------------------------------------------
  always @* begin : mem_mux
    cs0 = ~scrambled_ram_addr[14] & cs;
    cs1 = scrambled_ram_addr[14] & cs;

    if (scrambled_ram_addr[14]) begin
      muxed_read_data = read_data1;
    end
    else begin
      muxed_read_data = read_data0;
    end
  end

endmodule  // ram

//======================================================================
// EOF ram.v
//======================================================================
