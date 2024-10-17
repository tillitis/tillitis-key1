//======================================================================
//
// tb_tk1_spi_master.v
// -------------------
// Testbench for the TK1_SPI_MASTER core.
//
//
// Author: Joachim Strombergson
// Copyright (C) 2023 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only
//
//======================================================================

`default_nettype none `timescale 1ns / 1ns

module tb_tk1_spi_master ();

  //----------------------------------------------------------------
  // Internal constant and parameter definitions.
  //----------------------------------------------------------------
  parameter DEBUG = 1;

  parameter CLK_HALF_PERIOD = 1;
  parameter CLK_PERIOD = 2 * CLK_HALF_PERIOD;

  parameter MISO_ALL_ZERO = 0;
  parameter MISO_ALL_ONE = 1;
  parameter MISO_MOSI = 2;
  parameter MISO_INV_MOSI = 3;


  //----------------------------------------------------------------
  // Register and Wire declarations.
  //----------------------------------------------------------------
  reg  [31 : 0] cycle_ctr;
  reg  [31 : 0] error_ctr;
  reg  [31 : 0] tc_ctr;
  reg           monitor;
  reg           verbose;

  reg           tb_clk;
  reg           tb_reset_n;
  wire          tb_spi_ss;
  wire          tb_spi_sck;
  wire          tb_spi_mosi;
  wire          tb_spi_miso;
  reg           tb_spi_enable;
  reg           tb_spi_enable_vld;
  reg           tb_spi_start;
  reg  [ 7 : 0] tb_spi_tx_data;
  reg           tb_spi_tx_data_vld;
  wire [ 7 : 0] tb_spi_rx_data;
  wire          tb_spi_ready;

  wire          mem_model_WPn;
  wire          mem_model_HOLDn;

  reg  [ 1 : 0] tb_miso_mux_ctrl;

  reg           my_tb_spi_ss;


  //----------------------------------------------------------------
  // Assignments.
  //----------------------------------------------------------------
  assign mem_model_WPn = 1'h1;


  //----------------------------------------------------------------
  // Device Under Test.
  //----------------------------------------------------------------
  tk1_spi_master dut (
      .clk(tb_clk),
      .reset_n(tb_reset_n),

      .spi_ss  (tb_spi_ss),
      .spi_sck (tb_spi_sck),
      .spi_mosi(tb_spi_mosi),
      .spi_miso(tb_spi_miso),

      .spi_enable(tb_spi_enable),
      .spi_enable_vld(tb_spi_enable_vld),
      .spi_start(tb_spi_start),
      .spi_tx_data(tb_spi_tx_data),
      .spi_tx_data_vld(tb_spi_tx_data_vld),
      .spi_rx_data(tb_spi_rx_data),
      .spi_ready(tb_spi_ready)
  );


  //----------------------------------------------------------------
  // spi_memory
  //----------------------------------------------------------------
  W25Q80DL spi_memory (
      .CSn(tb_spi_ss),
      .CLK(tb_spi_sck),
      .DIO(tb_spi_mosi),
      .DO(tb_spi_miso),
      .WPn(mem_model_WPn),
      .HOLDn(mem_model_HOLDn)
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
    if (monitor) begin
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
      $display("");
      $display("State of DUT at cycle: %08d", cycle_ctr);
      $display("------------");
      $display("Inputs and outputs:");
      $display("spi_ss: 0x%1x, spi_sck: 0x%1x, spi_mosi: 0x%1x, spi_miso:0x%1x", dut.spi_ss,
               dut.spi_sck, dut.spi_mosi, dut.spi_miso);
      $display("spi_enable_vld: 0x%1x, spi_enable: 0x%1x", dut.spi_enable_vld, dut.spi_enable);
      $display("spi_tx_data_vld: 0x%1x, spi_tx_data: 0x%02x", dut.spi_tx_data_vld, dut.spi_tx_data);
      $display("spi_start: 0x%1x, spi_ready: 0x%1x, spi_rx_data: 0x%02x", dut.spi_start,
               dut.spi_ready, dut.spi_rx_data);
      $display("");


      $display("");
      $display("Internal state:");
      $display("spi_bit_ctr_rst: 0x%1x, spi_bit_ctr_inc: 0x%1x, spi_bit_ctr_reg: 0x%02x",
               dut.spi_bit_ctr_rst, dut.spi_bit_ctr_inc, dut.spi_bit_ctr_reg);
      $display("");
      $display("spi_ctrl_reg: 0x%02x, spi_ctrl_new: 0x%02x, spi_ctrl_we: 0x%1x", dut.spi_ctrl_reg,
               dut.spi_ctrl_new, dut.spi_ctrl_we);

      $display("");
      $display("spi_tx_data_new: 0x%1x, spi_tx_data_nxt: 0x%1x, spi_tx_data_we: 0x%1x",
               dut.spi_tx_data_new, dut.spi_tx_data_nxt, dut.spi_tx_data_we);
      $display("spi_tx_data_reg: 0x%02x, spi_tx_data_new: 0x%02x", dut.spi_tx_data_reg,
               dut.spi_tx_data_new);
      $display("");
      $display("spi_rx_data_nxt: 0x%1x, spi_rx_data_we: 0x%1x", dut.spi_rx_data_nxt,
               dut.spi_rx_data_we);
      $display("spi_rx_data_reg: 0x%02x, spi_rx_data_new: 0x%02x", dut.spi_rx_data_reg,
               dut.spi_rx_data_new);
      $display("spi_rx_data_reg0: 0x%1x, spi_rx_data_new0: 0x%1x", dut.spi_rx_data_reg[0],
               dut.spi_rx_data_new[0]);
      $display("spi_rx_data_reg1: 0x%1x, spi_rx_data_new1: 0x%1x", dut.spi_rx_data_reg[1],
               dut.spi_rx_data_new[1]);
      $display("spi_rx_data_reg2: 0x%1x, spi_rx_data_new2: 0x%1x", dut.spi_rx_data_reg[2],
               dut.spi_rx_data_new[2]);
      $display("spi_rx_data_reg3: 0x%1x, spi_rx_data_new3: 0x%1x", dut.spi_rx_data_reg[3],
               dut.spi_rx_data_new[3]);
      $display("spi_rx_data_reg4: 0x%1x, spi_rx_data_new4: 0x%1x", dut.spi_rx_data_reg[4],
               dut.spi_rx_data_new[4]);
      $display("spi_rx_data_reg5: 0x%1x, spi_rx_data_new5: 0x%1x", dut.spi_rx_data_reg[5],
               dut.spi_rx_data_new[5]);
      $display("spi_rx_data_reg6: 0x%1x, spi_rx_data_new6: 0x%1x", dut.spi_rx_data_reg[6],
               dut.spi_rx_data_new[6]);
      $display("spi_rx_data_reg7: 0x%1x, spi_rx_data_new7: 0x%1x", dut.spi_rx_data_reg[7],
               dut.spi_rx_data_new[7]);
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
        $display("--- %02d tests completed - %02d test cases did not complete successfully.",
                 tc_ctr, error_ctr);
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
      cycle_ctr          = 0;
      error_ctr          = 0;
      tc_ctr             = 0;
      monitor            = 0;

      tb_clk             = 1'h0;
      tb_reset_n         = 1'h1;
      tb_spi_enable      = 1'h0;
      tb_spi_enable_vld  = 1'h0;
      tb_spi_start       = 1'h0;
      tb_spi_tx_data     = 8'h0;
      tb_spi_tx_data_vld = 1'h0;
      tb_miso_mux_ctrl   = MISO_MOSI;
    end
  endtask  // init_sim


  //----------------------------------------------------------------
  // enable_spi
  //
  // Enable the SPI-interface
  //----------------------------------------------------------------
  task enable_spi;
    begin
      if (verbose) begin
        $display("enable_spi: Started");
      end

      tb_spi_enable     = 1'h1;
      tb_spi_enable_vld = 1'h1;
      #(CLK_PERIOD);
      tb_spi_enable_vld = 1'h0;
      #(CLK_PERIOD);

      if (verbose) begin
        $display("enable_spi: Completed");
      end
    end
  endtask  // enable_spi


  //----------------------------------------------------------------
  // disable_spi
  //
  // Disable the SPI-interface
  //----------------------------------------------------------------
  task disable_spi;
    begin
      if (verbose) begin
        $display("disable_spi: Started");
      end

      tb_spi_enable     = 1'h0;
      tb_spi_enable_vld = 1'h1;
      #(CLK_PERIOD);
      tb_spi_enable_vld = 1'h0;
      #(CLK_PERIOD);

      if (verbose) begin
        $display("disable_spi: Completed");
      end
    end
  endtask  // disable_spi


  //----------------------------------------------------------------
  // xfer_byte
  //
  // Wait until the SPI-master is ready, then send input byte
  // and return the received byte.
  //----------------------------------------------------------------
  task xfer_byte(input [7 : 0] to_mem, output [7 : 0] from_mem);
    begin
      if (verbose) begin
        $display("xfer_byte: Trying to send 0x%02x to mem", to_mem);
      end

      tb_spi_tx_data     = to_mem;
      tb_spi_tx_data_vld = 1'h1;
      #(CLK_PERIOD);
      tb_spi_tx_data_vld = 1'h0;
      #(CLK_PERIOD);

      while (tb_spi_ready == 1'h0) begin
        #(CLK_PERIOD);
      end
      #(CLK_PERIOD);

      tb_spi_start = 1'h1;
      #(CLK_PERIOD);
      tb_spi_start = 1'h0;
      #(CLK_PERIOD);

      while (tb_spi_ready == 1'h0) begin
        #(CLK_PERIOD);
      end
      #(CLK_PERIOD);

      from_mem = tb_spi_rx_data;
      #(CLK_PERIOD);
      if (verbose) begin
        $display("xfer_byte: Received 0x%02x from mem", from_mem);
      end
    end
  endtask  // xfer_byte


  //----------------------------------------------------------------
  // read_mem_range()
  //
  // Read out a specified memory range. Result is printed,
  //----------------------------------------------------------------
  task read_mem_range(input [23 : 0] address, input integer num_bytes);
    begin : read_mem_range
      reg [7 : 0] rx_byte;
      integer i;

      if (verbose) begin
        $display("read_mem_range: Reading out %d bytes starting at address 0x%06x", num_bytes,
                 address);
      end

      #(2 * CLK_PERIOD);
      enable_spi();
      #(2 * CLK_PERIOD);

      // Send read command 0x03.
      xfer_byte(8'h03, rx_byte);

      // Send adress 0x000000.
      xfer_byte(address[23 : 16], rx_byte);
      xfer_byte(address[15 : 8], rx_byte);
      xfer_byte(address[7 : 0], rx_byte);

      // Read out num_bytes bytes.
      for (i = 0; i < num_bytes; i = i + 1) begin
        xfer_byte(8'h00, rx_byte);
        $display("--- tc_read_mem_range: Byte 0x%06x: 0x%02x", address + i, rx_byte);
      end

      disable_spi();
      #(2 * CLK_PERIOD);

      if (verbose) begin
        $display("read_mem_range: Completed");
      end
    end
  endtask  // read_mem_range


  //----------------------------------------------------------------
  // read_status()
  //----------------------------------------------------------------
  task read_status();
    begin : read_status
      reg [ 7 : 0] dummy;
      reg [15 : 0] status;
      enable_spi();
      #(2 * CLK_PERIOD);
      xfer_byte(8'h05, dummy);
      xfer_byte(8'h00, status[15 : 8]);
      xfer_byte(8'h00, status[7 : 0]);
      #(2 * CLK_PERIOD);
      disable_spi();
      $display("--- read_status: 0x%04x", status);
    end
  endtask  // read_status


  //----------------------------------------------------------------
  // tc_get_device_id()
  //
  // Test case that reads out the device ID.
  //----------------------------------------------------------------
  task tc_get_device_id;
    begin : tc_get_id
      reg [7 : 0] rx_byte;
      tc_ctr  = tc_ctr + 1;
      monitor = 0;

      $display("");
      $display("--- tc_get_device_id: Read out device id from the memory.");

      #(2 * CLK_PERIOD);
      enable_spi();
      #(2 * CLK_PERIOD);

      // Send 0xab command.
      $display("--- tc_get_device_id: Sending 0xab command.");
      xfer_byte(8'hab, rx_byte);
      #(CLK_PERIOD);

      // Dummy bytes.
      xfer_byte(8'h00, rx_byte);
      $display("--- tc_get_device_id: Got 0x%02x after dummy byte 1", rx_byte);
      xfer_byte(8'h00, rx_byte);
      $display("--- tc_get_device_id: Got 0x%02x after dummy byte 2", rx_byte);
      xfer_byte(8'h00, rx_byte);
      $display("--- tc_get_device_id: Got 0x%02x after dummy byte 3", rx_byte);

      // Get the ID byte.
      xfer_byte(8'h00, rx_byte);
      $display("--- tc_get_device_id: Got ID 0x%02x after dummy byte 4", rx_byte);
      xfer_byte(8'h00, rx_byte);
      $display("--- tc_get_device_id: Got ID 0x%02x after dummy byte 5", rx_byte);
      xfer_byte(8'h00, rx_byte);
      $display("--- tc_get_device_id: Got ID 0x%02x after dummy byte 6", rx_byte);
      xfer_byte(8'h00, rx_byte);
      $display("--- tc_get_device_id: Got ID 0x%02x after dummy byte 7", rx_byte);
      xfer_byte(8'h00, rx_byte);
      $display("--- tc_get_device_id: Got ID 0x%02x after dummy byte 8", rx_byte);

      disable_spi();
      #(2 * CLK_PERIOD);

      $display("--- tc_get_device_id: completed.");
      $display("");
    end
  endtask  // tc_get_device_id


  //----------------------------------------------------------------
  // tc_get_jedec_id()
  //
  // Test case that reads out the JEDEC ID.
  //----------------------------------------------------------------
  task tc_get_jedec_id;
    begin : tc_get_id
      reg [7 : 0] rx_byte;
      tc_ctr  = tc_ctr + 1;
      monitor = 0;
      verbose = 0;

      $display("");
      $display("--- tc_get_jedec_id: Read out JEDEC device id, type and capacity from the memory.");

      #(2 * CLK_PERIOD);
      enable_spi();
      #(2 * CLK_PERIOD);

      // Send 0x9f command.
      $display("--- tc_get_jedec_id: Sending 0xab command.");
      xfer_byte(8'h9f, rx_byte);

      // Send dummy bytes and get response back.
      xfer_byte(8'h00, rx_byte);
      $display("--- tc_get_jedec_id: Got manufacture ID 0x%02x", rx_byte);
      xfer_byte(8'h00, rx_byte);
      $display("--- tc_get_jedec_id: Got memory type 0x%02x", rx_byte);
      xfer_byte(8'h00, rx_byte);
      $display("--- tc_get_jedec_id: Got memory capacity 0x%02x", rx_byte);

      disable_spi();
      #(2 * CLK_PERIOD);

      $display("--- tc_get_jedec_id: completed.");
      $display("");

      verbose = 1;
    end
  endtask  // tc_get_jedec_id


  //----------------------------------------------------------------
  // tc_get_unique_device_id()
  //
  // Test case that reads out the JEDEC ID.
  // Expected: 0xdc02030405060708
  //----------------------------------------------------------------
  task tc_get_unique_device_id;
    begin : tc_get_id
      reg [7 : 0] rx_byte;
      integer i;
      tc_ctr  = tc_ctr + 1;
      monitor = 0;
      verbose = 0;

      $display("");
      $display("--- tc_get_unique_device_id: Read out unique id from the memory");
      $display("--- tc_get_unique_device_id: Expected result: 0xdc02030405060708");

      #(2 * CLK_PERIOD);
      enable_spi();
      #(2 * CLK_PERIOD);

      // Send 0x9f command.
      $display("--- tc_get_unique_device_id: Sending 0x4b command.");
      xfer_byte(8'h4b, rx_byte);

      // Send four dummy bytes and get response back.
      xfer_byte(8'h00, rx_byte);
      xfer_byte(8'h00, rx_byte);
      xfer_byte(8'h00, rx_byte);
      xfer_byte(8'h00, rx_byte);

      // Send eight bytes and get unique device id back.
      $display("--- tc_get_unique_device_id: reading out the unique device ID");
      for (i = 0; i < 8; i = i + 1) begin
        xfer_byte(8'h00, rx_byte);
        $display("--- tc_get_unique_device_id: 0x%02x", rx_byte);
      end

      disable_spi();
      #(2 * CLK_PERIOD);

      $display("--- tc_get_unique_device_id: completed.");
      $display("");

      verbose = 1;
    end
  endtask  // tc_get_unique_device_id


  //----------------------------------------------------------------
  // tc_get_manufacturer_id()
  //
  // Test case that reads out the device ID.
  //----------------------------------------------------------------
  task tc_get_manufacturer_id;
    begin : tc_get_id
      reg [7 : 0] rx_byte;
      tc_ctr  = tc_ctr + 1;
      monitor = 0;

      $display("");
      $display("--- tc_get_manufacturer_id: Read out device id from the memory.");

      #(2 * CLK_PERIOD);
      enable_spi();
      #(2 * CLK_PERIOD);

      // Send 0x90 command.
      $display("--- tc_get_manufacturer_id: Sending 0xab command.");
      xfer_byte(8'h90, rx_byte);

      // Dummy bytes.
      xfer_byte(8'h00, rx_byte);
      $display("--- tc_get_manufacturer_id: Got 0x%02x after dummy byte 1", rx_byte);
      xfer_byte(8'h00, rx_byte);
      $display("--- tc_get_manufacturer_id: Got 0x%02x after dummy byte 2", rx_byte);
      xfer_byte(8'h00, rx_byte);
      $display("--- tc_get_manufacturer_id: Got 0x%02x after dummy byte 3", rx_byte);

      // Get the ID byte.
      xfer_byte(8'h00, rx_byte);
      $display("--- tc_get_manufacturer_id: Got ID 0x%02x after dummy byte 4", rx_byte);
      xfer_byte(8'h00, rx_byte);
      $display("--- tc_get_manufacturer_id: Got ID 0x%02x after dummy byte 5", rx_byte);

      disable_spi();
      #(2 * CLK_PERIOD);

      $display("--- tc_get_manufacturer_id: completed.");
      $display("");
    end
  endtask  // tc_get_manufacturer_id


  //----------------------------------------------------------------
  // tc_read_mem()
  //
  // Test case that reads out the first 16 bytes of the memory.
  //----------------------------------------------------------------
  task tc_read_mem;
    begin : tc_get_id
      reg [7 : 0] rx_byte;
      integer i;
      tc_ctr  = tc_ctr + 1;
      monitor = 0;
      verbose = 0;

      $display("");
      $display("--- tc_read_mem: Read out the first 16 bytes from the memory.");

      #(2 * CLK_PERIOD);
      enable_spi();
      #(2 * CLK_PERIOD);

      // Send read command 0x03.
      $display("--- tc_read_mem: Sending 0x03 command.");
      xfer_byte(8'h03, rx_byte);

      // Send adress 0x000000.
      $display("--- tc_read_mem: Sending 24 bit address 0x000000.");
      xfer_byte(8'h00, rx_byte);
      xfer_byte(8'h00, rx_byte);
      xfer_byte(8'h00, rx_byte);

      // Read out 16 bytes.
      $display("--- tc_read_mem: Reading out 16 bytes from the memory.");
      for (i = 1; i < 17; i = i + 1) begin
        xfer_byte(8'h00, rx_byte);
        $display("--- tc_read_mem: Byte %d: 0x%02x", i, rx_byte);
      end

      disable_spi();
      #(2 * CLK_PERIOD);

      $display("--- tc_read_mem: completed.");
      $display("");
    end
  endtask  // tc_read_mem


  //----------------------------------------------------------------
  // tc_rmr_mem()
  //
  // Test case that reads out the first 16 bytes of the memory,
  // erase the same area, reads out the contents again, writes
  // a known pattern and the reads it out again.
  //----------------------------------------------------------------
  task tc_rmr_mem;
    begin : tc_get_id
      reg [7 : 0] rx_byte;
      integer i;
      tc_ctr  = tc_ctr + 1;
      monitor = 0;
      verbose = 0;

      $display("");
      $display("--- tc_rmr_mem: Read out the first 16 bytes from the memory.");
      read_mem_range(24'h000000, 16);

      $display("");
      $display("--- tc_rmr_mem: Status before write enable:");
      read_status();

      // Set write enable mode.
      enable_spi();
      //      #(2 * CLK_PERIOD);
      xfer_byte(8'h06, rx_byte);
      //      #(2 * CLK_PERIOD);
      disable_spi();
      #(2 * CLK_PERIOD);
      $display("--- tc_rmr_mem: Status after write enable:");
      read_status();

      // Erase sector. Command 0x20 followed by 24 bit address.
      enable_spi();
      #(2 * CLK_PERIOD);
      xfer_byte(8'h20, rx_byte);
      xfer_byte(8'h00, rx_byte);
      xfer_byte(8'h00, rx_byte);
      xfer_byte(8'h00, rx_byte);
      disable_spi();
      #(4096 * CLK_PERIOD);
      $display("--- tc_rmr_mem: Content of memory after erase.");
      read_mem_range(24'h000000, 16);

      disable_spi();
      #(2 * CLK_PERIOD);

      $display("--- tc_rmr_mem: completed.");
      $display("");
    end
  endtask  // tc_rmr_mem


  //----------------------------------------------------------------
  // tk1_spi_master_test
  //----------------------------------------------------------------
  initial begin : tk1_spi_master_test
    $display("");
    $display("   -= Testbench for tk1_spi_master started =-");
    $display("     =======================================");
    $display("");

    init_sim();
    reset_dut();
    disable_spi();

    verbose = 1;

    //      tc_get_device_id();
    tc_get_jedec_id();
    //      tc_get_manufacturer_id();
    tc_get_unique_device_id();
    tc_read_mem();
    //      tc_rmr_mem();

    display_test_result();
    $display("");
    $display("   -= Testbench for tk1_spi_master completed =-");
    $display("     =========================================");
    $display("");
    $finish;
  end  // tk1_spi_master_test
endmodule  // tb_tk1_spi_master

//======================================================================
// EOF tb_tk1_spi_master.v
//======================================================================
