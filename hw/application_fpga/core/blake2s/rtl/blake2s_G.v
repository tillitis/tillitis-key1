//======================================================================
//
// blake2s_G.v
// -----------
// Verilog 2001 implementation of the G function in the
// blake2s hash function core. This is pure combinational logic in a
// separade module to allow us to build versions  with 1, 2, 4
// and even 8 parallel compression functions.
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

module blake2s_G(
                 input wire [31 : 0]  a,
                 input wire [31 : 0]  b,
                 input wire [31 : 0]  c,
                 input wire [31 : 0]  d,
                 input wire [31 : 0]  m0,
                 input wire [31 : 0]  m1,

                 output wire [31 : 0] a_prim,
                 output wire [31 : 0] b_prim,
                 output wire [31 : 0] c_prim,
                 output wire [31 : 0] d_prim
                );


  //----------------------------------------------------------------
  // Wires.
  //----------------------------------------------------------------
  reg [31 : 0] a1;
  reg [31 : 0] a2;
  reg [31 : 0] b1;
  reg [31 : 0] b2;
  reg [31 : 0] b3;
  reg [31 : 0] b4;
  reg [31 : 0] c1;
  reg [31 : 0] c2;
  reg [31 : 0] d1;
  reg [31 : 0] d2;
  reg [31 : 0] d3;
  reg [31 : 0] d4;


  //----------------------------------------------------------------
  // Concurrent connectivity for ports.
  //----------------------------------------------------------------
  assign a_prim = a2;
  assign b_prim = b4;
  assign c_prim = c2;
  assign d_prim = d4;


  //----------------------------------------------------------------
  // G_function
  //----------------------------------------------------------------
  always @*
    begin : G_function
      a1 = a + b + m0;

      d1 = d ^ a1;
      d2 = {d1[15 : 0], d1[31 : 16]};

      c1 = c + d2;

      b1 = b ^ c1;
      b2 = {b1[11 : 0], b1[31 : 12]};

      a2 = a1 + b2 + m1;

      d3 = d2 ^ a2;
      d4 = {d3[7 : 0], d3[31 : 8]};

      c2 = c1 + d4;

      b3 = b2 ^ c2;
      b4 = {b3[6 : 0], b3[31 : 7]};
    end // G_function
endmodule // blake2s_G

//======================================================================
// EOF blake2s_G.v
//======================================================================
