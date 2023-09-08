//======================================================================
//
// blake2s_m_select.v
// ------------------
// Verilog 2001 implementation of the message word selection in the
// blake2 hash function core. Based on the given round and mode, we
// extract the indices for the eight m words to select.
// The words are then selected and returned. This is basically a
// mux based implementation of the permutation table in combination
// with the actual word selection.
//
//
// Note that we use the mode to signal which indices to select
// for a given round. This is because we don't do 8 G-functions
// in a single cycle.
//
//
// Author: Joachim Strömbergson
// Copyright (c) 2018, Assured AB
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or
// without modification, are permitted provided that the following
// conditions are met:
//
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in
//    the documentation and/or other materials provided with the
//    distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//======================================================================

module blake2s_m_select(
                        input wire           clk,
                        input wire           reset_n,

                        input wire           load,
                        input wire [511 : 0] m,

                        input wire [3 : 0]   round,
                        input wire           mode,

                        output wire [31 : 0] G0_m0,
                        output wire [31 : 0] G0_m1,
                        output wire [31 : 0] G1_m0,
                        output wire [31 : 0] G1_m1,
                        output wire [31 : 0] G2_m0,
                        output wire [31 : 0] G2_m1,
                        output wire [31 : 0] G3_m0,
                        output wire [31 : 0] G3_m1
                       );


  //----------------------------------------------------------------
  // regs.
  //----------------------------------------------------------------
  reg [31 : 0] m_mem [0 : 15];


  //----------------------------------------------------------------
  // Wires.
  //----------------------------------------------------------------
  reg [3 : 0] i_G0_m0;
  reg [3 : 0] i_G0_m1;
  reg [3 : 0] i_G1_m0;
  reg [3 : 0] i_G1_m1;
  reg [3 : 0] i_G2_m0;
  reg [3 : 0] i_G2_m1;
  reg [3 : 0] i_G3_m0;
  reg [3 : 0] i_G3_m1;


  //----------------------------------------------------------------
  // Concurrent connectivity for ports.
  //----------------------------------------------------------------
  // Eight parallel, muxes that extract the message block words.
  assign G0_m0 = m_mem[i_G0_m0];
  assign G0_m1 = m_mem[i_G0_m1];
  assign G1_m0 = m_mem[i_G1_m0];
  assign G1_m1 = m_mem[i_G1_m1];
  assign G2_m0 = m_mem[i_G2_m0];
  assign G2_m1 = m_mem[i_G2_m1];
  assign G3_m0 = m_mem[i_G3_m0];
  assign G3_m1 = m_mem[i_G3_m1];


  //----------------------------------------------------------------
  // reg_update
  //
  // Update functionality for all registers in the core.
  // All registers are positive edge triggered with synchronous,
  // active low reset. All registers have write enable.
  //----------------------------------------------------------------
  always @ (posedge clk)
    begin : reg_update
      integer i;

      if (!reset_n)
        begin
          for (i = 0 ; i < 16 ; i = i + 1)
            m_mem[i] <= 32'h0;
        end
      else
        begin
          if (load)
            begin
              // Big to little endian conversion during register load.
              m_mem[00] <= {m[0487 : 0480], m[0495 : 0488], m[0503 : 0496], m[0511 : 0504]};
              m_mem[01] <= {m[0455 : 0448], m[0463 : 0456], m[0471 : 0464], m[0479 : 0472]};
              m_mem[02] <= {m[0423 : 0416], m[0431 : 0424], m[0439 : 0432], m[0447 : 0440]};
              m_mem[03] <= {m[0391 : 0384], m[0399 : 0392], m[0407 : 0400], m[0415 : 0408]};
              m_mem[04] <= {m[0359 : 0352], m[0367 : 0360], m[0375 : 0368], m[0383 : 0376]};
              m_mem[05] <= {m[0327 : 0320], m[0335 : 0328], m[0343 : 0336], m[0351 : 0344]};
              m_mem[06] <= {m[0295 : 0288], m[0303 : 0296], m[0311 : 0304], m[0319 : 0312]};
              m_mem[07] <= {m[0263 : 0256], m[0271 : 0264], m[0279 : 0272], m[0287 : 0280]};
              m_mem[08] <= {m[0231 : 0224], m[0239 : 0232], m[0247 : 0240], m[0255 : 0248]};
              m_mem[09] <= {m[0199 : 0192], m[0207 : 0200], m[0215 : 0208], m[0223 : 0216]};
              m_mem[10] <= {m[0167 : 0160], m[0175 : 0168], m[0183 : 0176], m[0191 : 0184]};
              m_mem[11] <= {m[0135 : 0128], m[0143 : 0136], m[0151 : 0144], m[0159 : 0152]};
              m_mem[12] <= {m[0103 : 0096], m[0111 : 0104], m[0119 : 0112], m[0127 : 0120]};
              m_mem[13] <= {m[0071 : 0064], m[0079 : 0072], m[0087 : 0080], m[0095 : 0088]};
              m_mem[14] <= {m[0039 : 0032], m[0047 : 0040], m[0055 : 0048], m[0063 : 0056]};
              m_mem[15] <= {m[0007 : 0000], m[0015 : 0008], m[0023 : 0016], m[0031 : 0024]};
            end
        end
    end // reg_update


  //----------------------------------------------------------------
  // get_indices
  //
  // Get the indices from the permutation table given the
  // round and the G function mode. This is the SIGMA table.
  //----------------------------------------------------------------
  always @*
    begin : get_indices
      i_G0_m0 = 4'd0;
      i_G0_m1 = 4'd0;
      i_G1_m0 = 4'd0;
      i_G1_m1 = 4'd0;
      i_G2_m0 = 4'd0;
      i_G2_m1 = 4'd0;
      i_G3_m0 = 4'd0;
      i_G3_m1 = 4'd0;

      case ({round, mode})
        0: begin
          i_G0_m0 = 4'd00;
          i_G0_m1 = 4'd01;
          i_G1_m0 = 4'd02;
          i_G1_m1 = 4'd03;
          i_G2_m0 = 4'd04;
          i_G2_m1 = 4'd05;
          i_G3_m0 = 4'd06;
          i_G3_m1 = 4'd07;
        end

        1: begin
          i_G0_m0 = 4'd08;
          i_G0_m1 = 4'd09;
          i_G1_m0 = 4'd10;
          i_G1_m1 = 4'd11;
          i_G2_m0 = 4'd12;
          i_G2_m1 = 4'd13;
          i_G3_m0 = 4'd14;
          i_G3_m1 = 4'd15;
        end

        2: begin
          i_G0_m0 = 4'd14;
          i_G0_m1 = 4'd10;
          i_G1_m0 = 4'd04;
          i_G1_m1 = 4'd08;
          i_G2_m0 = 4'd09;
          i_G2_m1 = 4'd15;
          i_G3_m0 = 4'd13;
          i_G3_m1 = 4'd06;
        end

        3: begin
          i_G0_m0 = 4'd01;
          i_G0_m1 = 4'd12;
          i_G1_m0 = 4'd00;
          i_G1_m1 = 4'd02;
          i_G2_m0 = 4'd11;
          i_G2_m1 = 4'd07;
          i_G3_m0 = 4'd05;
          i_G3_m1 = 4'd03;
        end

        4: begin
          i_G0_m0 = 4'd11;
          i_G0_m1 = 4'd08;
          i_G1_m0 = 4'd12;
          i_G1_m1 = 4'd00;
          i_G2_m0 = 4'd05;
          i_G2_m1 = 4'd02;
          i_G3_m0 = 4'd15;
          i_G3_m1 = 4'd13;
        end

        5: begin
          i_G0_m0 = 4'd10;
          i_G0_m1 = 4'd14;
          i_G1_m0 = 4'd03;
          i_G1_m1 = 4'd06;
          i_G2_m0 = 4'd07;
          i_G2_m1 = 4'd01;
          i_G3_m0 = 4'd09;
          i_G3_m1 = 4'd04;
        end

        6: begin
          i_G0_m0 = 4'd07;
          i_G0_m1 = 4'd09;
          i_G1_m0 = 4'd03;
          i_G1_m1 = 4'd01;
          i_G2_m0 = 4'd13;
          i_G2_m1 = 4'd12;
          i_G3_m0 = 4'd11;
          i_G3_m1 = 4'd14;
        end

        7: begin
          i_G0_m0 = 4'd02;
          i_G0_m1 = 4'd06;
          i_G1_m0 = 4'd05;
          i_G1_m1 = 4'd10;
          i_G2_m0 = 4'd04;
          i_G2_m1 = 4'd00;
          i_G3_m0 = 4'd15;
          i_G3_m1 = 4'd08;
        end

        8: begin
          i_G0_m0 = 4'd09;
          i_G0_m1 = 4'd00;
          i_G1_m0 = 4'd05;
          i_G1_m1 = 4'd07;
          i_G2_m0 = 4'd02;
          i_G2_m1 = 4'd04;
          i_G3_m0 = 4'd10;
          i_G3_m1 = 4'd15;
        end

        9: begin
          i_G0_m0 = 4'd14;
          i_G0_m1 = 4'd01;
          i_G1_m0 = 4'd11;
          i_G1_m1 = 4'd12;
          i_G2_m0 = 4'd06;
          i_G2_m1 = 4'd08;
          i_G3_m0 = 4'd03;
          i_G3_m1 = 4'd13;
        end

        10: begin
          i_G0_m0 = 4'd02;
          i_G0_m1 = 4'd12;
          i_G1_m0 = 4'd06;
          i_G1_m1 = 4'd10;
          i_G2_m0 = 4'd00;
          i_G2_m1 = 4'd11;
          i_G3_m0 = 4'd08;
          i_G3_m1 = 4'd03;
        end

        11: begin
          i_G0_m0 = 4'd04;
          i_G0_m1 = 4'd13;
          i_G1_m0 = 4'd07;
          i_G1_m1 = 4'd05;
          i_G2_m0 = 4'd15;
          i_G2_m1 = 4'd14;
          i_G3_m0 = 4'd01;
          i_G3_m1 = 4'd09;
        end

        12: begin
          i_G0_m0 = 4'd12;
          i_G0_m1 = 4'd05;
          i_G1_m0 = 4'd01;
          i_G1_m1 = 4'd15;
          i_G2_m0 = 4'd14;
          i_G2_m1 = 4'd13;
          i_G3_m0 = 4'd04;
          i_G3_m1 = 4'd10;
        end

        13: begin
          i_G0_m0 = 4'd00;
          i_G0_m1 = 4'd07;
          i_G1_m0 = 4'd06;
          i_G1_m1 = 4'd03;
          i_G2_m0 = 4'd09;
          i_G2_m1 = 4'd02;
          i_G3_m0 = 4'd08;
          i_G3_m1 = 4'd11;
        end

        14: begin
          i_G0_m0 = 4'd13;
          i_G0_m1 = 4'd11;
          i_G1_m0 = 4'd07;
          i_G1_m1 = 4'd14;
          i_G2_m0 = 4'd12;
          i_G2_m1 = 4'd01;
          i_G3_m0 = 4'd03;
          i_G3_m1 = 4'd09;
        end

        15: begin
          i_G0_m0 = 4'd05;
          i_G0_m1 = 4'd00;
          i_G1_m0 = 4'd15;
          i_G1_m1 = 4'd04;
          i_G2_m0 = 4'd08;
          i_G2_m1 = 4'd06;
          i_G3_m0 = 4'd02;
          i_G3_m1 = 4'd10;
        end

        16: begin
          i_G0_m0 = 4'd06;
          i_G0_m1 = 4'd15;
          i_G1_m0 = 4'd14;
          i_G1_m1 = 4'd09;
          i_G2_m0 = 4'd11;
          i_G2_m1 = 4'd03;
          i_G3_m0 = 4'd00;
          i_G3_m1 = 4'd08;
        end

        17: begin
          i_G0_m0 = 4'd12;
          i_G0_m1 = 4'd02;
          i_G1_m0 = 4'd13;
          i_G1_m1 = 4'd07;
          i_G2_m0 = 4'd01;
          i_G2_m1 = 4'd04;
          i_G3_m0 = 4'd10;
          i_G3_m1 = 4'd05;
        end

        18: begin
          i_G0_m0 = 4'd10;
          i_G0_m1 = 4'd02;
          i_G1_m0 = 4'd08;
          i_G1_m1 = 4'd04;
          i_G2_m0 = 4'd07;
          i_G2_m1 = 4'd06;
          i_G3_m0 = 4'd01;
          i_G3_m1 = 4'd05;
        end

        19: begin
          i_G0_m0 = 4'd15;
          i_G0_m1 = 4'd11;
          i_G1_m0 = 4'd09;
          i_G1_m1 = 4'd14;
          i_G2_m0 = 4'd03;
          i_G2_m1 = 4'd12;
          i_G3_m0 = 4'd13;
          i_G3_m1 = 4'd00;
        end

        default: begin end
      endcase // case ({round, mode})
    end

endmodule // blake2s_m_select

//======================================================================
// EOF blake2s_m_select.v
//======================================================================
