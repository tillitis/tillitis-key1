//======================================================================
//
// tk1.v
// -----
// Top level information, debug and control core for the tk1 design.
//
//
// Author: Joachim Strombergson
// Copyright (C) 2022 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only
//
//======================================================================

`default_nettype none

module tk1(
	   input wire           clk,
	   input wire           reset_n,

	   input wire           cpu_trap,
	   output wire          fw_app_mode,

	   input wire  [31 : 0] cpu_addr,
	   input wire           cpu_instr,
	   input wire           cpu_valid,
	   output wire          force_trap,
	   output               system_reset,

	   output wire [14 : 0] ram_addr_rand,
	   output wire [31 : 0] ram_data_rand,

`ifdef INCLUDE_SPI_MASTER
	   output wire          spi_ss,
	   output wire          spi_sck,
	   output wire          spi_mosi,
	   input wire           spi_miso,
`endif // INCLUDE_SPI_MASTER

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
  localparam ADDR_NAME0         = 8'h00;
  localparam ADDR_NAME1         = 8'h01;
  localparam ADDR_VERSION       = 8'h02;

  localparam ADDR_SWITCH_APP    = 8'h08;

  localparam ADDR_LED           = 8'h09;
  localparam LED_R_BIT          = 2;
  localparam LED_G_BIT          = 1;
  localparam LED_B_BIT          = 0;

  localparam ADDR_GPIO          = 8'h0a;
  /* verilator lint_off UNUSED */
  localparam GPIO1_BIT          = 0;
  localparam GPIO2_BIT          = 1;
  /* verilator lint_on UNUSED */
  localparam GPIO3_BIT          = 2;
  localparam GPIO4_BIT          = 3;

  localparam ADDR_APP_START     = 8'h0c;
  localparam ADDR_APP_SIZE      = 8'h0d;

  localparam ADDR_BLAKE2S       = 8'h10;
  localparam ADDR_SYSCALL       = 8'h12;

  localparam ADDR_CDI_FIRST     = 8'h20;
  localparam ADDR_CDI_LAST      = 8'h27;

  localparam ADDR_UDI_FIRST     = 8'h30;
  localparam ADDR_UDI_LAST      = 8'h31;

  localparam ADDR_RAM_ADDR_RAND = 8'h40;
  localparam ADDR_RAM_DATA_RAND = 8'h41;

  localparam ADDR_CPU_MON_CTRL  = 8'h60;
  localparam ADDR_CPU_MON_FIRST = 8'h61;
  localparam ADDR_CPU_MON_LAST  = 8'h62;

  localparam ADDR_SYSTEM_RESET  = 8'h70;

`ifdef INCLUDE_SPI_MASTER
  localparam ADDR_SPI_EN        = 8'h80;
  localparam ADDR_SPI_XFER      = 8'h81;
  localparam ADDR_SPI_DATA      = 8'h82;
  localparam ADDR_SPI_CMD       = 8'h83;
`endif // INCLUDE_SPI_MASTER

  localparam TK1_NAME0    = 32'h746B3120; // "tk1 "
  localparam TK1_NAME1    = 32'h6d6b6466; // "mkdf"
  localparam TK1_VERSION  = 32'h00000005;

  localparam FW_RAM_FIRST = 32'hd0000000;
  localparam FW_RAM_LAST  = 32'hd00007ff;

  localparam ROM_PREFIX   = 2'h0;


  //----------------------------------------------------------------
  // Registers including update variables and write enable.
  //----------------------------------------------------------------
  reg [31 : 0] cdi_mem [0 : 7];
  reg          cdi_mem_we;

  reg          fw_app_mode_reg;
  reg          fw_app_mode_new;
  reg          fw_app_mode_we;
  reg          fw_app_mode_rst;

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

  reg [31 : 0] blake2s_addr_reg;
  reg          blake2s_addr_we;

  reg [31 : 0] syscall_addr_reg;
  reg          syscall_addr_we;

  reg [23 : 0] cpu_trap_ctr_reg;
  reg [23 : 0] cpu_trap_ctr_new;
  reg [2 : 0]  cpu_trap_led_reg;
  reg [2 : 0]  cpu_trap_led_new;
  reg          cpu_trap_led_we;

  reg [14 : 0] ram_addr_rand_reg;
  reg          ram_addr_rand_we;
  reg [31 : 0] ram_data_rand_reg;
  reg          ram_data_rand_we;

  reg          system_reset_reg;
  reg          system_reset_new;

  reg          cpu_mon_en_reg;
  reg          cpu_mon_en_we;
  reg [31 : 0] cpu_mon_first_reg;
  reg          cpu_mon_first_we;
  reg [31 : 0] cpu_mon_last_reg;
  reg          cpu_mon_last_we;

  reg          force_trap_reg;
  reg          force_trap_set;


`ifdef INCLUDE_SPI_MASTER
  reg           spi_access_ctrl_reg;
  reg           spi_access_ctrl_new;
  reg           spi_access_ctrl_we;

  reg [31 : 0] spi_cmd_addr_reg;
  reg          spi_cmd_addr_we;
`endif // INCLUDE_SPI_MASTER


  //----------------------------------------------------------------
  // Wires.
  //----------------------------------------------------------------
  /* verilator lint_off UNOPTFLAT */
  reg [31 : 0] tmp_read_data;
  reg          tmp_ready;
  /* verilator lint_on UNOPTFLAT */

  reg [2 : 0]  muxed_led;

  wire [31:0]  udi_rdata;

`ifdef INCLUDE_SPI_MASTER
  reg          spi_enable;
  reg          spi_enable_vld;
  reg          spi_start;
  reg [7 : 0]  spi_tx_data;
  reg          spi_tx_data_vld;
  wire         spi_ready;
  wire [7 : 0] spi_rx_data;
`endif // INCLUDE_SPI_MASTER


  //----------------------------------------------------------------
  // Concurrent connectivity for ports etc.
  //----------------------------------------------------------------
  assign read_data   = tmp_read_data;
  assign ready       = tmp_ready;

  assign fw_app_mode = fw_app_mode_reg;

  assign force_trap = force_trap_reg;

  assign gpio3 = gpio3_reg;
  assign gpio4 = gpio4_reg;

  assign ram_addr_rand = ram_addr_rand_reg;
  assign ram_data_rand = ram_data_rand_reg;

  assign system_reset = system_reset_reg;


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
                            .RGB0PWM(muxed_led[LED_R_BIT]),
                            .RGB1PWM(muxed_led[LED_G_BIT]),
                            .RGB2PWM(muxed_led[LED_B_BIT]),
                            .CURREN(1'b1)
    );
  /* verilator lint_on PINMISSING */

`ifdef INCLUDE_SPI_MASTER
  tk1_spi_master spi_master(
			    .clk(clk),
			    .reset_n(reset_n),

			    .spi_ss(spi_ss),
			    .spi_sck(spi_sck),
			    .spi_mosi(spi_mosi),
			    .spi_miso(spi_miso),

			    .spi_enable(spi_enable),
			    .spi_enable_vld(spi_enable_vld),
			    .spi_start(spi_start),
			    .spi_tx_data(spi_tx_data),
			    .spi_tx_data_vld(spi_tx_data_vld),
			    .spi_rx_data(spi_rx_data),
			    .spi_ready(spi_ready)
			    );
`endif // INCLUDE_SPI_MASTER


    udi_rom rom_i(
		  .addr(address[0]),
		  .data(udi_rdata)
		 );


  //----------------------------------------------------------------
  // reg_update
  //----------------------------------------------------------------
  always @ (posedge clk)
    begin : reg_update
      if (!reset_n) begin
	fw_app_mode_reg     <= 1'h1;
        led_reg             <= 3'h6;
        gpio1_reg           <= 2'h0;
        gpio2_reg           <= 2'h0;
        gpio3_reg           <= 1'h0;
        gpio4_reg           <= 1'h0;
        app_start_reg       <= 32'h0;
        app_size_reg        <= 32'h0;
        blake2s_addr_reg    <= 32'h0;
        syscall_addr_reg    <= 32'h0;
	cdi_mem[0]          <= 32'h0;
	cdi_mem[1]          <= 32'h0;
	cdi_mem[2]          <= 32'h0;
	cdi_mem[3]          <= 32'h0;
	cdi_mem[4]          <= 32'h0;
	cdi_mem[5]          <= 32'h0;
	cdi_mem[6]          <= 32'h0;
	cdi_mem[7]          <= 32'h0;
	cpu_trap_ctr_reg    <= 24'h0;
	cpu_trap_led_reg    <= 3'h0;
        cpu_mon_en_reg      <= 1'h0;
	cpu_mon_first_reg   <= 32'h0;
	cpu_mon_last_reg    <= 32'h0;
	ram_addr_rand_reg   <= 15'h0;
	ram_data_rand_reg   <= 32'h0;
	force_trap_reg      <= 1'h0;
	system_reset_reg    <= 1'h0;

`ifdef INCLUDE_SPI_MASTER
	spi_cmd_addr_reg    <= 32'h0;
	spi_access_ctrl_reg <= 1'h0;
`endif // INCLUDE_SPI_MASTER
      end

      else begin
	cpu_trap_ctr_reg <= cpu_trap_ctr_new;

	system_reset_reg <= system_reset_new;

        gpio1_reg[0] <= gpio1;
        gpio1_reg[1] <= gpio1_reg[0];

        gpio2_reg[0] <= gpio2;
        gpio2_reg[1] <= gpio2_reg[0];

	// When not in FW mode, Disable things that are
	// explitly enabled.
	if (!fw_app_mode) begin
`ifdef INCLUDE_SPI_MASTER
	  spi_access_ctrl_reg <= 1'h0;
`endif // INCLUDE_SPI_MASTER
	end

	if (fw_app_mode_we) begin
	  fw_app_mode_reg <= fw_app_mode_new;
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

        if (blake2s_addr_we) begin
          blake2s_addr_reg <= write_data;
        end

        if (syscall_addr_we) begin
          syscall_addr_reg <= write_data;
        end

	if (cdi_mem_we) begin
	  cdi_mem[address[2 : 0]] <= write_data;
	end

	if (ram_addr_rand_we) begin
	  ram_addr_rand_reg <= write_data[14 : 0];
 	end

	if (ram_data_rand_we) begin
	  ram_data_rand_reg <= write_data;
 	end

	if (cpu_trap_led_we) begin
	  cpu_trap_led_reg <= cpu_trap_led_new;
	end

	if (cpu_mon_en_we) begin
	  cpu_mon_en_reg <= 1'h1;
	end

	if (cpu_mon_first_we) begin
	  cpu_mon_first_reg <= write_data;
	end

	if (cpu_mon_last_we) begin
	  cpu_mon_last_reg <= write_data;
	end

	if (force_trap_set) begin
	  force_trap_reg <= 1'h1;
	end

`ifdef INCLUDE_SPI_MASTER
	if (spi_cmd_addr_we) begin
	  spi_cmd_addr_reg <= write_data;
	end

	if (spi_access_ctrl_we) begin
	  spi_access_ctrl_reg <= write_data[0];
	end
`endif // INCLUDE_SPI_MASTER

      end
    end // reg_update


  //----------------------------------------------------------------
  // trap_led_logic
  //----------------------------------------------------------------
  always @*
    begin : trap_led_logic
      cpu_trap_led_new = 3'h0;
      cpu_trap_led_we  = 1'h0;

      cpu_trap_ctr_new = cpu_trap_ctr_reg + 1'h1;

      if (cpu_trap_ctr_reg == 24'h0) begin
	cpu_trap_led_new = cpu_trap_led_reg ^ 3'h4;
	cpu_trap_led_we  = 1'h1;
      end

      if (cpu_trap) begin
	muxed_led = cpu_trap_led_reg;
      end else begin
	muxed_led = led_reg;
      end
    end


  //----------------------------------------------------------------
  // security_monitor
  //
  // Monitor events and state changes in the SoC, and handle
  // security violations. We currently check for:
  //
  // Any access to RAM but outside of the size of the physical mem.
  //
  // Trying to execute instructions in FW-RAM.
  //
  // Trying to execute code in mem area set to be data access only.
  // This requires execution monitor to have been setup and
  // enabled.
  //----------------------------------------------------------------
  always @*
    begin : security_monitor
      force_trap_set = 1'h0;

	if (cpu_valid) begin
	  if (cpu_addr[31 : 30] == 2'h1 & |cpu_addr[29 : 17]) begin
	      force_trap_set = 1'h1;
	  end

	  if (cpu_instr) begin
	    if ((cpu_addr >= FW_RAM_FIRST) &&
		(cpu_addr <= FW_RAM_LAST)) begin
	      force_trap_set = 1'h1;
	    end

	    if (cpu_mon_en_reg) begin
	      if ((cpu_addr >= cpu_mon_first_reg) &&
		  (cpu_addr <= cpu_mon_last_reg)) begin
		force_trap_set = 1'h1;
	      end
	    end
	  end
	end
    end


  //----------------------------------------------------------------
  // fw_app_mode_ctrl
  //
  // Logic that implements the switch between FW mode and App
  // mode. The FW mode can explicitly be enabled via API, but only if
  // the instruction writing to the API comes from the ROM.
  //
  // As soon as an instruction is executed from RAM, the mode is
  // switched to App mode. This means that after reset the device is
  // in App mode, and will be set to App mode when the FW starts
  // the loaded app.
  // ----------------------------------------------------------------
  always @*
    begin : fw_app_mode_ctrl
      fw_app_mode_new = 1'h0;
      fw_app_mode_we  = 1'h0;

      if (cpu_valid & cpu_instr) begin
	if (cpu_addr[31 : 30] == ROM_PREFIX) begin
	  if (fw_app_mode_rst) begin
	    fw_app_mode_new = 1'h0;
	    fw_app_mode_we  = 1'h1;
	  end
	end
	else begin
	  fw_app_mode_new = 1'h1;
	  fw_app_mode_new = 1'h1;
	end
      end
    end


  //----------------------------------------------------------------
  // api
  //----------------------------------------------------------------
  always @*
    begin : api
      led_we           = 1'h0;
      gpio3_we         = 1'h0;
      gpio4_we         = 1'h0;
      app_start_we     = 1'h0;
      app_size_we      = 1'h0;
      blake2s_addr_we  = 1'h0;
      cdi_mem_we       = 1'h0;
      cdi_mem_we       = 1'h0;
      ram_addr_rand_we = 1'h0;
      ram_data_rand_we = 1'h0;
      system_reset_new = 1'h0;
      cpu_mon_en_we    = 1'h0;
      cpu_mon_first_we = 1'h0;
      cpu_mon_last_we  = 1'h0;
      cpu_mon_en_we    = 1'h0;
      fw_app_mode_rst  = 1'h0;
      tmp_read_data    = 32'h0;
      tmp_ready        = 1'h0;

`ifdef INCLUDE_SPI_MASTER
      spi_cmd_addr_we  = 1'h0;
      spi_enable_vld   = 1'h0;
      spi_start        = 1'h0;
      spi_tx_data_vld  = 1'h0;

      spi_enable       = write_data[0] & spi_access_ctrl_reg;
      spi_tx_data      = write_data[7 : 0] & {8{spi_access_ctrl_reg}};

`endif // INCLUDE_SPI_MASTER

      if (cs) begin
	tmp_ready = 1'h1;
        if (we) begin
	  if (address == ADDR_SWITCH_APP) begin
	    fw_app_mode_rst = 1'h1;
	  end

	  if (address == ADDR_LED) begin
	    led_we = 1'h1;
	  end

	  if (address == ADDR_GPIO) begin
	    gpio3_we = 1'h1;
	    gpio4_we = 1'h1;
	  end

          if (address == ADDR_APP_START) begin
	    if (!fw_app_mode_reg) begin
              app_start_we = 1'h1;
            end
	  end

          if (address == ADDR_APP_SIZE) begin
	    if (!fw_app_mode_reg) begin
              app_size_we = 1'h1;
            end
	  end

          if (address == ADDR_SYSTEM_RESET) begin
            system_reset_new = 1'h1;
          end

          if (address == ADDR_BLAKE2S) begin
	    if (!fw_app_mode_reg) begin
              blake2s_addr_we = 1'h1;
            end
	  end

          if (address == ADDR_SYSCALL) begin
	    if (!fw_app_mode_reg) begin
              syscall_addr_we = 1'h1;
            end
	  end

	  if ((address >= ADDR_CDI_FIRST) && (address <= ADDR_CDI_LAST)) begin
	    if (!fw_app_mode_reg) begin
	      cdi_mem_we = 1'h1;
	    end
	  end

          if (address == ADDR_RAM_ADDR_RAND) begin
 	    if (!fw_app_mode_reg) begin
              ram_addr_rand_we = 1'h1;
	    end
	  end

          if (address == ADDR_RAM_DATA_RAND) begin
	    if (!fw_app_mode_reg) begin
              ram_data_rand_we = 1'h1;
            end
 	  end

	  if (address == ADDR_CPU_MON_CTRL) begin
	    cpu_mon_en_we = 1'h1;
	  end

	  if (address == ADDR_CPU_MON_FIRST) begin
	    if (!cpu_mon_en_reg) begin
	      cpu_mon_first_we = 1'h1;
	    end
	  end

	  if (address == ADDR_CPU_MON_LAST) begin
	    if (!cpu_mon_en_reg) begin
	      cpu_mon_last_we = 1'h1;
	    end
	  end

`ifdef INCLUDE_SPI_MASTER
	  if (address == ADDR_SPI_EN) begin
	    if (!fw_app_mode_reg) begin
	      spi_access_ctrl_we  = 1'h1;
	    end
	  end

	  if (address == ADDR_SPI_XFER) begin
	    spi_start = spi_access_ctrl_reg;
	  end

	  if (address == ADDR_SPI_DATA) begin
	    spi_tx_data_vld = spi_access_ctrl_reg;
	  end

          if (address == ADDR_SPI_CMD) begin
	    if (!fw_app_mode_reg) begin
              spi_cmd_addr_we = 1'h1;
            end
	  end
`endif // INCLUDE_SPI_MASTER

	end
        else begin
	  if (address == ADDR_NAME0) begin
	    tmp_read_data = TK1_NAME0;
          end

	  if (address == ADDR_NAME1) begin
	    tmp_read_data = TK1_NAME1;
	  end

	  if (address == ADDR_VERSION) begin
	    tmp_read_data = TK1_VERSION;
	  end

	  if (address == ADDR_SWITCH_APP) begin
	    tmp_read_data[0] = fw_app_mode_reg;
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

          if (address == ADDR_BLAKE2S) begin
            tmp_read_data = blake2s_addr_reg;
	  end

          if (address == ADDR_SYSCALL) begin
            tmp_read_data = syscall_addr_reg;
	  end

	  if ((address >= ADDR_CDI_FIRST) && (address <= ADDR_CDI_LAST)) begin
	    tmp_read_data = cdi_mem[address[2 : 0]];
	  end

	  if ((address >= ADDR_UDI_FIRST) && (address <= ADDR_UDI_LAST)) begin
	    if (!fw_app_mode_reg) begin
	      tmp_read_data = udi_rdata;
	    end
	  end

`ifdef INCLUDE_SPI_MASTER
	  if (address == ADDR_SPI_XFER) begin
	    if (spi_access_ctrl_reg) begin
	    tmp_read_data[0] = spi_ready;
	    end
	  end

	  if (address == ADDR_SPI_DATA) begin
	    if (spi_access_ctrl_reg) begin
	      tmp_read_data[7 : 0] = spi_rx_data;
	    end
	  end

          if (address == ADDR_SPI_CMD) begin
	    tmp_read_data = spi_cmd_addr_reg;
          end
`endif // INCLUDE_SPI_MASTER

        end
      end
    end // api

endmodule // tk1

//======================================================================
// EOF tk1.v
//======================================================================
