//======================================================================
//
// mta1.v
// ------
// Top level information, debug and control core for the mta1 design.
//
//
// Author: Joachim Strombergson
// Copyright (C) 2022 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only
//
//======================================================================

`default_nettype none

module mta1(
	    input wire           clk,
	    input wire           reset_n,

	    output wire          fw_app_mode,

            output wire          led_r,
            output wire          led_g,
            output wire          led_b,

            input wire           gpio1,
            input wire           gpio2,
            output wire          gpio3,
            output wire          gpio4,

	    input wire           cs,
	    input wire           we,

	    input wire  [7 : 0]  address,
	    input wire [31 : 0]  write_data,
	    output wire [31 : 0] read_data,
	    output wire          ready
	   );


  //----------------------------------------------------------------
  // Internal constant and parameter definitions.
  //----------------------------------------------------------------
  localparam ADDR_NAME0      = 8'h00;
  localparam ADDR_NAME1      = 8'h01;
  localparam ADDR_VERSION    = 8'h02;

  localparam ADDR_SWITCH_APP = 8'h08;

  localparam ADDR_LED        = 8'h09;
  localparam LED_R_BIT       = 2;
  localparam LED_G_BIT       = 1;
  localparam LED_B_BIT       = 0;

  localparam ADDR_GPIO       = 8'h0a;
  localparam GPIO1_BIT       = 0;
  localparam GPIO2_BIT       = 1;
  localparam GPIO3_BIT       = 2;
  localparam GPIO4_BIT       = 3;

  localparam ADDR_APP_START  = 8'h0c;
  localparam ADDR_APP_SIZE   = 8'h0d;

  localparam ADDR_CDI_FIRST  = 8'h20;
  localparam ADDR_CDI_LAST   = 8'h27;

  localparam ADDR_UDI_FIRST  = 8'h30;
  localparam ADDR_UDI_LAST   = 8'h31;

  localparam MTA1_NAME0      = 32'h6d746131; // "mta1"
  localparam MTA1_NAME1      = 32'h6d6b6466; // "mkdf"
  localparam MTA1_VERSION    = 32'h00000004;


  //----------------------------------------------------------------
  // Registers including update variables and write enable.
  //----------------------------------------------------------------
  reg [31 : 0] cdi_mem [0 : 7];
  reg          cdi_mem_we;

  reg [31 : 0] udi_mem [0 : 1];
  initial $readmemh(`UDI_HEX, udi_mem);

  reg          switch_app_reg;
  reg          switch_app_we;

  reg [2 : 0]  led_reg;
  reg          led_we;

  reg [1 : 0]  gpio1_reg;
  reg [1 : 0]  gpio2_reg;
  reg          gpio3_reg;
  reg          gpio3_we;
  reg          gpio4_reg;
  reg          gpio4_we;

  reg [31 : 0] app_start_reg;
  reg          app_start_we;

  reg [31 : 0] app_size_reg;
  reg          app_size_we;


  //----------------------------------------------------------------
  // Wires.
  //----------------------------------------------------------------
  /* verilator lint_off UNOPTFLAT */
  reg [31 : 0] tmp_read_data;
  reg          tmp_ready;
  /* verilator lint_on UNOPTFLAT */


  //----------------------------------------------------------------
  // Concurrent connectivity for ports etc.
  //----------------------------------------------------------------
  assign read_data   = tmp_read_data;
  assign ready       = tmp_ready;

  assign fw_app_mode = switch_app_reg;

  assign gpio3 = gpio3_reg;
  assign gpio4 = gpio4_reg;


  //----------------------------------------------------------------
  // Module instance.
  //----------------------------------------------------------------
  /* verilator lint_off PINMISSING */
  SB_RGBA_DRV #(
                .CURRENT_MODE("0b1"),       // half-current mode
                .RGB0_CURRENT("0b000001"),  // 2 mA
                .RGB1_CURRENT("0b000001"),  // 2 mA
                .RGB2_CURRENT("0b000001")   // 2 mA
                ) RGBA_DRV (
                            .RGB0(led_r),
                            .RGB1(led_g),
                            .RGB2(led_b),
                            .RGBLEDEN(1'h1),
                            .RGB0PWM(led_reg[LED_R_BIT]),
                            .RGB1PWM(led_reg[LED_G_BIT]),
                            .RGB2PWM(led_reg[LED_B_BIT]),
                            .CURREN(1'b1)
    );
  /* verilator lint_on PINMISSING */


  //----------------------------------------------------------------
  // reg_update
  //----------------------------------------------------------------
  always @ (posedge clk)
    begin : reg_update
      if (!reset_n) begin
	switch_app_reg <= 1'h0;
        led_reg        <= 3'h6;
        gpio1_reg      <= 2'h0;
        gpio2_reg      <= 2'h0;
        gpio3_reg      <= 1'h0;
        gpio4_reg      <= 1'h0;
        app_start_reg  <= 32'h0;
        app_size_reg   <= 32'h0;
	cdi_mem[0]     <= 32'h0;
	cdi_mem[1]     <= 32'h0;
	cdi_mem[2]     <= 32'h0;
	cdi_mem[3]     <= 32'h0;
	cdi_mem[4]     <= 32'h0;
	cdi_mem[5]     <= 32'h0;
	cdi_mem[6]     <= 32'h0;
	cdi_mem[7]     <= 32'h0;
      end

      else begin
        gpio1_reg[0] <= gpio1;
        gpio1_reg[1] <= gpio1_reg[0];

        gpio2_reg[0] <= gpio2;
        gpio2_reg[1] <= gpio2_reg[0];

	if (switch_app_we) begin
	  switch_app_reg <= 1'h1;
	end

        if (led_we) begin
          led_reg <= write_data[2 : 0];
        end

        if (gpio3_we) begin
          gpio3_reg <= write_data[GPIO3_BIT];
        end

        if (gpio4_we) begin
          gpio4_reg <= write_data[GPIO4_BIT];
        end

        if (app_start_we) begin
          app_start_reg <= write_data;
        end

        if (app_size_we) begin
          app_size_reg <= write_data;
        end

	if (cdi_mem_we) begin
	  cdi_mem[address[2 : 0]] <= write_data;
	end
      end
    end // reg_update


  //----------------------------------------------------------------
  // api
  //----------------------------------------------------------------
  always @*
    begin : api
      switch_app_we = 1'h0;
      led_we        = 1'h0;
      gpio3_we      = 1'h0;
      gpio4_we      = 1'h0;
      app_start_we  = 1'h0;
      app_size_we   = 1'h0;
      cdi_mem_we    = 1'h0;
      cdi_mem_we    = 1'h0;
      tmp_read_data = 32'h00000000;
      tmp_ready     = 1'h0;

      if (cs) begin
	tmp_ready = 1'h1;
        if (we) begin
	  if (address == ADDR_SWITCH_APP) begin
	    switch_app_we = 1'h1;
	  end

	  if (address == ADDR_LED) begin
	    led_we = 1'h1;
	  end

	  if (address == ADDR_GPIO) begin
	    gpio3_we = 1'h1;
	    gpio4_we = 1'h1;
	  end

          if (address == ADDR_APP_START) begin
	    if (!switch_app_reg) begin
              app_start_we = 1'h1;
            end
	  end

          if (address == ADDR_APP_SIZE) begin
	    if (!switch_app_reg) begin
              app_size_we = 1'h1;
            end
	  end

	  if ((address >= ADDR_CDI_FIRST) && (address <= ADDR_CDI_LAST)) begin
	    if (!switch_app_reg) begin
	      cdi_mem_we = 1'h1;
	    end
	  end
	end

        else begin
	  if (address == ADDR_NAME0) begin
	    tmp_read_data = MTA1_NAME0;
          end

	  if (address == ADDR_NAME1) begin
	    tmp_read_data = MTA1_NAME1;
	  end

	  if (address == ADDR_VERSION) begin
	    tmp_read_data = MTA1_VERSION;
	  end

	  if (address == ADDR_SWITCH_APP) begin
	    tmp_read_data = {32{switch_app_reg}};
	  end

	  if (address == ADDR_LED) begin
	    tmp_read_data = {29'h0, led_reg};
	  end

	  if (address == ADDR_GPIO) begin
	    tmp_read_data = {28'h0, gpio4_reg, gpio3_reg,
                             gpio2_reg[1], gpio1_reg[1]};
	  end

          if (address == ADDR_APP_START) begin
            tmp_read_data = app_start_reg;
          end

          if (address == ADDR_APP_SIZE) begin
            tmp_read_data = app_size_reg;
          end

	  if ((address >= ADDR_CDI_FIRST) && (address <= ADDR_CDI_LAST)) begin
	    tmp_read_data = cdi_mem[address[2 : 0]];
	  end

	  if ((address >= ADDR_UDI_FIRST) && (address <= ADDR_UDI_LAST)) begin
	    tmp_read_data = udi_mem[address[0]];
	  end
        end
      end
    end // api

endmodule // mta1

//======================================================================
// EOF mta1.v
//======================================================================
