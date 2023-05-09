//======================================================================
//
// tb_tk1.v
// --------
// Testbench for the TK1 core.
//
//
// Author: Joachim Strombergson
// Copyright (C) 2023 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only
//
//======================================================================

`default_nettype none

module tb_tk1();

  //----------------------------------------------------------------
  // Internal constant and parameter definitions.
  //----------------------------------------------------------------
  parameter DEBUG     = 1;

  parameter CLK_HALF_PERIOD = 1;
  parameter CLK_PERIOD = 2 * CLK_HALF_PERIOD;

  localparam ADDR_NAME0         = 8'h00;
  localparam ADDR_NAME1         = 8'h01;
  localparam ADDR_VERSION       = 8'h02;

  localparam ADDR_SWITCH_APP    = 8'h08;

  localparam ADDR_LED           = 8'h09;
  localparam LED_R_BIT          = 2;
  localparam LED_G_BIT          = 1;
  localparam LED_B_BIT          = 0;

  localparam ADDR_GPIO          = 8'h0a;
  localparam GPIO1_BIT          = 0;
  localparam GPIO2_BIT          = 1;
  localparam GPIO3_BIT          = 2;
  localparam GPIO4_BIT          = 3;

  localparam ADDR_APP_START     = 8'h0c;
  localparam ADDR_APP_SIZE      = 8'h0d;

  localparam ADDR_BLAKE2S       = 8'h10;

  localparam ADDR_CDI_FIRST     = 8'h20;
  localparam ADDR_CDI_LAST      = 8'h27;

  localparam ADDR_UDI_FIRST     = 8'h30;
  localparam ADDR_UDI_LAST      = 8'h31;

  localparam ADDR_RAM_ASLR      = 8'h40;
  localparam ADDR_RAM_SCRAMBLE  = 8'h41;

  localparam ADDR_CPU_MON_CTRL  = 8'h60;
  localparam ADDR_CPU_MON_FIRST = 8'h61;
  localparam ADDR_CPU_MON_LAST  = 8'h62;


  //----------------------------------------------------------------
  // Register and Wire declarations.
  //----------------------------------------------------------------
  reg [31 : 0]  cycle_ctr;
  reg [31 : 0]  error_ctr;
  reg [31 : 0]  tc_ctr;
  reg           tb_monitor;

  reg           tb_clk;
  reg           tb_reset_n;
  reg           tb_cpu_trap;
  wire          tb_fw_app_mode;

  reg  [31 : 0] tb_cpu_addr;
  reg           tb_cpu_instr;
  reg           tb_cpu_valid;
  wire          tb_force_trap;

  wire [14 : 0] tb_ram_aslr;
  wire [31 : 0] tb_ram_scramble;

  wire          tb_led_r;
  wire          tb_led_g;
  wire          tb_led_b;

  reg           tb_gpio1;
  reg           tb_gpio2;
  wire          tb_gpio3;
  wire          tb_gpio4;

  reg           tb_cs;
  reg           tb_we;
  reg [7 : 0]   tb_address;
  reg  [31 : 0] tb_write_data;
  wire [31 : 0] tb_read_data;
  wire          tb_ready;


  //----------------------------------------------------------------
  // Device Under Test.
  //----------------------------------------------------------------
  tk1 dut(
          .clk(tb_clk),
          .reset_n(tb_reset_n),

	  .cpu_trap(tb_cpu_trap),
	  .fw_app_mode(tb_fw_app_mode),

	  .cpu_addr(tb_cpu_addr),
	  .cpu_instr(tb_cpu_instr),
	  .cpu_valid(tb_cpu_valid),
	  .force_trap(tb_force_trap),

	  .ram_aslr(tb_ram_aslr),
	  .ram_scramble(tb_ram_scramble),

          .led_r(tb_led_r),
          .led_g(tb_led_g),
          .led_b(tb_led_b),

	  .gpio1(tb_gpio1),
	  .gpio2(tb_gpio2),
	  .gpio3(tb_gpio3),
	  .gpio4(tb_gpio4),

          .cs(tb_cs),
          .we(tb_we),
          .address(tb_address),
          .write_data(tb_write_data),
          .read_data(tb_read_data),
	  .ready(tb_ready)
         );


  //----------------------------------------------------------------
  // clk_gen
  //
  // Always running clock generator process.
  //----------------------------------------------------------------
  always
    begin : clk_gen
      #CLK_HALF_PERIOD;
      tb_clk = !tb_clk;
    end // clk_gen


  //----------------------------------------------------------------
  // sys_monitor()
  //
  // An always running process that creates a cycle counter and
  // conditionally displays information about the DUT.
  //----------------------------------------------------------------
  always
    begin : sys_monitor
      cycle_ctr = cycle_ctr + 1;
      #(CLK_PERIOD);
      if (tb_monitor)
        begin
          dump_dut_state();
        end
    end


  //----------------------------------------------------------------
  // dump_dut_state()
  //
  // Dump the state of the dump when needed.
  //----------------------------------------------------------------
  task dump_dut_state;
    begin : dump_dut_state
      $display("State of DUT at cycle: %08d", cycle_ctr);
      $display("------------");
      $display("Inputs and outputs:");
      $display("tb_cpu_trap: 0x%1x, fw_app_mode: 0x%1x", tb_cpu_trap, tb_fw_app_mode);
      $display("cpu_addr: 0x%08x, cpu_instr: 0x%1x, cpu_valid: 0x%1x, force_tap: 0x%1x",
	       tb_cpu_addr, tb_cpu_instr, tb_cpu_valid, tb_force_trap);
      $display("ram_aslr: 0x%08x, ram_scramble: 0x%08x", tb_ram_aslr, tb_ram_scramble);
      $display("led_r: 0x%1x, led_g: 0x%1x, led_b: 0x%1x", tb_led_r, tb_led_g, tb_led_b);
      $display("ready: 0x%1x, cs: 0x%1x, we: 0x%1x, address: 0x%02x", tb_ready, tb_cs, tb_we, tb_address);
      $display("write_data: 0x%08x, read_data: 0x%08x", tb_write_data, tb_read_data);
      $display("");

      $display("Internal state:");
      $display("tmp_read_ready: 0x%1x, tmp_read_data: 0x%08x", dut.tmp_ready, dut.tmp_read_data);

      $display("");
      $display("");
    end
  endtask // dump_dut_state


  //----------------------------------------------------------------
  // reset_dut()
  //
  // Toggle reset to put the DUT into a well known state.
  //----------------------------------------------------------------
  task reset_dut;
    begin
      $display("--- Toggle reset.");
      tb_reset_n = 0;
      #(2 * CLK_PERIOD);
      tb_reset_n = 1;
    end
  endtask // reset_dut


  //----------------------------------------------------------------
  // display_test_result()
  //
  // Display the accumulated test results.
  //----------------------------------------------------------------
  task display_test_result;
    begin
      if (error_ctr == 0)
        begin
          $display("--- All %02d test cases completed successfully", tc_ctr);
        end
      else
        begin
          $display("--- %02d tests completed - %02d test cases did not complete successfully.",
                   tc_ctr, error_ctr);
        end
    end
  endtask // display_test_result


  //----------------------------------------------------------------
  // init_sim()
  //
  // Initialize all counters and testbed functionality as well
  // as setting the DUT inputs to defined values.
  //----------------------------------------------------------------
  task init_sim;
    begin
      cycle_ctr     = 0;
      error_ctr     = 0;
      tc_ctr        = 0;
      tb_monitor    = 0;

      tb_clk        = 1'h0;
      tb_reset_n    = 1'h1;

      tb_cpu_addr   = 32'h0;
      tb_cpu_instr  = 1'h0;
      tb_cpu_valid  = 1'h0;
      tb_cpu_trap   = 1'h0;

      tb_gpio1      = 1'h0;
      tb_gpio2      = 1'h0;

      tb_cs         = 1'h0;
      tb_we         = 1'h0;
      tb_address    = 8'h0;
      tb_write_data = 32'h0;
    end
  endtask // init_sim


  //----------------------------------------------------------------
  // read_word()
  //
  // Read a data word from the given address in the DUT.
  // the word read will be available in the global variable
  // read_data.
  //----------------------------------------------------------------
  task read_word(input [11 : 0]  address, input [31 : 0] expected);
    begin : read_word
      reg [31 : 0] read_data;

      tb_address   = address;
      tb_cs        = 1'h1;

      #(CLK_HALF_PERIOD);
      read_data = tb_read_data;

      #(CLK_HALF_PERIOD);
      tb_cs        = 1'h0;

      if (DEBUG)
        begin
	  if (read_data == expected) begin
            $display("--- Reading 0x%08x from 0x%02x.", read_data, address);
	  end else begin
            $display("--- Error: Got 0x%08x when reading from 0x%02x, expected 0x%08x",
		     read_data, address, expected);
	    error_ctr = error_ctr + 1;
	  end
          $display("");
        end
    end
  endtask // read_word


  //----------------------------------------------------------------
  // test1()
  //----------------------------------------------------------------
  task test1;
    begin
      tc_ctr = tc_ctr + 1;
      tb_monitor = 1;

      $display("");
      $display("--- test1: started.");

      #(100 * CLK_PERIOD);
      tb_monitor = 0;

      $display("--- test1: completed.");
      $display("");
    end
  endtask // test1


  //----------------------------------------------------------------
  // tk1_test
  //----------------------------------------------------------------
  initial
    begin : tk1_test
      $display("");
      $display("   -= Testbench for tk1 started =-");
      $display("     ===========================");
      $display("");

      init_sim();
      reset_dut();
      test1();

      display_test_result();
      $display("");
      $display("   -= Testbench for tk1 completed =-");
      $display("     =============================");
      $display("");
      $finish;
    end // tk1_test
endmodule // tb_tk1

//======================================================================
// EOF tb_tk1.v
//======================================================================
