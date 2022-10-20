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
// Copyright (C) 2022 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only
//
//======================================================================

`default_nettype none

//`define VERBOSE

`ifdef VERBOSE
  `define verbose(debug_command) debug_command
`else
  `define verbose(debug_command)
`endif


module application_fpga(
			input wire           clk,

			output wire          valid,
			output wire [03 : 0] wstrb,
			output wire [31 : 0] addr,
			output wire [31 : 0] wdata,
			output wire [31 : 0] rdata,
			output wire          ready,

                        output wire          interface_rx,
                        input wire           interface_tx,

			input wire           touch_event,

			input wire           app_gpio1,
			input wire           app_gpio2,
			output wire          app_gpio3,
			output wire          app_gpio4,

	                output wire          led_r,
	                output wire          led_g,
	                output wire          led_b
                       );


  //----------------------------------------------------------------
  // Local parameters
  //----------------------------------------------------------------
  // Top level mem area prefixes.
  localparam ROM_PREFIX      = 2'h0;
  localparam RAM_PREFIX      = 2'h1;
  localparam RESERVED_PREFIX = 2'h2;
  localparam MMIO_PREFIX     = 2'h3;

  // MMIO core mem sub-prefixes.
  localparam TRNG_PREFIX        = 6'h00;
  localparam TIMER_PREFIX       = 6'h01;
  localparam UDS_PREFIX         = 6'h02;
  localparam UART_PREFIX        = 6'h03;
  localparam TOUCH_SENSE_PREFIX = 6'h04;
  localparam TK1_PREFIX         = 6'h3f;


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
  wire          reset_n;

  wire          cpu_valid;
  wire [03 : 0] cpu_wstrb;
  wire [31 : 0] cpu_addr;
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


  //----------------------------------------------------------------
  // Concurrent assignments.
  //----------------------------------------------------------------
  assign valid = cpu_valid;
  assign wstrb = cpu_wstrb;
  assign addr  = cpu_addr;
  assign wdata = cpu_wdata;
  assign rdata = muxed_rdata_reg;
  assign ready = muxed_ready_reg;


  //----------------------------------------------------------------
  // Module instantiations.
  //----------------------------------------------------------------
  reset_gen #(.RESET_CYCLES(200))
    reset_gen_inst(.clk(clk), .rst_n(reset_n));


  picorv32 #(
	     .ENABLE_COUNTERS(0),
	     .LATCHED_MEM_RDATA(0),
	     .TWO_STAGE_SHIFT(0),
	     .TWO_CYCLE_ALU(0),
	     .CATCH_MISALIGN(0),
	     .CATCH_ILLINSN(0),
	     .COMPRESSED_ISA(1),
	     .ENABLE_MUL(1),
	     .ENABLE_DIV(0),
	     .BARREL_SHIFTER(0)
	     ) cpu(
		   .clk(clk),
		   .resetn(reset_n),

		   .mem_valid(cpu_valid),
		   .mem_addr (cpu_addr),
		   .mem_wdata(cpu_wdata),
		   .mem_wstrb(cpu_wstrb),
		   .mem_rdata(muxed_rdata_reg),
		   .mem_ready(muxed_ready_reg),

                   // Defined unsed ports. Makes lint happy,
                   // but still needs to help lint with empty ports.
                   /* verilator lint_off PINCONNECTEMPTY */
                   .irq(32'h0),
                   .eoi(),
                   .trap(),
                   .trace_valid(),
                   .trace_data(),
                   .mem_instr(),
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
        muxed_ready_reg <= 1'h0;
        muxed_rdata_reg <= 32'h0;
      end

      else begin
        muxed_ready_reg <= muxed_ready_new;
        muxed_rdata_reg <= muxed_rdata_new;
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
      ram_address         = cpu_addr[16 : 2];
      ram_write_data      = cpu_wdata;

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
        case (area_prefix)
          ROM_PREFIX: begin
	    `verbose($display("Access to ROM area");)
            rom_cs          = 1'h1;
	    muxed_rdata_new = rom_read_data;
	    muxed_ready_new = rom_ready;
          end

          RAM_PREFIX: begin
	   `verbose($display("Access to RAM area");)
            ram_cs          = 1'h1;
	    muxed_rdata_new = ram_read_data;
	    muxed_ready_new = ram_ready;
          end

          RESERVED_PREFIX: begin
	   `verbose($display("Access to RESERVED area");)
	    muxed_rdata_new = 32'h00000000;
	    muxed_ready_new = 1'h1;
          end

          MMIO_PREFIX: begin
	   `verbose($display("Access to MMIO area");)
            case (core_prefix)
              TRNG_PREFIX: begin
		`verbose($display("Access to TRNG core");)
                trng_cs         = 1'h1;
	        muxed_rdata_new = trng_read_data;
	        muxed_ready_new = trng_ready;
              end

              TIMER_PREFIX: begin
		`verbose($display("Access to TIMER core");)
                timer_cs        = 1'h1;
	        muxed_rdata_new = timer_read_data;
	        muxed_ready_new = timer_ready;
              end

              UDS_PREFIX: begin
		`verbose($display("Access to UDS core");)
                uds_cs          = 1'h1;
	        muxed_rdata_new = uds_read_data;
	        muxed_ready_new = uds_ready;
              end

              UART_PREFIX: begin
		`verbose($display("Access to UART core");)
                uart_cs         = 1'h1;
	        muxed_rdata_new = uart_read_data;
	        muxed_ready_new = uart_ready;
              end

              TOUCH_SENSE_PREFIX: begin
		`verbose($display("Access to TOUCH_SENSE core");)
                touch_sense_cs  = 1'h1;
	        muxed_rdata_new = touch_sense_read_data;
	        muxed_ready_new = touch_sense_ready;
              end

	      TK1_PREFIX: begin
		`verbose($display("Access to TK1 core");)
                tk1_cs          = 1'h1;
	        muxed_rdata_new = tk1_read_data;
	        muxed_ready_new = tk1_ready;
              end

              default: begin
		`verbose($display("UNDEFINED MMIO");)
		muxed_rdata_new = 32'h00000000;
		muxed_ready_new = 1'h1;
              end
            endcase // case (core_prefix)
          end // case: MMIO_PREFIX

          default: begin
	   `verbose($display("UNDEFINED AREA");)
	    muxed_rdata_new = 32'h0;
	    muxed_ready_new = 1'h1;
          end
        endcase // case (area_prefix)
      end
    end

endmodule // application_fpga

//======================================================================
// EOF application_fpga.v
//======================================================================
