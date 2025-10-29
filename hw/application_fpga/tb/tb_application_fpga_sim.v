//======================================================================
//
// tb_application_fpga_sim.v
// -------------------------
// Top level module of the application_fpga.
// The design exposes a UART interface to allow a host to
// send commands and receive responses as needed load, execute and
// communicate with applications.
//
//
// SPDX-FileCopyrightText: 2022 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause
//
//======================================================================

`timescale 1ns / 1ns

module tb_application_fpga_sim ();

  //----------------------------------------------------------------
  // Internal constant and parameter definitions.
  //----------------------------------------------------------------
  parameter CLK_HALF_PERIOD = 1;
  parameter CLK_PERIOD = 2 * CLK_HALF_PERIOD;

  //----------------------------------------------------------------
  // Register and Wire declarations.
  //----------------------------------------------------------------
  reg  tb_clk = 0;
  wire tb_interface_rx;
  reg  tb_interface_tx = 1'h1;  // Set to 1 to simulate inactive UART
  reg  tb_interface_ch552_cts = 1'h1;  // Set to 1 to simulate OK to send
  reg  tb_interface_fpga_cts;
  wire tb_spi_ss;
  wire tb_spi_sck;
  wire tb_spi_mosi;
  reg  tb_spi_miso;
  reg  tb_touch_event;
  reg  tb_app_gpio1;
  reg  tb_app_gpio2;
  wire tb_app_gpio3;
  wire tb_app_gpio4;
  wire tb_led_r;
  wire tb_led_g;
  wire tb_led_b;

  //----------------------------------------------------------------
  // Device Under Test.
  //----------------------------------------------------------------
  application_fpga_sim dut (
      .clk(tb_clk),
      .interface_rx(tb_interface_rx),
      .interface_tx(tb_interface_tx),
      .interface_ch552_cts(tb_interface_ch552_cts),
      .interface_fpga_cts(tb_interface_fpga_cts),
      .spi_ss(tb_spi_ss),
      .spi_sck(tb_spi_sck),
      .spi_mosi(tb_spi_mosi),
      .spi_miso(tb_spi_miso),
      .touch_event(tb_touch_event),
      .app_gpio1(tb_app_gpio1),
      .app_gpio2(tb_app_gpio2),
      .app_gpio3(tb_app_gpio3),
      .app_gpio4(tb_app_gpio4),
      .led_r(tb_led_r),
      .led_g(tb_led_g),
      .led_b(tb_led_b)
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
  // finish
  //
  // End simulation
  //----------------------------------------------------------------
  initial begin
    // End simulation after XXX time units (set by timescale)
    #20000000;
    $display("TIMEOUT");
    $finish;
  end

  //----------------------------------------------------------------
  // Fill memories with data
  //----------------------------------------------------------------
  initial begin
    $readmemh("tb/output_spram0.hex", dut.ram_inst.spram0.mem);
    $readmemh("tb/output_spram1.hex", dut.ram_inst.spram1.mem);
    $readmemh("tb/output_spram2.hex", dut.ram_inst.spram2.mem);
    $readmemh("tb/output_spram3.hex", dut.ram_inst.spram3.mem);
  end

  //----------------------------------------------------------------
  // dumpfile
  //
  // Save waveform file
  //----------------------------------------------------------------
  initial begin
    $dumpfile("tb_application_fpga_sim.fst");
    $dumpvars(0, tb_application_fpga_sim);
  end

endmodule  // tb_application_fpga_sim

//======================================================================
// EOF tb_application_fpga_sim.v
//======================================================================
