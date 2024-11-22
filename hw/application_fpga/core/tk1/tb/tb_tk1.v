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

module tb_tk1 ();

  //----------------------------------------------------------------
  // Internal constant and parameter definitions.
  //----------------------------------------------------------------
  parameter DEBUG = 0;

  parameter CLK_HALF_PERIOD = 1;
  parameter CLK_PERIOD = 2 * CLK_HALF_PERIOD;

  localparam ADDR_NAME0 = 8'h00;
  localparam ADDR_NAME1 = 8'h01;
  localparam ADDR_VERSION = 8'h02;

  localparam ADDR_LED = 8'h09;
  localparam LED_R_BIT = 2;
  localparam LED_G_BIT = 1;
  localparam LED_B_BIT = 0;

  localparam ADDR_GPIO = 8'h0a;
  localparam GPIO1_BIT = 0;
  localparam GPIO2_BIT = 1;
  localparam GPIO3_BIT = 2;
  localparam GPIO4_BIT = 3;

  localparam ADDR_APP_START = 8'h0c;
  localparam ADDR_APP_SIZE = 8'h0d;

  localparam ADDR_BLAKE2S = 8'h10;

  localparam ADDR_CDI_FIRST = 8'h20;
  localparam ADDR_CDI_LAST = 8'h27;

  localparam ADDR_UDI_FIRST = 8'h30;
  localparam ADDR_UDI_LAST = 8'h31;

  localparam ADDR_RAM_ADDR_RAND = 8'h40;
  localparam ADDR_RAM_DATA_RAND = 8'h41;

  localparam ADDR_CPU_MON_CTRL = 8'h60;
  localparam ADDR_CPU_MON_FIRST = 8'h61;
  localparam ADDR_CPU_MON_LAST = 8'h62;

  localparam ADDR_SPI_EN = 8'h80;
  localparam ADDR_SPI_XFER = 8'h81;
  localparam ADDR_SPI_DATA = 8'h82;

  localparam APP_RAM_START = 32'h40000000;

  //----------------------------------------------------------------
  // Register and Wire declarations.
  //----------------------------------------------------------------
  reg  [31 : 0] cycle_ctr;
  reg  [31 : 0] error_ctr;
  reg  [31 : 0] tc_ctr;
  reg           tb_monitor;
  reg           tb_main_monitor;
  reg           tb_spi_monitor;

  reg           tb_clk;
  reg           tb_reset_n;
  reg           tb_cpu_trap;
  wire          tb_system_mode;

  reg  [31 : 0] tb_cpu_addr;
  reg           tb_cpu_instr;
  reg           tb_cpu_valid;
  wire          tb_force_trap;

  wire [14 : 0] tb_ram_addr_rand;
  wire [31 : 0] tb_ram_data_rand;

  wire          tb_led_r;
  wire          tb_led_g;
  wire          tb_led_b;

  reg           tb_gpio1;
  reg           tb_gpio2;
  wire          tb_gpio3;
  wire          tb_gpio4;

  wire          tb_spi_ss;
  wire          tb_spi_sck;
  wire          tb_spi_mosi;
  wire          tb_spi_miso;

  reg           tb_cs;
  reg           tb_we;
  reg  [ 7 : 0] tb_address;
  reg  [31 : 0] tb_write_data;
  wire [31 : 0] tb_read_data;
  wire          tb_ready;

  //----------------------------------------------------------------
  // Continuous assignments.
  //----------------------------------------------------------------
  // Inverted loopback of SPI data lines.
  assign tb_spi_miso = ~tb_spi_mosi;


  //----------------------------------------------------------------
  // Device Under Test.
  //----------------------------------------------------------------
  tk1 dut (
      .clk(tb_clk),
      .reset_n(tb_reset_n),

      .cpu_trap(tb_cpu_trap),
      .system_mode(tb_system_mode),

      .cpu_addr  (tb_cpu_addr),
      .cpu_instr (tb_cpu_instr),
      .cpu_valid (tb_cpu_valid),
      .force_trap(tb_force_trap),

      .ram_addr_rand(tb_ram_addr_rand),
      .ram_data_rand(tb_ram_data_rand),

      .led_r(tb_led_r),
      .led_g(tb_led_g),
      .led_b(tb_led_b),

      .gpio1(tb_gpio1),
      .gpio2(tb_gpio2),
      .gpio3(tb_gpio3),
      .gpio4(tb_gpio4),

      .spi_ss  (tb_spi_ss),
      .spi_sck (tb_spi_sck),
      .spi_mosi(tb_spi_mosi),
      .spi_miso(tb_spi_miso),

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
  always begin : clk_gen
    #CLK_HALF_PERIOD;
    tb_clk = !tb_clk;
  end  // clk_gen


  //----------------------------------------------------------------
  // sys_monitor()
  //
  // An always running process that creates a cycle counter and
  // conditionally displays information about the DUT.
  //----------------------------------------------------------------
  always begin : sys_monitor
    cycle_ctr = cycle_ctr + 1;
    #(CLK_PERIOD);
    if (tb_monitor) begin
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
      if (tb_main_monitor) begin
        $display("Inputs and outputs:");
        $display("tb_cpu_trap: 0x%1x, system_mode: 0x%1x", tb_cpu_trap, tb_system_mode);
        $display("cpu_addr: 0x%08x, cpu_instr: 0x%1x, cpu_valid: 0x%1x, force_tap: 0x%1x",
                 tb_cpu_addr, tb_cpu_instr, tb_cpu_valid, tb_force_trap);
        $display("ram_addr_rand: 0x%08x, ram_data_rand: 0x%08x", tb_ram_addr_rand,
                 tb_ram_data_rand);
        $display("led_r: 0x%1x, led_g: 0x%1x, led_b: 0x%1x", tb_led_r, tb_led_g, tb_led_b);
        $display("ready: 0x%1x, cs: 0x%1x, we: 0x%1x, address: 0x%02x", tb_ready, tb_cs, tb_we,
                 tb_address);
        $display("write_data: 0x%08x, read_data: 0x%08x", tb_write_data, tb_read_data);
        $display("");

        $display("Internal state:");
        $display("tmp_read_ready: 0x%1x, tmp_read_data: 0x%08x", dut.tmp_ready, dut.tmp_read_data);
        $display("");
      end

      if (tb_spi_monitor) begin
        $display("SPI I/O and internal state:");
        $display("spi_ss: 0x%1x, spi_sck: 0x%1x, spi_mosi: 0x%1x, spi_miso: 0x%1x", tb_spi_ss,
                 tb_spi_sck, tb_spi_mosi, tb_spi_miso);
      end

      $display("");
      $display("");
    end
  endtask  // dump_dut_state


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
  endtask  // reset_dut


  //----------------------------------------------------------------
  // display_test_result()
  //
  // Display the accumulated test results.
  //----------------------------------------------------------------
  task display_test_result;
    begin
      if (error_ctr == 0) begin
        $display("--- All %02d test cases completed successfully", tc_ctr);
      end
      else begin
        $display("--- %02d tests completed - %02d errors detected.", tc_ctr, error_ctr);
      end
    end
  endtask  // display_test_result


  //----------------------------------------------------------------
  // init_sim()
  //
  // Initialize all counters and testbed functionality as well
  // as setting the DUT inputs to defined values.
  //----------------------------------------------------------------
  task init_sim;
    begin
      cycle_ctr       = 0;
      error_ctr       = 0;
      tc_ctr          = 0;
      tb_monitor      = 0;
      tb_main_monitor = 0;
      tb_spi_monitor  = 0;
      tb_clk          = 1'h0;
      tb_reset_n      = 1'h1;

      tb_cpu_addr     = 32'h0;
      tb_cpu_instr    = 1'h0;
      tb_cpu_valid    = 1'h0;
      tb_cpu_trap     = 1'h0;

      tb_gpio1        = 1'h0;
      tb_gpio2        = 1'h0;

      tb_cs           = 1'h0;
      tb_we           = 1'h0;
      tb_address      = 8'h0;
      tb_write_data   = 32'h0;
    end
  endtask  // init_sim


  //----------------------------------------------------------------
  // write_word()
  //
  // Write the given word to the DUT using the DUT interface.
  //----------------------------------------------------------------
  task write_word(input [11 : 0] address, input [31 : 0] word);
    begin
      if (DEBUG) begin
        $display("--- Writing 0x%08x to 0x%02x.", word, address);
        $display("");
      end

      tb_address = address;
      tb_write_data = word;
      tb_cs = 1;
      tb_we = 1;
      #(CLK_PERIOD);
      tb_cs = 0;
      tb_we = 0;
    end
  endtask  // write_word


  //----------------------------------------------------------------
  // read_word()
  //
  // Read a data word from the given address in the DUT.
  // the word read will be available in the global variable
  // tb_read_data.
  //----------------------------------------------------------------
  task read_word(input [11 : 0] address);
    begin : read_word
      reg [31 : 0] read_data;

      tb_address = address;
      tb_cs      = 1'h1;

      #(CLK_PERIOD);
      read_data = tb_read_data;

      #(CLK_PERIOD);
      tb_cs = 1'h0;
    end
  endtask  // read_word


  //----------------------------------------------------------------
  // read_check_word()
  //
  // Read a data word from the given address in the DUT.
  // the word read will be available in the global variable
  // read_data.
  //
  // The function also checks that the data read matches
  // the expected value or not.
  //----------------------------------------------------------------
  task read_check_word(input [11 : 0] address, input [31 : 0] expected);
    begin : read_check_word
      reg [31 : 0] read_data;

      tb_address = address;
      tb_cs      = 1'h1;

      #(CLK_PERIOD);
      read_data = tb_read_data;

      #(CLK_PERIOD);
      tb_cs = 1'h0;

      if (read_data == expected) begin
        if (DEBUG) begin
          $display("--- Reading 0x%08x from 0x%02x.", read_data, address);
        end
      end
      else begin
        $display("--- Error: Got 0x%08x when reading from 0x%02x, expected 0x%08x", read_data,
                 address, expected);
        error_ctr = error_ctr + 1;
      end
    end
  endtask  // read_check_word


  //----------------------------------------------------------------
  // `check_equal()
  //
  // Check that two values are equal
  //----------------------------------------------------------------
  `define check_equal(_value, _expected) \
  begin \
    if ((_value) != (_expected)) begin \
      $display("--- Error: (%s) != (%s)\n               0x%x != 0x%x", `"_value`", `"_expected`", (_value), (_expected)); \
      error_ctr = error_ctr + 1; \
    end \
  end  // `check_equal


  //----------------------------------------------------------------
  // fetch_instruction()
  //
  // Simulate fetch of an instruction at specified address.
  //----------------------------------------------------------------
  task fetch_instruction(input [31:0] address);
    begin : fetch_instruction
      tb_cpu_addr  = address;
      tb_cpu_instr = 1'h1;
      tb_cpu_valid = 1'h1;
      #(CLK_PERIOD);
      tb_cpu_addr  = 32'h0;
      tb_cpu_instr = 1'h0;
      tb_cpu_valid = 1'h0;
    end
  endtask  // fetch_instruction


  //----------------------------------------------------------------
  // test1()
  // Read out name and version.
  //----------------------------------------------------------------
  task test1;
    begin
      tc_ctr = tc_ctr + 1;

      $display("");
      $display("--- test1: Read out name and version started.");

      read_check_word(ADDR_NAME0, 32'h746B3120);
      read_check_word(ADDR_NAME1, 32'h6d6b6466);
      read_check_word(ADDR_VERSION, 32'h00000005);

      $display("--- test1: completed.");
      $display("");
    end
  endtask  // test1


  //----------------------------------------------------------------
  // test2()
  // Read out UDI.
  //----------------------------------------------------------------
  task test2;
    begin
      tc_ctr = tc_ctr + 1;

      $display("");
      $display("--- test2: Read out UDI started.");

      read_check_word(ADDR_UDI_FIRST, 32'h00010203);
      read_check_word(ADDR_UDI_LAST, 32'h04050607);

      $display("--- test2: completed.");
      $display("");
    end
  endtask  // test2


  //----------------------------------------------------------------
  // test3()
  // Read out CDI.
  //----------------------------------------------------------------
  task test3;
    begin
      tc_ctr = tc_ctr + 1;

      $display("");
      $display("--- test3: Write and read CDI started.");
      $display("--- test3: Write CDI.");
      write_word(ADDR_CDI_FIRST + 0, 32'hf0f1f2f3);
      write_word(ADDR_CDI_FIRST + 1, 32'he0e1e2e3);
      write_word(ADDR_CDI_FIRST + 2, 32'hd0d1d2d3);
      write_word(ADDR_CDI_FIRST + 3, 32'hc0c1c2c3);
      write_word(ADDR_CDI_FIRST + 4, 32'ha0a1a2a3);
      write_word(ADDR_CDI_FIRST + 5, 32'h90919293);
      write_word(ADDR_CDI_FIRST + 6, 32'h80818283);
      write_word(ADDR_CDI_FIRST + 7, 32'h70717273);

      $display("--- test3: Read CDI.");
      read_check_word(ADDR_CDI_FIRST + 0, 32'hf0f1f2f3);
      read_check_word(ADDR_CDI_FIRST + 1, 32'he0e1e2e3);
      read_check_word(ADDR_CDI_FIRST + 2, 32'hd0d1d2d3);
      read_check_word(ADDR_CDI_FIRST + 3, 32'hc0c1c2c3);
      read_check_word(ADDR_CDI_FIRST + 4, 32'ha0a1a2a3);
      read_check_word(ADDR_CDI_FIRST + 5, 32'h90919293);
      read_check_word(ADDR_CDI_FIRST + 6, 32'h80818283);
      read_check_word(ADDR_CDI_LAST + 0, 32'h70717273);

      $display("--- test3: Fetch instruction from app RAM.");
      fetch_instruction(APP_RAM_START);

      $display("--- test3: Try to write CDI again.");
      write_word(ADDR_CDI_FIRST + 0, 32'hfffefdfc);
      write_word(ADDR_CDI_FIRST + 1, 32'hefeeedec);
      write_word(ADDR_CDI_FIRST + 2, 32'hdfdedddc);
      write_word(ADDR_CDI_FIRST + 3, 32'hcfcecdcc);
      write_word(ADDR_CDI_FIRST + 4, 32'hafaeadac);
      write_word(ADDR_CDI_FIRST + 5, 32'h9f9e9d9c);
      write_word(ADDR_CDI_FIRST + 6, 32'h8f8e8d8c);
      write_word(ADDR_CDI_FIRST + 7, 32'h7f7e7d7c);

      $display("--- test3: Read CDI again.");
      read_check_word(ADDR_CDI_FIRST + 0, 32'hf0f1f2f3);
      read_check_word(ADDR_CDI_FIRST + 1, 32'he0e1e2e3);
      read_check_word(ADDR_CDI_FIRST + 2, 32'hd0d1d2d3);
      read_check_word(ADDR_CDI_FIRST + 3, 32'hc0c1c2c3);
      read_check_word(ADDR_CDI_FIRST + 4, 32'ha0a1a2a3);
      read_check_word(ADDR_CDI_FIRST + 5, 32'h90919293);
      read_check_word(ADDR_CDI_FIRST + 6, 32'h80818283);
      read_check_word(ADDR_CDI_LAST + 0, 32'h70717273);

      $display("--- test3: completed.");
      $display("");
    end
  endtask  // test3


  //----------------------------------------------------------------
  // test4()
  // Write and read blake2s entry point.
  //----------------------------------------------------------------
  task test4;
    begin
      tc_ctr = tc_ctr + 1;

      $display("");
      $display("--- test4: Write and read blake2s entry point in fw mode started.");
      $display("--- test4: Reset DUT to switch to fw mode.");
      reset_dut();

      $display("--- test4: Write Blake2s entry point.");
      write_word(ADDR_BLAKE2S, 32'hcafebabe);

      $display("--- test4: Read Blake2s entry point.");
      read_check_word(ADDR_BLAKE2S, 32'hcafebabe);

      $display("--- test4: Fetch instruction from app RAM.");
      fetch_instruction(APP_RAM_START);

      $display("--- test4: Write Blake2s entry point again.");
      write_word(ADDR_BLAKE2S, 32'hdeadbeef);

      $display("--- test4: Read Blake2s entry point again");
      read_check_word(ADDR_BLAKE2S, 32'hcafebabe);

      $display("--- test4: completed.");
      $display("");
    end
  endtask  // test4


  //----------------------------------------------------------------
  // test5()
  // Write and read APP start address end size.
  //----------------------------------------------------------------
  task test5;
    begin
      tc_ctr = tc_ctr + 1;

      $display("");
      $display("--- test5: Write and read app start and size in fw mode started.");
      $display("--- test5: Reset DUT to switch to fw mode.");
      reset_dut();

      $display("--- test5: Write app start address and size.");
      write_word(ADDR_APP_START, 32'h13371337);
      write_word(ADDR_APP_SIZE, 32'h47114711);

      $display("--- test5: Read app start address and size.");
      read_check_word(ADDR_APP_START, 32'h13371337);
      read_check_word(ADDR_APP_SIZE, 32'h47114711);

      $display("--- test5: Fetch instruction from app RAM.");
      fetch_instruction(APP_RAM_START);

      $display("--- test5: Write app start address and size again.");
      write_word(ADDR_APP_START, 32'hdeadbeef);
      write_word(ADDR_APP_SIZE, 32'hf00ff00f);

      $display("--- test5: Read app start address and size.");
      read_check_word(ADDR_APP_START, 32'h13371337);
      read_check_word(ADDR_APP_SIZE, 32'h47114711);

      $display("--- test5: completed.");
      $display("");
    end
  endtask  // test5


  //----------------------------------------------------------------
  // test6()
  // Write RAM address and data randomizatio in fw mode.
  //----------------------------------------------------------------
  task test6;
    begin
      tc_ctr = tc_ctr + 1;

      $display("");
      $display("--- test6: Write RAM addr and data randomization in fw mode.");
      $display("--- test6: Reset DUT to switch to fw mode.");
      reset_dut();

      $display("--- test6: Write to ADDR_RAM_ADDR_RAND and ADDR_RAM_DATA_RAND .");
      write_word(ADDR_RAM_ADDR_RAND, 32'h13371337);
      write_word(ADDR_RAM_DATA_RAND, 32'h47114711);

      $display(
          "--- test6: Check value in dut ADDR_RAM_ADDR_RAND and ADDR_RAM_DATA_RAND registers.");
      $display("--- test6: ram_addr_rand_reg: 0x%04x, ram_data_rand_reg: 0x%08x",
               dut.ram_addr_rand, dut.ram_data_rand);
      `check_equal(dut.ram_addr_rand, 32'h13371337 & {15{1'b1}});
      `check_equal(dut.ram_data_rand, 32'h47114711);

      $display("--- test6: Fetch instruction from app RAM.");
      fetch_instruction(APP_RAM_START);

      $display("--- test6: Write to ADDR_RAM_ADDR_RAND and ADDR_RAM_DATA_RAND again.");
      write_word(ADDR_RAM_ADDR_RAND, 32'hdeadbeef);
      write_word(ADDR_RAM_DATA_RAND, 32'hf00ff00f);

      $display(
          "--- test6: Check value in dut ADDR_RAM_ADDR_RAND and ADDR_RAM_DATA_RAND registers.");
      $display("--- test6: ram_addr_rand_reg: 0x%04x, ram_data_rand_reg: 0x%08x",
               dut.ram_addr_rand, dut.ram_data_rand);
      `check_equal(dut.ram_addr_rand, 32'h13371337 & {15{1'b1}});
      `check_equal(dut.ram_data_rand, 32'h47114711);

      $display("--- test6: completed.");
      $display("");
    end
  endtask  // test6


  //----------------------------------------------------------------
  // test7()
  // LED control.
  //----------------------------------------------------------------
  task test7;
    begin
      tc_ctr = tc_ctr + 1;

      $display("");
      $display("--- test7: LED control started.");

      $display("--- test7: LEDs R: 0x%1x, G: 0x%1x, B: 0x%1x", tb_led_r, tb_led_g, tb_led_g);
      $display("--- test7: Writing to LED control address to invert LED output.");
      write_word(ADDR_LED, 32'h0);
      $display("--- test7: LEDs R: 0x%1x, G: 0x%1x, B: 0x%1x", tb_led_r, tb_led_g, tb_led_g);

      $display("--- test7: completed.");
      $display("");
    end
  endtask  // test7


  //----------------------------------------------------------------
  // test8()
  // GPIO control.
  //----------------------------------------------------------------
  task test8;
    begin
      tc_ctr = tc_ctr + 1;

      $display("");
      $display("--- test8: GPIO control started.");

      $display("--- test8: Set Inputs for GPIO 1 and 2 high.");
      tb_gpio1 = 1'h1;
      tb_gpio2 = 1'h1;
      #(2 * CLK_PERIOD);
      $display("--- test8: Check that we can read GPIO 1 and 2 as high.");
      read_check_word(ADDR_GPIO, 32'h3);

      $display("--- test8: Set GPIO 3 and 4 high by writing to the registers.");
      write_word(ADDR_GPIO, 32'hf);
      $display("--- test8: gpio3: 0x%1x, gpio4: 0x%1x", tb_gpio3, tb_gpio4);

      $display("--- test8: completed.");
      $display("");
    end
  endtask  // test8


  //----------------------------------------------------------------
  // test9()
  // EXE monitor control and detection.
  //----------------------------------------------------------------
  task test9;
    begin
      tc_ctr = tc_ctr + 1;

      $display("");
      $display("--- test9: EXE monitor control and detection started.");

      $display("--- test9: Define and enable a memory area.");
      write_word(ADDR_CPU_MON_FIRST, 32'h10000000);
      write_word(ADDR_CPU_MON_LAST, 32'h20000000);
      write_word(ADDR_CPU_MON_CTRL, 32'h1);

      $display("--- test9: cpu_mon_first_reg: 0x%08x, cpu_mon_last_reg: 0x%08x",
               dut.cpu_mon_first_reg, dut.cpu_mon_last_reg);

      $display("--- test9: Try to redefine memory area after enabling monitor.");
      write_word(ADDR_CPU_MON_FIRST, 32'hdeadbabe);
      write_word(ADDR_CPU_MON_LAST, 32'hdeadcafe);
      $display("--- test9: cpu_mon_first_reg: 0x%08x, cpu_mon_last_reg: 0x%08x",
               dut.cpu_mon_first_reg, dut.cpu_mon_last_reg);

      $display("--- test9: force_trap before illegal access: 0x%1x", tb_force_trap);
      $display("--- test9: Creating an illegal access.");

      tb_cpu_addr  = 32'h13371337;
      tb_cpu_instr = 1'h1;
      tb_cpu_valid = 1'h1;
      #(2 * CLK_PERIOD);
      $display("--- test9: cpu_addr: 0x%08x, cpu_instr: 0x%1x, cpu_valid: 0x%1x", tb_cpu_addr,
               tb_cpu_instr, tb_cpu_valid);
      $display("--- test9: force_trap: 0x%1x", tb_force_trap);
      `check_equal(tb_force_trap, 1);

      $display("--- test9: restore CPU mem interface.");
      tb_cpu_addr  = 32'h0;
      tb_cpu_instr = 1'h0;
      tb_cpu_valid = 1'h0;

      $display("--- test9: completed.");
      $display("");
    end
  endtask  // test9


  //----------------------------------------------------------------
  // test10()
  // SPI master loopback test.
  //----------------------------------------------------------------
  task test10;
    begin
      tc_ctr         = tc_ctr + 1;
      tb_monitor     = 0;
      tb_spi_monitor = 0;

      $display("");
      $display("--- test10: Loopback in SPI Master started.");
      $display("--- test10: Reset DUT to switch to fw mode.");
      reset_dut();

      #(CLK_PERIOD);

      // Sending 0xa7 trough the inverting loopback.
      $display("--- test10: Sending a byte.");
      write_word(ADDR_SPI_EN, 32'h1);
      write_word(ADDR_SPI_DATA, 32'ha7);
      write_word(ADDR_SPI_XFER, 32'h1);

      // Ready ready flag in SPI until it is set.
      read_word(ADDR_SPI_XFER);
      while (!tb_read_data) begin
        read_word(ADDR_SPI_XFER);
      end
      $display("--- test10: Byte should have been sent.");

      // 0x58 is the inverse of 0xa7.
      #(2 * CLK_PERIOD);
      read_check_word(ADDR_SPI_DATA, 32'h58);
      write_word(ADDR_SPI_EN, 32'h0);

      tb_monitor     = 0;
      tb_spi_monitor = 0;

      $display("--- test10: completed.");
      $display("");
    end
  endtask  // test10


  //----------------------------------------------------------------
  // tk1_test
  //----------------------------------------------------------------
  initial begin : tk1_test
    $display("");
    $display("   -= Testbench for tk1 started =-");
    $display("     ===========================");
    $display("");

    init_sim();
    reset_dut();

    test1();
    test2();
    test3();
    test4();
    test5();
    test6();
    test7();
    test8();
    test9();
    test9();
    test10();

    display_test_result();
    $display("");
    $display("   -= Testbench for tk1 completed =-");
    $display("     =============================");
    $display("");
    $finish;
  end  // tk1_test
endmodule  // tb_tk1

//======================================================================
// EOF tb_tk1.v
//======================================================================
