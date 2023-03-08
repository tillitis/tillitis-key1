//======================================================================
//
// application_fpga.v
// ------------------
// Top level module of the application FPGA.
// The design exposes a UART interface to allow a host to
// send commands and receive resposes as needed load, execute and
// communicate with applications.
//
//
// Author: Joachim Strombergson
// Copyright (C) 2022 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only
//
//======================================================================

`default_nettype none

module application_fpga(
                        output wire interface_rx,
                        input wire  interface_tx,

			input wire  touch_event,

			input wire  app_gpio1,
			input wire  app_gpio2,
			output wire app_gpio3,
			output wire app_gpio4,

	                output wire led_r,
	                output wire led_g,
	                output wire led_b
                       );


  //----------------------------------------------------------------
  // Local parameters
  //----------------------------------------------------------------
  // Top level mem area prefixes.
  localparam ROM_PREFIX      = 2'h0;
  localparam RAM_PREFIX      = 2'h1;
  localparam RESERVED_PREFIX = 2'h2;
  localparam MMIO_PREFIX     = 2'h3;

  // MMIO core sub-prefixes.
  localparam TRNG_PREFIX        = 6'h00;
  localparam TIMER_PREFIX       = 6'h01;
  localparam UDS_PREFIX         = 6'h02;
  localparam UART_PREFIX        = 6'h03;
  localparam TOUCH_SENSE_PREFIX = 6'h04;
  localparam FW_RAM_PREFIX      = 6'h10;
  localparam TK1_PREFIX         = 6'h3f;

  // Instruction used to cause a trap.
  localparam ILLEGAL_INSTRUCTION = 32'h0;


  //----------------------------------------------------------------
  // Registers, memories with associated wires.
  //----------------------------------------------------------------
  reg [31 : 0] muxed_rdata_reg;
  reg [31 : 0] muxed_rdata_new;

  reg          muxed_ready_reg;
  reg          muxed_ready_new;


  //----------------------------------------------------------------
  // Wires.
  //----------------------------------------------------------------
  wire          clk;
  wire          reset_n;

  wire          cpu_trap;
  wire          cpu_valid;
  wire          cpu_instr;
  wire [03 : 0] cpu_wstrb;
  /* verilator lint_off UNUSED */
  wire [31 : 0] cpu_addr;
  /* verilator lint_on UNUSED */
  wire [31 : 0] cpu_wdata;

  /* verilator lint_off UNOPTFLAT */
  reg           rom_cs;
  /* verilator lint_on UNOPTFLAT */
  reg  [11 : 0] rom_address;
  wire [31 : 0] rom_read_data;
  wire          rom_ready;

  reg           ram_cs;
  reg  [3 : 0]  ram_we;
  reg  [14 : 0] ram_address;
  reg  [31 : 0] ram_write_data;
  wire [31 : 0] ram_read_data;
  wire          ram_ready;

  /* verilator lint_off UNOPTFLAT */
  reg           trng_cs;
  /* verilator lint_on UNOPTFLAT */
  reg           trng_we;
  reg  [7 : 0]  trng_address;
  reg [31 : 0]  trng_write_data;
  wire [31 : 0] trng_read_data;
  wire          trng_ready;

  /* verilator lint_off UNOPTFLAT */
  reg           timer_cs;
  /* verilator lint_on UNOPTFLAT */
  reg           timer_we;
  reg  [7 : 0]  timer_address;
  reg  [31 : 0] timer_write_data;
  wire [31 : 0] timer_read_data;
  wire          timer_ready;

  /* verilator lint_off UNOPTFLAT */
  reg           uds_cs;
  /* verilator lint_on UNOPTFLAT */
  reg  [7 : 0]  uds_address;
  wire [31 : 0] uds_read_data;
  wire          uds_ready;

  /* verilator lint_off UNOPTFLAT */
  reg           uart_cs;
  /* verilator lint_on UNOPTFLAT */
  reg           uart_we;
  reg  [7 : 0]  uart_address;
  reg  [31 : 0] uart_write_data;
  wire [31 : 0] uart_read_data;
  wire          uart_ready;

  /* verilator lint_off UNOPTFLAT */
  reg           fw_ram_cs;
  /* verilator lint_on UNOPTFLAT */
  reg  [3 : 0]  fw_ram_we;
  reg  [8 : 0]  fw_ram_address;
  reg  [31 : 0] fw_ram_write_data;
  wire [31 : 0] fw_ram_read_data;
  wire          fw_ram_ready;

  /* verilator lint_off UNOPTFLAT */
  reg           touch_sense_cs;
  /* verilator lint_on UNOPTFLAT */
  reg           touch_sense_we;
  reg  [7 : 0]  touch_sense_address;
  wire [31 : 0] touch_sense_read_data;
  wire          touch_sense_ready;

  /* verilator lint_off UNOPTFLAT */
  reg           tk1_cs;
  /* verilator lint_on UNOPTFLAT */
  reg           tk1_we;
  reg  [7 : 0]  tk1_address;
  reg [31 : 0]  tk1_write_data;
  wire [31 : 0] tk1_read_data;
  wire          tk1_ready;
  wire          fw_app_mode;
  wire          force_trap;
  wire [14 : 0] ram_aslr;
  wire [31 : 0] ram_scramble;


  //----------------------------------------------------------------
  // Module instantiations.
  //----------------------------------------------------------------
  clk_reset_gen #(.RESET_CYCLES(200))
  reset_gen_inst(.clk(clk), .rst_n(reset_n));


  picorv32 #(
	     .ENABLE_COUNTERS(0),
	     .TWO_STAGE_SHIFT(0),
	     .CATCH_MISALIGN(0),
	     .COMPRESSED_ISA(1),
	     .ENABLE_FAST_MUL(1),
	     .BARREL_SHIFTER(1)
	     ) cpu(
		   .clk(clk),
		   .resetn(reset_n),
		   .trap(cpu_trap),

		   .mem_valid(cpu_valid),
		   .mem_ready(muxed_ready_reg),
		   .mem_addr (cpu_addr),
		   .mem_wdata(cpu_wdata),
		   .mem_wstrb(cpu_wstrb),
		   .mem_rdata(muxed_rdata_reg),

                   // Defined unused ports. Makes lint happy. But
                   // we still needs to help lint with empty ports.
                   /* verilator lint_off PINCONNECTEMPTY */
                   .irq(32'h0),
                   .eoi(),
                   .trace_valid(),
                   .trace_data(),
                   .mem_instr(cpu_instr),
                   .mem_la_read(),
                   .mem_la_write(),
                   .mem_la_addr(),
                   .mem_la_wdata(),
                   .mem_la_wstrb(),
                   .pcpi_valid(),
                   .pcpi_insn(),
                   .pcpi_rs1(),
                   .pcpi_rs2(),
                   .pcpi_wr(1'h0),
                   .pcpi_rd(32'h0),
                   .pcpi_wait(1'h0),
                   .pcpi_ready(1'h0)
                   /* verilator lint_on PINCONNECTEMPTY */
	          );


  rom rom_inst(
               .cs(rom_cs),
               .address(rom_address),
               .read_data(rom_read_data),
	       .ready(rom_ready)
               );


  ram ram_inst(
               .clk(clk),
               .reset_n(reset_n),

               .cs(ram_cs),
               .we(ram_we),
               .address(ram_address),
               .write_data(ram_write_data),
               .read_data(ram_read_data),
	       .ready(ram_ready)
               );


  fw_ram fw_ram_inst(
		     .clk(clk),
		     .reset_n(reset_n),

		     .fw_app_mode(fw_app_mode),

		     .cs(fw_ram_cs),
		     .we(fw_ram_we),
		     .address(fw_ram_address),
		     .write_data(fw_ram_write_data),
		     .read_data(fw_ram_read_data),
		     .ready(fw_ram_ready)
		    );


  rosc trng_inst(
		   .clk(clk),
		   .reset_n(reset_n),
		   .cs(trng_cs),
		   .we(trng_we),
		   .address(trng_address),
		   .write_data(trng_write_data),
		   .read_data(trng_read_data),
		   .ready(trng_ready)
		  );


  timer timer_inst(
                   .clk(clk),
                   .reset_n(reset_n),

                   .cs(timer_cs),
                   .we(timer_we),
                   .address(timer_address),
                   .write_data(timer_write_data),
                   .read_data(timer_read_data),
		   .ready(timer_ready)
                  );


  uds uds_inst(
               .clk(clk),
               .reset_n(reset_n),

	       .fw_app_mode(fw_app_mode),

               .cs(uds_cs),
               .address(uds_address),
               .read_data(uds_read_data),
	       .ready(uds_ready)
               );


  uart uart_inst(
                 .clk(clk),
                 .reset_n(reset_n),

                 .rxd(interface_tx),
                 .txd(interface_rx),

                 .cs(uart_cs),
                 .we(uart_we),
                 .address(uart_address),
                 .write_data(uart_write_data),
                 .read_data(uart_read_data),
                 .ready(uart_ready)
                 );


  touch_sense touch_sense_inst(
			       .clk(clk),
			       .reset_n(reset_n),

			       .touch_event(touch_event),

			       .cs(touch_sense_cs),
			       .we(touch_sense_we),
			       .address(touch_sense_address),
			       .read_data(touch_sense_read_data),
			       .ready(touch_sense_ready)
			       );


  tk1 tk1_inst(
	       .clk(clk),
	       .reset_n(reset_n),

	       .fw_app_mode(fw_app_mode),

	       .cpu_addr(cpu_addr),
	       .cpu_instr(cpu_instr),
	       .cpu_valid(cpu_valid),
	       .cpu_trap(cpu_trap),
	       .force_trap(force_trap),

               .ram_aslr(ram_aslr),
	       .ram_scramble(ram_scramble),

               .led_r(led_r),
               .led_g(led_g),
               .led_b(led_b),

               .gpio1(app_gpio1),
               .gpio2(app_gpio2),
               .gpio3(app_gpio3),
               .gpio4(app_gpio4),

	       .cs(tk1_cs),
	       .we(tk1_we),
	       .address(tk1_address),
	       .write_data(tk1_write_data),
	       .read_data(tk1_read_data),
	       .ready(tk1_ready)
	      );


  //----------------------------------------------------------------
  // Reg_update.
  // Posedge triggered with synchronous, active low reset.
  //----------------------------------------------------------------
  always @(posedge clk)
    begin : reg_update
      if (!reset_n) begin
        muxed_rdata_reg <= 32'h0;
        muxed_ready_reg <= 1'h0;
      end

      else begin
        muxed_rdata_reg <= muxed_rdata_new;
        muxed_ready_reg <= muxed_ready_new;
      end
    end


  //----------------------------------------------------------------
  // cpu_mem_ctrl
  // CPU memory decode and control logic.
  //----------------------------------------------------------------
  always @*
    begin : cpu_mem_ctrl
      reg [1 : 0] area_prefix;
      reg [5 : 0] core_prefix;

      area_prefix         = cpu_addr[31 : 30];
      core_prefix         = cpu_addr[29 : 24];

      muxed_ready_new     = 1'h0;
      muxed_rdata_new     = 32'h0;

      rom_cs              = 1'h0;
      rom_address         = cpu_addr[13 : 2];

      ram_cs              = 1'h0;
      ram_we              = cpu_wstrb;
      ram_address         = cpu_addr[16 : 2] ^ ram_aslr;
      ram_write_data      = cpu_wdata ^ ram_scramble ^ {2{cpu_addr[15 : 0]}};

      fw_ram_cs           = 1'h0;
      fw_ram_we           = cpu_wstrb;
      fw_ram_address      = cpu_addr[10 : 2];
      fw_ram_write_data   = cpu_wdata;

      trng_cs             = 1'h0;
      trng_we             = |cpu_wstrb;
      trng_address        = cpu_addr[9 : 2];
      trng_write_data     = cpu_wdata;

      timer_cs            = 1'h0;
      timer_we            = |cpu_wstrb;
      timer_address       = cpu_addr[9 : 2];
      timer_write_data    = cpu_wdata;

      uds_cs              = 1'h0;
      uds_address         = cpu_addr[9 : 2];

      uart_cs             = 1'h0;
      uart_we             = |cpu_wstrb;
      uart_address        = cpu_addr[9 : 2];
      uart_write_data     = cpu_wdata;

      touch_sense_cs      = 1'h0;
      touch_sense_we      = |cpu_wstrb;
      touch_sense_address = cpu_addr[9 : 2];

      tk1_cs              = 1'h0;
      tk1_we              = |cpu_wstrb;
      tk1_address         = cpu_addr[9 : 2];
      tk1_write_data      = cpu_wdata;

      if (cpu_valid && !muxed_ready_reg) begin
	if (force_trap) begin
	  muxed_rdata_new = ILLEGAL_INSTRUCTION;
	  muxed_ready_new = 1'h1;
	end
	else begin
          case (area_prefix)
            ROM_PREFIX: begin
              rom_cs          = 1'h1;
	      muxed_rdata_new = rom_read_data;
	      muxed_ready_new = rom_ready;
            end

            RAM_PREFIX: begin
              ram_cs          = 1'h1;
	      muxed_rdata_new = ram_read_data ^ ram_scramble ^ {2{cpu_addr[15 : 0]}};
	      muxed_ready_new = ram_ready;
            end

            RESERVED_PREFIX: begin
	      muxed_rdata_new = 32'h0;
	      muxed_ready_new = 1'h1;
            end

            MMIO_PREFIX: begin
              case (core_prefix)
		TRNG_PREFIX: begin
                  trng_cs         = 1'h1;
	          muxed_rdata_new = trng_read_data;
	          muxed_ready_new = trng_ready;
		end

		TIMER_PREFIX: begin
                  timer_cs        = 1'h1;
	          muxed_rdata_new = timer_read_data;
	          muxed_ready_new = timer_ready;
		end

		UDS_PREFIX: begin
                  uds_cs          = 1'h1;
	          muxed_rdata_new = uds_read_data;
	          muxed_ready_new = uds_ready;
		end

		UART_PREFIX: begin
                  uart_cs         = 1'h1;
	          muxed_rdata_new = uart_read_data;
	          muxed_ready_new = uart_ready;
		end

		TOUCH_SENSE_PREFIX: begin
                  touch_sense_cs  = 1'h1;
	          muxed_rdata_new = touch_sense_read_data;
	          muxed_ready_new = touch_sense_ready;
		end

		FW_RAM_PREFIX: begin
                  fw_ram_cs       = 1'h1;
	          muxed_rdata_new = fw_ram_read_data;
	          muxed_ready_new = fw_ram_ready;
		end

		TK1_PREFIX: begin
                  tk1_cs          = 1'h1;
	          muxed_rdata_new = tk1_read_data;
	          muxed_ready_new = tk1_ready;
		end

		default: begin
		  muxed_rdata_new = 32'h0;
		  muxed_ready_new = 1'h1;
		end
              endcase // case (core_prefix)
            end // case: MMIO_PREFIX

            default: begin
	      muxed_rdata_new = 32'h0;
	      muxed_ready_new = 1'h1;
            end
          endcase // case (area_prefix)
	end
      end
    end
endmodule // application_fpga

//======================================================================
// EOF application_fpga.v
//======================================================================
