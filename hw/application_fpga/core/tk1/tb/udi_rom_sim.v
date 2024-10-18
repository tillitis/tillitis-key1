//======================================================================
//
// udi_rom_sim.v
// ---------
// Simulation version of the UDI ROM.
//
//
// Author: Joachim Strömbergson.
// Copyright (C) 2023 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only
//
//======================================================================

module udi_rom (
    input  wire [ 0:0] addr,
    output wire [31:0] data
);

  reg [31 : 0] tmp_data;
  assign data = tmp_data;

  always @* begin : addr_mux
    if (addr) begin
      tmp_data = 32'h04050607;
    end
    else begin
      tmp_data = 32'h00010203;
    end
  end

endmodule  // udi_rom

//======================================================================
// EOF udi_rom_sim.v
//======================================================================
